/* Copyright (C) 2012 Krzysztof Stachowiak */

/* 
 * This file is part of stat-toolkit.
 * 
 * stat-toolkit is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 * 
 * stat-toolkit is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 * 
 * You should have received a copy of the GNU General Public License
 * along with stat-toolkit; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA  02110-1301  USA
 */

#include <algorithm>
using std::find;

#include <map>
using std::map;

#include <string>
using std::string;

#include <iostream>
using std::istream;
using std::ostream;
using std::cin;
using std::cout;
using std::endl;

#include <boost/xpressive/xpressive.hpp>
using boost::xpressive::sregex;
using boost::xpressive::smatch;
using boost::xpressive::s1;
using boost::xpressive::s2;
using boost::xpressive::_d;
using boost::xpressive::_s;
using boost::xpressive::_;
using boost::xpressive::eos;

#include <unistd.h>

#include "util.h"
#include "groupby.h"

// Handle the command line arguments.
// ----------------------------------

struct arguments {
	char delim;
	bool print_headers;
	bool expect_data_header;
	vector<vector<uint32_t>> dimensions;
	vector<string> aggr_strs;
};

vector<uint32_t> parse_dim_arg(string const& arg) {

	vector<uint32_t> result;
	
	uint32_t index;

	char base_str[arg.size() + 1];
	strcpy(base_str, arg.c_str());

	char* pch = strtok(base_str, " ");
	while(pch) {
		stringstream converter;
		converter << pch;
		converter >> index;
		if(converter.fail()) {
			stringstream errorss;
			errorss << "Failed parsing dimension index \"" << pch << "\".";
			throw errorss.str();
		}

		result.push_back(index);
		pch = strtok(0, " ");
	}

	return result;
}

arguments parse_args(int argc, char** argv) {
	
	arguments args;
	args.delim = '\t';
	args.print_headers = false;
	args.expect_data_header = false;

	int c;
	while((c = getopt(argc, argv, "a:d:D:RhH")) != -1) {
		switch(c) {
		case 'a':
			args.aggr_strs.emplace_back(optarg);
			break;

		case 'd':
			if(string(optarg).size() != 1)
				throw string("The delimiter is expected to be"
						"a single character.");
			args.delim = optarg[0];
			if(!isprint(args.delim) && args.delim != '\t')
				throw string("Cannot use the given character"
						"as a delimiter.");
			break;

		case 'D':
			args.dimensions.push_back(parse_dim_arg(optarg));
			break;

		case 'h':
			args.print_headers = true;
			break;

		case 'H':
			args.expect_data_header = true;
			break;

		case '?':
			if(optopt == 'a')
				throw string("Option -a requires an aggregator"
						"argument.");

			if(optopt == 'd')
				throw string("Option -d requires a delimiter"
						"argument.");

			if(optopt == 'D')
				throw string("Option -D requires a dimension"
						"argument.");

			// Notice a fallthrough. It is here although it should
			// not happen unless someone changes the getopt options
			// definition.

		default:
			throw string("Illegal option -") + (char)optopt + ".";
		}
	}

	return args;
}

// Reading additional data from the input header.
map<uint32_t, string> process_header(istream& in, arguments const& args) {
	map<uint32_t, string> result;
	string line;
	getline(in, line);
	vector<string> row = split(line, args.delim);
	for(uint32_t i = 0; i < row.size(); ++i)
		result[i] = row[i];
	return result;
}

// The groupping phase.
// --------------------

groupby::groupper perform_groupping(
		istream& in, 
		arguments const& args) {

	// Flatten the dimension definitions.
	vector<uint32_t> groupbys;
	for(auto const& v : args.dimensions)
		for(auto dim : v)
			if(find(begin(groupbys), end(groupbys), dim) == end(groupbys))
				groupbys.push_back(dim);
			else
				throw string("Column index repeated in the dimension "
						"definition.");

	groupby::groupper g;
	string line;
	while(true) {
		getline(in, line);
		if(!in.good())
			break;
		vector<string> row = split(line, args.delim);
		g.consume_row(groupbys, args.aggr_strs, row);
	}

	return g;
}

// The arrangement and printing phase.
// -----------------------------------

vector<groupby::group_result> sort_group_results(
		vector<groupby::group_result> const& results,
		arguments const& args) {

	// Copy the input collection.
	vector<groupby::group_result> groups(begin(results), end(results));

	// Sort the group by the dimensions.
	sort(begin(groups), end(groups),
			[&args](groupby::group_result const& lhs,
				groupby::group_result const& rhs) {

		for(auto const& dim : args.dimensions) {

			// Compare the values from the columns determined by the
			// dimension; return at the forst that is not equal for the
			// both groups.
			for(auto const& i : dim) {
				const string& l = lhs.get_def_at(i);	
				const string& r = rhs.get_def_at(i);	
				if(l != r)
					return l < r;
			}
		}

		// Getting here means that the group definitions are equal, which
		// is by no means legal.
		throw string("Attempted comparing groups defined equally.");
	});

	return groups;
}

typedef vector<pair<uint32_t, string>> dim_t;

inline
dim_t get_group_dim(groupby::group_result const& grp, vector<uint32_t> const& dim) {
	vector<pair<uint32_t, string>> result;
	for(uint32_t col : dim)
		result.emplace_back(col, grp.get_def_at(col));
	return result;
}

inline
string dim_caption(dim_t d) {
	stringstream ss;
	for(auto const& pr : d)
		ss << pr.second << " ";
	return ss.str();
}

template<class It>
void print_row(It grp_begin, It grp_end,
		vector<dim_t> const& sorted_columns,
		ostream& out,
		arguments const& args) {

	throw string("Unimplemented method.");
}

template<class It>
void print_page(It grp_begin, It grp_end,
		uint32_t dim_offset,
		ostream& out,
		arguments const& args) {

	// Determine the page's columns.
	// -----------------------------
	set<dim_t> col_set;
	for(auto grp_it = grp_begin; grp_it != grp_end; ++grp_it)
		col_set.insert(get_group_dim(*grp_it, args.dimensions[dim_offset + 1]));

	vector<dim_t> sorted_columns(begin(col_set), end(col_set));
	sort(begin(sorted_columns), end(sorted_columns),
		[&args](dim_t const& lhs, dim_t const& rhs) {
			uint32_t num_defs = lhs.size();	
			if(num_defs != rhs.size())
				throw string("Attempted comparing dimensionf of differend sizes");
			for(uint32_t i = 0; i < num_defs; ++i) {
				if(lhs.at(i).first != rhs.at(i).first)
					return lhs.at(i).first < rhs.at(i).first
				if(lhs.at(i).second != rhs.at(i).second)
					return lhs.at(i).second < rhs.at(i).second
			}

			return false;
		});

	// Print all rows.
	// ---------------
	auto row_begin = grp_begin;
	auto it = grp_begin;

	dim_t current_row = get_group_dim(*it, args.dimensions[dim_offset]);
	do {
		dim_t group_row = get_group_dim(*it, args.dimensions[dim_offset]);
		if(group_row != current_row) {
			out << dim_caption(current_row);
			print_row(row_begin, it, sorted_columns, out, args);

			row_begin = it;
			current_row = get_group_dim(*it, args.dimensions[dim_offset]);
		}

	} while((++it) != grp_end);

	out << dim_caption(current_row);
	print_row(row_begin, it, sorted_columns, out, args);
}

void print_table(groupby::groupper const& g, ostream& out, arguments const& args) {

	auto sorted_groups = sort_group_results(g.copy_result(), args);

	if(args.dimensions.size() == 2) {

		// Print just one page.
		// --------------------
		print_page(begin(sorted_groups), end(sorted_groups), 0, out, args);

	} else if(args.dimensions.size() == 3) {

		// Print all the pages.
		// --------------------

		auto page_begin = begin(sorted_groups);
		auto it = begin(sorted_groups);

		dim_t current_page = get_group_dim(*it, args.dimensions[0]);
		do {
			dim_t group_page = get_group_dim(*it, args.dimensions[0]);

			// Check if we've entered another page.
			if(group_page != current_page) {

				// Print current page.
				out << "Page: " << dim_caption(current_page) << endl;
				print_page(page_begin, it, 1, out, args);

				// Begin another page.
				page_begin = it;
				current_page = get_group_dim(*it, args.dimensions[0]);
			}

		} while((++it) != end(sorted_groups));

		// Finalize the remaining page.
		out << "Page: " << dim_caption(current_page) << endl;
		print_page(page_begin, it, 1, out, args);

	} else {
		throw string("Only 2 or 3 dimensions supported.");
	}
}

int main(int argc, char** argv) {
	try {
		// Parse and validate the arguments.
		arguments args = parse_args(argc, argv);
		if(args.aggr_strs.empty())
			throw string("At leas one aggregator must be defined.");

		if(args.dimensions.size() != 2 && args.dimensions.size() != 3)
			throw string("Only 2 or 3 dimensions are supported.");

		// Headers variant.
		map<uint32_t, string> mapping;
		if(args.expect_data_header)
			mapping = process_header(cin, args);

		// Perform the processing.
		groupby::groupper g = perform_groupping(cin, args);
		print_table(g, cout, args);

		return 0;

	} catch(string ex) {
		cout << "Error : " << ex << endl;
		return 1;
	}
}
