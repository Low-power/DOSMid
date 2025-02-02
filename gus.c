/*
 * Library to access the GUS (Gravis UltraSound) hardware, relying on
 * the ULTRAMID API.
 *
 * Copyright (C) 2018 Mateusz Viste
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

#ifdef MSDOS

#include "defines.h"
#include <dos.h>   /* _dos_getvect(), MK_FP, FP_SEG */
#include <stdint.h>
#include "gus.h"

#ifndef MK_FP
#define MK_FP(S,O) (void __far *)(((unsigned long int)(S) << 16) + (unsigned long int)(O))
#endif

static void (__interrupt __far *ultramidfn)(void);

/* detect whether or not a GUS is present in the system (relying on the
 * ultramid API), returns ULTRAMID's vector, or 0 on error */
int gus_find(void) {
  int v;
  /* scan vectors 0x78 to 0x7f, offset 0x103 of the vector's segment is
   *  expected to contain the string 'ULTRAMID' */
  for (v = 0x78; v < 0x80; v++) {
    void far *vaddr = _dos_getvect(v);
    uint32_t far *sig = MK_FP(FP_SEG(vaddr), 0x103);
    // Little endian only
    if (sig[0] == 0x52544c55 && sig[1] == 0x44494d41) return(v);
  }
  return(0);
}

/* opens an ULTRAMID session */
void gus_open(int v) {
  ultramidfn = _dos_getvect(v);
  /* "open" ultramid, TSR_APP_START (26) */
  __asm mov ax, 26
  ultramidfn();
}

/* preload patch p into memory */
void gus_loadpatch(int p) {
  /* TSR_LOAD_PATCH (12) */
  __asm mov ax, 12
  __asm mov cx, p
  ultramidfn();
}

/* unload all patches from memory */
void gus_unloadpatches(void) {
  /* TSR_UNLOAD_ALL_PATCHES (15) */
  __asm mov ax, 15
  ultramidfn();
}

/* sends a MIDI byte to ULTRAMID */
void gus_write(int b) {
  /* TSR_MIDI_OUT (16) */
  __asm mov ax, 16
  __asm mov cx, b
  ultramidfn();
}

/* release all notes */
void gus_allnotesoff(void) {
  /* TSR_ALL_NOTES_OFF (18) */
  __asm mov ax, 18
  ultramidfn();
}

/* close the ULTRAMID API */
void gus_close(void) {
  /* "close" ultramid, TSR_APP_END (27) */
  __asm mov ax, 27
  ultramidfn();
}

#endif
