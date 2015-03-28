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

#include <stdlib.h> /* malloc */
#include <string.h> /* strcmp() */

#include "mem.h"
#include "midi.h"   /* include self for control */


/* PRIVATE ROUTINES USED FOR INTERNAL PROCESSING ONLY */


/* fetch a variable length quantity value from a given offset. returns number of bytes read */
static int midi_fetch_variablelen_fromfile(FILE *fd, unsigned long *result) {
  int bytebuff;
  int offset = 0;
  *result = 0;
  for (;;) {
    bytebuff = fgetc(fd);
    *result <<= 7;
    *result |= (bytebuff & 127);
    if ((bytebuff & 128) == 0) break;
  }
  return(offset);
}

/* reads a MIDI file and computes a map of chunks (ie a list of offsets) */
static int midi_getchunkmap(FILE *fd, struct midi_chunkmap_t *chunklist, int maxchunks) {
  int chunkid, i;
  unsigned char hdr[8];
  unsigned long readres;
  /*unsigned long chunklen;*/
  for (chunkid = 0; chunkid < maxchunks; chunkid++) {
    readres = fread(hdr, 1, 8, fd);
    if (readres != 8) break;
    chunklist[chunkid].offset = ftell(fd);
    /* read chunk's id */
    for (i = 0; i < 4; i++) chunklist[chunkid].id[i] = hdr[i];
    chunklist[chunkid].id[4] = 0; /* string terminator */
    /* compute the length */
    chunklist[chunkid].len = 0;
    for (i = 4; i < 8; i++) {
      chunklist[chunkid].len <<= 8;
      chunklist[chunkid].len |= hdr[i];
    }
    /* skip to next chunk */
    fseek(fd, chunklist[chunkid].len, SEEK_CUR);
  }
  return(chunkid);
}


/* PUBLIC INTERFACE */


struct midi_chunk_t *midi_readchunk(FILE *fd) {
  unsigned char hdr[8];
  unsigned long readres;
  int i;
  unsigned long len = 0;
  struct midi_chunk_t *res;
  readres = fread(hdr, 1, 8, fd);
  if (readres != 8) return(NULL);
  /* compute the length */
  for (i = 4; i < 8; i++) {
    len <<= 8;
    len |= hdr[i];
  }
  res = malloc(sizeof(struct midi_chunk_t) + (unsigned int)len);
  if (res == NULL) return(NULL);
  /* copy the id string */
  res->id[0] = hdr[0];
  res->id[1] = hdr[1];
  res->id[2] = hdr[2];
  res->id[3] = hdr[3];
  res->id[4] = 0;
  /* assign data length */
  res->datalen = len;
  /* copy actual data */
  readres = fread(res->data, 1, (unsigned int)res->datalen, fd);
  if (readres != res->datalen) {
    free(res);
    return(NULL);
  }
  return(res);
}

int midi_readhdr(FILE *fd, int *format, int *tracks, unsigned int *timeunitdiv, struct midi_chunkmap_t *chunklist, int maxchunks) {
  struct midi_chunk_t *chunk;
  unsigned char rmidbuff[12];
  /* test for RMID format and rewind if not found */
  /* a RMID file starts with RIFFxxxxRMID (xxxx being the data size) */
  /* read first 12 bytes - if unable, return an error */
  if (fread(rmidbuff, 1, 12, fd) != 12) return(-5);
  /* if no RMID header, then rewind the file assuming it's normal MIDI */
  if ((rmidbuff[0] != 'R') || (rmidbuff[1] != 'I')  || (rmidbuff[2] != 'F') || (rmidbuff[3] != 'F')
   || (rmidbuff[8] != 'R') || (rmidbuff[9] != 'M') || (rmidbuff[10] != 'I') || (rmidbuff[11] != 'D')) {
    rewind(fd);
  }
  /* Read the first chunk of data (should be the MIDI header) */
  chunk = midi_readchunk(fd);
  /* check id */
  if (strcmp(chunk->id, "MThd") != 0) {
    free(chunk);
    return(-4);
  }
  /* check len - must be at least 6 bytes */
  if (chunk->datalen < 6) {
    free(chunk);
    return(-3);
  }
  *format = (chunk->data[0] << 8) | chunk->data[1];
  *tracks = (chunk->data[2] << 8) | chunk->data[3];

  *timeunitdiv = chunk->data[4];
  *timeunitdiv <<= 8;
  *timeunitdiv |= chunk->data[5];

  /*  default tempo -> quarter note (1 beat) == 500'000 microseconds (0.5s), ie 120 bpm.
      a delta time unit is therefore (0.5s / DIV) long. */
  /* check the timeunit */
  if (chunk->data[4] & 128) {
    free(chunk);
    return(-2);
  }
  /* check format - must be 0, 1 or 2 */
  if ((*format < 0) || (*format > 2)) {
    free(chunk);
    return(-1);
  }
  /* read the chunk map */
  midi_getchunkmap(fd, chunklist, maxchunks);
  return(0);
}

/* parse a track object and returns the id of the first events in the linked
 * list. channelsusage contains 16 flags indicating what channels are used. */
long midi_track2events(FILE *fd, char **title, int titlenodes, int titlemaxlen, char *copyright, int copyrightmaxlen, unsigned short *channelsusage) {
  unsigned long deltatime;
  unsigned char statusbyte = 0, tmp;
  struct midi_event_t event;
  long result = -1, lastnode = -1, newnode;

  for (;;) {
    /* read the delta time first - variable length */
    midi_fetch_variablelen_fromfile(fd, &deltatime);
    /* check the type of the event */
    /* if it's a byte with MSB set, we are dealing with running status (so it's same status as last time */
    tmp = fgetc(fd);
    if ((tmp & 128) != 0) {
        statusbyte = tmp;
      } else { /* get back one byte */
        fseek(fd, -1, SEEK_CUR);
    }
    event.data.raw[0] = statusbyte;
    event.type = EVENT_NONE;
    event.deltatime = deltatime;
    event.next = -1;
    if (statusbyte == 0xFF) { /* META event */
        unsigned long metalen;
        unsigned long i;
        int subtype, endoftrack = 0;
        subtype = fgetc(fd); /* fetch the META subtype fields */
        midi_fetch_variablelen_fromfile(fd, &metalen);
        /* printf("got META[0x%02X] of %ld bytes\n", subtype, metalen); */
        switch (subtype) {
          case 1: /* text */
            fseek(fd, metalen, SEEK_CUR);
            break;
          case 2: /* copyright notice */
            for (i = 0; i < metalen; i++) {
              if (i == copyrightmaxlen) break; /* avoid overflow */
              copyright[i] = fgetc(fd);
            }
            copyright[i] = 0;
            for (; i < metalen; i++) fgetc(fd); /* skip the rest, if we had to truncate the string */
            break;
          case 3: /* track name */
            /* find the first empty track */
            {
              int titid = -1, t;
              for (t = 0; t < titlenodes; t++) {
                if (title[t][0] == 0) {
                  titid = t;
                  break;
                }
              }
              if (titid >= 0) {
                  for (i = 0; i < metalen; i++) {
                    if (i == titlemaxlen) break; /* avoid overflow */
                    title[titid][i] = fgetc(fd);
                  }
                  title[titid][i] = 0;
                } else {
                  i = 0;
              }
            }
            for (; i < metalen; i++) fgetc(fd); /* skip the rest, if we had to truncate the string */
            break;
          case 4: /* instrument name */
          case 5: /* lyric */
            fseek(fd, metalen, SEEK_CUR);
            break;
          case 0x21:  /* MIDI port -- no support for multi-MIDI files, I just ignore it */
            fseek(fd, metalen, SEEK_CUR);
            break;
          case 0x2F: /* end of track */
            endoftrack = 1;
            break;
          case 0x51:  /* set tempo */
            if (metalen != 3) {
                puts("TEMPO ERROR");
              } else {
                event.type = EVENT_TEMPO;
                event.data.tempoval = fgetc(fd);
                event.data.tempoval <<= 8;
                event.data.tempoval |= fgetc(fd);
                event.data.tempoval <<= 8;
                event.data.tempoval |= fgetc(fd);
            }
            break;
          case 0x54:  /* SMPTE offset -> since I expect only format 0/1 files, I ignore this because I want to start playing asap anyway */
            fseek(fd, metalen, SEEK_CUR);
            break;
          case 0x58:  /* Time signature */
            if (metalen != 4) {
                puts("INVALID TIME SIGNATURE");
              } else {
                fseek(fd, metalen, SEEK_CUR);
            }
            break;
          case 0x59:  /* key signature */
            if (metalen != 2) {
                puts("INVALID KEY SIGNATURE!");
              } else {
                event.type = 128 | 3; /* 128 to mark it as a 'raw event', then its length */
                event.data.raw[1] = fgetc(fd);
                event.data.raw[2] = fgetc(fd);
            }
            break;
          case 0x7F:  /* proprietary event -> this is non-standard stuff, I ignore it */
            fseek(fd, metalen, SEEK_CUR);
            break;
          default:
            printf("Unhandled meta event [0x%02X]\n", subtype);
            fseek(fd, metalen, SEEK_CUR);  /* skip the meta data */
            break;
        }
        if (endoftrack) break;
      } else if ((statusbyte >= 0xF0) && (statusbyte <= 0xF7)) { /* SYSEX event */
        unsigned long sysexlen;
        midi_fetch_variablelen_fromfile(fd, &sysexlen); /* get length */
        /* printf("got SYSEX of %ld bytes\n", sysexlen); */
        fseek(fd, sysexlen, SEEK_CUR);
      } else if ((statusbyte >= 0x80) && (statusbyte <= 0xEF)) { /* else it's a note-related command */
        event.data.note.chan = statusbyte & 0x0F;
        switch (statusbyte & 0xF0) { /* I care only about NoteOn/NoteOff events */
          case 0x80:  /* Note OFF */
            event.type = EVENT_NOTEOFF;
            event.data.note.note = fgetc(fd) & 127; /* a note must be in range 0..127 */
            event.data.note.velocity = fgetc(fd);
            break;
          case 0x90:  /* Note ON */
            *channelsusage |= (1 << event.data.note.chan); /* update the channel usage flags */
            event.type = EVENT_NOTEON;
            event.data.note.note = fgetc(fd) & 127;
            event.data.note.velocity = fgetc(fd);
            if (event.data.note.velocity == 0) event.type = EVENT_NOTEOFF; /* if no velocity, it's in fact a note OFF */
            break;
          case 0xA0:  /* key after-touch */
            /* puts("KEY AFTER-TOUCH"); */
            event.type = 128 | 3; /* 128 to mark it as a 'raw event', then its length */
            event.data.raw[1] = fgetc(fd);
            event.data.raw[2] = fgetc(fd);
            break;
          case 0xB0:  /* control change */
            event.type = 128 | 3; /* 128 to mark it as a 'raw event', then its length */
            /* puts("CONTROL CHANGE"); */
            event.data.raw[1] = fgetc(fd);
            event.data.raw[2] = fgetc(fd);
            break;
          case 0xC0:  /* program (patch) change */
            event.type = EVENT_PROGCHAN;
            event.data.prog.chan = statusbyte & 0x0F;
            event.data.prog.prog = fgetc(fd);
            break;
          case 0xD0:  /* channel after-touch */
            event.type = 128 | 2; /* 128 to mark it as a 'raw event', then its length */
            event.data.raw[1] = fgetc(fd);
            break;
          case 0xE0:  /* pitch wheel change */
            event.type = 128 | 3; /* 128 to mark it as a 'raw event', then its length */
            event.data.raw[1] = fgetc(fd);
            event.data.raw[2] = fgetc(fd);
            break;
          default:
            puts("unknown note data");
            break;
        }
      } else { /* else it's an error - free memory we allocated and return NULL */
        printf("Err. at offset %04lX (bytebuff = 0x%02X)\n", ftell(fd), statusbyte);
        return(-1);
    }
    /* add the event to the queue */
    if (event.type != EVENT_NONE) {
      int pullpushres;
      struct midi_event_t far *eventptr;
      eventptr = &event;
      /* add the event to the queue */
      newnode = newevent();
      if (newnode < 0) {
        puts("OUT OF MEMORY");
        break;
      }
      pullpushres = pushevent(eventptr, newnode);
      if (pullpushres != 0) {
        printf("PUSHEVENT ERROR %02X\n", pullpushres);
        break;
      }
      if (result < 0) {
          result = newnode;
          lastnode = newnode;
        } else {
          if (pullevent(lastnode, eventptr) != 0) {
            puts("PULLEVENT ERROR");
            break;
          }
          event.next = newnode;
          pushevent(&event, lastnode);
          lastnode = newnode;
      }
    }
  }
  return(result);
}

/* merge two MIDI tracks into a single (serialized) one. returns a "pointer"
 * to the unique track. I take care here to not allocate/free memory here.
 * All notes are already in RAM after all. totlen is filled with the total
 * time of the merged tracks (in miliseconds). */
long midi_mergetrack(long t0, long t1, unsigned long *totlen, unsigned int timeunitdiv) {
  long res = -1, lasteventid = -1, selectedid;
  int selected;
  unsigned long curtempo = 500000l, utotlen = 0;
  struct midi_event_t event[2], tmpevent;
  if (totlen != NULL) *totlen = 0;
  /* fetch first events for both tracks */
  if (t0 >= 0) pullevent(t0, &event[0]);
  if (t1 >= 0) pullevent(t1, &event[1]);
  /* start looping */
  while ((t0 >= 0) || (t1 >= 0)) {
    /* compare both tracks, and select the soonest one */
    if (t0 >= 0) {
        selected = 0;
        selectedid = t0;
        if ((t1 >= 0) && (event[1].deltatime < event[0].deltatime)) {
          selected = 1;
          selectedid = t1;
        }
      } else {
        selected = 1;
        selectedid = t1;
    }
    /* if no result yet, assign it */
    if (res < 0) {
        res = selectedid;
        lasteventid = selectedid;
      } else { /* otherwise attach the selected track to the last note, and update last pointer */
        pullevent(lasteventid, &tmpevent);
        tmpevent.next = selectedid;
        pushevent(&tmpevent, lasteventid);
        lasteventid = selectedid;
    }
    /* increment timer */
    if (totlen != NULL) {
      utotlen += event[selected].deltatime * curtempo / timeunitdiv;
      while (utotlen >= 1000000lu) {
        utotlen -= 1000000lu;
        *totlen += 1;
      }
      if (event[selected].type == EVENT_TEMPO) curtempo = event[selected].data.tempoval;
    }
    /* decrement timer on the non-selected track, and synch it to xms */
    /* move along on the selected track, and fetch it from xms */
    if (selected == 0) {
        event[1].deltatime -= event[0].deltatime;
        pushevent(&event[1], t1);
        t0 = event[0].next;
        if (t0 >= 0) pullevent(t0, &event[0]);
      } else {
        event[0].deltatime -= event[1].deltatime;
        pushevent(&event[0], t0);
        t1 = event[1].next;
        if (t1 >= 0) pullevent(t1, &event[1]);
    }
  }
  return(res);
}
