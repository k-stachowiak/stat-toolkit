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

#ifndef GROUPBY_H
#define GROUPBY_H

#include <utility>
using std::pair;

#include <boost/xpressive/xpressive.hpp>
using boost::xpressive::sregex;
using boost::xpressive::smatch;
using boost::xpressive::s1;
using boost::xpressive::s2;
using boost::xpressive::_d;
using boost::xpressive::_s;
using boost::xpressive::_;
using boost::xpressive::eos;

#include "aggr.h"

/// @brief A class to define the groups and store their aggregations.
///
/// This class defines a single group of the results.
///	The groups are distinguished by the values of the groupping fields.
///	The data rows from the considered input set may or may not match
///	a given group based on the given groupping fields. Once a matching
///	group is determined, values from the considered row should be put
///	in the group's aggregators.
class group {

	/// The field indices and the values that discriminate this
	/// group from the others.
	vector<pair<uint32_t, string>> _definition;

	/// The list of the pairs of the indices of the fields to be
	/// aggregated and their respective aggregators.
	vector<pair<uint32_t, aggr::ptr>> _aggregators;

	/// @brief A private constructor to enforce usage of the factory function.
	group() {}
public:
	/// @brief Checks whether a given row belongs to this group.
	///
	/// @param[in] row The data row to be checked.
	///
	/// @returns True if the row matches this group, false otherwise.
	bool matches_row(const vector<string>& row) {
		for(const auto& def : _definition)
			if(row.at(def.first) != def.second)
				return false;
		
		return true;
	}

	/// @brief Getter for the definition.
	const vector<pair<uint32_t, string>>& get_definition() const {
		return _definition;
	}

	/// @brief Getter for the aggregators.
	const vector<pair<uint32_t, aggr::ptr>>& get_aggregators() const {
		return _aggregators;
	}

	/// @brief Consumes the row, i.e. feeds all the aggregators with
	/// 	the values from the respective fields.
	///
	/// @param[in] row The row, the values from are to be put in this
	///	group's aggregators.
	void consume_row(const vector<string>& row) {
		for(auto& aggr : _aggregators) {
			double value;
			stringstream ss;
			ss << row[aggr.first];
			ss >> value;
			if(ss.fail())
				throw string("Failed parsing a value for an aggregator.");
			aggr.second->put(value);
		}
	}

	/// @brief Parses the aggregator string extracting the index of the aggregated field
	/// 	and building an appropriate aggregator object.
	///
	/// @param[in] aggr_str The string to be parsed.
	/// @param[out] field The result field index.
	/// @param[out] aggr The aggregator constructed based on the according
	///	part of the input string.
	static void parse_aggr_str(string aggr_str, uint32_t& field, aggr::ptr& aggr) {

		smatch match;

		// Recognize the field to aggregator mapping.
		sregex base_re = *_s >> (s1 = +_d) >> +_s >> (s2 = +_) >> eos;
		if(!regex_match(aggr_str, match, base_re))
			throw string("Unrecognize aggregator string \"") + aggr_str + "\".";

		// Parse the field index.
		stringstream field_ss;
		field_ss << match[1];
		field_ss >> field;

		if(field_ss.fail())
			throw string("Failed parsing the field mapping of an aggregator.");

		// Parse the aggregator string.
		auto a = aggr::create_from_string(match.str(2));
		aggr = move(a);
	}

	/// @brief Builds a group that matches a given row.
	///
	/// This function gets called whenever an input data row arrives that
	///	doesn't match any of the previously found groups. Therefore
	///	a new group must be created that the new data row will match.
	///	This function creates such a group that will match the provided
	///	input row.
	///
	/// @param[in] groupbys The indices of the groupping columns.
	/// @param[in] aggr_strs The strings for constructing the aggregators.
	/// @param[in] row The template row for creating a new group.
	static group from_row(
			const vector<uint32_t>& groupbys,
			const vector<string>& aggr_strs,
			const vector<string>& row) {

		group result;

		for(uint32_t gb : groupbys)
			result._definition.emplace_back(gb, row[gb]);

		for(const auto& as : aggr_strs) {
			uint32_t field;
			aggr::ptr aggr;
			parse_aggr_str(as, field, aggr);
			result._aggregators.emplace_back(field, move(aggr));
		}

		return result;
	}
};


#endif
