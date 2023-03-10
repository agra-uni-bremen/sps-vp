/*
 * Copyright (c) 2020,2021 Group of Computer Architecture, University of Bremen
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

#ifndef RISCV_ISA_SYMBOLIC_CTX_H
#define RISCV_ISA_SYMBOLIC_CTX_H

#include <map>
#include <vector>
#include <optional>

#include <stdbool.h>
#include <sys/types.h>
#include <clover/clover.h>

class SymbolicContext {
private:
	unsigned current_packet_index = 0;
	unsigned packet_sequence_length = 0;

	std::map<klee::ref<klee::Expr>, bool> constraints;
	bool enforcing_assume = false;

	std::map<unsigned, std::vector<clover::ConcreteStore>> partially_explored;

public:
	clover::Solver solver;
	clover::Trace trace;
	clover::ExecutionContext ctx;

	SymbolicContext(void);

	void assume(std::shared_ptr<clover::BitVector> constraint);
	bool setupNewValues(void);

	// Prepare execution of the software with a packet sequence
	// of the specified length. That is, expect the software to
	// receive exactly N packets and terminate software execution
	// after the Nth packet has been processed.
	//
	// This also reset the current packet sequence index and should
	// thus be called before restarting software execution.
	void prepare_packet_sequence(unsigned);

	unsigned current_index(void);
	unsigned current_length(void);

	// Indicate that an additional packet of the packet sequence
	// has been fully processed by the executed RISC-V software.
	//
	// Returns true, if the VP should terminate software execution
	// thereafter, i.e. if the packet that has just been processed
	// was the last packet of the packet sequence.
	bool processed_packet(void);

	void early_exit(unsigned k);
	std::optional<clover::ConcreteStore> random_partial(unsigned k);
	void clear_partial(void);
};

extern SymbolicContext symbolic_context;

#endif
