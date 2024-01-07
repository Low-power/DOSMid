/*
 * x86 port I/O for UNIX
 * Copyright 2015-2024 Rivoreo
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are met:
 *
 * 1. Redistributions of source code must retain the above copyright notice,
 *    this list of conditions and the following disclaimer.
 *
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
 * AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE
 * LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
 * CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
 * SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
 * INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
 * CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
 * ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
 * POSSIBILITY OF SUCH DAMAGE.
 */

#if !defined MSDOS && (defined __i386__ || defined __amd64__ || defined __x86_64__ || defined _X86_ || defined __IA32__ || defined _M_IX86 || defined _M_AMD64)

#include <sys/param.h>
#if defined __FreeBSD__ && !defined __FreeBSD_kernel__
#define __FreeBSD_kernel__
#elif defined __LINUX__ && !defined __linux__
#define __linux__
#endif

#if defined __FreeBSD_kernel__ || defined __linux__
#ifdef __FreeBSD_kernel__
#include <sys/types.h>
#include <sys/ioctl.h>
#include <dev/io/iodev.h>
#include <machine/iodev.h>
#endif
#include <unistd.h>
#include <fcntl.h>

static int port_io_fd = -1;
#endif

void open_port_io_device() {
#if defined __FreeBSD_kernel__ || defined __linux__
	if(port_io_fd != -1) return;
	port_io_fd = open(
#ifdef __FreeBSD_kernel__
		"/dev/io",
#else
		"/dev/port",
#endif
		O_RDWR
	);
#endif
}

unsigned int inp(unsigned int port) {
#if defined __FreeBSD_kernel__ || defined __linux__
	//if(port_io_fd == -1) open_port_io_device();
	if(port_io_fd != -1) {
#ifdef __FreeBSD_kernel__
		struct iodev_pio_req req = { IODEV_PIO_READ, port, 1 };
		if(ioctl(port_io_fd, IODEV_PIO, &req) == 0) return req.val;
#elif defined __linux__
		unsigned char b;
		if(lseek(port_io_fd, port, SEEK_SET) == port && read(port_io_fd, &b, 1) == 1) return b;
#endif
	}
#endif
	unsigned char value;
#ifdef __GNUC__
	__asm__ volatile("inb	%w1, %0\n" : "=a"(value) : "d"(port));
#else
	__asm		in	value, port
#endif
	return value;
}

unsigned int outp(unsigned int port, unsigned int value) {
#if defined __FreeBSD_kernel__ || defined __linux__
	//if(port_io_fd == -1) open_port_io_device();
	if(port_io_fd != -1) {
#ifdef __FreeBSD_kernel__
		struct iodev_pio_req req = { IODEV_PIO_WRITE, port, 1, value };
		if(ioctl(port_io_fd, IODEV_PIO, &req) == 0) return req.val;
#elif defined __linux__
		unsigned char b = value;
		if(lseek(port_io_fd, port, SEEK_SET) == port && write(port_io_fd, &b, 1) == 1) return b;
#endif
	}
#endif
#ifdef __GNUC__
	__asm__ volatile("outb	%b0, %w1\n" :: "a"(value), "d"(port));
#else
	__asm		out	port, value
#endif
	return value & 0xff;
}

#endif
