/*
   Crude memory management for DOSMid

   Copyright (c) 2014, Mateusz Viste
   All rights reserved.

Redistribution and use in source and binary forms, with or without
modification, are permitted provided that the following conditions are met:

1. Redistributions of source code must retain the above copyright notice,
   this list of conditions and the following disclaimer.

2. Redistributions in binary form must reproduce the above copyright notice,
   this list of conditions and the following disclaimer in the documentation
   and/or other materials provided with the distribution.

THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE
LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
POSSIBILITY OF SUCH DAMAGE.
*/

#include <malloc.h>  /* _ffree(), _fmalloc() */
#include <stdlib.h>  /* malloc(), free() */
#include <string.h>  /* memcpy() */

#include "xms.h"
#include "midi.h"
#include "mem.h" /* include self for control */

static unsigned char far *mempool = NULL;
static int memmode = 0;
static struct xms_struct xms;

/* initializes the memory module using 'mode' method */
int mem_init(unsigned int memsize, int mode) {
  memmode = mode;
  if (memmode == MEM_XMS) {
      return(xms_init(&xms, memsize));
    } else {
      unsigned long bytesize;
      bytesize = memsize;
      bytesize <<= 10;  /* KiBs to bytes */
      mempool = _fmalloc(bytesize);
      if (mempool == NULL) return(0);
      xms.memsize = bytesize;
      return(bytesize);
  }
}

/* pull the eventid event from xms memory into *ptr */
int pullevent(long eventid, void far *ptr) {
  if (memmode == MEM_XMS) {
      return(xms_pull(&xms, eventid * sizeof(struct midi_event_t), ptr, sizeof(struct midi_event_t)));
    } else {
      _fmemcpy(ptr, mempool + eventid * sizeof(struct midi_event_t), sizeof(struct midi_event_t));
      return(0);
  }
}

/* push the event pointed by *ptr into xms */
int pushevent(void far *ptr, long eventid) {
  if (memmode == MEM_XMS) {
      return(xms_push(&xms, ptr, sizeof(struct midi_event_t), eventid * sizeof(struct midi_event_t)));
    } else {
      _fmemcpy(mempool + eventid * sizeof(struct midi_event_t), ptr, sizeof(struct midi_event_t));
      return(0);
  }
}

/* returns a free eventid for a new event */
long newevent(void) {
  static long eventid = 0;
  if ((eventid + 1) * sizeof(struct midi_event_t) <= xms.memsize) return(eventid++);
  return(-1);
}

/* closes / deallocates the memory module */
void mem_close(void) {
  if (memmode == MEM_XMS) {
      xms_close(&xms);
    } else {
      xms.memsize = 0;
      _ffree(mempool);
  }
}
