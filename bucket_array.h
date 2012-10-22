namespace buck_arr {

/// @brief Defines one of the bucket's coordinates.
class coord {
	/// The label values definint the coordinate.
	map<string, string> _label_map;

	/// The data column extension of the coordinate.
	string _data_ext;

	/// @brief Private constructor.
	coord(const map<string, string>& label_map,
	      const string& data_ext = "")
	: _label_map(label_map)
	, _data_ext(data_ext) {}

public:
	/// @brief Main factory method.
	///
	/// @param[in] definitions The column indices to define the coordinate.
	/// @param[in] data_ext The data column extending this dimension.
	///	pass an empty string to indicate no extension.
	/// @param[in] columns The columns of the original data set.
	/// @param[in] row The data row for the coordinate to be built with.
	static coord from_def_and_row(
			const vector<string>& label_map_defs,
			const string& data_ext,
			const vector<string>& columns,
			const vector<string>& row) {

		// For each of the definition labels an according index is found
		// by searching the columns array. Then the data piece from the
		// row at the given index is associated with the given
		// definition label.

		map<string, string> label_map;
		for(auto& def : definitions) {
			uint32_t index = index_of(columns, def);
			label_map[def] = row[index];
		}

		return coordinate(label_map, data_ext);
	}

	/// @brief Extends the coordinate with a data column.
	void extend(const string& ext) {
		if(!_data_ext.empty())
			_data_ext.append(" ");
		_data_ext.append(ext);
	}

	// General analysis.
	// -----------------
	
	/// @brief Allows checking whether the coordinate has been defined.
	bool defined() const {
		return !_label_map.empty();
	}

	/// @brief Allows checking whether an extension has been defined 
	///	for the dimension associated with this coordinate.
	bool extended() const {
		return !_data_ext.empty();
	}

	// Data consumption.
	// -----------------
	
	/// @brief Checks whether a given data row matches the values defined
	///	by this coordinate.
	///
	/// @param[in] columns The columns of the original data set.
	/// @param[in] row The row to be checked.
	///
	/// @returns True if the given row matches this coordinate, false otherwise.
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

	/// @brief Computes hash value for the boost's unordered map.
	friend size_t hash_value(const coordinate& coord) {
		size_t seed = 0;
		for(auto& pr : coord._labels) {
			hash_combine(seed, pr.first);
			hash_combine(seed, pr.second);
		}
		hash_combine(seed, _data_ext);
		return seed;
	}

	/// @brief Determines the equality condition for the boost's unordered map.
	friend bool operator==(const coordinate& lhs, const coordinate& rhs) {
		return hash_value(lhs) == hash_value(rhs);
	}
};

/// @brief Defines a multidimensional position of a bucket.
class position {
	/// The coordinates defining this position.
	vector<coord> _coords;

public:
	/// @brief Constructor allowing building of a position object
	///	from a list of coordinates.
	position(initializer_list<coord> coords) {
		if(coords.empty())
			throw string("Position must have at least one coordinate");

		for(const auto& coord : coords)
			_coords.push_back(coord);
	}

	/// @brief Access to the position's coordinates.
	///
	/// @param index The index of the requested coordinate.
	const coordinate& coord(uint32_t index) const {
		return _coords.at(index);
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
/// @tparam BUCKET A bucket class that will be fed with according data.
///	A BUCKET type must provide:
///	* a static "from_str" factory method,
///	* operator()(string) for consuming the values.
template<class BUCKET>
class table {
	// Configuration.
	// --------------
	
	/// The dimension defining columns the indexing is:
	///	...[dimension][label]
	vector<vector<string>> _dim_defs;

	/// The bucket mapping: consumed column ind. -> buck. constr. str.
	vector<pair<uint32_t, string>> _bucket_constrs;

	/// The dimension that is to be extended with the data columns.
	uint32_t _data_ext_dim;

	/// The dimension that is to be extended with the bucket definition.
	uint32_t _buck_ext_dim;

	// State.
	// ------
	
	/// The map defining the bucket positions.
	unordered_map<position, BUCKET> _buckets;

	/// @brief Generates coordinates based on the general definitions.
	///
	/// This function doesn't take into account any coordinate extensions
	///	that may be a result of the multiple data columns existing in
	///	the original data set or the necessity of generating additional
	///	coordinates for each aggregator.
	///
	/// @param[in] columns The columns of the original data set.
	/// @param[in] row The data row to be processed.
	/// @param[out] base_coords The base coordinates found by this function.
	/// @param[out] unused_columns The columns that have not been used by the
	///	dimension definitions. They will define the data extendions.
	void gen_base_coords(
			const vector<string>& columns,
			const vector<string>& row,
			vector<coord>& base_coords,
			vector<string> unused_columns) {

		base_coords.clear();

		// "ac" stands for "all columns", "b" and "e" stand for begin and end.
		auto acb = begin(columns);
		auto ace = begin(columns);

		// Build the base coordinates. They have not been extended yet.
		vcector<coord> base_coords;
		for(const auto& def : _dim_defs) {

			// Add new base coordinate.
			base_coords.push_back(coord::from_def_cols_row(
				def, columns, row));

			// Remove the columns that we have used.
			for(const auto& col : def)
				ace = remove(acb, ace, col);
		}

		unused_columns = vector<string>(acb, ace);
	}

	/// @brief Generates the final position for each of the defined bucket
	///	constructors and then puts according data into the according
	///	buckets.
	///
	/// @param[in] coords The coordinates of the data row that have been 
	///	established so far (after taking into account possible data
	///	column extension).
	/// @param[in] column The columns of the original data set.
	/// @param[in] row The data row to be processed.
	void fill_buckets(
			const vector<coord>& coords,
			const vector<string>& columns,
			const vector<string>& row) {

		for(const auto& pr : _bucket_constrs) {

			// Generate the ultimate coordinates vector.
			vector<coord> buck_ext_coords(coords);

			// Perform the bucket extension.
			buck_ext_coords[buck_ext_dim].extend(columns[pr.first]);

			// Initialize the position object.
			position pos(buck_ext_coords);

			// Construct bucket if necessary.
			if(_buckets.find(pos) == end(_buckets))
				_buckets[pos] = BUCKET::from_string(pr.second);

			// Feed the bucket with an according value.
			_buckets[pos](row[pr.first]);
		}
	}

public:
	/// @brief Simple member by member constructor.
	table(const vector<vector<string>>& dim_defs,
	      vector<pair<uint32_t, string>> bucket_constrs,
	      uint32_t data_ext_dim,
	      uint32_t buck_ext_dim)
	: _dim_defs(dim_defs)
	, _data_ext_dim(data_ext_dim)
	, _buck_ext_dim(buck_ext_dim) {}

	/// @brief Inserts data from a given row into this table.
	///
	/// @param[in] columns The columns of the original data.
	/// @param[in] row The row to be inserted.
	void consume_row(const vector<string>& columns, const vector<string>& row) {
		
		// Generate the positions associated with this row.
		// ------------------------------------------------
		vector<coord> base_coords;
		vector<string> unused_columns;
		gen_base_coords(columns, row, base_coords, unused_columns);

		// Is data columns extension required?
		// -----------------------------------
		if(unused_columns.empty())
			fill_buckets(base_coords, columns, row);
		else
			for(const auto& col : unused_columns) {
				vector<coord> data_ext_coords(base_coords);
				data_ext_coords[_data_ext_dim].extend(col);
				fill_buckets(data_ext_coords, columns, row);
			}
	}
};

}
