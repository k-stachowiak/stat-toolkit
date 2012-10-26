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

#ifndef UTIL_H
#define UTIL_H

#include <cstring>
using std::strcpy;

#include <algorithm>
using std::find;

#include <vector>
using std::vector;

#include <string>
using std::string;

/// Splits a string by a given delimiter.
vector<string> split(string str, char delim) {

	vector<string> result;
	string delims = string() + delim;
	char original[str.size() + 1];
	strcpy(original, str.c_str());

	char* token = strtok(original, delims.c_str());
	while(token) {
		result.emplace_back(token);
		token = strtok(0, delims.c_str());
	}
	
	return result;
}

/// Finds an index of a value in a givem collection.
/// Note that there is no bounds checking at the moment.
template<class COLLECTION, class VALUE_TYPE>
uint32_t index_of(const COLLECTION& col, const VALUE_TYPE& val) {
	return find(begin(col), end(col), val) - begin(col);
}

#endif
