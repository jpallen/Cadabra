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

#ifndef manipulator_hh_
#define manipulator_hh_

#include "storage.hh"
#include "algorithm.hh"
#include "display.hh"
#include <string>
#include <map>
#include <iostream>
#include <fstream>
#include <utility>
#include <stack>
#include <sigc++/object.h>
#include <modglue/pipe.hh>

// These are initiated in main.cc
extern std::ostream     *real_txtout;
extern std::ostream     *real_forcedout;
extern modglue::opipe    texout;
extern std::ofstream     debugout;
extern bool              interrupted;
extern bool              loginput;
extern bool              nowarnings;
extern bool              silentfail; 
extern std::vector<std::string> cmdline_arguments;

// All modules write to txtout, which we can point to whatever stream
// we like in order to redirect output to files.
extern std::ostream  *fake_txtout;
extern std::ostream  *fake_forcedout;
#define txtout (*fake_txtout)
#define forcedout (*fake_forcedout)

class stream_end_error {
	public:
		stream_end_error();
};

class exit_exception {
	public:
		exit_exception();
};

class manipulator : public sigc::trackable {
	public:
		manipulator();
		~manipulator();

		typedef exptree::iterator            iterator;
		typedef exptree::post_order_iterator post_order_iterator;
		typedef exptree::sibling_iterator    sibling_iterator;

		void replace_cmdline_args(std::string& oneline);
		void output_comment(const std::string& comment) const; // wrap in xml nodes if necessary
		bool handle_input();
		void open_stream(const std::string&);
		void open_stream(std::istream *);
		void cleanup_stream();
		bool receive_command(modglue::ipipe&);
		void set_prompt(const std::string&);
		void print_prompt() const;
		void output_status() const;
		void read_program_file();

		std::stack<std::istream *> streamstack;
		exptree_output             eo;
	private:
		/// A modified getline which ensures that the return string is one complete
		/// line of input, and no more than that. Will return false if there is no
		/// or not enough input (yet).
		bool              getline_precut(std::istream&, std::string&);
		std::string       getline_precut_buffer;
		bool              getline_was_eof;

		bool              is_whitespace_(const std::string& str) const;

		/// Activate all '@' nodes. The returned iterator points inside the
		/// expression which has to be printed (though not necessarily to the top node), 
		/// or to expressions.end() if the expression has been removed.
		exptree::iterator handle_active_nodes_(exptree::iterator);  
		nset_t::iterator  collect_labels_(exptree&, exptree::iterator);
		void              cleanup_new_expression_(exptree::iterator);
		exptree::iterator apply_pre_default_rules_(exptree::iterator);
		exptree::iterator apply_post_default_rules_(exptree::iterator);
		void              extract_properties_(exptree::iterator);
		exptree::iterator run_procedure(exptree::iterator, long);
		bool              handle_external_commands_(exptree::iterator&, exptree::iterator, exptree::iterator&);
		std::string       texify(const std::string&) const;

		class algo_info {
			public:
				algo_info(std::auto_ptr<algorithm> (*)(exptree&, iterator));

				std::auto_ptr<algorithm> (*create)(exptree&, iterator);
				stopwatch sw;
				long      calls;
		};

		typedef std::map<std::string, algo_info *> algorithm_map_t ;

		algorithm_map_t    algorithms;
		exptree            expressions;
		int                editing_equation;
		bool               keep_result;
		bool               display_result;
		unsigned int       last_used_equation_number;
		bool               utf8_output, status_output;

		std::string        input_buffer;
		std::string        refill_input_buffer; // used in @reset
		std::string        goto_label;
		std::string        bailout_label;
		std::string        prompt_string;
};

#endif
