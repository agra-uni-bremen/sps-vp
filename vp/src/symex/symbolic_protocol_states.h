#ifndef RISCV_VP_PROTOCOL_STATES_H
#define RISCV_VP_PROTOCOL_STATES_H

#include <memory>

#include <stddef.h>

#include "symbolic_format.h"

class ProtocolStates {
private:
	int sockfd;

public:
	ProtocolStates(std::string host, std::string service);
	~ProtocolStates(void);

	// Transmit a given network packet, received from the software,
	// to the state machine implementation and returns the
	// SymbolicFormat which should be used to respond to the request
	// send by the software.
	std::unique_ptr<SymbolicFormat> send_message(uint8_t *buf, size_t size);
};

#endif
