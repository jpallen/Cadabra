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

/** 

  Parser module, which takes the string output of the preprocessor.hh module
  and turns it into a tree. The output of preprocessor.hh is assumed to be
  valid and consistent, so the code here is rather simple.

*/

#ifndef parser_hh_
#define parser_hh_

#include <string>
#include <vector>
#include "storage.hh"

class parser { 
	public:
		parser(bool preprocess=false);
	  
		void erase();

		friend std::istream& operator>>(std::istream&, parser&);
		friend std::ostream& operator<<(std::ostream&, parser&);

		void remove_empty_nodes();

		exptree tree;
	private:
		bool              preprocess_;
		exptree::iterator parts;
		std::string       str;

		enum mode_t { m_skipwhite, m_name, m_findchildren, 
						  m_singlecharname, m_backslashname, 
						  m_childgroup, m_initialgroup, m_verbatim, m_property };

		void                   advance(unsigned int& i);
		unsigned char          get_token(unsigned int i);
		bool                   string2tree(const std::string& inp);
		bool                   is_number(const std::string& str) const;
		str_node::bracket_t    is_closing_bracket(const unsigned char& br) const;
		str_node::bracket_t    is_opening_bracket(const unsigned char& br) const;
		str_node::parent_rel_t is_link(const unsigned char& ln) const;

		std::vector<mode_t>                 current_mode;
		std::vector<str_node::bracket_t>    current_bracket;
		std::vector<str_node::parent_rel_t> current_parent_rel;
};

std::istream& operator>>(std::istream&, parser&);
std::ostream& operator<<(std::ostream&, parser&);

#endif
