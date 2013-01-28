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
using std::cin;
using std::cout;
using std::endl;

#include <string>
using std::string;

#include "aggr.h"

const string usage("Usage: aggr <aggr-constr-str>");

int main(int argc, char** argv) {
	try {
		if(argc != 2)
			throw usage;

		auto aggr = aggr::create_from_string(argv[1]);

		while(true) {
			double value;
			cin >> value;

			if(cin.fail())
				break;

			aggr->put(value);
		}

		cout << aggr->get() << endl;

		return 0;
	} catch(string& ex) {
		cout << "Error: " << ex << endl;
		return 1;
	}
}
