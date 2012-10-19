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

#include <vector>
using std::vector;

#include <unittest++/UnitTest++.h>
using namespace UnitTest;

#include "aggr.h"

static const double TOLERANCE = 0.01;
static const uint32_t SMALL_INT = 5;
static const double ANY_DOUBLE = 1.0;

TEST(count_aggregator_test) {

	uint32_t num_elements = SMALL_INT;

	aggr::count count_aggregator;
	for(uint32_t i = 0; i < num_elements; ++i)
		count_aggregator.put(ANY_DOUBLE);

	double expected = (double)num_elements;
	double actual = count_aggregator.get();

	CHECK_EQUAL(expected, actual);
}

TEST(sum_aggregator_test) {

	uint32_t num_elements = SMALL_INT;
	double element = ANY_DOUBLE;

	vector<double> collection(num_elements, element);

	aggr::sum sum_aggregator;
	for(double e : collection)
		sum_aggregator.put(e);

	double expected = (double)num_elements * element;
	double actual = sum_aggregator.get();

	CHECK_CLOSE(expected, actual, TOLERANCE);
}

TEST(mean_aggregator_test) {

	vector<double> collection { 1.0, 2.0, 3.0, 4.0 };

	aggr::mean mean_aggregator;
	for(double e : collection)
		mean_aggregator.put(e);

	double expected = 2.5;
	double actual = mean_aggregator.get();

	CHECK_CLOSE(expected, actual, TOLERANCE);
}

TEST(stdev_aggregator_test) {

	vector<double> collection { 2.0, 4.0, 4.0, 4.0, 5.0, 5.0, 7.0, 9.0 };

	aggr::stdev stdev_aggregator;
	for(double e : collection)
		stdev_aggregator.put(e);

	double expected = 2.138089935;
	double actual = stdev_aggregator.get();

	CHECK_CLOSE(expected, actual, TOLERANCE);
}

TEST(string_based_construction_test) {

	unique_ptr<aggr::aggregator> ptr;

	ptr = aggr::create_from_string("count");
	CHECK(dynamic_cast<aggr::count*>(ptr.get()));

	ptr = aggr::create_from_string("sum");
	CHECK(dynamic_cast<aggr::sum*>(ptr.get()));

	ptr = aggr::create_from_string("mean");
	CHECK(dynamic_cast<aggr::mean*>(ptr.get()));

	ptr = aggr::create_from_string("stdev");
	CHECK(dynamic_cast<aggr::stdev*>(ptr.get()));

	ptr = aggr::create_from_string("ci_gauss 75.0");
	CHECK(dynamic_cast<aggr::conf_int_gauss*>(ptr.get()));
}

int main() {
	return RunAllTests();
}
