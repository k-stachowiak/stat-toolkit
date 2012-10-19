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

#include <boost/math/distributions/normal.hpp>
using boost::math::normal;

#include <boost/xpressive/xpressive.hpp>
using boost::xpressive::sregex;
using boost::xpressive::smatch;
using boost::xpressive::s1;
using boost::xpressive::_d;
using boost::xpressive::_s;

namespace aggr {
	
	class aggregator {
	public:
		virtual ~aggregator() {}

		// Add a value to the distribution.
		virtual void put(double) = 0;

		// Gets the aggregated value.
		virtual double get() const = 0;

		// Gets the name of the distribution.
		virtual string get_name() const =0;
	};

	// Counts the numbers that are put into it.
	class count : public aggregator {
		int _count;
	public:
		count() : _count(0) {}
	
		// Aggregator interface.
		void put(double) { ++_count; }
		double get() const { return double(_count); }
		string get_name() const { return "count"; }
	};

	// Sums the  numbers that are put into it.
	class sum : public aggregator {
		double _sum;
	public:
		sum() : _sum(0) {}
	
		// Aggregator interface.
		void put(double value) { _sum += value; }
		double get() const { return _sum; }
		string get_name() const { return "sum"; }
	};

	// Computs the mean of the values that are put into it.
	class mean : public aggregator {
		sum _sum;
		count _count;
	public:
	
		// Aggregator interface.
		void put(double value) {
			_sum.put(value);
			_count.put(value);
		}

		double get() const {
			return _sum.get() / _count.get();
		}

		string get_name() const { return "mean"; }
	};

	// Computes the sample's standard deviation of the
	// values that are put into it.
	class stdev : public aggregator {
		double _a;
		double _q;
		double _k;
	public:
		stdev() : _a(0), _q(0), _k(0) {}
	
		// Aggregator interface.
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

		string get_name() const { return "stdev"; }
	};

	// Computes the gaussian confidence interval of the
	// values that are put into it.
	class conf_int_gauss : public aggregator {
		double _alpha;
		count _count;
		mean _mean;
		stdev _stdev;
	public:
		conf_int_gauss(double alpha) : _alpha(alpha) {}

		// Aggregator interface.
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

		string get_name() const { return "conf_int_gauss"; }
	};

	unique_ptr<aggregator> create_from_string(string str) {

		// Simple, no argument cases.
		// --------------------------
		if(str == "count") return unique_ptr<aggregator>(new count);
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

			return unique_ptr<aggregator>(new conf_int_gauss(conf_lvl));
		}

		// No case satisfied. Abort.
		// -------------------------
		throw string("Failed recognizing aggregator in : \"" + str + "\".");
	}
}

#endif
