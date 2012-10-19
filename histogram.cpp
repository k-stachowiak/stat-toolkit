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

#include <limits>
using std::numeric_limits;

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

// Returns the bucket size parsed from the input.
// On error throws a message as an exception.
double parse_args(int argc, char** argv) {

	int c;
	double bucket_size = 1.0;
	stringstream bucketss;

	while((c = getopt(argc, argv, "w:")) != -1) {
		switch(c) {
		case 'w':
			bucketss << optarg;
			bucketss >> bucket_size;
			if(bucketss.fail())
				throw string("Failed parsing the bucket width argument.");
			break;
		case '?':
			throw string("Option -w requires a bucket width argument.");

		default:
			throw usage;
		}
	}

	return bucket_size;
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

// Processes the buckets and prints appropriate result.
void print_results(const map<double, double>& buckets, double bucket_size, ostream& out) {

	double prev_index = numeric_limits<double>::infinity();

	// Note that it is assumed here (correctly) that
	// std::map sorts its contents internally.
	for(auto& pr : buckets) {

		double bucket_index = pr.first;

		// Enter zeros for omitted buckets.
		for(double i = prev_index + 1; i < bucket_index; i += 1.0)
			out << i * bucket_size << '\t' << 0.0 << endl;
	
		// Entry for the current bucket.
		out << bucket_index * bucket_size << '\t' << pr.second << endl;

		prev_index = bucket_index;
	}

}

int main(int argc, char** argv) {

	// Don't print internal getopt error messages.
	opterr = 0;

	double bucket_size;
	try {
		bucket_size = parse_args(argc, argv);
		hist::histogram h(bucket_size);
		process_input(h, cin);
		print_results(h.get_buckets(), bucket_size, cout);

	} catch(string ex) {
		cout << ex << endl;
		return 1;
	}

	return 0;
}

