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

#include <iostream>
using std::istream;
using std::ostream;
using std::cout;
using std::cin;
using std::endl;

#include <string>
using std::string;

#include <vector>
using std::vector;

#include <memory>
using std::unique_ptr;
using std::move;

#include <map>
using std::map;

#include <unistd.h>

#include "util.h"
#include "groupby.h"

// The input arguments analysis.
// =============================

/// The object that represents the inputa rguments of the program.
struct arguments {

	/// The field delimiter - default: tab character.
	char delim;

	/// Groupping fields.
	vector<uint32_t> groupbys;

	/// The aggregator definitions. Format:
	/// [(field_index, aggregator)]
	vector<string> aggr_strs;
};

/// Parses the program arguments building a proper object that reflects them.
arguments parse_args(int argc, char** argv) {

	arguments args;
	args.delim = '\t'; // Providing the default delimiter.
	stringstream converter;
	uint32_t index;

	int c;
	while((c = getopt(argc, argv, "a:d:g:")) != -1) {
		switch(c) {
		case 'a':
			args.aggr_strs.emplace_back(optarg);
			break;

		case 'd':
			if(string(optarg).size() != 1)
				throw string("The delimiter is expected to be a single character.");
	
			args.delim = optarg[0];

			if(!isprint(args.delim) && args.delim != '\t')
				throw string("Cannot use the given character as a delimiter.");

			break;

		case 'g':
			converter.seekg(0);
			converter.seekp(0);
			converter << optarg;
			converter >> index;
			if(converter.fail())
				throw string("Failed parsing groupping field index.");
			args.groupbys.push_back(index);
			break;

		case '?':
			if(optopt == 'a')
				throw string("Option -a requires an aggregator argument.");

			if(optopt == 'd')
				throw string("Option -d requires a delimiter argument.");

			if(optopt == 'g')
				throw string("Option -g requires a groupper argument.");

			// Notice a fallthrough. It is here although it should
			// not happen unless someone changes the getopt options definition.

		default:
			throw string("Illegal option -") + (char)optopt + ".";
		}
	}

	return args;
}

// The aggregation phase.
// ======================

/// Fetches the data from the input stream and finally prints out
/// the aggregations to the provided output stream.
groupby::groupper process_stream(istream& in, const arguments& args) {

	groupby::groupper groupper(args.groupbys, args.aggr_strs);
	string line;

	while(true) {
		getline(in, line);
		if(!in.good())
			break;

		vector<string> row = split(line, args.delim);
		groupper.consume_row(row);
	}

	return groupper;
}

// The result printing phase.
// ==========================

void print_results(groupby::groupper groupper,
		ostream& out, 
		const arguments& args) {

	// Print the groupping report.
	// ---------------------------

	// Print the header row.
	for(uint32_t g : args.groupbys)
		out << g << args.delim;

	for(uint32_t i = 0; i < args.aggr_strs.size(); ++i) {
		out << '"' << args.aggr_strs.at(i) << '"';
		if(i < (args.aggr_strs.size() - 1))
			out << args.delim;
	}

	out << endl;

	// Print the groups.
	groupper.for_each_group([&out,&args](const groupby::group& g) {
		for(const auto& d : g.get_definition())
			out << d.second << args.delim;

		uint32_t aggr_size = g.get_aggregators().size();
		for(uint32_t i = 0; i < aggr_size; ++i) {
			out << g.get_aggregators().at(i).second->get();
			if(i < (aggr_size - 1))
				out << args.delim;
		}
		out << endl;
	});
}

int main(int argc, char** argv) {

	// Don't print internal getopt error messages.
	opterr = 0;

	try {
		// Read and validate the arguments.
		arguments args = parse_args(argc, argv);
		if(args.groupbys.empty() || args.aggr_strs.empty())
			throw string("Missing groupping or aggregation definitions.");

		// Process the input stream.
		auto groupper = process_stream(cin, args);

		// Print the results.
		print_results(move(groupper), cout, args);

		return 0;

	} catch(string& ex) {
		cout << "Error : " << ex << endl;
		return 1;
	}
}

