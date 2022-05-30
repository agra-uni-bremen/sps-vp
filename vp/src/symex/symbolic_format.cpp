/*
 * Copyright (c) 2021 Group of Computer Architecture, University of Bremen
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

#include <vector>
#include <exception>
#include <iostream>

#include <err.h>
#include <limits.h>
#include <assert.h>
#include <endian.h>
#include <errno.h>
#include <fcntl.h>
#include <stdio.h>
#include <unistd.h>

#include <sys/stat.h>

#include "symbolic_format.h"
#include "symbolic_context.h"

// True if the given bit size is NOT aligned on a byte boundary.
#define HAS_PADDING(BITSIZE) (BITSIZE % CHAR_BIT != 0)

static size_t
to_byte_size(uint64_t bitsize)
{
	uint64_t rem;
	size_t bytesize;

	// Round to next byte boundary.
	if ((rem = bitsize % CHAR_BIT) == 0)
		bytesize = bitsize / CHAR_BIT;
	else
		bytesize = ((bitsize - rem) + CHAR_BIT) / CHAR_BIT;

	return bytesize;
}

SymbolicFormat::SymbolicFormat(SymbolicContext &_ctx, std::istream &stream)
  : ctx(_ctx.ctx), solver(_ctx.solver)
{
	data = bencode::decode(stream, bencode::no_check_eof);
	if (stream.bad())
		throw std::runtime_error("failed to read bencode data from socket");

	input = get_input();
	offset = input->getWidth();

	return;
}

std::shared_ptr<clover::ConcolicValue>
SymbolicFormat::make_symbolic(std::string name, uint64_t bitsize, size_t bytesize)
{
	auto symbolic_value = ctx.getSymbolicBytes(name, bytesize);
	if (HAS_PADDING(bitsize))
		symbolic_value = symbolic_value->extract(0, bitsize);

	env[name] = *(symbolic_value->symbolic);
	return symbolic_value;
}

std::shared_ptr<clover::ConcolicValue>
SymbolicFormat::get_value(bencode::list list, std::string name, uint64_t bitsize)
{
	size_t bytesize;
	int is_symbolic;
	std::vector<uint8_t> concrete_value;
	std::shared_ptr<clover::ConcolicValue> symbolic_value;

	bytesize = to_byte_size(bitsize);

	// Value is either:
	//
	//   1. A symbolic value identified by a list of (possibly empty)
	//      constraints expressed as a string in KLEE KQuery format.
	//   2. A concrete value identified by a list of (non-empty)
	//      integers each representing a single byte of a concrete
	//      bytevector.
	//
	// For concrete values, the specified bitsize must match the
	// length of the specified bytevector.

	is_symbolic = -1;
	for (auto elem : list) {
		if (is_symbolic == -1) {
			is_symbolic = std::get_if<bencode::string>(&elem) != nullptr;
			if (is_symbolic)
				symbolic_value = make_symbolic(name, bitsize, bytesize);
		}

		if (is_symbolic) {
			bencode::string constraint;
			try {
				constraint = std::get<bencode::string>(elem);
			} catch (const std::bad_variant_access&) {
				return nullptr;
			}
			auto bv = solver.fromString(env, constraint);

			// Enforce parsed constraint via symbolic_context.
			// TODO: Build full Env first and constrain after.
			symbolic_context.assume(bv);
		} else { // is_concrete
			bencode::integer intval;
			try {
				intval = std::get<bencode::integer>(elem);
			} catch (const std::bad_variant_access&) {
				return nullptr;
			}
			if (intval < 0 || intval > UINT8_MAX)
				return nullptr;
			concrete_value.push_back((uint8_t)intval);
		}
	}

	if (is_symbolic == -1) // unconstrained symbolic value
		return make_symbolic(name, bitsize, bytesize);

	if (is_symbolic) {
		return symbolic_value;
	} else {
		if (concrete_value.size() != bytesize)
			return nullptr;

		auto bvc = solver.BVC(concrete_value.data(), concrete_value.size(), true);
		if (HAS_PADDING(bitsize))
			bvc = bvc->extract(0, bitsize);
		return bvc;
	}
}

std::shared_ptr<clover::ConcolicValue>
SymbolicFormat::get_input(void)
{
	std::shared_ptr<clover::ConcolicValue> r = nullptr;

	auto list = std::get<bencode::list>(data);
	for (auto &elem : list) {
		auto field = std::get<bencode::list>(elem);
		if (field.size() != 3)
			throw std::invalid_argument("invalid bencode field");

		auto name = std::get<bencode::string>(field[0]);
		auto size = std::get<bencode::integer>(field[1]);
		auto list = std::get<bencode::list>(field[2]);

		auto v = get_value(list, name, (uint64_t)size);
		if (!v)
			throw std::invalid_argument("invalid bencode value format");

		if (!r) {
			r = v;
			continue;
		}
		r = r->concat(v);
	}

	assert(r != nullptr);
	assert(r->getWidth() % CHAR_BIT == 0);

	return r;
}

std::shared_ptr<clover::ConcolicValue>
SymbolicFormat::next_byte(void)
{
	if (offset == 0)
		return nullptr;

	assert(offset % CHAR_BIT == 0);
	offset -= CHAR_BIT;

	auto byte = input->extract(offset, CHAR_BIT);
	assert(byte->getWidth() == CHAR_BIT);

	return byte;
}

size_t
SymbolicFormat::remaining_bytes(void)
{
	if (offset == 0)
		return 0; // empty

	auto width = input->getWidth();
	assert(width % CHAR_BIT == 0);

	return (width - (width - offset)) / CHAR_BIT;
}
