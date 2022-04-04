FROM alpine:edge

RUN apk update && apk add --no-cache build-base cmake boost-dev z3-dev \
	llvm13-dev git gcc-riscv-none-elf newlib-riscv-none-elf

RUN adduser -G users -g 'RISC-V VP User' -D riscv-vp
ADD --chown=riscv-vp:users . /home/riscv-vp/riscv-vp

RUN su - riscv-vp -c 'make -C /home/riscv-vp/riscv-vp'
RUN su - riscv-vp -c "echo PATH=\"$PATH:/home/riscv-vp/riscv-vp/vp/build/bin\" >> /home/riscv-vp/.profile"
CMD su -l - riscv-vp
