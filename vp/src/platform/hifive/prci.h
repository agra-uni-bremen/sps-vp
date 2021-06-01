/*
 * Copyright (c) 2017-2018 Group of Computer Architecture, University of Bremen <riscv@systemc-verification.org>
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without restriction, including without limitation the rights
 * to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in all
 * copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
 * SOFTWARE.
 */

#ifndef RISCV_VP_PRCI_H
#define RISCV_VP_PRCI_H

#include <systemc>

#include <tlm_utils/simple_target_socket.h>

#include "core/common/irq_if.h"
#include "util/tlm_map.h"

struct PRCI : public sc_core::sc_module {
	tlm_utils::simple_target_socket<PRCI> tsock;

	// memory mapped configuration registers
	uint32_t hfrosccfg = 0;
	uint32_t hfxosccfg = 0;
	uint32_t pllcfg = 0;
	uint32_t plloutdiv = 0;

	enum {
		HFROSCCFG_REG_ADDR = 0x0,
		HFXOSCCFG_REG_ADDR = 0x4,
		PLLCFG_REG_ADDR = 0x8,
		PLLOUTDIV_REG_ADDR = 0xC,
	};

	vp::map::LocalRouter router = {"PRCI"};

	PRCI(sc_core::sc_module_name) {
		tsock.register_b_transport(this, &PRCI::transport);

		router
		    .add_register_bank({
		        {HFROSCCFG_REG_ADDR, &hfrosccfg},
		        {HFXOSCCFG_REG_ADDR, &hfxosccfg},
		        {PLLCFG_REG_ADDR, &pllcfg},
		        {PLLOUTDIV_REG_ADDR, &plloutdiv},
		    })
		    .register_handler(this, &PRCI::register_access_callback);
	}

	void register_access_callback(const vp::map::register_access_t &r) {
		/* Pretend that the crystal oscillator output is always ready. */
		if (r.read && r.vptr == &hfxosccfg)
			hfxosccfg = 1 << 31;

		r.fn();

		if ((r.vptr == &hfrosccfg) && r.nv)
			hfrosccfg |= 1 << 31;

		if ((r.vptr == &pllcfg) && r.nv)
			pllcfg |= 1 << 31;
	}

	void transport(tlm::tlm_generic_payload &trans, sc_core::sc_time &delay) {
		router.transport(trans, delay);
	}
};

#endif  // RISCV_VP_PRCI_H
