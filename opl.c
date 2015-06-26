/*
 * Library to access OPL2 hardware
 *
 * Copyright (c) 2015, Mateusz Viste
 * All rights reserved.
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

#include <conio.h> /* inp(), out() */
#include <dos.h>
#include <stdio.h>

#include "opl.h" /* include self for control */


static void oplregwr(unsigned short port, unsigned short reg, unsigned char data) {
  int i;
  /* select the register we want to write to, via the index register */
  outp(port, reg);
  /* OPL2 requires 3.3us to pass before writing to the data port. AdLib
   * recommends reading 6 times from the index register to make time pass. */
  i = 6;
  while (i--) inp(port);
  /* write the data to the data register */
  outp(port+1, data);
  /* OPL2 requires 23us to pass before writing to the data port. AdLib
   * recommends reading 35 times from the index register to make time pass. */
  i = 35;
  while (i--) inp(port);
}


/*
 * Initialize hardware upon startup
 */
void oplinit(unsigned short port, int opl3flag) {
  oplregwr(port, 0x01, 0x20);    /* enable Waveform Select */
  oplregwr(port, 0x04, 0x00);    /* turn off timers IRQs */
  oplregwr(port, 0x08, 0x40);    /* turn off CSW mode */
  oplregwr(port, 0xBD, 0x00);    /* set vibrato/tremolo depth to low, set melodic mode */
}
