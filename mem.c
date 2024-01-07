/*
 * Crude memory management for DOSMid
 *
 * Copyright (C) 2014-2018 Mateusz Viste
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
#include "xms.h"
#include <malloc.h>  /* _ffree(), _fmalloc() */
#else
#include <stdlib.h>
#endif
#include "midi.h"
#include "mem.h" /* include self for control */
#include <string.h>  /* memcpy() */

#ifndef MSDOS
#define _fmemcpy memcpy
#define _fmalloc malloc
#define _ffree free
#endif

#define LOWMEMBUFCOUNT 64    /* how many memory pools I can try using for 'noxms' allocations */
#define LOWMEMBUFSIZE  8192  /* how big each memory pool is, in bytes */

static unsigned char far *mempool[LOWMEMBUFCOUNT];
#ifdef MSDOS
static struct xms xms;
#endif
static long nexteventid = 0;
#ifdef MSDOS
unsigned short int mem_mode = 0;
#endif
size_t mem_allocated_size = 0; /* total allocated memory counter (bytes) */


/* initializes the memory module using 'mode' method, returns the number of
 * memory kilobytes allocated */
unsigned int mem_init(int mode) {
  nexteventid = 0;
  mem_allocated_size = 0;
#ifdef MSDOS
  mem_mode = mode;
  if (mem_mode == MEM_XMS) {
    return(xms_init(&xms, 16384));
  } else {
#endif
    /* try to allocate one mem pool so we have anything to start */
    mempool[0] = _fmalloc(LOWMEMBUFSIZE);
    if (mempool[0] == NULL) { /* if malloc() failed, then abort */
      return(0);
    }
    mem_allocated_size = LOWMEMBUFSIZE;
    return(LOWMEMBUFSIZE >> 10);
#ifdef MSDOS
  }
#endif
}


/* pull an xms memory block into *ptr */
int mem_pull(long addr, void far *ptr, int sz) {
#ifdef MSDOS
  if (mem_mode == MEM_XMS) {
    return(xms_pull(&xms, addr, ptr, sz));
  } else {
#endif
    _fmemcpy(ptr, mempool[addr >> 16] + (addr & 0xffffl), sz);
    return(0);
#ifdef MSDOS
  }
#endif
}


/* push the memory block pointed by *ptr into xms */
int mem_push(void far *ptr, long addr, int sz) {
#ifdef MSDOS
  if (mem_mode == MEM_XMS) {
    return(xms_push(&xms, ptr, sz, addr));
  } else {
#endif
    _fmemcpy(mempool[addr >> 16] + (addr & 0xffffl), ptr, sz);
    return(0);
#ifdef MSDOS
  }
#endif
}


/* pushes an event to memory, and link events as they come. take care to call
 * this with event == NULL to close the song. returns 0 on success, non-zero
 * otherwise */
int pusheventqueue(const struct midi_event *event, long int *root) {
  static struct midi_event lastevent;
  static long lasteventid;
  struct midi_event far *lasteventfarptr;

  if (root != NULL) {
    lasteventid = mem_alloc(sizeof(struct midi_event));
    if (lasteventid < 0) return(-1);
    *root = lasteventid;
    memcpy(&lastevent, event, sizeof(struct midi_event));
    return(0);
  }

  lasteventfarptr = &lastevent;

  if (event == NULL) {
    lastevent.next = -1;
    mem_push(lasteventfarptr, lasteventid, sizeof(struct midi_event));
    return(0);
  }

  lastevent.next = mem_alloc(sizeof(struct midi_event));
  if (lastevent.next < 0) return(-1);
  mem_push(lasteventfarptr, lasteventid, sizeof(struct midi_event));
  lasteventid = lastevent.next;
  memcpy(&lastevent, event, sizeof(struct midi_event));
  return(0);
}


/* returns a free eventid for a new event of sz bytes */
long mem_alloc(int sz) {
  long res;
#ifdef MSDOS
  if (mem_mode == MEM_XMS) {
    res = nexteventid;
    if ((nexteventid + sz) > xms.memsize) return(-1);
    nexteventid += sz;
    mem_allocated_size += sz;
    return(res);
  } else {
#endif
    long seg, offset;
    seg = nexteventid >> 16;
    offset = nexteventid & 0xffffl;
    /* detect segment boundaries */
    if (offset + sz > LOWMEMBUFSIZE) {
      if (sz > LOWMEMBUFSIZE) return(-1); /* don't bother if requested data is bigger than a single mem pool, we're fucked anyway */
      /* otherwise try using a new mem pool */
      seg += 1;
      offset = 0;
      if (seg >= LOWMEMBUFCOUNT) return(-1);
      mempool[seg] = _fmalloc(LOWMEMBUFSIZE); /* try to alloc the extra mem pool */
      if (mempool[seg] == NULL) return(-1); /* abort if alloc failed */
      mem_allocated_size += LOWMEMBUFSIZE;
    }
    res = (seg << 16) | offset;
    /* */
    nexteventid = (seg << 16) | (offset + sz);
    return(res);
#ifdef MSDOS
  }
#endif
}


void mem_clear(void) {
  nexteventid = 0;
  mem_allocated_size = 0;
  /* if using low mem, then leave only one buffer */
#ifdef MSDOS
  if (mem_mode != MEM_XMS) {
#endif
    int i;
    for (i = 1; i < LOWMEMBUFCOUNT; i++) {
      if (mempool[i] == NULL) break;
      _ffree(mempool[i]);
      mempool[i] = NULL;
    }
    mem_allocated_size = LOWMEMBUFSIZE;
#ifdef MSDOS
  }
#endif
}


/* closes / deallocates the memory module */
void mem_close(void) {
#ifdef MSDOS
  xms.memsize = 0;
  if (mem_mode == MEM_XMS) {
    xms_close(&xms);
  } else {
#endif
    int i;
    for (i = 0; i < LOWMEMBUFCOUNT; i++) {
      if (mempool[i] == NULL) break; /* stop at first NULL mempool */
      _ffree(mempool[i]);
      mempool[i] = NULL;
    }
#ifdef MSDOS
  }
#endif
}
