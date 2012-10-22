/* Copyright (C) 2012 Krzysztof Stachowiak */

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

/// @brief The structure for convenient storage of the input arguments.
struct arguments {

	/// The field separator.
	char delim;

	vector<uint32_t> page_fields;	///< The page field definitions.
	vector<uint32_t> row_fields;	///< The row field definitions.
	vector<uint32_t> column_fields;	///< The column field definitions.
};

/// @brief A finction that processes the input arguments and stores them in
/// 	a single structure.
///
/// @param[in] argc Classical arguments count.
/// @param[in] argv Classical arguments vector.
arguments parse_args(int argc, char** argv) {
	arguments args;
	args.delim = '\t'; // The default delimiter.
	int c;
	while((c = getopt(argc, argv, "c:p:r:d:")) != -1) {
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

/// @brief The function that builds a table based on the input stream.
///
/// @param[in] in The input stream to be processed.
/// @param[in] args The structire storing the input arguments.
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
\
	return tbl;
}

/// @brief Prints the output table/
///
/// @param[in] tbl The table to be printed.
/// @param[in] out The output stream to write the result to.
/// @param[in] args The structure with theinput arguments.
void print_table(const tab::table& tbl, ostream& out, const arguments& args) {

	// Reorganize data for printing.
	// -----------------------------
	unordered_map<tab::coordinate,
		unordered_map<tab::coordinate,
			unordered_map<tab::coordinate,
				vector<pair<string, string>>>>> output;

	tbl.for_each_valuemap([&output](const tab::position& pos,
					const map<string, string>& vals) {
		auto& vec = output[pos.get_page()][pos.get_row()][pos.get_column()];
		for(auto& pr : vals)
			vec.push_back(pr);
	});

	// Print all the pages.
	// --------------------
	for(auto& page_rest : output) {

		// Determine and handle the current page.
		const tab::coordinate& page = page_rest.first;
		if(page.defined())
			out << "Page : " << page.get_value_string() << endl;

		// Generate the page ehader.
		vector<string> header;
		auto& row_rest_1 = *begin(page_rest.second);
		for(auto& col_vals : row_rest_1.second) {
			string col_str = col_vals.first.get_value_string();
			for(auto& key_val : col_vals.second) {
				stringstream ss;
				ss << '"' << col_str << ' ' << key_val.first << '"';
				header.push_back(ss.str());
			}
		}

		out << args.delim;
		for(auto& str : header)
			out << str << args.delim;
		out << endl;

		// Process the rows.
		for(auto& row_rest : page_rest.second) {

			// Print the row label.
			string row_str = row_rest.first.get_value_string();
			out << row_str << args.delim;

			// Print the data values.
			for(auto& col_vals : row_rest.second)
				for(auto& key_val : col_vals.second)
					out << key_val.second << args.delim;

			out << endl;
		}
		out << endl;
	}
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
		print_table(tbl, cout, args);
		
		return 0;

	} catch(string ex) {
		cout << "Error : " << ex << endl;
		return 1;
	}
}

