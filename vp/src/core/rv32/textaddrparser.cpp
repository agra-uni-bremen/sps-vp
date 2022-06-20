#include <regex>
#include <fstream>

#include <stddef.h>
#include <inttypes.h>

#include "coverage.h"

using namespace rv32;

Coverage::TextAddrParser::TextAddrParser(std::string path)
{
	if (path.empty())
		return;

	std::ifstream file(path);
	if (!file.is_open())
		throw std::runtime_error("failed to open file " + path);

	std::string line;
	size_t lineNum = 1;

	while (std::getline(file, line)) {
		auto s = parseSegment(line);
		if (!s.has_value())
			throw std::runtime_error("falid to parse segment at " + path + ":" + std::to_string(lineNum));

		segments.push_back(*s);
		lineNum++;
	}
}

std::optional<Coverage::TextAddrParser::segment>
Coverage::TextAddrParser::parseSegment(std::string &line)
{
	std::regex re("0x([a-zA-Z0-9][a-zA-Z0-9]*) 0x([a-zA-Z0-9][a-zA-Z0-9]*)");

	std::smatch match;
	if (regex_search(line, match, re)) {
		if (match.size() != 3) /* match includes the entire string */
			return std::nullopt;

		auto start_addr = parseAddr(match[1].str());
		if (!start_addr.has_value())
			return std::nullopt;

		auto end_addr = parseAddr(match[2].str());
		if (!end_addr.has_value())
			return std::nullopt;

		return std::make_pair(*start_addr, *end_addr);
	}

	return std::nullopt;
}

std::optional<uint64_t>
Coverage::TextAddrParser::parseAddr(std::string str)
{
	uint64_t v;

	int r = sscanf(str.c_str(), "%" SCNx64, &v);
	if (r == 1)
		return v;
	else
		return std::nullopt;
}

bool
Coverage::TextAddrParser::is_included(uint64_t addr)
{
	if (segments.empty())
		return true;

	for (auto s : segments) {
		if (addr >= s.first && addr <= s.second)
			return true;
	}

	return false;
}
