#ifndef RISCV_VP_PROTOCOL_STATES_H
#define RISCV_VP_PROTOCOL_STATES_H

#include <memory>

#include <stddef.h>

#include "symbolic_format.h"
#include "symbolic_context.h"

class ProtocolStates {
private:
	int sockfd;
	SymbolicContext &ctx;

public:
	ProtocolStates(SymbolicContext &_ctx, std::string host, std::string service);
	~ProtocolStates(void);

	// Transmit a given network packet, received from the software,
	// to the state machine implementation and returns the
	// SymbolicFormat which should be used to respond to the request
	// send by the software.
	std::unique_ptr<SymbolicFormat> send_message(char *buf, size_t size);
};

#endif
