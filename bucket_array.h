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

namespace buck_arr {

/// @brief Defines one of the bucket's coordinates.
class coord {
	/// The label values definint the coordinate.
	map<string, string> _label_map;

	/// @brief Private constructor.
	coord(const map<string, string>& label_map)
	: _label_map(label_map) {}

public:
	/// @brief Main factory method.
	///
	/// @param[in] label_map def The column indices to define the coordinate.
	/// @param[in] columns The columns of the original data set.
	/// @param[in] row The data row for the coordinate to be built with.
	static coord from_def_cols_row(
			const vector<string>& label_map_def,
			const vector<string>& columns,
			const vector<string>& row) {

		// For each of the definition labels an according index is found
		// by searching the columns array. Then the data piece from the
		// row at the given index is associated with the given
		// definition label.

		map<string, string> label_map;
		for(auto& def : label_map_def) {
			uint32_t index = index_of(columns, def);
			label_map[def] = row[index];
		}

		return coord(label_map);
	}

	/// @brief Generates a string representation of this object.
	///
	/// @returns The string representation of this object.
	string to_string() const {
		stringstream ss;
		for(const auto& pr : _label_map) {
			ss << pr.first << " = " << pr.second << ", ";
		}
		return ss.str();
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

	/// @brief Generates a string representation of this object.
	///
	/// @returns The string representation of this object.
	string to_string() const {
		stringstream ss;
		for(uint32_t i = 0; i < _coords.size(); ++i) {
			ss << "coord(" << _coords.at(i).to_string() << ")";
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
class table {
	// Configuration.
	// --------------
	
	/// The dimension defining columns the indexing is:
	///	...[dimension][label]
	vector<vector<string>> _dim_defs;

	/// The bucket mapping: consumed column ind. -> buck. constr. str.
	vector<pair<uint32_t, string>> _bucket_constrs;

	// State.
	// ------
	
	/// The map defining the bucket positions.
	unordered_map<position, map<string, aggr::ptr>> _buckets;

public:
	/// @brief Simple member by member constructor.
	table(const vector<vector<string>>& dim_defs,
	      vector<pair<uint32_t, string>> bucket_constrs)
	: _dim_defs(dim_defs)
	, _bucket_constrs(bucket_constrs)
	{}

	/// @brief Inserts data from a given row into this table.
	///
	/// @param[in] columns The columns of the original data.
	/// @param[in] row The row to be inserted.
	void consume_row(const vector<string>& columns, const vector<string>& row) {

		stringstream ss;
		
		// Generate the positions associated with this row.
		// ------------------------------------------------
		vector<coord> base_coords;
		for(const auto& def : _dim_defs)
			base_coords.push_back(coord::from_def_cols_row(
				def, columns, row));

		// Initialize the position object.
		position pos(base_coords);

		// Read all the fields defined by the buckets.
		// -------------------------------------------
		for(const auto& pr : _bucket_constrs) {

			string col = columns.at(pr.first);
			string key = pr.second + "(" + col + ")";

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

	void for_each_aggr(function<void(const position&, const string&, double)> f) {
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
