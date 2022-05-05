#include <optional>
#include <system_error>

#include <assert.h>
#include <errno.h>
#include <netdb.h>
#include <stdint.h>
#include <unistd.h>

#include <sys/types.h>
#include <sys/socket.h>

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

static void
writeall(int fd, uint8_t *data, size_t len) {
	ssize_t ret, w;

	w = 0;
	do {
		assert(len >= (size_t)w);
		ret = write(fd, &data[w], len - (size_t)w);
		if (ret < 0)
			throw std::system_error(errno, std::generic_category());

		w += ret;
	} while ((size_t)w < len);
}

ProtocolStates::ProtocolStates(std::string host, std::string service)
{
	sockaddr_storage addr;
	std::optional<socklen_t> len;

	len = str2addr(host.c_str(), service.c_str(), (struct sockaddr*)&addr);
	if (!len.has_value())
		throw std::system_error(EADDRNOTAVAIL, std::generic_category());

	if ((sockfd = socket(addr.ss_family, SOCK_STREAM, 0)) == -1)
		throw std::system_error(errno, std::generic_category());
	if (connect(sockfd, (struct sockaddr*)&addr, *len) == -1)
		throw std::system_error(errno, std::generic_category());
}

ProtocolStates::~ProtocolStates(void)
{
	/* XXX: Use shutdown(3) as well? */
	close(sockfd);
}

std::unique_ptr<SymbolicFormat>
ProtocolStates::send_message(uint8_t *buf, size_t size)
{
	writeall(sockfd, buf, size);
	return nullptr;
}
