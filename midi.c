/*
 * A simple MIDI parsing library
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


int midi_readhdr(FILE *fd, int *format, int *tracks, unsigned short *timeunitdiv, struct midi_chunkmap_t *chunklist, int maxchunks) {
  struct midi_chunk_t *chunk;
  unsigned char rmidbuff[12];
  /* test for RMID format and rewind if not found */
  /* a RMID file starts with RIFFxxxxRMID (xxxx being the data size) */
  /* read first 12 bytes - if unable, return an error */
  if (fread(rmidbuff, 1, 12, fd) != 12) return(-7);
  /* if no RMID header, then rewind the file assuming it's normal MIDI */
  if ((rmidbuff[0] != 'R') || (rmidbuff[1] != 'I')  || (rmidbuff[2] != 'F') || (rmidbuff[3] != 'F')
   || (rmidbuff[8] != 'R') || (rmidbuff[9] != 'M') || (rmidbuff[10] != 'I') || (rmidbuff[11] != 'D')) {
    rewind(fd);
  }
  /* Read the first chunk of data (should be the MIDI header) */
  chunk = midi_readchunk(fd);
  if (chunk == NULL) {
    return(-6);
  }
  /* check id */
  if (strcmp(chunk->id, "MThd") != 0) {
    free(chunk);
    return(-5);
  }
  /* check len - must be at least 6 bytes */
  if (chunk->datalen < 6) {
    free(chunk);
    return(-4);
  }
  *format = (chunk->data[0] << 8) | chunk->data[1];
  *tracks = (chunk->data[2] << 8) | chunk->data[3];

  *timeunitdiv = chunk->data[4];
  *timeunitdiv <<= 8;
  *timeunitdiv |= chunk->data[5];
  /* timeunitdiv must be a positive number */
  if (*timeunitdiv < 1) {
    free(chunk);
    return(-3);
  }

  /* default tempo -> quarter note (1 beat) == 500'000 microseconds (0.5s), ie 120 bpm.
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


/* returns a negative value on error, 0 on success, 1 on end of track */
static int ld_meta(struct midi_event_t *event, FILE *fd, FILE *logfd, unsigned long *tracklen, char *title, int titlemaxlen, char *copyright, int copyrightmaxlen, char *text, int textmaxlen) {
  unsigned long metalen;
  unsigned long i;
  int subtype, result = 0;
  subtype = fgetc(fd); /* fetch the META subtype fields */
  midi_fetch_variablelen_fromfile(fd, &metalen);
  /* printf("got META[0x%02X] of %ld bytes\n", subtype, metalen); */
  switch (subtype) {
    case 1: /* text or marker - often used to describe the file... */
    case 6: /* marker */
      i = 0;
      if ((text != NULL) && (text[0] == 0) && (textmaxlen > 3)) { /* title might be NULL */
        for (; i < metalen; i++) {
          if (i+1 >= textmaxlen) break; /* avoid overflow */
          text[i] = fgetc(fd);
        }
        text[i] = 0;
        /* recompute the available maxlen */
        text += i;
        textmaxlen -= i;
        /* add a LF trailer, just in case we'd like to append more data */
        if (textmaxlen > 2) {
          *text = '\n';
          text++;
          *text = 0;
          textmaxlen--;
        }
      }
      /* skip the rest, if we had to truncate the string */
      fseek(fd, metalen - i, SEEK_CUR);
      if (logfd != NULL) fprintf(logfd, "%lu: TEXT OR MARKER EVENT\n", *tracklen);
      break;
    case 2: /* copyright notice */
      i = 0;
      if ((copyright != NULL) && (copyright[0] == 0)) { /* take care, copyright might be NULL */
        for (; i < metalen; i++) {
          if (i+1 >= copyrightmaxlen) break; /* avoid overflow */
          copyright[i] = fgetc(fd);
        }
        copyright[i] = 0;
      }
      fseek(fd, metalen - i, SEEK_CUR); /* skip the rest, if we had to truncate the string */
      break;
    case 3: /* track name */
      i = 0;
      if (title != NULL) { /* title might be NULL */
        for (; i < metalen; i++) {
          if (i+1 >= titlemaxlen) break; /* avoid overflow */
          title[i] = fgetc(fd);
        }
        title[i] = 0;
      }
      fseek(fd, metalen - i, SEEK_CUR); /* skip the rest, if we had to truncate the string */
      break;
    case 4: /* instrument name */
      fseek(fd, metalen, SEEK_CUR);
      if (logfd != NULL) fprintf(logfd, "%lu: INSTRUMENT EVENT (ignored)\n", *tracklen);
      break;
    case 5: /* lyric */
      fseek(fd, metalen, SEEK_CUR);
      if (logfd != NULL) fprintf(logfd, "%lu: LYRIC EVENT (ignored)\n", *tracklen);
      break;
    case 0x21:  /* MIDI port -- no support for multi-MIDI files, I just ignore it */
      fseek(fd, metalen, SEEK_CUR);
      if (logfd != NULL) fprintf(logfd, "%lu: MIDI PORT EVENT (ignored)\n", *tracklen);
      break;
    case 0x2F: /* end of track */
      if (logfd != NULL) fprintf(logfd, "%lu: END OF TRACK\n", *tracklen);
      result = 1;
      break;
    case 0x51:  /* set tempo */
      if (metalen != 3) {
        if (logfd != NULL) fprintf(logfd, "%lu: TEMPO ERROR\n", *tracklen);
        return(-1);
      } else {
        event->type = EVENT_TEMPO;
        event->data.tempoval = fgetc(fd);
        event->data.tempoval <<= 8;
        event->data.tempoval |= fgetc(fd);
        event->data.tempoval <<= 8;
        event->data.tempoval |= fgetc(fd);
        if (logfd != NULL) fprintf(logfd, "%lu: TEMPO -> %lu\n", *tracklen, event->data.tempoval);
      }
      break;
    case 0x54:  /* SMPTE offset -> since I expect only format 0/1 files, I ignore this because I want to start playing asap anyway */
      fseek(fd, metalen, SEEK_CUR);
      if (logfd != NULL) fprintf(logfd, "%lu: SMPTE OFFSET (ignored)\n", *tracklen);
      break;
    case 0x58:  /* Time signature */
      if (metalen != 4) {
        if (logfd != NULL) fprintf(logfd, "%lu: INVALID TIME SIGNATURE!\n", *tracklen);
        return(-1);
      } else {
        fseek(fd, metalen, SEEK_CUR);
        if (logfd != NULL) fprintf(logfd, "%lu: TIME SIGNATURE (ignored)\n", *tracklen);
      }
      break;
    case 0x59:  /* key signature */
      if (metalen != 2) {
        if (logfd != NULL) fprintf(logfd, "%lu: INVALID KEY SIGNATURE!\n", *tracklen);
        return(-1);
      } else {
        fseek(fd, metalen, SEEK_CUR);
        if (logfd != NULL) fprintf(logfd, "%lu: KEY SIGNATURE (ignored)\n", *tracklen);
      }
      break;
    case 0x7F:  /* proprietary event -> this is non-standard stuff, I ignore it */
      fseek(fd, metalen, SEEK_CUR);
      if (logfd != NULL) fprintf(logfd, "%lu: PROPRIETARY EVENT (ignored)\n", *tracklen);
      break;
    default:
      fseek(fd, metalen, SEEK_CUR);  /* skip the meta data */
      if (logfd != NULL) fprintf(logfd, "%lu: UNHANDLED META EVENT [0x%02Xh] (ignored)\n", *tracklen, subtype);
      break;
  }
  return(result);
}


/* returns a negative value on error, 0 on success, 1 on end of track */
static int ld_sysex(struct midi_event_t *event, FILE *fd, FILE *logfd, unsigned char statusbyte, unsigned long *tracklen) {
  unsigned long sysexlen;
  int sysexleneven; /* can be int, guaranteed to be less than 4K */
  unsigned char *sysexbuff;
  midi_fetch_variablelen_fromfile(fd, &sysexlen); /* get length */
  sysexlen += 1; /* add one byte for the status byte that is not counted, but that we will add to the top of the buffer later */
  if (logfd != NULL) fprintf(logfd, "%lu: SYSEX EVENT OF %ld BYTES ON CHAN #%d\n", *tracklen, sysexlen, statusbyte & 0x0F);
  if (sysexlen > 4096) { /* skip SYSEX events that are more than 4K big */
    fseek(fd, sysexlen, SEEK_CUR);
    return(0);
  }
  /* read the sysex string */
  sysexleneven = sysexlen + 2; /* add two bytes for the sysex length that I will add in front of the actual sysex string */
  if ((sysexleneven & 1) != 0) sysexleneven++; /* make sysexleneven an even number (XMS moves MUST occur on even numbers of bytes) */
  sysexbuff = malloc(sysexleneven);
  if (sysexbuff == NULL) {
    if (logfd != NULL) fprintf(logfd, "%lu: SYSEX MALLOC FAILED FOR %ld BYTES\n", *tracklen, sysexleneven);
    return(MIDI_OUTOFMEM);
  }
  event->type = EVENT_SYSEX;

  ((unsigned short *)sysexbuff)[0] = sysexlen;
  sysexbuff[2] = statusbyte; /* I store the entire sysex string in memory */
  fread(sysexbuff + 3, 1, sysexlen - 1, fd); /* read sysexlen-1 because I have already read the status byte */
  event->data.sysex.sysexptr = mem_alloc(sysexleneven);
  if (event->data.sysex.sysexptr >= 0) {
    mem_push(sysexbuff, event->data.sysex.sysexptr, sysexleneven);
  } else {
    event->type = EVENT_NONE;
    free(sysexbuff);
    sysexbuff = NULL;
    if (logfd != NULL) fprintf(logfd, "%lu: SYSEX MEM_ALLOC FAILED FOR %ld BYTES\n", *tracklen, sysexlen);
    return(MIDI_OUTOFMEM);
  }
  free(sysexbuff);
  return(0);
}


static int ld_note(struct midi_event_t *event, FILE *fd, FILE *logfd, unsigned char statusbyte, unsigned long *tracklen, unsigned short *channelsusage) {
  unsigned char ubuff[2]; /* micro buffer for loading data */
  switch (statusbyte & 0xF0) { /* I care only about NoteOn/NoteOff events */
    case 0x80:  /* Note OFF */
      fread(ubuff, 1, 2, fd);
      event->type = EVENT_NOTEOFF;
      event->data.note.chan = statusbyte & 0x0F;
      event->data.note.note = ubuff[0] & 127; /* a note must be in range 0..127 */
      event->data.note.velocity = ubuff[1];
      break;
    case 0x90:  /* Note ON */
      fread(ubuff, 1, 2, fd);
      *channelsusage |= (1 << event->data.note.chan); /* update the channel usage flags */
      event->type = EVENT_NOTEON;
      event->data.note.chan = statusbyte & 0x0F;
      event->data.note.note = ubuff[0] & 127;
      event->data.note.velocity = ubuff[1];
      if (event->data.note.velocity == 0) event->type = EVENT_NOTEOFF; /* if no velocity, it's in fact a note OFF */
      break;
    case 0xA0:  /* key after-touch */
      /* puts("KEY AFTER-TOUCH"); */
      fread(ubuff, 1, 2, fd);
      event->type = EVENT_KEYPRESSURE;
      event->data.keypressure.chan = statusbyte & 0x0F;
      event->data.keypressure.note = ubuff[0];
      event->data.keypressure.pressure = ubuff[1];
      break;
    case 0xB0:  /* control change */
      /* puts("CONTROL CHANGE"); */
      fread(ubuff, 1, 2, fd);
      event->type = EVENT_CONTROL;
      event->data.control.chan = statusbyte & 0x0F;
      event->data.control.id = ubuff[0];
      event->data.control.val = ubuff[1];
      break;
    case 0xC0:  /* program (patch) change */
      event->type = EVENT_PROGCHAN;
      event->data.prog.chan = statusbyte & 0x0F;
      event->data.prog.prog = fgetc(fd);
      break;
    case 0xD0:  /* channel after-touch (aka "channel pressure") */
      event->type = EVENT_CHANPRESSURE;
      event->data.chanpressure.chan = statusbyte & 0x0F;
      event->data.chanpressure.pressure = fgetc(fd);
      break;
    case 0xE0:  /* pitch wheel change */
      fread(ubuff, 1, 2, fd);
      event->type = EVENT_PITCH;
      event->data.pitch.chan = statusbyte & 0x0F;
      event->data.pitch.wheel = ubuff[1];
      event->data.pitch.wheel <<= 7;
      event->data.pitch.wheel |= ubuff[0];
      break;
    default:
      if (logfd != NULL) fprintf(logfd, "%lu: Unknown note data\n", *tracklen);
      return(-1);
      break;
  }
  return(0);
}


/* parse a track object and returns the id of the first events in the linked
 * list. channelsusage contains 16 flags indicating what channels are used.
 * titlemaxlen and copyrightmaxlen are the maximum lengths of the strings,
 * including the NULL terminator.
 * returns MIDI_EMPTYTRACK if no event found in the track
 * returns MIDI_TRACKERROR if the track is corrupted
 * returns MIDI_OUTOFMEM if failed to store events in memory */
long midi_track2events(FILE *fd, char *title, int titlemaxlen, char *copyright, int copyrightmaxlen, char *text, int textmaxlen, unsigned short *channelsusage, FILE *logfd, unsigned long *tracklen) {
  unsigned long deltatime;
  unsigned char statusbyte = 0;
  struct midi_event_t event;
  long result = MIDI_EMPTYTRACK;
  unsigned long ignoreddeltas = 0;

  /* zero out title and copyright strings, if provided */
  if (titlemaxlen > 0) title[0] = 0;
  if (copyrightmaxlen > 0) copyright[0] = 0;
  if (textmaxlen > 0) text[0] = 0;

  *tracklen = 0;

  for (;;) {
    int r;
    /* read the delta time first - variable length */
    midi_fetch_variablelen_fromfile(fd, &deltatime);
    *tracklen += deltatime;
    /* check the type of the event */
    /* if it's a byte with MSB set, we are dealing with running status (so it's same status as last time */
    r = fgetc(fd);
    if (r == EOF) return(MIDI_TRACKERROR);
    if ((r & 128) != 0) {
      statusbyte = r;
    } else { /* get back one byte */
      fseek(fd, -1, SEEK_CUR);
    }
    event.type = EVENT_NONE;
    event.deltatime = deltatime;
    event.next = -1;
    if (statusbyte == 0xFF) { /* META event */
      r = ld_meta(&event, fd, logfd, tracklen, title, titlemaxlen, copyright, copyrightmaxlen, text, textmaxlen);
      if (r < 0) return(MIDI_TRACKERROR);
      if (r == 1) break; /* end of track */
    } else if ((statusbyte >= 0xF0) && (statusbyte <= 0xF7)) { /* SYSEX event */
      r = ld_sysex(&event, fd, logfd, statusbyte, tracklen);
      if (r != 0) return(MIDI_TRACKERROR);
    } else if ((statusbyte >= 0x80) && (statusbyte <= 0xEF)) { /* else it's a note-related command */
      r = ld_note(&event, fd, logfd, statusbyte, tracklen, channelsusage);
      if (r != 0) return(MIDI_TRACKERROR);
    } else { /* else it's an error - free memory we allocated and return NULL */
      if (logfd != NULL) fprintf(logfd, "Err. at offset %04lX (bytebuff = 0x%02X)\n", ftell(fd), statusbyte);
      return(MIDI_TRACKERROR);
    }
    /* add the event to the queue */
    if (event.type == EVENT_NONE) {
      ignoreddeltas += event.deltatime;
    } else {
      int pusheventres;
      event.deltatime += ignoreddeltas; /* add any previously ignored delta times */
      ignoreddeltas = 0;
      /* add the event to the queue */
      if (result == MIDI_EMPTYTRACK) { /* this is the first event in the queue */
        pusheventres = pusheventqueue(&event, &result);
      } else {
        pusheventres = pusheventqueue(&event, NULL);
      }
      if (pusheventres != 0) {
        return(MIDI_OUTOFMEM);
      }
    }
  }
  if (result >= 0) {
    if (pusheventqueue(NULL, NULL) != 0) return(MIDI_OUTOFMEM); /* flush last event in buffer to memory */
  }
  return(result);
}


/* merge two MIDI tracks into a single (serialized) one. returns a "pointer"
 * to the unique track. I take care not to allocate/free memory here.
 * All notes are already in RAM after all. totlen is filled with the total
 * time of the merged tracks (in seconds). */
long midi_mergetrack(long t0, long t1, unsigned long *totlen, unsigned short timeunitdiv) {
  long res = -1, lasteventid = -1, selectedid;
  int selected;
  unsigned long curtempo = 500000l, utotlen = 0;
  struct midi_event_t event[2], lastevent;

  if (totlen != NULL) *totlen = 0;
  /* fetch first events for both tracks */
  if (t0 >= 0) mem_pull(t0, &event[0], sizeof(struct midi_event_t));
  if (t1 >= 0) mem_pull(t1, &event[1], sizeof(struct midi_event_t));
  /* start looping */
  while ((t0 >= 0) || (t1 >= 0)) {
    /* compare both tracks, and select the soonest one */
    if (t0 >= 0) {
      if ((t1 >= 0) && (event[1].deltatime < event[0].deltatime)) {
        selected = 1;
        selectedid = t1;
      } else {
        selected = 0;
        selectedid = t0;
      }
    } else {
      selected = 1;
      selectedid = t1;
    }
    /* on first iteration, make sure to assign a result */
    if (lasteventid < 0) {
      res = selectedid;
    } else if (lastevent.next != selectedid) {                        /* otherwise attach selected */
      lastevent.next = selectedid;                                    /* track to last note, and   */
      mem_push(&lastevent, lasteventid, sizeof(struct midi_event_t)); /* update last pointer (if   */
    }                                                                 /* not good already)         */
    /* save the last event into buffer for later, and remember its id */
    lasteventid = selectedid;
    memcpy(&lastevent, &(event[selected]), sizeof(struct midi_event_t));
    /* increment timer */
    if ((totlen != NULL) && (event[selected].deltatime != 0)) {
      utotlen += event[selected].deltatime * curtempo / timeunitdiv;
      while (utotlen >= 1000000lu) {
        utotlen -= 1000000lu;
        *totlen += 1;
      }
    }
    if (event[selected].type == EVENT_TEMPO) curtempo = event[selected].data.tempoval;
    /* decrement timer on the non-selected track, and synch it to xms if needed */
    /* then move along on the selected track, and fetch it from xms */
    if (selected == 0) {
      if ((t1 >= 0) && (event[0].deltatime != 0)) {
        event[1].deltatime -= event[0].deltatime;
        mem_push(&event[1], t1, sizeof(struct midi_event_t));
      }
      t0 = event[0].next;
      if (t0 >= 0) mem_pull(t0, &event[0], sizeof(struct midi_event_t));
    } else { /* selected == 1 */
      if ((t0 >= 0) && (event[1].deltatime != 0)) {
        event[0].deltatime -= event[1].deltatime;
        mem_push(&event[0], t0, sizeof(struct midi_event_t));
      }
      t1 = event[1].next;
      if (t1 >= 0) mem_pull(t1, &event[1], sizeof(struct midi_event_t));
    }
  }
  return(res);
}
