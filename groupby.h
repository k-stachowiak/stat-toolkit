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

#ifndef GROUPBY_H
#define GROUPBY_H

#include <utility>
using std::pair;

#include <vector>
using std::vector;

#include <string>
using std::string;

#include <functional>
using std::function;

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

namespace groupby {

// This class defines a single group of the results.
// The groups are distinguished by the values of the groupping fields.
// The data rows from the considered input set may or may not match
// a given group based on the given groupping fields. Once a matching
// group is determined, values from the considered row should be put
// in the group's aggregators.
class group {

	// The field indices and the values that discriminate this
	// group from the others.
	vector<pair<uint32_t, string>> _definition;

	// The list of the pairs of the indices of the fields to be
	// aggregated and their respective aggregators.
	vector<pair<uint32_t, aggr::ptr>> _aggregators;

public:
	// Only allow constructing from the prepared members.
	group(vector<pair<uint32_t, string>> definition,
		vector<pair<uint32_t, aggr::ptr>> aggrs)
	: _definition(definition)
	, _aggregators(move(aggrs))
	{}

	// Checks whether a given row belongs to this group.
	bool matches_row(vector<string> const& row) const {
		for(auto const& def : _definition)
			if(row.at(def.first) != def.second)
				return false;
		
		return true;
	}

	// Getter for the definition.
	vector<pair<uint32_t, string>> const& get_definition() const {
		return _definition;
	}

	// Getter for the aggregators.
	vector<pair<uint32_t, aggr::ptr>> const& get_aggregators() const {
		return _aggregators;
	}

	// Consumes the row, i.e. feeds all the aggregators with
	// the values from the respective fields.
	void consume_row(vector<string> const& row) const {
		for(auto& aggr : _aggregators) {
			double value;
			stringstream ss;
			ss << row[aggr.first];
			ss >> value;
			if(ss.fail()) {
				stringstream rowss;
				for(string const& s : row)
					rowss << s << " ";
				throw string("Failed parsing a value for an aggregator. "
						"Row: " + rowss.str());
			}
			aggr.second->put(value);
		}
	}
};

struct group_result {
	vector<pair<uint32_t, string>> definition;
	vector<pair<uint32_t, double>> aggregators;

	// Gets a value defining the group at a column given by the ket index.
	string const& get_def_at(uint32_t key) const {
		for(auto const& pr : definition)
			if(pr.first == key)
				return pr.second;
		throw string("Requested a definition for a non-existent key.");
	}
};

class groupper {

	// Configuration.
	// --------------
	vector<uint32_t> _groupbys;
	vector<string> _aggr_strs;

	// State.
	// ------
	vector<group> _groups;

	// Parses the aggregator construction string.
	static void parse_aggr_str(
			string const& aggr_str,
			uint32_t& field,
			aggr::ptr& aggr) {

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

	// Creates a group based on a definitions and a given row.
	// The result is a group that fulfils the conditions of matching the
	// provided row assuming the input definitions.
	static group group_from_row(
			vector<uint32_t> groupbys,
			vector<string> aggr_strs,
			vector<string> const& row) {

		// Assign labels to the definitions.
		vector<pair<uint32_t, string>> definition;
		for(uint32_t gb : groupbys)
			definition.emplace_back(gb, row[gb]);

		// Initialize a fresh set of aggregators.
		vector<pair<uint32_t, aggr::ptr>> aggrs;
		for(auto const& as : aggr_strs) {
			uint32_t field;
			aggr::ptr aggr;
			parse_aggr_str(as, field, aggr);
			aggrs.emplace_back(field, move(aggr));
		}

		return { definition, move(aggrs) };
	}

public:
	groupper(vector<uint32_t> groupbys, vector<string> aggr_strs)
	: _groupbys(groupbys)
	, _aggr_strs(aggr_strs)
	{}

	// Accepts a row and assigns it to the first matching group.
	// If no matching group exists a new group is created based on the row
	// and the according definitions and the row is stored in the newly
	// created group.
	void consume_row(vector<string> const& row) {

		// Determine the index of the group to which the given data row
		// belongs.
		uint32_t index;

		// Try among existing groups.
		bool found = false;
		for(uint32_t i = 0; i < _groups.size(); ++i)
			if(_groups[i].matches_row(row)) {
				found = true;
				index = i;
				break;
			}

		// If no matching group, create a new one.
		if(!found) {
			index = _groups.size();
			_groups.push_back(group_from_row(
				_groupbys, _aggr_strs, row));
		}

		// Add the row at the determined index.
		_groups[index].consume_row(row);
	}

	// Allows iteration over all the groups.
	void for_each_group(function<void(group const&)> f) const {
		for(auto const& g : _groups)
			f(g);
	}

	vector<group_result> copy_result() const {
		vector<group_result> result;
		for(auto const& g : _groups) {
			vector<pair<uint32_t, string>> definition;
			vector<pair<uint32_t, double>> aggregators;
			for(auto const& d : g.get_definition())
				definition.push_back(d);
			for(auto const& a : g.get_aggregators())
				aggregators.emplace_back(a.first, a.second->get());
			result.push_back({ definition, aggregators });
		}
		return result;
	}
};

}

#endif
