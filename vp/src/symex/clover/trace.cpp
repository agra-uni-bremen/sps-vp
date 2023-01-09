#include <queue>

#include <assert.h>
#include <stddef.h>
#include <stdlib.h>

#include <iostream>

#include <clover/clover.h>
#include <klee/Expr/Constraints.h>
#include <klee/Expr/ExprUtil.h>

#include "fns.h"

using namespace clover;

Trace::Trace(Solver &_solver)
    : solver(_solver), cm(cs), assume_cm(assume_cs)
{
	pathCondsRoot = new Node;
	pathCondsCurrent = nullptr;
}

Trace::~Trace(void)
{
	std::queue<Node *> nodes;

	nodes.push(pathCondsRoot);
	while (!nodes.empty()) {
		Node *node = nodes.front();
		nodes.pop();

		if (node->true_branch)
			nodes.push(node->true_branch);
		if (node->false_branch)
			nodes.push(node->false_branch);

		delete node;
	}
}

void
Trace::reset(void)
{
	cs = klee::ConstraintSet();
	pathCondsCurrent = nullptr;
}

bool
Trace::addBranch(std::shared_ptr<Trace::Branch> branch, bool condition)
{
	bool ret = false;

	Node *node = nullptr;
	if (pathCondsCurrent != nullptr) {
		node = pathCondsCurrent;
	} else {
		node = pathCondsRoot;
	}

	assert(node);
	if (node->isPlaceholder()) {
		node->value = branch;
		ret = true;
	}

	if (condition) {
		if (!node->true_branch)
			node->true_branch = new Node;
		pathCondsCurrent = node->true_branch;
	} else {
		if (!node->false_branch)
			node->false_branch = new Node;
		pathCondsCurrent = node->false_branch;
	}

	return ret;
}

void
Trace::add(bool condition, std::shared_ptr<BitVector> bv, uint32_t pc, unsigned pktSeqLen)
{
	auto c = (condition) ? bv->eqTrue() : bv->eqFalse();
	cm.addConstraint(c->expr);

	auto br = std::make_shared<Branch>(Branch(bv, false, pc, pktSeqLen));
	addBranch(br, condition);
}

void
Trace::assume(std::shared_ptr<BitVector> constraint)
{
	assume_cm.addConstraint(constraint->expr);
}

klee::Query
Trace::getQuery(std::shared_ptr<BitVector> bv)
{
	auto expr = cm.simplifyExpr(cs, bv->expr);
	return klee::Query(cs, expr);
}

klee::Query
Trace::newQuery(klee::ConstraintSet &cs, Path &path)
{
	size_t query_idx = path.size() - 1;
	auto cm = klee::ConstraintManager(cs);

	for (auto c : assume_cs)
		cm.addConstraint(c);

	for (size_t i = 0; i < path.size(); i++) {
		auto branch = path.at(i).first;
		auto cond = path.at(i).second;

		auto bv = branch->bv;
		auto bvcond = (cond) ? bv->eqTrue() : bv->eqFalse();

		if (i < query_idx) {
			cm.addConstraint(bvcond->expr);
			continue;
		}

		auto expr = cm.simplifyExpr(cs, bvcond->expr);

		// This is the last expression on the path. By negating
		// it we can potentially discover a new path.
		branch->wasNegated = true;
		return klee::Query(cs, expr).negateExpr();
	}

	throw "unreachable";
}

std::optional<klee::Assignment>
Trace::findNewPath(unsigned k)
{
	std::optional<klee::Assignment> assign;

	do {
		klee::ConstraintSet cs;

		Path path;
		if (!pathCondsRoot->randomUnnegated(k, path))
			return std::nullopt; /* all branches exhausted */

		auto query = newQuery(cs, path);
		/* std::cout << "Attempting to negate new query at: 0x" << std::hex << path.back().first->addr << std::dec << std::endl; */
		assign = solver.getAssignment(query);
	} while (!assign.has_value()); /* loop until we found a sat assignment */

	assert(assign.has_value());
	return assign;
}

std::optional<klee::Assignment>
Trace::fromAssume(void)
{
	auto q = klee::Query(assume_cs, nullptr).withFalse().negateExpr();
	return solver.getAssignment(q);
}

ConcreteStore
Trace::getStore(const klee::Assignment &assign)
{
	ConcreteStore store;
	for (auto const &b : assign.bindings) {
		auto array = b.first;
		auto value = b.second;

		std::string name = array->getName();
		store[name] = intFromVector(value);
	}

	return store;
}
