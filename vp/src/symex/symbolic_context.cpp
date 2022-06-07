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

#include <assert.h>
#include <stdlib.h>

#include "symbolic_explore.h"
#include "symbolic_context.h"

#define TIMEOUT_ENV "SYMEX_TIMEOUT"

// We need to pass the SymbolicContext which includes the solver,
// tracer, … to the sc_main method somehow. This cannot be done using
// function paramaters, for this reason a global variable is used
// instead.
SymbolicContext symbolic_context = SymbolicContext();

SymbolicContext::SymbolicContext(void)
	: solver(), trace(solver), ctx(solver)
{
	char *tm;

	if ((tm = getenv(TIMEOUT_ENV))) {
		auto timeout = klee::time::Span(tm);
		solver.setTimeout(timeout);
	}
}

void
SymbolicContext::assume(std::shared_ptr<clover::BitVector> constraint)
{
	auto assumed_constrained = constraint->eqTrue();
	auto expr = assumed_constrained->expr;

	bool newConstraint = false;
	if (!constraints.count(expr)) {
		trace.assume(assumed_constrained);

		constraints[expr] = true;
		newConstraint = true;
	}

	// Force all cores to exit to enforce new constraints.
	if (newConstraint) {
		enforcing_assume = true;
		symbolic_exploration::stop_assume();
	}
}

bool
SymbolicContext::setupNewValues(void)
{
	if (enforcing_assume) {
		enforcing_assume = false;
		auto assign = trace.fromAssume();
		if (!assign.has_value())
			throw std::runtime_error("unsatisfiable constraints added via assume");
		ctx.setupNewValues(trace.getStore(*assign));
		return true;
	}

	return ctx.setupNewValues(trace);
}

void
SymbolicContext::prepare_packet_sequence(unsigned k)
{
	current_packet_index = 0;
	packet_sequence_length = k;
}

unsigned
SymbolicContext::current_length(void)
{
	return packet_sequence_length;
}

bool
SymbolicContext::processed_packet(void)
{
	return ++current_packet_index >= packet_sequence_length;
}

void
SymbolicContext::early_exit(unsigned k)
{
	auto store = ctx.getPrevStore();
	// XXX: Store can be empty if packet didn't contain symbolic fields.
	//assert(!store.empty() && "early_exit ConcreteStore was empty");
	partially_explored[k].push_back(store);
}

clover::ConcreteStore
SymbolicContext::random_partial(unsigned k)
{
	size_t idx;

	if (partially_explored[k].empty()) {
		clover::ConcreteStore s;
		return s; // return empty ConcreteStore
	}

	assert(!partially_explored[k].empty());
	idx = rand() % partially_explored[k].size();

	clover::ConcreteStore store = partially_explored[k].at(idx);
	partially_explored[k].erase(partially_explored[k].begin() + idx);
	return store;
}
