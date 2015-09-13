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


#define LOWMEMBUFCOUNT 10    /* how many memory pools to allocate for 'noxms' allocations */
#define LOWMEMBUFSIZE 32768u /* how big each memory pool is, in bytes */

static unsigned char far *mempool[LOWMEMBUFCOUNT];
static int memmode = 0;
static struct xms_struct xms;
static long nexteventid = 0;


/* initializes the memory module using 'mode' method, returns the number of
 * memory kilobytes allocated */
int mem_init(unsigned int memsize, int mode) {
  memmode = mode;
  nexteventid = 0;
  if (memmode == MEM_XMS) {
    return(xms_init(&xms, memsize));
  } else {
    int i;
    xms.memsize = 0;
    for (i = 0; i < LOWMEMBUFCOUNT; i++) {
      mempool[i] = _fmalloc(LOWMEMBUFSIZE);
      if (mempool[i] == NULL) { /* malloc() failed, stop allocating */
        return(xms.memsize >> 10);
      }
      xms.memsize += LOWMEMBUFSIZE;
    }
    return(xms.memsize >> 10);
  }
}


/* pull an xms memory block into *ptr */
int mem_pull(long addr, void far *ptr, int sz) {
  if (memmode == MEM_XMS) {
    return(xms_pull(&xms, addr, ptr, sz));
  } else {
    _fmemcpy(ptr, mempool[addr >> 16] + (addr & 0xffffl), sz);
    return(0);
  }
}


/* push the memory block pointed by *ptr into xms */
int mem_push(void far *ptr, long addr, int sz) {
  if (memmode == MEM_XMS) {
    return(xms_push(&xms, ptr, sz, addr));
  } else {
    _fmemcpy(mempool[addr >> 16] + (addr & 0xffffl), ptr, sz);
    return(0);
  }
}


/* pushes an event to memory, and link events as they come. take care to call
 * this with event == NULL to close the song. returns 0 on success, non-zero
 * otherwise */
int pusheventqueue(struct midi_event_t *event, long *root) {
  static struct midi_event_t lastevent;
  static long lasteventid;
  struct midi_event_t far *lasteventfarptr;

  if (root != NULL) {
    lasteventid = mem_alloc(sizeof(struct midi_event_t));
    if (lasteventid < 0) return(-1);
    *root = lasteventid;
    memcpy(&lastevent, event, sizeof(struct midi_event_t));
    return(0);
  }

  lasteventfarptr = &lastevent;

  if (event == NULL) {
    lastevent.next = -1;
    mem_push(lasteventfarptr, lasteventid, sizeof(struct midi_event_t));
    return(0);
  }

  lastevent.next = mem_alloc(sizeof(struct midi_event_t));
  if (lastevent.next < 0) return(-1);
  mem_push(lasteventfarptr, lasteventid, sizeof(struct midi_event_t));
  lasteventid = lastevent.next;
  memcpy(&lastevent, event, sizeof(struct midi_event_t));
  return(0);
}


/* returns a free eventid for a new event of sz bytes */
/* TODO handle allocation of LOW MEM in different 'pools' */
long mem_alloc(int sz) {
  long res;
  if (memmode == MEM_XMS) {
    res = nexteventid;
    if ((nexteventid + sz) > xms.memsize) return(-1);
    nexteventid += sz;
    return(res);
  } else {
    long seg, offset;
    seg = nexteventid >> 16;
    offset = nexteventid & 0xffffl;
    /* detect segment boundaries */
    if (offset + sz > LOWMEMBUFSIZE) {
      seg += 1;
      offset = 0;
      if ((seg * LOWMEMBUFSIZE) + sz > xms.memsize) return(-1);
    }
    res = (seg << 16) | offset;
    /* */
    nexteventid = (seg << 16) | (offset + sz);
    return(res);
  }
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
    int i;
    for (i = 0; i < LOWMEMBUFCOUNT; i++) {
      if (mempool[i] == NULL) break; /* stop at first NULL mempool */
      _ffree(mempool[i]);
    }
  }
}
