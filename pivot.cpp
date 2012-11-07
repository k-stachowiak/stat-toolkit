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

#include <set>
using std::set;

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
	char delim;				// The input/output field separator.
	bool print_headers;			// Print row/column captions?
	bool expect_data_header;		// Expect column captions in 1st row?
	vector<vector<uint32_t>> dimensions;	// Pivot dimension definitions.
	vector<string> aggr_strs;		// Aggregators' construction strings.
};

// Peals out a single dimension definition which is expected to be a
// list of whitespace separated numbers indicating the indices of the
// columns to define a given dimension.
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

// Prepare the structure storing semantically defined input arguments.
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

	groupby::groupper g(groupbys, args.aggr_strs);
	string line;
	while(true) {
		getline(in, line);
		if(!in.good())
			break;
		vector<string> row = split(line, args.delim);
		g.consume_row(row);
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

// Creates an object defining a given dimension of a given group.
inline dim_t group_dim(
		groupby::group_result const& grp,
		vector<uint32_t> const& dim) {
	vector<pair<uint32_t, string>> result;
	for(uint32_t col : dim)
		result.emplace_back(col, grp.get_def_at(col));
	return result;
}

// Prints a caption based on a dimension object and an optional mapping
// of the column indices to the corresponding column names.
inline string dim_caption(
		dim_t d,
		bool has_map,
		map<uint32_t, string> mapping) {

	stringstream ss;
	for(auto const& pr : d) {
		if(has_map)	ss << mapping[pr.first];
		else		ss << pr.first;
		ss << " = " << pr.second << " ";
	}
	return ss.str();
}

// Prints a caption for a given aggregator.
inline string aggr_caption(
		string aggr_str,
		bool has_map,
		map<uint32_t, string> mapping) {

	// Parse the aggregator construction string.
	smatch match;
	sregex base_re = *_s >> (s1 = +_d) >> +_s >> (s2 = +_) >> eos;
	if(!regex_match(aggr_str, match, base_re))
		throw string("Unrecognize aggregator string \"") + aggr_str + "\".";

	uint32_t field;
	stringstream fieldss;
	fieldss << match[1];
	fieldss >> field;

	if(fieldss.fail())
		throw string("Failed parsing the field mapping of ana ggregator.");

	// Produce the result string.
	stringstream ss;

	if(has_map)
		ss << match.str(2) << "(" << mapping[field] << ") ";
	else
		ss << match.str(2) << "(" << field << ") ";

	return ss.str();
}

// Prints a row based on a range of the groupping results and a vector of the
// column defining dimensions upon which the results are to be stretched.
// If the given row doesn't contain a group for the given column (that may be
// present in another row) this feature allows to gracefully provide some
// bummer gap fillers which in turn allows to avoid confusion if values get
// placed in a wrong column, just because its real value was missing.
// A dimension offset is provided here as the row and column dimensions will be
// 0 and 1 or 1 and 2 depending on whether the pages dimension is present or not.
template<class It>
void print_row(It grp_begin, It grp_end,
		uint32_t dim_offset,
		vector<dim_t> const& sorted_columns,
		ostream& out,
		arguments const& args) {

	// Iterate over a sorted list of the columns.
	for(auto const& c : sorted_columns) {

		// Determine the group to be placed in this column.
		It grp;
		for(grp = grp_begin; grp != grp_end; ++grp)
			if(group_dim(*grp, args.dimensions[dim_offset + 1]) == c)
				break;

		if(grp == grp_end)
			out << "x" << args.delim;

		else 
			for(auto const& pr : grp->aggregators)
				out << pr.second << args.delim;
	}

	out << endl;
}

// Prints a page of the pivot table based on a range of the groupping results.
// The dimension offset argument prepare the function to work with the data that
// may optionally have the page dimension defined which comes at the beginning
// of the dimensions list. Therefore its presence or absence shifts the rest
// of the dimensions (the rows and columns) one way or another.
template<class It>
void print_page(It grp_begin, It grp_end,
		uint32_t dim_offset,
		bool has_map,
		map<uint32_t, string> mapping,
		ostream& out,
		arguments const& args) {

	// Determine the page's columns.
	// -----------------------------
	set<dim_t> col_set;
	for(auto grp_it = grp_begin; grp_it != grp_end; ++grp_it)
		col_set.insert(group_dim(*grp_it, args.dimensions[dim_offset + 1]));

	vector<dim_t> sorted_columns(begin(col_set), end(col_set));
	sort(begin(sorted_columns), end(sorted_columns),
		[&args](dim_t const& lhs, dim_t const& rhs) {
		
			uint32_t num_defs = lhs.size();	

			if(num_defs != rhs.size())
				throw string("Attempted comparing dimensionf"
					" of differend sizes");

			for(uint32_t i = 0; i < num_defs; ++i) {
				if(lhs.at(i).first != rhs.at(i).first)
					return lhs.at(i).first < rhs.at(i).first;

				if(lhs.at(i).second != rhs.at(i).second)
					return lhs.at(i).second < rhs.at(i).second;
			}

			return false;
		});

	// Print the column captions.
	// --------------------------

	if(args.print_headers) {
		// Print the row caption header.
		for(uint32_t col : args.dimensions[dim_offset])
			if(has_map)
				out << mapping[col] << " ";
			else
				out << col << " ";
		out << args.delim;
		
		// Print all the column captions.
		for(dim_t const& d : sorted_columns)
			for(string const& a : args.aggr_strs)
				out << dim_caption(d, has_map, mapping) << ' '
					<< aggr_caption(a, has_map, mapping)
					<< args.delim;
		out << endl;
	}

	// Print all rows.
	// ---------------
	auto row_begin = grp_begin;
	auto it = grp_begin;

	dim_t current_row = group_dim(*it, args.dimensions[dim_offset]);
	do {
		dim_t group_row = group_dim(*it, args.dimensions[dim_offset]);
		if(group_row != current_row) {
			if(args.print_headers) {
				for(auto const& pr : current_row)
					out << pr.second << " ";
				out << args.delim;
			}
			print_row(row_begin, it, dim_offset, sorted_columns, out, args);

			row_begin = it;
			current_row = group_dim(*it, args.dimensions[dim_offset]);
		}

	} while((++it) != grp_end);

	if(args.print_headers) {
		for(auto const& pr : current_row)
			out << pr.second << " ";
		out << args.delim;
	}
	print_row(row_begin, it, dim_offset, sorted_columns, out, args);
}

// Prints a pivot table based on the groupper or rather its resulting grouppings.
void print_table(groupby::groupper const& g,
		bool has_map,
		map<uint32_t, string> mapping,
		ostream& out,
		arguments const& args) {

	auto sorted_groups = sort_group_results(g.copy_result(), args);

	if(args.dimensions.size() == 2) {

		// Print just one page.
		// --------------------
		print_page(begin(sorted_groups),
				end(sorted_groups),
				0,
				has_map,
				mapping,
				out,
				args);

	} else if(args.dimensions.size() == 3) {

		// Print all the pages.
		// --------------------

		auto page_begin = begin(sorted_groups);
		auto it = begin(sorted_groups);

		dim_t current_page = group_dim(*it, args.dimensions[0]);
		do {
			dim_t group_page = group_dim(*it, args.dimensions[0]);

			// Check if we've entered another page.
			if(group_page != current_page) {

				// Print current page.
				if(args.print_headers)
					out << "Page: "
					    << dim_caption(current_page, has_map, mapping)
					    << endl;

				print_page(page_begin,
						it,
						1,
						has_map,
						mapping,
						out,
						args);

				// Begin another page.
				page_begin = it;
				current_page = group_dim(*it, args.dimensions[0]);
			}

		} while((++it) != end(sorted_groups));

		// Finalize the remaining page.
		if(args.print_headers)
			out << "Page: "
			    << dim_caption(current_page, has_map, mapping)
			    << endl;

		print_page(page_begin,
				it,
				1,
				has_map,
				mapping,
				out,
				args);

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
		print_table(g, args.expect_data_header, mapping, cout, args);

		return 0;

	} catch(string ex) {
		cout << "Error : " << ex << endl;
		return 1;
	}
}
