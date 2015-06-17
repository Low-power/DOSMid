/*
   A simple MIDI parsing library

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

#include <stdio.h>

#ifndef midi_h_sentinel
#define midi_h_sentinel

enum midi_midievents {
  EVENT_NOTEOFF = 0,
  EVENT_NOTEON = 1,
  EVENT_TEMPO = 2,
  EVENT_RAW = 3,
  EVENT_PROGCHAN = 4,
  EVENT_NONE = 100
};

struct midi_chunk_t {
  char id[5];
  unsigned long datalen;
  unsigned char data[1];
};

struct midi_chunkmap_t {
  long offset;
  long len;
  char id[5];
};

struct midi_event_note_t {
  unsigned char note;
  unsigned char chan;
  unsigned char velocity;
};

struct midi_event_prog_t {
  unsigned char prog;
  unsigned char chan;
};

struct midi_event_t {
  long next;
  unsigned long deltatime;
  unsigned int type; /* if bit 7 set (>127) then it's a raw length */
  union {
    struct midi_event_note_t note;
    struct midi_event_prog_t prog;
    unsigned char raw[3];
    unsigned long tempoval;
  } data;
};

struct midi_chunk_t *midi_readchunk(FILE *fd);

int midi_readhdr(FILE *fd, int *format, int *tracks, unsigned int *timeunitdiv, struct midi_chunkmap_t *chunklist, int maxchunks);

/* parse a track object and returns the id of the first events in the linked list */
long midi_track2events(FILE *fd, char **title, int titlenodes, int titlemaxlen, char *copyright, int copyrightmaxlen, unsigned short *channelsusage, FILE *logfd);

/* merge two MIDI tracks into a single (serialized) one. returns a "pointer"
 * to the unique track. I take care here to not allocate/free memory here.
 * All notes are already in RAM after all. totlen is filled with the total
 * time of the merged tracks (in miliseconds). */
long midi_mergetrack(long t0, long t1, unsigned long *totlen, unsigned int timeunitdiv);

#endif
