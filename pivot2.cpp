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

#include "groupby.h"

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

// Handle the command line arguments.
// ----------------------------------

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
int main(int argc, char** argv) {
	return 0;
}
