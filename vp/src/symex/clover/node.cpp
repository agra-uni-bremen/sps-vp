#include <assert.h>
#include <stdlib.h>

#include <clover/clover.h>

using namespace clover;

#define CHECK_BRANCH(BRANCH, ...) \
	(BRANCH && BRANCH->randomUnnegated(__VA_ARGS__))

Trace::Node::Node(void)
{
	value = nullptr;

	true_branch = nullptr;
	false_branch = nullptr;
}

bool
Trace::Node::isPlaceholder(void)
{
	return this->value == nullptr;
}

bool
Trace::Node::randomUnnegated(unsigned k, Path &path)
{
	// TODO: Consider traversing tree iteratively instead of
	// recursively. Otherwise, we might ran into a stack
	// overflow with larger execution trees.
	//
	// See https://gitlab.informatik.uni-bremen.de/riscv/clover/-/issues/8

	if (isPlaceholder())
		return false;

	// Second part of pair is modified by index later
	path.push_back(std::make_pair(value, false));
	size_t idx = path.size() - 1;

	/* Randomly traverse true or false branch first */
	if (rand() % 2 == 0) {
		if (CHECK_BRANCH(true_branch, k, path)) {
			path[idx].second = true;
			return true;
		} else if (CHECK_BRANCH(false_branch, k, path)) {
			path[idx].second = false;
			return true;
		}
	} else {
		if (CHECK_BRANCH(false_branch, k, path)) {
			path[idx].second = false;
			return true;
		} else if (CHECK_BRANCH(true_branch, k, path)) {
			path[idx].second = true;
			return true;
		}
	}

	/* This prefers node in the lower tree */
	if (value->pktSeqLen >= k && !value->wasNegated && (!true_branch || !false_branch)) {
		path[idx].second = (true_branch != nullptr);
		if (path[idx].second)
			assert(false_branch == nullptr);

		/* Found undiscovered path */
		return true;
	}

	path.pop_back(); // node is not on path
	return false;
}
