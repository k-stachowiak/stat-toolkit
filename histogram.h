#ifndef HISTOGRAM_H
#define HISTOGRAM_H

#include <map>
using std::map;

#include <cmath>
using std::floor;

namespace hist {
	
	// Builds a histogram of the tata it receives.
	class histogram {

		// Parameters.
		double _bucket_size;

		// State.
		map<double, double> _buckets; // ceneter -> sum

	public:
		histogram(double bucket_size) : _bucket_size(bucket_size) {}

		void put(double value) {
			
			// Establish the bucket
			double scaled = value / _bucket_size;
			double scaled_shifted = scaled + 0.5;
			double buck_index = floor(scaled_shifted);

			// Add to appropriate bucket.
			if(_buckets.find(buck_index) == end(_buckets))
				_buckets[buck_index] = 1.0;
			else
				_buckets[buck_index] += 1.0;
		}

		const map<double, double>& get_buckets() const { return _buckets; }
	};

}

#endif
