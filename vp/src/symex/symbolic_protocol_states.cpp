#include <optional>
#include <istream>
#include <fstream>
#include <system_error>

#include <assert.h>
#include <errno.h>
#include <netdb.h>
#include <stdint.h>
#include <unistd.h>

#include <sys/types.h>
#include <sys/socket.h>
#include <ext/stdio_filebuf.h>

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
	sockaddr_storage addr;
	std::optional<socklen_t> len;

	len = str2addr(host.c_str(), service.c_str(), (struct sockaddr*)&addr);
	if (!len.has_value())
		throw std::system_error(EADDRNOTAVAIL, std::generic_category());

	if ((sockfd = socket(addr.ss_family, SOCK_STREAM, 0)) == -1)
		throw std::system_error(errno, std::generic_category());
	if (connect(sockfd, (struct sockaddr*)&addr, *len) == -1)
		throw std::runtime_error("couldn't connect to SPS server");
}

ProtocolStates::~ProtocolStates(void)
{
	/* XXX: Use shutdown(3) as well? */
	close(sockfd);
}

void
ProtocolStates::send_message(char *buf, size_t size)
{
	// If we haven't passed the entire low-level SISL specification
	// to the software, we raise in exception (this shouldn't happen).
	if (!empty())
		throw std::runtime_error("previous message has not been fully received");

	// See https://gcc.gnu.org/onlinedocs/libstdc++/manual/ext_io.html
	__gnu_cxx::stdio_filebuf<char> sbuf(sockfd, std::ios::in|std::ios::out);
	std::iostream sock(&sbuf);

	// Send message received by client to server.
	std::string out(buf, size);
	bencode::encode(sock, out);

	// Block until the state machine server has a response
	// for us and convert that response to a SymbolicFormat.
	auto data = bencode::decode(sock, bencode::no_check_eof);

	// Hack to create symbolic format from parsed data.
	std::string tmpFile = "/tmp/protocol_state_format";
	std::fstream tmp(tmpFile, std::ios::binary|std::ios::trunc|std::ios::out);
	if (!tmp.is_open())
		throw std::runtime_error("failed to open file");
	bencode::encode(tmp, data);
	
	// XXX: Assumption previous messages has been fully received.
	// See exception throw above.
	lastMsg = std::make_unique<SymbolicFormat>(ctx, tmpFile);
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
