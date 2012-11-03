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

#include <cstdio>

#include <iostream>
using std::istream;
using std::ostream;
using std::cin;
using std::cout;
using std::endl;

#include <set>
using std::set;

#include <cstring>
using std::strcpy;

#include <boost/xpressive/xpressive.hpp>
using boost::xpressive::sregex;
using boost::xpressive::smatch;
using boost::xpressive::s1;
using boost::xpressive::s2;
using boost::xpressive::_d;
using boost::xpressive::_s;
using boost::xpressive::_;
using boost::xpressive::eos;

#include <boost/unordered_set.hpp>
using boost::unordered_set;

#include "util.h"
#include "aggr_array.h"

#include <unistd.h>

struct arguments {
	char delim;
	bool extend_rows;
	bool print_headers;
	bool expect_data_header;
	vector<vector<uint32_t>> dimensions;
	vector<pair<uint32_t, string>> aggr_map;
};

pair<uint32_t, string> parse_aggr_arg(string arg) {

	smatch match;
	sregex re = (s1 = +_d) >> +_s >> (s2 = +_) >> eos;

	if(!regex_match(arg, match, re)) {
		stringstream errorss;
		errorss << "Failed parsing an aggregator argument \"" << arg << "\".";
		throw errorss.str();
	}

	uint32_t index;
	stringstream indexss;
	indexss << match[1];
	indexss >> index;
	if(indexss.fail()) {
		stringstream errorss;
		errorss << "Failed parsing the index part of the aggregator arument \""
			<< arg << "\".";
		throw errorss.str();
	}

	string aggr_str(match[2]);

	return make_pair(index, aggr_str);
}

vector<uint32_t> parse_dim_arg(string arg) {

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
	args.extend_rows = false;
	args.print_headers = false;

	int c;
	while((c = getopt(argc, argv, "a:d:D:RhH")) != -1) {
		switch(c) {
		case 'a':
			args.aggr_map.push_back(parse_aggr_arg(optarg));
			break;

		case 'd':
			if(string(optarg).size() != 1)
				throw string("The delimiter is expected to be a single character.");

			args.delim = optarg[0];

			if(!isprint(args.delim) && args.delim != '\t')
				throw string("Cannot use the given character as a delimiter.");

			break;

		case 'D':
			args.dimensions.push_back(parse_dim_arg(optarg));
			break;

		case 'R':
			args.extend_rows = true;
			break;

		case 'h':
			args.print_headers = true;
			break;

		case 'H':
			args.expect_data_header = true;
			break;

		case '?':
			if(optopt == 'a')
				throw string("Option -a requires an aggregator argument.");

			if(optopt == 'd')
				throw string("Option -d requires a delimiter argument.");

			if(optopt == 'D')
				throw string("Option -D requires a dimension argument.");

			// Notice a fallthrough. It is here although it should
			// not happen unless someone changes the getopt options definition.

		default:
			throw string("Illegal option -") + (char)optopt + ".";
		}
	}

	return args;
}

map<uint32_t, string> process_header(istream& in, const arguments& args) {
	map<uint32_t, string> result;
	string line;
	getline(in, line);
	vector<string> row = split(line, args.delim);
	for(uint32_t i = 0; i < row.size(); ++i)
		result[i] = row[i];
	return result;
}

void process_stream(
		istream& in,
		const map<uint32_t, string>& column_map,
		const arguments& args,
		aggr_arr::array& arr) {

	string line;
	while(true) {
		getline(in, line);
		if(!in.good())
			break;
		vector<string> row = split(line, args.delim);
		arr.consume_row(row, args.expect_data_header, column_map);
	}
}

template<class COLLECTION>
vector<aggr_arr::coord> sort_coords(const COLLECTION& collection) {
	vector<aggr_arr::coord> result(begin(collection), end(collection));
	sort(begin(result), end(result), aggr_arr::coord_compare);
	return result;
}

void print_page(const unordered_map<aggr_arr::coord,
			unordered_map<aggr_arr::coord,
				map<string, double>>>& page,
		ostream& out,
		const map<uint32_t, string>& column_map,
		const arguments& args) {

	// Gather all the dimensions.
	// --------------------------
	unordered_set<aggr_arr::coord> all_row_coords;
	unordered_set<aggr_arr::coord> all_column_coords;
	unordered_set<string> all_aggr_names;

	for(const auto& rcoord_columns : page) {
		all_row_coords.insert(rcoord_columns.first);
		for(const auto& ccoord_aggrs : rcoord_columns.second) {
			all_column_coords.insert(ccoord_aggrs.first);
			for(const auto& name_value : ccoord_aggrs.second)
				all_aggr_names.insert(name_value.first);
		}
	}

	// Print the values in the correct order.
	// --------------------------------------
	
	// Note that the end-lines will be printed more often for the case
	// of the extended rows which actually shouldn't be surprising at all.
	if(args.extend_rows) {

		// Print columns header.
		if(args.print_headers) {
			for(const auto& c : sort_coords(all_column_coords)) {
				if(args.expect_data_header)
					out << c.to_string(column_map) << args.delim;
				else
					out << c.str() << args.delim;
			}
			out << endl;
		}

		// Print data.
		for(const auto& r : sort_coords(all_row_coords)) {
			for(const auto& a : all_aggr_names) {

				// Row header.
				if(args.print_headers) {
					if(args.expect_data_header)
						out << r.to_string(column_map) << "; "
							<< a << args.delim;
					else
						out << r.str() << "; " << a << args.delim;
				}
				
				// Row data.
				for(const auto& c : all_column_coords)
					out << page.at(r).at(c).at(a) << args.delim;

				// Note that we do endl for each aggregator here.
				out << endl;
			}
		}
	} else {
		
		// Pront columns header.
		if(args.print_headers) {
			for(const auto& c : sort_coords(all_column_coords)) {
				for(const auto& a : all_aggr_names) {
					if(args.expect_data_header)
						cout << c.to_string(column_map) << "; "
							<< a << args.delim;
					else
						cout << c.str() << "; " << a << args.delim;
				}
			}
			out << endl;
		}

		// Print data.
		for(const auto& r : sort_coords(all_row_coords)) {

			// Row header.
			if(args.print_headers) {
				if(args.expect_data_header)
					out << r.to_string(column_map) << args.delim;
				else
					out << r.str() << args.delim;
			}

			// Print data.
			for(const auto& c : sort_coords(all_column_coords)) {
				for(const auto& a : all_aggr_names) {
					out << page.at(r).at(c).at(a) << args.delim;
				}
			}

			// Note that we do endl for each row here.
			out << endl;
		}
	}
}

void print_results(
		const aggr_arr::array& arr,
		ostream& out,
		const map<uint32_t, string>& column_map,
		const arguments& args) {

	// Handle the case with the pages and without them separately.
	
	// The case with pages.
	if(args.dimensions.size() == 3) {

		// Build a 3D map.
		unordered_map<aggr_arr::coord,
			unordered_map<aggr_arr::coord,
				unordered_map<aggr_arr::coord,
					map<string, double>>>> aggrs;

		// Put the results in the proper cells.
		arr.for_each_aggr([&aggrs](
				const aggr_arr::position& pos,
				const string& aggr_name,
				double aggr_value) {
			const auto& page = pos.coordinate(0);
			const auto& row = pos.coordinate(1);
			const auto& column = pos.coordinate(2);
			aggrs[page][row][column].insert(make_pair(aggr_name, aggr_value));
		});

		// Print each page separately.
		for(const auto& c_page : aggrs) {
			out << "Page " << c_page.first.str() << endl;
			print_page(c_page.second, out, column_map, args);
		}

	// The case without pages.
	} else {
		// 2 dimensions assumed due to the check from outside this
		// function.

		// Build a 2D map.
		unordered_map<aggr_arr::coord,
			unordered_map<aggr_arr::coord,
				map<string, double>>> aggrs;

		// Put the results in the proper cells.
		arr.for_each_aggr([&aggrs](
				const aggr_arr::position& pos,
				const string& aggr_name,
				double aggr_value) {
			const auto& row= pos.coordinate(0);
			const auto& column = pos.coordinate(1);
			aggrs[row][column].insert(make_pair(aggr_name, aggr_value));
		});

		// Only one page to be printed.
		print_page(aggrs, out, column_map, args);
	}
}

int main(int argc, char** argv) {

	// Don't print internal getopt error messages.
	opterr = 0;

	try {
		// Parse and validate the arguments.
		arguments args = parse_args(argc, argv);
		if(args.aggr_map.empty())
			throw string("At leas one aggregator must be defined.");

		if(args.dimensions.size() != 2 && args.dimensions.size() != 3)
			throw string("Only 2 or 3 dimensions are supported.");

		// Initialize the array.
		aggr_arr::array arr(args.dimensions, args.aggr_map);

		// Process header if required.
		// Note that if the mapping is not expected the variable will
		// be empty, yet it is still passed to maintain the concise
		// interface of the further processing procedures.
		map<uint32_t, string> mapping;
		if(args.expect_data_header)
			mapping = process_header(cin, args);
		
		// Fill the array with the data from the stdin.
		process_stream(cin, mapping, args, arr);

		// Format and print the results.
		print_results(arr, cout, mapping, args);

		return 0;
	
	} catch(string ex) {
		cout << "Error : " << ex << endl;
		return 1;
	}

	return 0;
}
