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

vector<uint32_t> parse_dim_arg(const string& arg) {

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
map<uint32_t, string> process_header(istream& in, const arguments& args) {
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
		const arguments& args) {

	// Flatten the dimension definitions.
	vector<uint32_t> groupbys;
	for(const auto& v : args.dimensions)
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

void print_table(groupby::groupper const& g, ostream& out, const arguments& args) {

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
