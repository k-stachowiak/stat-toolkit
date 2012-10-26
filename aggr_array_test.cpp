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

#include <iostream>
using std::cout;
using std::endl;

#include <unittest++/UnitTest++.h>
using namespace UnitTest;

#include "aggr_array.h"

static const double TOLERANCE = 0.01;

TEST(test_name) {

	// Prepare input. ACHTUNG! Special define shortcut used
	// for defining strings.
	// -----------------------------------------------------

#define _(x) string(#x)

	// The input data table.
	vector<vector<string>> rows {
		{ _(0),    _(0),    _(0),    _(1) },
		{ _(0),    _(0),    _(2),    _(3) },
		{ _(0),    _(1),    _(4),    _(5) },
		{ _(0),    _(1),    _(6),    _(7) },
		{ _(1),    _(0),    _(8),    _(9) },
		{ _(1),    _(0),    _(0),    _(1) },
		{ _(1),    _(1),    _(2),    _(3) },
		{ _(1),    _(1),    _(4),    _(5) }
	};

	// The basic dimension definitions.
	vector<vector<uint32_t>> dim_defs {
		{ 1, 2 }
	};

	// The bucket constructors.
	vector<pair<uint32_t, string>> bucket_constrs {
		{ 2, _(count) },
		{ 2, _(sum) },
		{ 3, _(mean) },
		{ 3, _(stdev) }
	};

#undef _

	// Instantiate SUT.
	// ----------------
	aggr_arr::array arr(dim_defs, bucket_constrs);

	// Exercise SUT.
	// -------------
	for(const auto& row : rows)
		arr.consume_row(row);

	// Verify the results.
	// -------------------
	arr.for_each_aggr([](
			const aggr_arr::position& pos,
			const string& aggr_str,
			double value) {
			
		cout << "pos(" << pos.str() << "), " <<
			"aggr(" << aggr_str << ")," <<
			"val(" << value << ")" << endl;
	});


	CHECK(true);
}


int main() {
	return RunAllTests();
}