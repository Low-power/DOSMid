/*
 * MUS loader for DOSMid
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

#include <stdio.h>
#include <string.h> /* memcpy() */

#include <unistd.h> /* DEBUG REMOVE ME */

#include "mem.h"
#include "midi.h"

#include "mus.h" /* include self for control */


/* pushes an event to memory. take care to call this with event == NULL to
 * close the song. */
static void addevent(struct midi_event_t *event, long *root) {
  static struct midi_event_t lastevent;
  static long nexteventid;
  struct midi_event_t far *lasteventfarptr;

  if (root != NULL) {
    nexteventid = newevent();
    *root = nexteventid;
    memcpy(&lastevent, event, sizeof(struct midi_event_t));
    return;
  }

  lasteventfarptr = &lastevent;

  if (event == NULL) {
    lastevent.next = -1;
    pushevent(&lastevent, nexteventid);
    return;
  }

  lastevent.next = newevent();
  pushevent(lasteventfarptr, nexteventid);
  printf("added event: %ld (next: %ld)\n", nexteventid, lastevent.next);
  nexteventid = lastevent.next;
  memcpy(&lastevent, event, sizeof(struct midi_event_t));
  sleep(1);
}


/* loads a MUS file into memory, returns the id of the first event on success,
 * or -1 on error. */
long mus_load(FILE *fd) {
  unsigned char hdr_or_chanvol[16];
  unsigned short scorestart;
  int bytebuff, loadflag = 0;
  unsigned long event_dtime, nextwait = 0;
  unsigned short event_type;
  unsigned short event_channel;
  long res = -1;
  struct midi_event_t midievent;

  rewind(fd);
  /* read the 16 bytes header first, and populate hdr data */
  if (fread(hdr_or_chanvol, 1, 16, fd) != 16) return(-8);
  if ((hdr_or_chanvol[0] != 'M') || (hdr_or_chanvol[1] != 'U') || (hdr_or_chanvol[2] != 'S') || (hdr_or_chanvol[3] != 0x1A)) {
    return(-9);
  }
  scorestart = hdr_or_chanvol[6] | (hdr_or_chanvol[7] << 8);
  printf("Score starts at 0x%04X\n", scorestart);
  sleep(5);
  /* position the next reading position to first event */
  fseek(fd, scorestart, SEEK_SET);
  /* set tempo to 140 bpm (428571 us per quarter note) */
  memset(&midievent, 0, sizeof(midievent));
  midievent.type = EVENT_TEMPO;
  midievent.data.tempoval = 428571l;
  addevent(&midievent, &res);

  /* since now on, hdr_or_chanvol is used to store volume of channels */
  memset(hdr_or_chanvol, 0, 16);

  /* read events and translate them into midi events */
  for (;;) {
    bytebuff = fgetc(fd);
    if (bytebuff < 0) return(-5); /* if EOF, abort with error */
    event_channel = bytebuff & 0x0F;
    bytebuff >>= 4;
    event_type = bytebuff & 7;
    bytebuff >>= 3;
    event_dtime = bytebuff; /* if the 'last' bit is set, remember to read time after the event later */
    /* printf("bytebuff = 0x%02X\n", bytebuff); TODO remove me */
    /* sleep(1); */ /* TODO remove me */
    /* read complementary data, if any */
    switch (event_type) {
      case 0: /* release note (1 byte follows) */
        /* printf("NOTE OFF [0x%04X]\n", ftell(fd)); TODO remove me */
        bytebuff = fgetc(fd);
        if ((bytebuff & 128) != 0) return(-13); /* MSB should always be zero */
        memset(&midievent, 0, sizeof(midievent));
        midievent.deltatime = nextwait;
        midievent.type = EVENT_NOTEOFF;
        midievent.data.note.note = bytebuff;
        midievent.data.note.chan = event_channel;
        midievent.data.note.velocity = 0;
        addevent(&midievent, NULL);
        break;
      case 1: /* play note (2 bytes follow) */
        /* puts("NOTE ON"); TODO remove me */
        bytebuff = fgetc(fd);
        memset(&midievent, 0, sizeof(midievent));
        midievent.deltatime = nextwait;
        midievent.type = EVENT_NOTEON;
        midievent.data.note.note = bytebuff & 127;
        midievent.data.note.chan = event_channel;
        if ((bytebuff & 128) != 0) {
          midievent.data.note.velocity = fgetc(fd);
        } else {
          midievent.data.note.velocity = hdr_or_chanvol[event_channel];
        }
        addevent(&midievent, NULL);
        break;
      case 2: /* pitch wheel (1 byte follows) */
        bytebuff = fgetc(fd);
        /* TODO */
        break;
      case 3: /* sysex (1 byte follows) */
        /* printf("SYSEX [0x%04X]\n", ftell(fd)); TODO remove me */
        bytebuff = fgetc(fd);
        if ((bytebuff & 128) != 0) return(-11); /* MSB should always be zero */
        /* TODO */
        break;
      case 4: /* change program (2 bytes follow) */
        /* printf("CHNG PROG [0x%04X]\n", ftell(fd)); TODO remove me */
        bytebuff = fgetc(fd);
        bytebuff = fgetc(fd);
        /* TODO */
        break;
      case 6: /* end of song (no byte follow) */
        addevent(NULL, NULL);
        loadflag = 1;
        break;
      default: /* unknown event type - abort */
        return(-3);
        break;
    }
    /* if file loaded fine, break out of the loop now */
    if (loadflag != 0) break;
    /* if dtime is non-zero, read the number of ticks to wait before next note */
    nextwait = 0;
    while (event_dtime != 0) {
      bytebuff = fgetc(fd);
      if (bytebuff < 0) return(-6);
      nextwait <<= 7;
      nextwait += (bytebuff & 127);
      event_dtime = bytebuff & 128;
    }
  }
  /* */
  printf("MUS loaded - returning '%ld'\n", res); /* TODO remove me */
  sleep(2);
  return(res);
}
