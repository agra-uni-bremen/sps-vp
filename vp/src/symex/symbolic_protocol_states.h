#ifndef RISCV_VP_PROTOCOL_STATES_H
#define RISCV_VP_PROTOCOL_STATES_H

#include <memory>
#include <istream>
#include <stddef.h>
#include <stdbool.h>
#include <clover/clover.h>
#include <ext/stdio_filebuf.h>

#include "symbolic_format.h"
#include "symbolic_context.h"

class ProtocolStates {
private:
	enum {
		SPS_DATA = 0x0,
		SPS_RST  = 0x1,
	};

	int sockfd;
	SymbolicContext &ctx;

	// See https://gcc.gnu.org/onlinedocs/libstdc++/manual/ext_io.html
	//
	// XXX: For some reason, using a combined std::iostream for both
	// reading and writing from/to the socket doesn't work.
	__gnu_cxx::stdio_filebuf<char> *inbuf = nullptr;
	__gnu_cxx::stdio_filebuf<char> *outbuf = nullptr;
	std::istream *sockin = nullptr;
	std::ostream *sockout = nullptr;

	std::unique_ptr<SymbolicFormat> lastMsg = nullptr;

public:
	ProtocolStates(SymbolicContext &_ctx, std::string host, std::string service);
	~ProtocolStates(void);

	// Transmit a given network packet, received from the software,
	// to the SPS server and block until the SPS server returns a
	// response (i.e. a new low-level SISL message).
	void send_message(char *buf, size_t size);

	// Methods to operate on the data from the previous low-level
	// SISL message send by the SPS server.
	std::shared_ptr<clover::ConcolicValue> next_byte(void);
	size_t remaining_bytes(void);
	bool empty(void);
};

#endif
