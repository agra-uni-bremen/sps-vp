/*
 * Copyright (c) 2022 Group of Computer Architecture, University of Bremen
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <https://www.gnu.org/licenses/>.
 */

#ifndef RISCV_VP_COVERAGE_H
#define RISCV_VP_COVERAGE_H

#include <map>
#include <vector>
#include <utility>
#include <string>
#include <stdint.h>
#include <stdbool.h>

#include "elf_loader.h"
#include "mem_if.h"

namespace rv32 {

class Coverage {
private:
	ELFLoader &loader;

	typedef std::pair<bool, bool> branch_coverage;
	std::map<uint64_t, branch_coverage> branch_instrs;

public:
	class TextAddrParser {
	private:
		typedef std::pair<uint64_t, uint64_t> segment;

		std::vector<segment> segments;
		std::optional<uint64_t> parseAddr(std::string);
		std::optional<segment> parseSegment(std::string&);

	public:
		TextAddrParser(std::string path);
		bool is_included(uint64_t addr);
	};

	TextAddrParser parser;
	instr_memory_if *instr_mem = nullptr;

	Coverage(ELFLoader &_loader, std::string specfile = "")
	  : loader(_loader), parser(specfile) {
		return;
	}

	void init(void);
	void init_section(const Elf32_Shdr *section);

	void cover_branch(uint64_t, bool);
	double dump_branch_coverage(void);
};

}

#endif
