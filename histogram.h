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

#ifndef HISTOGRAM_H
#define HISTOGRAM_H

#include <limits>
using std::numeric_limits;

#include <map>
using std::map;

#include <cmath>
using std::floor;

namespace hist {
	
/// @brief Builds a histogram of the tata it receives.
///
/// This class contains a caching mechanism so that the re is the raw buckets
/// 	map that is created based on the input data and nothing else. Upon
///	request a refined buckets map is created that also contains empty
///	buckets if any are needed.
class histogram {

	// Parameters.
	// -----------
	double _bucket_size;	///< The width of the histogram's interval.

	// State.
	// ------
	map<double, double> _raw_buckets;	///< Maps bucket-center -> count 
	bool _cache_valid;			///< Flag indicating cache validity
	map<double, double> _cached_buckets;	///< The cached output buckets.

public:
	/// @brief The constructor.
	///
	/// @param[in] bucket_size The requested size of a bucket.
	histogram(double bucket_size)
	: _bucket_size(bucket_size)
	, _cache_valid(false) {}

	/// @brief The finction for inserting a value into the histogram.
	///
	/// @param[in] value The value to be inserted.
	void put(double value) {
		
		// Establish the bucket
		double scaled = value / _bucket_size;
		double scaled_shifted = scaled + 0.5;
		double buck_index = floor(scaled_shifted);

		// Add to appropriate bucket.
		if(_raw_buckets.find(buck_index) == end(_raw_buckets))
			_raw_buckets[buck_index] = 1.0;
		else
			_raw_buckets[buck_index] += 1.0;

		// Invalidate cache.
		_cache_valid = false;
	}

	/// @brief The function that returns the refined variant of the buckets'
	/// 	map. It is cached so if anu value has been put in this histogram
	///	the cache must be rebuilt upon a call to this function.
	///
	/// @returns A map representing the refined buckets.
	map<double, double> get_buckets() {

		if(!_cache_valid) {
			_cached_buckets.clear();
			double prev_index = numeric_limits<double>::infinity();
			for(auto& pr : _raw_buckets) {
				double bucket_index = pr.first;
				for(double i = prev_index + 1; i < bucket_index; i += 1.0)
					_cached_buckets[i * _bucket_size] = 0.0;
				_cached_buckets[bucket_index * _bucket_size] = pr.second;
				prev_index = bucket_index;
			}
			_cache_valid = true;
		}

		return _cached_buckets;
	}
};

}

#endif
