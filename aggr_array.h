#ifndef AGGR_ARRAY_H
#define AGGR_ARRAY_H

#include <map>
using std::map;

#include <string>
using std::string;

#include <vector>
using std::vector;

#include <utility>
using std::pair;

#include <functional>
using std::function;

#include <iostream>
using std::cout;
using std::endl;

#include <boost/functional/hash.hpp>
using boost::hash_combine;

#include <boost/unordered_map.hpp>
using boost::unordered_map;

#include "util.h"
#include "aggr.h"

namespace aggr_arr {

/// @brief Defines one of the bucket's coordinates.
class coord {
	/// The label values definint the coordinate.
	map<uint32_t, string> _label_map;

	/// @brief Private constructor.
	coord(const map<uint32_t, string>& label_map)
	: _label_map(label_map) {}

public:
	/// @brief Main factory method.
	///
	/// @param[in] label_map def The column indices to define the coordinate.
	/// @param[in] columns The columns of the original data set.
	/// @param[in] row The data row for the coordinate to be built with.
	static coord from_def_row(
			const vector<uint32_t>& label_map_def,
			const vector<string>& row) {

		// For each of the definition labels an according index is found
		// by searching the columns array. Then the data piece from the
		// row at the given index is associated with the given
		// definition label.

		map<uint32_t, string> label_map;
		for(auto& def : label_map_def) {
			label_map[def] = row[def];
		}

		return coord(label_map);
	}

	/// @brief Generates a string representation of this object.
	///
	/// @returns The string representation of this object.
	string str() const {
		stringstream ss;
		for(const auto& pr : _label_map)
			ss << "col " << pr.first << " = " << pr.second << ", ";
		return ss.str();
	}

	/// @brief Generates a string representation of this object based on an
	///	externally provided column index to name mappings.
	///
	/// @param[in] mapping The column index to name mapping.
	///
	/// @returns The string representation of the object.
	string to_string(map<uint32_t, string> mapping) const {
		stringstream ss;
		for(const auto& pr : _label_map)
			ss << mapping[pr.first] << " = " << pr.second << ", ";
		return ss.str();
	}

	// Comparison operator for sorting.
	friend bool coord_compare(const coord& lhs, const coord& rhs) {
			
		// Define some helper symbols
		typedef map<uint32_t, string>::const_iterator lblmapit;
		lblmapit lb = begin(lhs._label_map);
		lblmapit le = end(lhs._label_map);
		lblmapit rb = begin(rhs._label_map);
		lblmapit re = end(rhs._label_map);

		stringstream converter;

		for(auto lit = lb, rit = rb; lit != le && rit != re; ++lit, ++rit) {

			string lstr = lit->second;
			string rstr = rit->second;

			// Try converting, otherwise compare lexicographically.
			bool converted = true;

			double ldbl;
			converter.seekg(0, std::ios_base::beg);
			converter.str(lstr);
			converter >> ldbl;
			if(!converter)
				converted = false;

			double rdbl;
			converter.seekg(0, std::ios_base::beg);
			converter.str(rstr);
			converter >> rdbl;
			if(!converter)
				converted = false;

			// Decide on the comparison domain.
			if(converted) {
				if(ldbl != rdbl)
					return ldbl < rdbl;

			} else {
				if(lstr != rstr)
					return lstr < rstr;
			}

		}

		// If we've got here then all the lbels must have been equal.
		return false;
	}

	// Prepares the type to be stored in a hash based container.
	// ---------------------------------------------------------

	/// @brief Computes hash value for the boost's unordered map.
	friend size_t hash_value(const coord& coord) {
		size_t seed = 0;
		for(auto& pr : coord._label_map) {
			hash_combine(seed, pr.first);
			hash_combine(seed, pr.second);
		}
		return seed;
	}

	/// @brief Determines the equality condition for the boost's unordered map.
	friend bool operator==(const coord& lhs, const coord& rhs) {
		return hash_value(lhs) == hash_value(rhs);
	}
};

bool coord_compare(const coord& lhs, const coord& rhs);

/// @brief Defines a multidimensional position of a bucket.
class position {
	/// The coords defining this position.
	vector<coord> _coords;

public:
	/// @brief Constructor allowing building of a position object
	///	from a list of coords.
	position(vector<coord> coords) {
		if(coords.size() == 0)
			throw string("Position must have at least one coord");

		for(const auto& coord : coords)
			_coords.push_back(coord);
	}

	const coord& coordinate(uint32_t index) const {
		return _coords.at(index);
	}

	/// @brief Generates a string representation of this object.
	///
	/// @returns The string representation of this object.
	string str() const {
		stringstream ss;
		for(uint32_t i = 0; i < _coords.size(); ++i) {
			ss << "coord(" << _coords.at(i).str() << ")";
			if(i < (_coords.size() - 1))
				ss << ", ";
		}
		return ss.str();
	}

	// Prepares the type to be stored in a hash based container.
	// ---------------------------------------------------------

	/// @brief Computes the hash value for the boost's unordered map.
	friend size_t hash_value(const position& pos) {
		size_t seed = 0;
		for(const auto& c : pos._coords)
			hash_combine(seed, c);
		return seed;
	}

	/// @brief Defines the equality condition for the boost's unordered map.
	friend bool operator==(const position& lhs, const position& rhs) {
		return hash_value(lhs) == hash_value(rhs);
	}
};

/// @brief The data organizing table
class array {
	// Configuration.
	// --------------
	
	/// The dimension defining columns the indexing is:
	///	...[dimension][column-index]
	vector<vector<uint32_t>> _dim_defs;

	/// The bucket mapping: consumed column ind. -> buck. constr. str.
	vector<pair<uint32_t, string>> _bucket_constrs;

	// State.
	// ------
	
	/// The map defining the bucket positions.
	unordered_map<position, map<string, aggr::ptr>> _buckets;

public:
	/// @brief Simple member by member constructor.
	array(const vector<vector<uint32_t>>& dim_defs,
	      vector<pair<uint32_t, string>> bucket_constrs)
	: _dim_defs(dim_defs)
	, _bucket_constrs(bucket_constrs)
	{}

	bool num_dimensions() const {
		return _dim_defs.size();
	}

	/// @brief Inserts data from a given row into this table.
	///
	/// @param[in] row The row to be inserted.
	void consume_row(const vector<string>& row,
			bool col_mapped,
			map<uint32_t, string> column_map) {

		stringstream ss;
		
		// Generate the positions associated with this row.
		// ------------------------------------------------
		vector<coord> base_coords;
		for(const auto& def : _dim_defs)
			base_coords.push_back(coord::from_def_row(def, row));

		// Initialize the position object.
		position pos(base_coords);

		// Read all the fields defined by the buckets.
		// -------------------------------------------
		for(const auto& pr : _bucket_constrs) {

			stringstream keyss;
			if(col_mapped)
				keyss << pr.second << "(" << column_map[pr.first] << ")";
			else
				keyss << pr.second << "(" << pr.first << ")";

			string key(keyss.str());

			// Construct bucket if necessary.
			if(_buckets.find(pos) == end(_buckets))
				_buckets[pos] = map<string, aggr::ptr>();

			if(_buckets[pos].find(key) == end(_buckets[pos]))
				_buckets[pos][key] = aggr::create_from_string(pr.second);

			// Feed the bucket with an according value.
			double value;
			ss.seekg(0);
			ss.seekp(0);
			ss << row[pr.first];
			ss >> value;

			if(ss.fail())
				throw string("Conversion failure.");

			_buckets[pos][key]->put(value);
		}
	}

	void for_each_aggr(function<void(const position&, const string&, double)> f) const {
		for(const auto& pos_bucks : _buckets) {
			const position& pos = pos_bucks.first;
			for(const auto& key_aggr : pos_bucks.second) {
				const string& key = key_aggr.first;
				double value = key_aggr.second->get();
				f(pos, key, value);
			}
		}
	}
};

}

#endif
