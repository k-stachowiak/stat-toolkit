/* 
 * This file is part of stat-toolkit.
 * 
 * stat-toolkit is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 3 of the License, or
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
using std::cin;
using std::cout;
using std::endl;

#include <vector>
using std::vector;

#include <string>
using std::string;

#include <unistd.h>

#include "tabularize.h"

/*
 * TODO:
 * - make the value expansion's direction configurable so that by default
 *   additional values spawn additional columns, but in the altered mode
 *   the additional rows are generated.
 */

struct arguments {

	/// The field separator.
	char delim;

	/// Coordinate fields.
	vector<uint32_t> page_fields;
	vector<uint32_t> row_fields;
	vector<uint32_t> column_fields;
};

arguments parse_args(int argc, char** argv) {
	arguments args;
	args.delim = '\t'; // The default delimiter.
	int c;
	while((c = getopt(argc, argv, "c:p:r:")) != -1) {
		stringstream ss;
		uint32_t index;
		switch(c) {
		case 'c':
			ss << optarg;
			ss >> index;
			if(ss.fail())
				throw string("Failed parsing a column index option");
			args.column_fields.push_back(index);
			break;

		case 'd':
			if(string(optarg).size() != 1)
				throw string("The delimiter is expected to be a single character.");
	
			args.delim = optarg[0];

			if(!isprint(args.delim) && args.delim != '\t')
				throw string("Cannot use the given character as a delimiter.");

			break;

		case 'p':
			ss << optarg;
			ss >> index;
			if(ss.fail())
				throw string("Failed parsing a page index option");
			args.page_fields.push_back(index);
			break;

		case 'r':
			ss << optarg;
			ss >> index;
			if(ss.fail())
				throw string("Failed parsing a row index option");
			args.row_fields.push_back(index);
			break;

		case '?':
			throw string("Options require an index argument.");

		default:
			throw string("Illegal option -") + (char)optopt + ".";
		}
	}

	return args;
}

tab::table build_table(istream& in, const arguments& args) {

	string line;

	// Read the column headers.
	getline(in, line);
	vector<string> columns = split(line, args.delim);
	if(in.fail())
		throw string("Failed reading the column headers.");

	vector<string> page_columns;
	for(uint32_t i : args.page_fields)
		page_columns.push_back(columns[i]);

	vector<string> row_columns;
	for(uint32_t i : args.row_fields)
		row_columns.push_back(columns[i]);

	vector<string> column_columns;
	for(uint32_t i : args.column_fields)
		column_columns.push_back(columns[i]);

	// Initialize the result object.
	tab::table tbl(page_columns, column_columns, row_columns);
	
	// Read the data rows.
	while(true) {
		getline(in, line);
		if(!in.good())
			break;

		vector<string> row = split(line, args.delim);
		tbl.consume_row(columns, row);
	}

	return tbl;
}

void print_table(const tab::table& tbl, ostream& out) {

	// TODO: Implement according to the algoritnm in the comments below.

	// Reorganize data for printing.
	// -----------------------------

	// Generate the header row.
	// ------------------------
	
	// Print all the pages.
	// --------------------

	tbl.for_each_valuemap([](const tab::position& pos,
				 const vector<pair<string, string>>& data) {

		if(pos.get_page().defined())
			cout << "p " << pos.get_page().get_value_string() << '\t';

		cout << "r " << pos.get_row().get_value_string() << '\t';
		cout << "c " << pos.get_column().get_value_string() << '\t';

		for(auto& pr : data) {
			cout << "v " << pr.first << " = " << pr.second << '\t';
		}
		cout << endl;
	});
}

int main(int argc, char** argv) {

	opterr = 0;

	try {
		// Parse and validate the command line options.
		arguments args = parse_args(argc, argv);
		if(args.column_fields.empty() ||
		   args.row_fields.empty()) {
			throw string("Missing column, or row index definitions.");
		}

		// Process input.
		tab::table tbl = build_table(cin, args);

		// Print output.
		print_table(tbl, cout);
		
		return 0;

	} catch(string ex) {
		cout << "Error : " << ex << endl;
		return 1;
	}
}

