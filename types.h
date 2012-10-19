#ifndef TYPES_H 
#define TYPES_H

#include <map>
using std::map;

#include <sstream>
using std::stringstream;

#include <string>
using std::string;

#include <vector>
using std::vector;

#include <algorithm>
using std::find;

#include <boost/functional/hash.hpp>
using boost::hash_combine;

#include "aggr.h"

namespace types {

	// Coordinate binds a subset of the data table attributes and associates them
	// with concrete values. It may be vieved as a simple filter.
	// Eg. Coordinate may mean "All the data rows that have value x at column X,
	// value y at column Y, etc.
	//
	// This class is used to determine the pivot table dimensions: pages, rows and columns
	// thus the "coordinate" name.
	class coordinate {

		// Configuration.
		map<string, string> _values;

		// The constructor is private as the objects
		// will be created by a factory method.
		coordinate(map<string, string> values) : _values(values) {}

	public:
		// A factory method.
		// The definitions are the columns to define the coordinate.
		// Columns is the list of the table's column captions.
		// Row is the data row for the coordinate to be built with.
		static coordinate from_def_and_row(
				const vector<string>& definitions,
				const vector<string>& columns,
				const vector<string>& row) {

			// For each of the definition labels an according index is found
			// by searching the columns array. Then the data piece from the
			// row at the given index is associated with the given definition
			// label.

			map<string, string> values;
			for(auto& def : definitions) {
				int index = find(begin(columns), end(columns), def) - begin(columns);
				values[def] = row[index];
			}

			return coordinate(values);
		}

		// Concatenates all values. This may serve as a sort of the
		// coordinate identifier.
		string get_value_string() const {
			stringstream ss;
			for(auto& pr : _values)
				ss << pr.second << ' ';
			return ss.str();
		}

		// Checks whether a given data row matches the values defined by this coordinate.
		bool matches(const vector<string>& columns, const vector<string>& row) {
			for(auto& pr : _values) {
				int index = find(begin(columns), end(columns), pr.first) - begin(columns);
				if(pr.second != row[index])
					return false;
			}
			return true;
		}

		// Prepares the type to be stored in a hash based container.
		// ---------------------------------------------------------

		friend size_t hash_value(const coordinate& coord) {
			size_t seed = 0;
			for(auto& pr : coord._values) {
				hash_combine(seed, pr.first);
				hash_combine(seed, pr.second);
			}
			return seed;
		}

		friend bool operator==(const coordinate& lhs, const coordinate& rhs) {
			return hash_value(lhs) == hash_value(rhs);
		}
	};
}

#endif
