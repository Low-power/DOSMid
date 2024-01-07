/*
 * Common code for writing data to LPT
 *
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

#if defined CMSLPT || defined OPLLPT

#ifdef MSDOS
#include <conio.h>
#else
#include "unixpio.h"
#include <sys/param.h>
#if defined __FreeBSD__ && !defined __FreeBSD_kernel__
#define __FreeBSD_kernel__
#elif defined __LINUX__ && !defined __linux__
#define __linux__
#endif
#endif

// Copied from CMSLPT project
void write_lpt(unsigned int port, unsigned int byte, unsigned int ctrl) {
  outp(port, byte);
  port += 2;
  outp(port, ctrl);
  outp(port, ctrl ^ 4);         // toggle WR
  inp(port);
  if (ctrl & 1) {
    inp(port);
    inp(port);
    inp(port);
    inp(port);
    inp(port);
  }
  outp(port, ctrl);
}

#ifdef __FreeBSD_kernel__
#include <dev/ppbus/ppi.h>
#include <dev/ppbus/ppbconf.h>
#include <sys/ioctl.h>
#include <unistd.h>
#include <stdint.h>
#include "timer.h"

void write_lpt_fd(int fd, unsigned int data, unsigned int ctrl) {
  uint8_t value = data;
  ioctl(fd, PPISDATA, &value);
  value = ctrl;
  ioctl(fd, PPISCTRL, &value);
  value = ctrl ^ 4;
  ioctl(fd, PPISCTRL, &value);
  udelay((ctrl & 1) ? 4 : 1);
  value = data;
  ioctl(fd, PPISDATA, &value);
}
#elif defined __linux__
#include <sys/ioctl.h>
#include <linux/ppdev.h>
#include "timer.h"

void write_lpt_fd(int fd, unsigned int data, unsigned int ctrl) {
  unsigned char value = data;
  ioctl(fd, PPWDATA, &value);
  value = ctrl;
  ioctl(fd, PPWCONTROL, &value);
  value = ctrl ^ 4;
  ioctl(fd, PPWCONTROL, &value);
  udelay((ctrl & 1) ? 4 : 1);
  value = data;
  ioctl(fd, PPWDATA, &value);
}
#elif !defined MSDOS
#include <stdlib.h>

void write_lpt_fd(int fd, unsigned int data, unsigned int ctrl) {
  abort();
}
#endif

#endif
