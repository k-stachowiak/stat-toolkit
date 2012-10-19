#include <utility>
using std::pair;

#include <iostream>
using std::istream;
using std::ostream;
using std::cout;
using std::cin;
using std::endl;

#include <string>
using std::string;

#include <vector>
using std::vector;

#include <memory>
using std::unique_ptr;
using std::move;

#include <map>
using std::map;

#include <boost/xpressive/xpressive.hpp>
using boost::xpressive::sregex;
using boost::xpressive::smatch;
using boost::xpressive::s1;
using boost::xpressive::s2;
using boost::xpressive::_d;
using boost::xpressive::_s;
using boost::xpressive::_;
using boost::xpressive::eos;

#include <unistd.h>

#include "util.h"
#include "aggr.h"

// Helper typedefs
// ===============
typedef unique_ptr<aggr::aggregator> aggr_ptr;

// The input arguments analysis.
// =============================

/// The object that represents the inputa rguments of the program.
struct arguments {

	/// The field delimiter - default: tab character.
	char delim;

	/// Groupping fields.
	vector<uint32_t> groupbys;

	/// The aggregator definitions. Format:
	/// [(field_index, aggregator)]
	vector<string> aggr_strs;
};

/// Parses the program arguments building a proper object that reflects them.
arguments parse_args(int argc, char** argv) {

	arguments args;
	args.delim = '\t'; // Providing the default delimiter.
	stringstream converter;
	uint32_t index;

	int c;
	while((c = getopt(argc, argv, "a:d:g:")) != -1) {
		switch(c) {
		case 'a':
			args.aggr_strs.emplace_back(optarg);
			break;

		case 'd':
			if(string(optarg).size() != 1)
				throw string("The delimiter is expected to be a single character.");
	
			args.delim = optarg[0];

			if(!isprint(args.delim) && args.delim != '\t')
				throw string("Cannot use the given character as a delimiter.");

			break;

		case 'g':
			converter.seekg(0);
			converter.seekp(0);
			converter << optarg;
			converter >> index;
			if(converter.fail())
				throw string("Failed parsing groupping field index.");
			args.groupbys.push_back(index);
			break;

		case '?':
			if(optopt == 'a')
				throw string("Option -a requires an aggregator argument.");

			if(optopt == 'd')
				throw string("Option -d requires a delimiter argument.");

			if(optopt == 'g')
				throw string("Option -g requires a groupper argument.");

			// Notice a fallthrough. It is here although it should
			// not happen unless someone changes the getopt options definition.

		default:
			throw string("Illegal option -") + (char)optopt + ".";
		}
	}

	return args;
}

// The aggregation phase.
// ======================

/// Parses the aggregator string extracting the index of the aggregated field
/// and building an appropriate aggregator object.
void parse_aggr_str(string aggr_str, uint32_t& field, aggr_ptr& aggr) {

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

/// A class to define the groups and store their aggregations.
struct group {

	/// The field indices and the values that discriminate this
	/// group from the others.
	vector<pair<uint32_t, string>> definition;

	/// The list of the pairs of the indices of the fields to be
	/// aggregated and their respective aggregators.
	vector<pair<uint32_t, aggr_ptr>> aggregators;

	/// Checks whether a given row belongs to this group.
	bool matches_row(const vector<string>& row) {
		for(const auto& def : definition)
			if(row.at(def.first) != def.second)
				return false;
		
		return true;
	}

	/// Consumes the row, i.e. feeds all the aggregators with
	/// the values from the respective fields.
	void consume_row(const vector<string>& row) {
		for(auto& aggr : aggregators) {
			double value;
			stringstream ss;
			ss << row[aggr.first];
			ss >> value;
			if(ss.fail())
				throw string("Failed parsing a value for an aggregator.");
			aggr.second->put(value);
		}
	}

	/// Builds a group that matches a given row.
	static group from_row(
			const vector<uint32_t>& groupbys,
			const vector<string>& aggr_strs,
			const vector<string>& row) {

		group result;

		for(uint32_t gb : groupbys)
			result.definition.emplace_back(gb, row[gb]);

		for(const auto& as : aggr_strs) {
			uint32_t field;
			aggr_ptr aggr;
			parse_aggr_str(as, field, aggr);
			result.aggregators.emplace_back(field, move(aggr));
		}

		return result;
	}
};

/// Fetches the data from the input stream and finally prints out
/// the aggregations to the provided output stream.
vector<group> process_stream(istream& in, const arguments& args) {

	vector<group> groups;
	string line;

	while(true) {
		// Read new row line and parse it into the internal row format.
		getline(in, line);
		if(!in.good())
			break;

		vector<string> row = split(line, args.delim);

		// Determine the group to which the new row belongs.
		uint32_t index;
		bool found = false;
		for(uint32_t i = 0; i < groups.size(); ++i) {
			if(groups[i].matches_row(row)) {
				found = true;
				index = i;
				break;
			}
		}
	
		if(!found) {
			index = groups.size();
			groups.push_back(group::from_row(args.groupbys, args.aggr_strs, row));
		}

		// Register the row with the according group.
		groups[index].consume_row(row);
	}

	return groups;
}

// The result printing phase.
// ==========================

void print_results(const vector<group>& groups, ostream& out, const arguments& args) {

	// Print the groupping report.
	// ---------------------------

	// Print the header row.
	for(uint32_t g : args.groupbys)
		out << g << args.delim;

	for(uint32_t i = 0; i < args.aggr_strs.size(); ++i) {
		out << '"' << args.aggr_strs.at(i) << '"';
		if(i < (args.aggr_strs.size() - 1))
			out << args.delim;
	}

	out << endl;

	// Print the groups.
	for(const group& g : groups) {
		for(const auto& d : g.definition)
			out << d.second << args.delim;

		for(uint32_t i = 0; i < g.aggregators.size(); ++i) {
			out << g.aggregators.at(i).second->get();
			if(i < (g.aggregators.size() - 1))
				out << args.delim;
		}
		out << endl;
	}
}

int main(int argc, char** argv) {

	// Don't print internal getopt error messages.
	opterr = 0;

	try {
		// Read and validate the arguments.
		arguments args = parse_args(argc, argv);
		if(args.groupbys.empty() || args.aggr_strs.empty())
			throw string("Missing groupping or aggregation definitions.");

		// Process the input stream.
		auto groups = process_stream(cin, args);

		// Print the results.
		print_results(groups, cout, args);

		return 0;
	}
	catch(string ex) {
		cout << "Error : " << ex << endl;
		return 1;
	}
}

