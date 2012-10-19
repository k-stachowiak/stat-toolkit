#ifndef UTIL_H
#define UTIL_H

#include <cstring>
using std::strcpy;

#include <algorithm>
using std::find;

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
