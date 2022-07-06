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

#include <fstream>

#include "instr.h"
#include "core_defs.h"
#include "coverage.h"

using namespace rv32;

/* Executable section attribute in ELF section header */
enum {
	SHF_EXECINSTR = 0x04,
};

void
Coverage::init(void) {
	// Assumption: Each executable section contains
	// **only** RISC-V instructions. This is a heuristic
	// which may not necessarily hold for all ELF binaries.
	for (auto section : loader.get_sections()) {
		if (section->sh_flags & SHF_EXECINSTR)
			init_section(section);
	}
}

void
Coverage::init_section(const Elf32_Shdr *section)
{
	uint64_t addr = section->sh_addr;
	uint64_t end_addr = addr + section->sh_size;

	while (addr < end_addr) {
		uint32_t mem_word = instr_mem->load_instr(addr);
		auto instr = Instruction(mem_word);

		uint64_t last_addr = addr;
		if (instr.is_compressed()) {
			instr.decode_and_expand_compressed(RV32);
			addr += sizeof(uint16_t);
		} else {
			instr.decode_normal(RV32);
			addr += sizeof(uint32_t);
		}

		if (parser.is_included(addr)) {
			instrs[addr] = false;
			if (instr.opcode() == Opcode::OP_BEQ)
				branch_instrs[last_addr] = std::make_pair(false, false);
		}
	}
}

void
Coverage::cover_instr(uint64_t addr)
{
	if (!instrs.count(addr))
		return;

	instrs[addr] = true;
}

void
Coverage::cover_branch(uint64_t addr, bool condition)
{
	if (!branch_instrs.count(addr))
		return;

	branch_coverage &bc = branch_instrs.at(addr);
	if (condition)
		bc.first = true;
	else
		bc.second = true;
}

size_t
Coverage::executed_branches(void)
{
	size_t executed_branches = 0;

	for (auto pair : branch_instrs) {
		branch_coverage &bc = pair.second;
		if (bc.first)
			executed_branches++;
		if (bc.second)
			executed_branches++;
	}

	return executed_branches;
}

double
Coverage::dump_branch_coverage(void)
{
	size_t total_branches = (branch_instrs.size() * 2);
	size_t executed_branches = 0;

	for (auto pair : branch_instrs) {
		branch_coverage &bc = pair.second;
		if (bc.first)
			executed_branches++;
		if (bc.second)
			executed_branches++;
	}

	return (double(executed_branches) / double(total_branches)) * 100;
}

double
Coverage::dump_instr_coverage(void)
{
	size_t total_instrs = instrs.size();
	size_t executed_instrs = 0;

	for (auto pair : instrs) {
		if (pair.second)
			executed_instrs++;
	}

	return (double(executed_instrs) / double(total_instrs)) * 100;
}
