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
using std::cin;
using std::cout;
using std::endl;

#include <sstream>
using std::stringstream;

#include <string>
using std::string;

#include <unistd.h>

#include "histogram.h"

const string usage("Usage: histogram [-w bucket-width]");

struct arguments {
	double bucket_size;
	char delim;
};

// Returns the bucket size parsed from the input.
// On error throws a message as an exception.
arguments parse_args(int argc, char** argv) {

	arguments args;
	args.bucket_size = 1.0;
	args.delim = '\t';
	stringstream bucketss;

	int c;
	while((c = getopt(argc, argv, "d:w:")) != -1) {
		switch(c) {
		case 'd':
			if(string(optarg).size() != 1)
				throw string("Delimiterm ust be given by a single character");
			args.delim = optarg[0];
			if(!isprint(args.delim) && args.delim != '\t')
				throw string("Illegal character requested as a delimiter");
			break;

		case 'w':
			bucketss << optarg;
			bucketss >> args.bucket_size;
			if(bucketss.fail())
				throw string("Failed parsing the bucket width argument.");
			break;

		case '?':
			throw string("Options require arguments.");

		default:
			throw usage;
		}
	}

	return args;
}

// Reads numbers from stdin and stuffs them in the histogram.
void process_input(hist::histogram& h, istream& in) {
	while(true) {
		double value;
		in >> value;

		if(in.eof())
			break;

		if(in.fail())
			throw string("Failed reading a number from stdin.");

		h.put(value);
	}
}

int main(int argc, char** argv) {

	// Don't print internal getopt error messages.
	opterr = 0;

	try {
		arguments args = parse_args(argc, argv);
		hist::histogram h(args.bucket_size);
		process_input(h, cin);
		for(const auto& pr : h.get_buckets())
			cout << pr.first << args.delim << pr.second << endl;

	} catch(string ex) {
		cout << ex << endl;
		return 1;
	}

	return 0;
}

