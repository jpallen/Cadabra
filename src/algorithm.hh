/* 

	Cadabra: a field-theory motivated computer algebra system.
	Copyright (C) 2001-2009  Kasper Peeters <kasper.peeters@aei.mpg.de>

   This program is free software: you can redistribute it and/or
   modify it under the terms of the GNU General Public License as
   published by the Free Software Foundation, either version 3 of the
   License, or (at your option) any later version.

   This program is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
   General Public License for more details.

   You should have received a copy of the GNU General Public License
   along with this program.  If not, see <http://www.gnu.org/licenses/>.
 
*/

#ifndef algorithm_hh_
#define algorithm_hh_

#include <modglue/pipe.hh>
#include "stopwatch.hh"
#include "storage.hh"
#include "props.hh"
#include "display.hh"
#include <map>

// These are initiated in main.cc
#include <fstream>
extern std::ostream  *fake_txtout;
#define txtout (*fake_txtout)
extern std::ostream  *fake_forcedout;
#define forcedout (*fake_forcedout)
extern modglue::opipe texout;
extern std::ofstream  debugout;
extern bool           interrupted;
extern stopwatch      globaltime;

/// An error class which can be thrown if there is no local way to recover
/// from an error. Will be handled at the top level and will lead to 
/// the current expression being removed from the tree.
class consistency_error : public std::logic_error {
	public:
		consistency_error(const std::string&);
};

/// An error class which should be thrown by any algorithm as soon as it
/// detects that the "interrupted" flag has been set by the sigint signal 
/// handler. 
class algorithm_interrupted : public std::logic_error {
	public:
		algorithm_interrupted();
		algorithm_interrupted(const std::string&);
};


/// Base class for objects which represent algorithms, i.e. which get
/// expanded by the manipulator when they are encountered in the tree.
class active_node {
	public:
		typedef exptree::iterator            iterator;
		typedef exptree::post_order_iterator post_order_iterator;
		typedef exptree::sibling_iterator    sibling_iterator;

		active_node(exptree&, iterator);
		virtual ~active_node() {};

		// Helpers for iterating over or inspecting argument lists.
		sibling_iterator args_begin() const;
		sibling_iterator args_end() const;
		unsigned int     number_of_args() const;
		bool             has_argument(const std::string&) const;

		iterator this_command;
	protected:
		exptree& tr;
		mutable sibling_iterator args_begin_;
		mutable sibling_iterator args_end_;
		
		// Return the number of elements in the first range for which an identical element
		// is present in the second range.
		template<class BinaryPredicate>
		unsigned int intersection_number(sibling_iterator, sibling_iterator, 
													sibling_iterator, sibling_iterator, BinaryPredicate) const;
};

/// \brief Base class for all algorithms, containing generic routines and in particular
///        the logic for index classification.
///
/// All commands in cadabra are classes which derive from the algorithm class. An object
/// gets instantiated by the logic in the manipulator class, which then activates the
/// algorithm.
/// This base class contains various functions that are quite generic and often needed.
/// In particular, all logic that deals with index classification is located here.
class algorithm : public active_node {
	public:
		algorithm(exptree&, iterator);
		virtual ~algorithm();

		class constructor_error {
			public:
				constructor_error();
		};

		enum result_t {
			l_no_action,
			l_applied,
			l_error
		};

		enum global_success_t {
			g_not_yet_started=0,
			g_arguments_accepted=1,
			g_operand_determined=2,
			g_applied=4,
			g_apply_failed=6
		};

		virtual bool     is_output_module() const;

		virtual bool     can_apply(iterator);
		virtual bool     can_apply(sibling_iterator, sibling_iterator);
		// These return their result in the return value
		virtual result_t apply(iterator&);
		virtual result_t apply(sibling_iterator&, sibling_iterator&);

		// These give result in global_success.
		bool             apply_recursive(iterator&, bool check_consistency=true, int act_at_level=-1,
													bool called_by_manipulator=false, bool until_nochange=false);
	   void             apply(unsigned int last_used_equation_number, bool multiple, bool until_nochange,
									  bool make_copy, int act_at_level=-1, bool called_by_manipulator=false);

		// Per-call information
		bool             expression_modified;
		iterator         subtree;        // subtree to be displayed
		unsigned int     equation_number;

		// External handling of scalar expressions
		

		// Global information
		global_success_t global_success;
		unsigned int     number_of_calls;
		unsigned int     number_of_modifications;
		bool             suppress_normal_output;
		bool             discard_command_node;

		// Any output should be handled using this output object.
		exptree_output  *eo;

		// Given an \expression node, check consistency
		bool      check_consistency(iterator) const;
		bool      check_index_consistency(iterator) const;

		static stopwatch index_sw;
		static stopwatch get_dummy_sw;

		void report_progress(const std::string&, int todo, int done, int count=2);
		stopwatch report_progress_stopwatch;
	protected:
		unsigned int last_used_equation_number; // FIXME: this is a hack, just to see this in 'eqn'.

		std::vector<std::pair<sibling_iterator, sibling_iterator> > marks;
		iterator                       previous_expression;
		bool                           dont_iterate;

		// Index stuff
		int      index_parity(iterator) const;
		static bool less_without_numbers(nset_t::iterator, nset_t::iterator);
		static bool equal_without_numbers(nset_t::iterator, nset_t::iterator);
		
		/// Finding objects in sets.
		typedef std::pair<sibling_iterator, sibling_iterator> range_t;
		typedef std::vector<range_t>                          range_vector_t;

		bool     contains(sibling_iterator from, sibling_iterator to, sibling_iterator arg);
		void     find_argument_lists(range_vector_t&, bool only_comma_lists=true) const;       
		template<class Iter>
		range_vector_t::iterator find_arg_superset(range_vector_t&, Iter st, Iter nd);
		range_vector_t::iterator find_arg_superset(range_vector_t&, sibling_iterator it);

		/// Take a single non-product node in a sum and wrap it in a 
		/// product node, so it can be handled on the same footing as a proper product.
		bool     is_single_term(iterator);
		bool     is_nonprod_factor_in_prod(iterator);
		bool     prod_wrap_single_term(iterator&);
		bool     prod_unwrap_single_term(iterator&);

		/// Figure out whether two objects (commonly indices) are separated by a derivative
		/// operator, as in \partial_{a}{A_{b}} C^{b}.
		/// If the last iterator is pointing to a valid node, check whether it is
		/// independent of the derivative (using the Depends property).
		bool     separated_by_derivative(iterator, iterator, iterator check_dependence) const;

		// Given a node with non-zero multiplier, distribute this
		// multiplier up the tree when the node is a \sum node, or push it into the
		// \prod node if that is the parent. Do this recursively
		// in case a child is a sum as well. Note that 'pushup' is actually 'pushdown'
      // in the case of sums.
		void     pushup_multiplier(iterator);

		// Turn a node into a '1' or '0' node.
		void     node_zero(iterator);
		void     node_one(iterator);
		void     node_integer(iterator, int);

		// Find all dummy index pairs in an expression. This takes into account
		// products and sums.
		// Note: dummy indices do not always come in pairs, for instance
		//            a_{m n} ( b^{n p} + q^{n p} )
		// 
		/// A map from a pattern to the position where it occurs in the tree. 
		typedef std::multimap<exptree, exptree::iterator, tree_exact_less_no_wildcards_mod_prel_obj> index_map_t;
		/// A map from the position of each index to the sequential index.
		typedef std::map<exptree::iterator, int, exptree::iterator_base_less>    index_position_map_t;

		void     fill_index_position_map(iterator, const index_map_t&, index_position_map_t&) const;
		void     fill_map(index_map_t&, sibling_iterator, sibling_iterator) const;
		bool     rename_replacement_dummies(iterator, bool still_inside_algo=false);
		void     print_classify_indices(iterator) const;
		void     determine_intersection(index_map_t& one, index_map_t& two, index_map_t& target,
										  bool move_out=false) const; 
		void     classify_add_index(iterator it, index_map_t& ind_free, index_map_t& ind_dummy) const;
		void     classify_indices_up(iterator, index_map_t& ind_free, index_map_t& ind_dummy) const;
		void     classify_indices(iterator, index_map_t& ind_free, index_map_t& ind_dummy) const;
		int      max_numbered_name_one(const std::string& nm, const index_map_t * one) const;
		int      max_numbered_name(const std::string&, const index_map_t *m1, const index_map_t *m2=0,
											const index_map_t *m3=0, const index_map_t *m4=0, const index_map_t *m5=0) const;
		exptree get_dummy(const list_property *, const index_map_t *m1, const index_map_t *m2=0,
											const index_map_t *m3=0, const index_map_t *m4=0, const index_map_t *m5=0) const;
		exptree get_dummy(const list_property *, iterator) const;
		exptree get_dummy(const list_property *, iterator, iterator) const;

	private:
		void     cancel_modification();
		void     copy_expression(exptree::iterator) const;
		bool     prepare_for_modification(bool make_copy);
		/// Given a node with zero multiplier, propagate this zero
		/// upwards in the tree.  Changes the iterator so that it points
		/// to the next node in a post_order traversal (post_order:
		/// children first, then node). The second node is the topmost
		/// node, beyond which this routine is not allowed to touch the
		/// tree (i.e. the 2nd iterator will always remain valid).
		void     propagate_zeroes(post_order_iterator&, const iterator&);
		void     dumpmap(std::ostream&, const index_map_t&) const;

		bool cleanup_anomalous_products(exptree& tr, exptree::iterator& it);
};

void cleanup_expression(exptree&);
void cleanup_expression(exptree&, exptree::iterator&); // may change the pointer!
void cleanup_sums_products(exptree&, exptree::iterator&);
void cleanup_nests(exptree&tr, exptree::iterator &it, bool ignore_bracket_types=false);
void cleanup_nests_below(exptree&tr, exptree::iterator it, bool ignore_bracket_types=false);

template<class T>
std::auto_ptr<algorithm> create(exptree& tr, exptree::iterator it)
	{
	return std::auto_ptr<algorithm>(new T(tr, it));
	}

/// Determine the number of elements in the first range which also occur in the
/// second range.
template<class BinaryPredicate>
unsigned int active_node::intersection_number(sibling_iterator from1, sibling_iterator to1,
															 sibling_iterator from2, sibling_iterator to2,
															 BinaryPredicate fun) const
	{
	unsigned int ret=0;
	sibling_iterator it1=from1;
	while(it1!=to1) {
		sibling_iterator it2=from2;
		while(it2!=to2) {
			if(fun(*it1,*it2))
				++ret;
			++it2;
			}
		++it1;
		}
	return ret;
	}

#endif
