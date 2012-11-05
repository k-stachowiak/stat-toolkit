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

#include "groupby.h"

static const double TOLERANCE = 0.01;

TEST(arbitrary_test_case) {

	// Prepare data.
	// -------------

#define _(x) string(#x)
	
	vector<uint32_t> groupbys { 0, 1, 2 };
	vector<string> aggr_strs { _(3 sum) };
	vector<vector<string>> data {
		{ _(x1), _(y1), _(z1), _(1) },
		{ _(x1), _(y1), _(z1), _(1) },
		{ _(x1), _(y1), _(z1), _(1) },
		{ _(x1), _(y1), _(z2), _(2) },
		{ _(x1), _(y1), _(z2), _(2) },
		{ _(x1), _(y1), _(z2), _(2) },
		{ _(x1), _(y2), _(z1), _(3) },
		{ _(x1), _(y2), _(z1), _(3) },
		{ _(x1), _(y2), _(z1), _(3) },
		{ _(x1), _(y2), _(z2), _(4) },
		{ _(x1), _(y2), _(z2), _(4) },
		{ _(x1), _(y2), _(z2), _(4) },
		{ _(x2), _(y1), _(z1), _(5) },
		{ _(x2), _(y1), _(z1), _(5) },
		{ _(x2), _(y1), _(z1), _(5) },
		{ _(x2), _(y1), _(z2), _(6) },
		{ _(x2), _(y1), _(z2), _(6) },
		{ _(x2), _(y1), _(z2), _(6) },
		{ _(x2), _(y2), _(z1), _(7) },
		{ _(x2), _(y2), _(z1), _(7) },
		{ _(x2), _(y2), _(z1), _(7) },
		{ _(x2), _(y2), _(z2), _(8) },
		{ _(x2), _(y2), _(z2), _(8) },
		{ _(x2), _(y2), _(z2), _(8) }
	};
#undef _

	// Instantiate and exercise SUT.
	// -----------------------------
	groupby::groupper g;
	for(const auto& row : data)
		g.consume_row(groupbys, aggr_strs, row);

	// Verify result.
	// --------------	
	g.for_each_group([](const groupby::group& g) {
		// Print the definition.
		for(const auto& pr : g.get_definition())
			cout << pr.first << " = " << pr.second << " ";
		// Print the aggregators.
		for(const auto& pr : g.get_aggregators())
			cout << "aggr(" << pr.first << ") = "
				<< pr.second->get() << " ";

		cout << endl;
	});

	CHECK(true);
}

int main() {
	return RunAllTests();
}
