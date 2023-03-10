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

#include <optional>
#include <fstream>
#include <system_error>

#include <assert.h>
#include <errno.h>
#include <netdb.h>
#include <stdint.h>
#include <unistd.h>

#include <sys/types.h>
#include <sys/socket.h>

#include "bencode.hpp"
#include "symbolic_protocol_states.h"

static std::optional<socklen_t>
str2addr(const char *host, const char *service, struct sockaddr *dest)
{
	std::optional<socklen_t> len;
	struct addrinfo hint, *res, *r;

	memset(&hint, '\0', sizeof(hint));
	hint.ai_family = AF_UNSPEC;
	hint.ai_socktype = SOCK_STREAM;

	if (getaddrinfo(host, service, &hint, &res))
		return std::nullopt;

	len = std::nullopt;
	for (r = res; r != NULL; r = r->ai_next) {
		switch (r->ai_family) {
		case AF_INET6:
		case AF_INET:
			len = r->ai_addrlen;
			memcpy(dest, r->ai_addr, *len);
			break;
		default:
			break;
		}
	}

	freeaddrinfo(res);
	return len;
}

ProtocolStates::ProtocolStates(SymbolicContext &_ctx, std::string host, std::string service)
  : ctx(_ctx)
{
	int infd, outfd;
	sockaddr_storage addr;
	std::optional<socklen_t> len;

	len = str2addr(host.c_str(), service.c_str(), (struct sockaddr*)&addr);
	if (!len.has_value())
		throw std::system_error(EADDRNOTAVAIL, std::generic_category());

	if ((sockfd = socket(addr.ss_family, SOCK_STREAM, 0)) == -1)
		throw std::system_error(errno, std::generic_category());
	if (connect(sockfd, (struct sockaddr*)&addr, *len) == -1)
		throw std::runtime_error("couldn't connect to SPS server");

	//
	// See https://gcc.gnu.org/onlinedocs/libstdc++/manual/ext_io.html
	//

	if ((infd = dup(sockfd)) == -1)
		throw std::system_error(errno, std::generic_category());
	inbuf = new __gnu_cxx::stdio_filebuf<char>(infd, std::ios::binary|std::ios::in);
	sockin = new std::istream(inbuf);

	if ((outfd = dup(sockfd)) == -1)
		throw std::system_error(errno, std::generic_category());
	outbuf = new __gnu_cxx::stdio_filebuf<char>(outfd, std::ios::binary|std::ios::out);
	sockout = new std::ostream(outbuf);

	// close original sockfd as it has been dup'ed above.
	if (close(sockfd) == -1)
		throw std::system_error(errno, std::generic_category());
}

ProtocolStates::~ProtocolStates(void)
{
	delete sockin;
	delete inbuf;

	delete sockout;
	delete outbuf;
}

void
ProtocolStates::reset(void)
{
	bencode::encode(*sockout, bencode::list{SPS_RST, 0x0});
	sockout->flush();
	if (sockout->bad())
		throw std::runtime_error("failed reset SPS state machine");

	// Reset lastMsg
	lastMsg = nullptr;
}

void
ProtocolStates::send_message(char *buf, size_t size)
{
	// If we haven't passed the entire low-level SISL specification
	// to the software, we raise in exception (this shouldn't happen).
	if (!empty())
		throw std::runtime_error("previous message has not been fully received");

	// Send message received by client to server.
	std::string out(buf, size);
	bencode::encode(*sockout, bencode::list{SPS_DATA, out});
	sockout->flush();
	if (sockout->bad())
		throw std::runtime_error("failed to write bencode data to socket");

	// XXX: Assumption previous messages has been fully received.
	// See exception throw above.
	lastMsg = std::make_unique<SymbolicFormat>(ctx, *sockin);
}

std::shared_ptr<clover::ConcolicValue>
ProtocolStates::next_byte(void)
{
	if (!lastMsg)
		throw std::invalid_argument("no low-level SISL message available");
	return lastMsg->next_byte();
}

size_t
ProtocolStates::remaining_bytes(void)
{
	if (!lastMsg)
		return 0;
	return lastMsg->remaining_bytes();
}

bool
ProtocolStates::empty(void)
{
	return remaining_bytes() == 0;
}
