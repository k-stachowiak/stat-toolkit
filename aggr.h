/* Copyright (C) 2012,2013 Krzysztof Stachowiak */

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

#ifndef AGGR_H
#define AGGR_H

#include <string>
using std::string;

#include <sstream>
using std::stringstream;

#include <memory>
using std::unique_ptr;

#include <limits>
using std::numeric_limits;

#include <boost/math/distributions/normal.hpp>
using boost::math::normal;

#include <boost/xpressive/xpressive.hpp>
using boost::xpressive::sregex;
using boost::xpressive::smatch;
using boost::xpressive::s1;
using boost::xpressive::_d;
using boost::xpressive::_s;

namespace aggr {

	// The class defines an interface for the objects that aggregate streams
	// of numbers. The way to use an aggregator is to first feed it with
	// a serie of numbers and then query it for the according aggregation.
	// Aggregators are supposed to be queried multiple times after any number
	// of the insertions of the input numbers.
	class aggregator {
	public:
		virtual ~aggregator() {}

		// Add a value to the distribution.
		virtual void put(double value) = 0;

		// Gets the aggregated value.
		virtual double get() const = 0;
	};

	// Helper typedef.
	typedef unique_ptr<aggregator> ptr;

	// Count of the elements put so far.
	class count : public aggregator {
		int _count;
	public:
		count() : _count(0) {}
		void put(double) { ++_count; }
		double get() const { return double(_count); }
	};

    // The minimum from the elements put so far.
    class min : public aggregator {
        double _min;
    public:
        min() : _min(numeric_limits<double>::infinity()) {}
        void put(double value) { if(value < _min) _min = value; }
        double get() const { return _min; }
    };

    // The maximum from the elements put so far.
    class max : public aggregator {
        double _max;
    public:
        max() : _max(-numeric_limits<double>::infinity()) {}
        void put(double value) { if(value > _max) _max = value; }
        double get() const { return _max; }
    };

	// Sums the numbers that have been put into it so far.
	class sum : public aggregator {
		double _sum;
	public:
		sum() : _sum(0) {}
		void put(double value) { _sum += value; }
		double get() const { return _sum; }
	};

	// Computs the mean of the values that have been put into it.
	// Note that it depends on two other aggregators : sum and count.
	class mean : public aggregator {
		sum _sum;
		count _count;
	public:
		void put(double value) {
			_sum.put(value);
			_count.put(value);
		}

		double get() const {
			return _sum.get() / _count.get();
		}
	};

	// Computes the standard deviation of the population
	// based on the input data interpreted as the sample.
	// The implementation is based on the algorithm for the
	// running standard deviation from the Wikipedia.
	class stdev : public aggregator {
		double _a;
		double _q;
		double _k;
	public:
		stdev() : _a(0), _q(0), _k(0) {}
	
		void put(double value) {
			double new_a = _a + (value - _a) / (_k + 1);
			double new_q = _q + (value - _a) * (value - new_a);
			_a = new_a;
			_q = new_q;			
			_k += 1.0;
		}

		double get() const {
			return sqrt(_q / (_k - 1));
		}
	};

	// Computes the gaussian confidence interval of the values that are
	// put into it. Note that it depends on the boost statistical helpers.
	class ci_gauss : public aggregator {
		double _alpha;
		count _count;
		mean _mean;
		stdev _stdev;
	public:
		ci_gauss(double alpha) : _alpha(alpha) {}

		void put(double value) {
			_count.put(value);
			_mean.put(value);
			_stdev.put(value);
		}

		double get() const {
			normal dist(_mean.get(), _stdev.get() / sqrt(_count.get()));
			double lower_p = (1.0 - _alpha) * 0.5;
			double upper_p = lower_p + _alpha;
			double lower = quantile(dist, lower_p);
			double upper = quantile(dist, upper_p);
			return upper - lower;
		}
	};

	// The function takes a so called constructor string as an argument,
	// and constructs an according aggregator implementation.
	ptr create_from_string(const string& str) {

		// Simple, no argument cases.
		// --------------------------
		if(str == "count") return unique_ptr<aggregator>(new count);
        if(str == "min") return unique_ptr<aggregator>(new min);
        if(str == "max") return unique_ptr<aggregator>(new max);
		if(str == "sum") return unique_ptr<aggregator>(new sum);
		if(str == "mean") return unique_ptr<aggregator>(new mean);
		if(str == "stdev") return unique_ptr<aggregator>(new stdev);

		// Cases that require parsing.
		// ---------------------------

		// Normal distribution based confidence interval aggregator.
		sregex cig_re = "ci_gauss" >> +_s >> (s1 = +_d >> '.' >> +_d);
		smatch match;
		if(regex_match(str, match, cig_re)) {

			double conf_lvl;

			stringstream ss;
			ss << match[1];
			ss >> conf_lvl;

			return unique_ptr<aggregator>(new ci_gauss(conf_lvl));
		}

		// No case satisfied. Abort.
		// -------------------------
		throw string("Failed recognizing aggregator in : \"" + str + "\".");
	}
}

#endif
