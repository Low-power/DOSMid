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

#include "mem.h"
#include "midi.h"

#include "mus.h" /* include self for control */

#define TICKLEN 7143l /* length of a single MUS tick, in us */


/* pushes an event to memory. take care to call this with event == NULL to
 * close the song. */
static void addevent(struct midi_event_t *event, long *root) {
  static struct midi_event_t lastevent;
  static long lasteventid;
  struct midi_event_t far *lasteventfarptr;

  if (root != NULL) {
    lasteventid = newevent();
    *root = lasteventid;
    memcpy(&lastevent, event, sizeof(struct midi_event_t));
    return;
  }

  lasteventfarptr = &lastevent;

  if (event == NULL) {
    lastevent.next = -1;
    pushevent(lasteventfarptr, lasteventid);
    return;
  }

  lastevent.next = newevent();
  pushevent(lasteventfarptr, lasteventid);
  lasteventid = lastevent.next;
  memcpy(&lastevent, event, sizeof(struct midi_event_t));
}


/* loads a MUS file into memory, returns the id of the first event on success,
 * or -1 on error. */
long mus_load(FILE *fd, unsigned long *totlen) {
  unsigned char hdr_or_chanvol[16];
  unsigned short scorestart;
  int bytebuff, bytebuff2, loadflag = 0;
  unsigned long event_dtime, nextwait = 0;
  unsigned short event_type;
  unsigned short event_channel;
  long res = -1;
  int tickduration = 0;
  long mslen = 0;
  struct midi_event_t midievent;

  rewind(fd);
  /* read the 16 bytes header first, and populate hdr data */
  if (fread(hdr_or_chanvol, 1, 16, fd) != 16) return(-8);
  if ((hdr_or_chanvol[0] != 'M') || (hdr_or_chanvol[1] != 'U') || (hdr_or_chanvol[2] != 'S') || (hdr_or_chanvol[3] != 0x1A)) {
    return(-9);
  }
  scorestart = hdr_or_chanvol[6] | (hdr_or_chanvol[7] << 8);
  /* position the next reading position to first event */
  fseek(fd, scorestart, SEEK_SET);
  /* set tempo to 140 bpm (428571 us per quarter note) */
  memset(&midievent, 0, sizeof(struct midi_event_t));
  midievent.type = EVENT_TEMPO;
  midievent.data.tempoval = TICKLEN;
  addevent(&midievent, &res);

  /* since now on, hdr_or_chanvol is used to store volume of channels */
  memset(hdr_or_chanvol, 0, 16);

  /* read events from the MUS file and translate them into midi events */
  for (;;) {
    bytebuff = fgetc(fd);
    if (bytebuff < 0) return(-5); /* if EOF, abort with error */
    event_channel = bytebuff & 0x0F;
    bytebuff >>= 4;
    event_type = bytebuff & 7;
    bytebuff >>= 3;
    event_dtime = bytebuff; /* if the 'last' bit is set, remember to read time after the event later */
    /* read complementary data, if any */
    switch (event_type) {
      case 0: /* release note (1 byte follows) */
        bytebuff = fgetc(fd);
        if ((bytebuff & 128) != 0) return(-13); /* MSB should always be zero */
        memset(&midievent, 0, sizeof(struct midi_event_t));
        midievent.deltatime = nextwait;
        midievent.type = EVENT_NOTEOFF;
        midievent.data.note.note = bytebuff;
        midievent.data.note.chan = event_channel;
        midievent.data.note.velocity = 0;
        addevent(&midievent, NULL);
        break;
      case 1: /* play note (2 bytes follow) */
        bytebuff = fgetc(fd);
        memset(&midievent, 0, sizeof(struct midi_event_t));
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
        memset(&midievent, 0, sizeof(struct midi_event_t));
        midievent.type = 128 | 3; /* 'raw' event 3 bytes long */
        midievent.data.raw[0] = 0xE0 | event_channel;
        midievent.data.raw[1] = (bytebuff << 6) & 127;
        midievent.data.raw[2] = bytebuff >> 1;
        addevent(&midievent, NULL);
        /* TODO - MIDI says that pitch wheel is a 14bit value with the center
         * being at 0x2000, but here it's 8bit... how do I translate this? */
        break;
      case 3: /* sysex (1 byte follows) */
        bytebuff = fgetc(fd);
        if ((bytebuff & 128) != 0) return(-11); /* MSB should always be zero */
        /* TODO */
        break;
      case 4: /* control (2 bytes follow) */
        bytebuff = fgetc(fd);
        bytebuff2 = fgetc(fd);
        if ((bytebuff == 0) && (bytebuff2 < 128)) { /* change program */
          memset(&midievent, 0, sizeof(struct midi_event_t));
          midievent.deltatime = nextwait;
          midievent.type = EVENT_PROGCHAN;
          midievent.data.prog.prog = bytebuff2;
          midievent.data.note.chan = event_channel;
          addevent(&midievent, NULL);
        } else if ((bytebuff >= 1) && (bytebuff <= 9)) { /* else it maps directly to a MIDI controller message */
          int tmpmap[10] = {0,0,1,7,10,11,91,93,64,67};
          memset(&midievent, 0, sizeof(struct midi_event_t));
          midievent.deltatime = nextwait;
          midievent.type = 128 | 3; /* 'raw' event 3 bytes long */
          midievent.data.raw[0] = 0xB0 | event_channel;
          midievent.data.raw[1] = tmpmap[bytebuff];
          midievent.data.raw[2] = bytebuff2;
          addevent(&midievent, NULL);
        }
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
      event_dtime = bytebuff & 128;
      nextwait <<= 7;
      nextwait |= (bytebuff & 127);
    }
    tickduration += nextwait;
    while (tickduration >= 1000) {
      mslen += TICKLEN; /* mslen is in miliseconds, while TICKLEN is in us */
      tickduration -= 1000;
    }
  }
  mslen += (tickduration * TICKLEN) / 1000;
  *totlen = mslen / 1000; /* totlen is in seconds */
  /* */
  return(res);
}
