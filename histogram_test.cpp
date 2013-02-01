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

#include <vector>
using std::vector;

#include <unittest++/UnitTest++.h>
using namespace UnitTest;

#include "histogram.h"

static const double TOLERANCE = 0.01;

TEST(one_num_per_bucket_test) {

	double bucket_width = 1.0;

	vector<double> collection { -1.0, 0.0, 1.0 };

	hist::histogram h(bucket_width);

	for(double e : collection)
		h.put(e);

	map<double, double> expected {
		{-1.0, 1.0},
		{0.0, 1.0},
		{1.0, 1.0} };

	const map<double, double>& actual = h.get_buckets();

	CHECK(expected == actual);
}

TEST(two_nums_per_bucket_test) {

	double bucket_width = 1.0;

	vector<double> collection { -1.1, -0.9, -0.1, 0.1, 0.9, 1.1 };

	hist::histogram h(bucket_width);

	for(double e : collection)
		h.put(e);

	map<double, double> expected {
		{-1.0, 2.0},
		{0.0, 2.0},
		{1.0, 2.0} };

	const map<double, double>& actual = h.get_buckets();

	CHECK(expected == actual);
}

TEST(empty_bucket_test) {

	double bucket_width = 1.0;

	vector<double> collection { -3.0, -1.0, 0.0, 1.0 };

	hist::histogram h(bucket_width);

	for(double e : collection)
		h.put(e);

	map<double, double> expected {
		{-3.0, 1.0},
		{-2.0, 0.0},
		{-1.0, 1.0},
		{0.0, 1.0},
		{1.0, 1.0} };

	const map<double, double>& actual = h.get_buckets();

	CHECK(expected == actual);
}

int main() {
	return RunAllTests();
}
