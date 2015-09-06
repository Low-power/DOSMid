/*
 * Crude memory management for DOSMid
 *
 * Copyright (c) 2014, 2015 Mateusz Viste
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

#include <malloc.h>  /* _ffree(), _fmalloc() */
#include <stdlib.h>  /* malloc(), free() */
#include <string.h>  /* memcpy() */

#include "xms.h"
#include "midi.h"
#include "mem.h" /* include self for control */

static unsigned char far *mempool = NULL;
static int memmode = 0;
static struct xms_struct xms;
static long nexteventid = 0;


/* initializes the memory module using 'mode' method */
int mem_init(unsigned int memsize, int mode) {
  memmode = mode;
  nexteventid = 0;
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


/* pull an xms memory block into *ptr */
int mem_pull(long addr, void far *ptr, int sz) {
  if (memmode == MEM_XMS) {
    return(xms_pull(&xms, addr, ptr, sz));
  } else {
    _fmemcpy(ptr, mempool + addr, sz);
    return(0);
  }
}


/* push the memory block pointed by *ptr into xms */
int mem_push(void far *ptr, long addr, int sz) {
  if (memmode == MEM_XMS) {
    return(xms_push(&xms, ptr, sz, addr));
  } else {
    _fmemcpy(mempool + addr, ptr, sz);
    return(0);
  }
}


/* pushes an event to memory, and link events as they come. take care to call
 * this with event == NULL to close the song. */
void pusheventqueue(struct midi_event_t *event, long *root) {
  static struct midi_event_t lastevent;
  static long lasteventid;
  struct midi_event_t far *lasteventfarptr;

  if (root != NULL) {
    lasteventid = mem_alloc(sizeof(struct midi_event_t));
    *root = lasteventid;
    memcpy(&lastevent, event, sizeof(struct midi_event_t));
    return;
  }

  lasteventfarptr = &lastevent;

  if (event == NULL) {
    lastevent.next = -1;
    mem_push(lasteventfarptr, lasteventid, sizeof(struct midi_event_t));
    return;
  }

  lastevent.next = mem_alloc(sizeof(struct midi_event_t));
  mem_push(lasteventfarptr, lasteventid, sizeof(struct midi_event_t));
  lasteventid = lastevent.next;
  memcpy(&lastevent, event, sizeof(struct midi_event_t));
}


/* returns a free eventid for a new event of sz bytes */
long mem_alloc(int sz) {
  if ((nexteventid + sz) < xms.memsize) {
    nexteventid += 1 + sz;
    return(nexteventid);
  }
  return(-1);
}


void mem_clear(void) {
  nexteventid = 0;
}


/* closes / deallocates the memory module */
void mem_close(void) {
  xms.memsize = 0;
  if (memmode == MEM_XMS) {
    xms_close(&xms);
  } else {
    _ffree(mempool);
  }
}
