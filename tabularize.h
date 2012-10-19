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

#ifndef TABULARIZE_H
#define TABULARIZE_H

#include <map>
using std::map;

#include <set>
using std::set;

#include <string>
using std::string;

#include <sstream>
using std::stringstream;

#include <vector>
using std::vector;

#include <algorithm>
using std::find;

#include <memory>
using std::unique_ptr;

#include <functional>
using std::function;

#include <utility>
using std::pair;

#include <boost/functional/hash.hpp>
using boost::hash_combine;

#include <boost/unordered_map.hpp>
using boost::unordered_map;

#include "util.h"

namespace tab {

/// Coordinate binds a subset of the data table attributes and associates them
/// with concrete values. It may be vieved as a simple filter. Eg. Coordinate
/// may mean "All the data rows that have value x at column X, value y at
/// column Y, etc.
///
/// This class is used to determine the pivot table dimensions: pages, rows and
/// columns thus the "coordinate" name.
class coordinate {

	// Configuration.
	// --------------
	map<string, string> _labels;

	// The constructor is private as the objects will be created by a
	// factory method.
	coordinate(map<string, string> labels) : _labels(labels) {}

public:
	/// A factory method.
	/// The definitions are the column indices to define the coordinate.
	/// Row is the data row for the coordinate to be built with.
	static coordinate from_def_and_row(
			const vector<string>& definitions,
			const vector<string>& columns,
			const vector<string>& row) {

		// For each of the definition labels an according index is found
		// by searching the columns array. Then the data piece from the
		// row at the given index is associated with the given
		// definition label.

		map<string, string> labels;
		for(auto& def : definitions) {
			uint32_t index = index_of(columns, def);
			labels[def] = row[index];
		}

		return coordinate(labels);
	}

	/// Checks whether the coordinate has any labels assigned.
	bool defined() const {
		return !_labels.empty();
	}

	/// Concatenates all values. This may serve as a sort of the coordinate
	/// identifier.
	string get_value_string() const {
		stringstream ss;
		for(auto& pr : _labels)
			ss << pr.first << " = " << pr.second << ' ';
		return ss.str();
	}

	/// Checks whether a given data row matches the values defined by this
	/// coordinate.
	bool matches(const vector<string>& columns, const vector<string>& row) {
		for(auto& pr : _labels) {
			uint32_t index = index_of(columns, pr.first);
			if(pr.second != row.at(index))
				return false;
		}

		return true;
	}

	// Prepares the type to be stored in a hash based container.
	// ---------------------------------------------------------

	friend size_t hash_value(const coordinate& coord) {
		size_t seed = 0;
		for(auto& pr : coord._labels) {
			hash_combine(seed, pr.first);
			hash_combine(seed, pr.second);
		}
		return seed;
	}

	friend bool operator==(const coordinate& lhs, const coordinate& rhs) {
		return hash_value(lhs) == hash_value(rhs);
	}
};

/// Determines a position of a value in the table set.
class position {

	// Configuration.
	// --------------

	coordinate _page;
	coordinate _row;
	coordinate _column;
public:
	position(const coordinate& page,
		const coordinate& row,
		const coordinate& column)
	: _page(page)
	, _row(row)
	, _column(column) {}

	const coordinate& get_page() const { return _page; }
	const coordinate& get_row() const { return _row; }
	const coordinate& get_column() const { return _column; }

	// Prepares the type to be stored in a hash based container.
	// ---------------------------------------------------------

	friend size_t hash_value(const position& pos) {
		size_t seed = 0;
		hash_combine(seed, pos._page);
		hash_combine(seed, pos._row);
		hash_combine(seed, pos._column);
		return seed;
	}

	friend bool operator==(const position& lhs, const position& rhs) {
		return hash_value(lhs) == hash_value(rhs);
	}
};

class table {

	// Configuration.
	// --------------
	vector<string> _page_fields;
	vector<string> _column_fields;
	vector<string> _row_fields;

	// State.
	// ------

	/// This is a map of the data fields: position -> column -> value.
	unordered_map<position, map<string, string>> _data;

public:
	table(vector<string> page_fields,
	      vector<string> column_fields,
	      vector<string> row_fields)
	: _page_fields(page_fields)
	, _column_fields(column_fields)
	, _row_fields(row_fields) {}

	void consume_row(const vector<string>& columns, const vector<string>& row) {

		// Establish the position of the given data row.
		position pos = row_position(columns, row);

		// If the position has not been visited yet,
		/// initialize aggregators there.
		if(_data.find(pos) == end(_data))
			_data[pos] = map<string, string>();

		add_data_at(columns, row, pos);
	}

	void for_each_valuemap(function<void(
				const position&,
				const map<string, string>&)> f) const {

		for(const auto& pr : _data)
			f(pr.first, pr.second);
	}

private:
	position row_position(const vector<string>& columns,
			      const vector<string>& row) const {
		coordinate p = coordinate::from_def_and_row(_page_fields, columns, row);
		coordinate c = coordinate::from_def_and_row(_column_fields, columns, row);
		coordinate r = coordinate::from_def_and_row(_row_fields, columns, row);
		return position(p, r, c);
	}
	
	void add_data_at(const vector<string>& columns,
			 const vector<string>& row,
			 const position& pos) {

		for(uint32_t i = 0; i < row.size(); ++i) {

			// See if the index is already defined as a label column.
			if(find(begin(_page_fields), end(_page_fields), columns.at(i)) 
				!= end(_page_fields)) {
				continue;
			}
			if(find(begin(_column_fields), end(_column_fields), columns.at(i)) 
				!= end(_column_fields)) {
				continue;
			}
			if(find(begin(_row_fields), end(_row_fields), columns.at(i)) 
				!= end(_row_fields)) {
				continue;
			}

			// If we've got here, the index is in the set of the
			// data indices.
			map<string, string>::iterator _;
			bool success;
			tie(_, success) = _data[pos].insert(make_pair(columns.at(i), row.at(i)));

			// Fail on duplicate. Uniqueness of these must be provided by the user.
			if(!success)
				throw string("Multiple values requested to be put in a single cell.");
		}
	}
};

}

#endif
