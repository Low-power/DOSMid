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

#include <string.h> /* bcmp() */

#include "bitfield.h"
#include "fio.h"
#include "mem.h"
#include "midi.h"   /* include self for control */

#define BSWAPL(x) ((((unsigned long)(x) & 0x000000FFul) << 24) | (((unsigned long)(x) & 0x0000FF00ul) << 8) | (((unsigned long)(x) & 0x00FF0000ul) >> 8) | (((unsigned long)(x) & 0xFF000000ul) >> 24))

extern unsigned char wbuff[];

/* PRIVATE ROUTINES USED FOR INTERNAL PROCESSING ONLY */

/* fetch a variable length quantity value from a given offset. returns number of bytes read */
static int midi_fetch_variablelen_fromfile(struct fiofile_t *f, unsigned long *result) {
  unsigned char bytebuff;
  int offset = 0;
  *result = 0;
  for (;;) {
    fio_read(f, &bytebuff, 1);
    *result <<= 7;
    *result |= (bytebuff & 127);
    if ((bytebuff & 128) == 0) break;
  }
  return(offset);
}


/* reads a MIDI file and computes a map of chunks (ie a list of offsets) */
static int midi_getchunkmap(struct fiofile_t *f, struct midi_chunkmap_t *chunklist, int maxchunks) {
  int chunkid;
  for (chunkid = 0; chunkid < maxchunks; chunkid++) {
    /* read chunk's id */
    if (fio_read(f, chunklist[chunkid].id, 4) != 4) break;
    /* compute the length */
    if (fio_read(f, &(chunklist[chunkid].len), 4) != 4) break;
    chunklist[chunkid].len = BSWAPL(chunklist[chunkid].len);
    /* remember chunk data offset */
    chunklist[chunkid].offset = fio_seek(f, FIO_SEEK_CUR, 0);
    /* skip to next chunk */
    fio_seek(f, FIO_SEEK_CUR, chunklist[chunkid].len);
  }
  return(chunkid);
}

static struct midi_chunk_t *midi_readchunk(void *dstbuf, struct fiofile_t *f) {
  unsigned char id[4];
  unsigned long len = 0;
  struct midi_chunk_t *res = dstbuf;
  if (fio_read(f, id, 4) != 4) return(NULL);
  /* fetch the length */
  if (fio_read(f, &len, 4) != 4) return(NULL);
  /* */
  /* copy the id string */
  res->id[0] = id[0];
  res->id[1] = id[1];
  res->id[2] = id[2];
  res->id[3] = id[3];
  /* assign data length */
  res->datalen = BSWAPL(len);
  /* copy actual data */
  if (fio_read(f, res->data, res->datalen) != res->datalen) {
    return(NULL);
  }
  return(res);
}


/* PUBLIC INTERFACE */


int midi_readhdr(struct fiofile_t *f, int *format, int *tracks, unsigned short *timeunitdiv, struct midi_chunkmap_t *chunklist, int maxchunks) {
  struct midi_chunk_t *chunk;
  unsigned char rmidbuff[12];
  /* test for RMID format and rewind if not found */
  /* a RMID file starts with RIFFxxxxRMID (xxxx being the data size) */
  /* read first 12 bytes - if unable, return an error */
  if (fio_read(f, rmidbuff, 12) != 12) return(-7);
  /* if no RMID header, then rewind the file assuming it's normal MIDI */
  if ((rmidbuff[0] != 'R') || (rmidbuff[1] != 'I')  || (rmidbuff[2] != 'F') || (rmidbuff[3] != 'F')
   || (rmidbuff[8] != 'R') || (rmidbuff[9] != 'M') || (rmidbuff[10] != 'I') || (rmidbuff[11] != 'D')) {
    fio_seek(f, FIO_SEEK_START, 0);
  }
  /* Read the first chunk of data (should be the MIDI header) */
  chunk = midi_readchunk(chunklist, f); /* I abuse the chunklist pointer here */
  if (chunk == NULL) return(-6);

  /* check id (MThd) and len (must be at least 6 bytes) */
  if ((bcmp(chunk->id, "MThd", 4) != 0) || (chunk->datalen < 6)) {
    return(-5);
  }

  *format = (chunk->data[0] << 8) | chunk->data[1];
  *tracks = (chunk->data[2] << 8) | chunk->data[3];

  *timeunitdiv = chunk->data[4];
  *timeunitdiv <<= 8;
  *timeunitdiv |= chunk->data[5];

  /* timeunitdiv must be a positive number */
  if (*timeunitdiv < 1) return(-3);

  /* default tempo -> quarter note (1 beat) == 500'000 microseconds (0.5s), ie 120 bpm.
     a delta time unit is therefore (0.5s / DIV) long. */

  if ((*format < 0) || (*format > 2)) return(-1);

  /* read the chunk map */
  midi_getchunkmap(f, chunklist, maxchunks);

  return(0);
}


/* returns a negative value on error, 0 on success, 1 on end of track */
#ifdef DBGFILE
static int ld_meta(struct midi_event_t *event, struct fiofile_t *f, FILE *logfd, unsigned long *tracklen, char *title, int titlemaxlen, char *copyright, int copyrightmaxlen, char *text, int textmaxlen) {
#else
static int ld_meta(struct midi_event_t *event, struct fiofile_t *f, unsigned long *tracklen, char *title, int titlemaxlen, char *copyright, int copyrightmaxlen, char *text, int textmaxlen) {
#endif

  unsigned long metalen;
  unsigned long i;
  int result = 0;
  unsigned char subtype;
  fio_read(f, &subtype, 1); /* fetch the META subtype fields */
  midi_fetch_variablelen_fromfile(f, &metalen);
  /* printf("got META[0x%02X] of %ld bytes\n", subtype, metalen); */
  switch (subtype) {
    case 1: /* text or marker - often used to describe the file... */
    case 6: /* marker */
      i = 0;
      if ((text != NULL) && (text[0] == 0) && (textmaxlen > 3)) { /* title might be NULL */
        for (; i < metalen; i++) {
          if (i+1 >= textmaxlen) break; /* avoid overflow */
          fio_read(f, text + i, 1);
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
      fio_seek(f, FIO_SEEK_CUR, metalen - i);
#ifdef DBGFILE
      if (logfd != NULL) fprintf(logfd, "%lu: TEXT OR MARKER EVENT\n", *tracklen);
#endif
      break;
    case 2: /* copyright notice */
      i = 0;
      if ((copyright != NULL) && (copyright[0] == 0)) { /* take care, copyright might be NULL */
        for (; i < metalen; i++) {
          if (i+1 >= copyrightmaxlen) break; /* avoid overflow */
          fio_read(f, copyright + i, 1);
        }
        copyright[i] = 0;
      }
      fio_seek(f, FIO_SEEK_CUR, metalen - i); /* skip the rest, if we had to truncate the string */
      break;
    case 3: /* track name */
      i = 0;
      if (title != NULL) { /* title might be NULL */
        for (; i < metalen; i++) {
          if (i+1 >= titlemaxlen) break; /* avoid overflow */
          fio_read(f, title + i, 1);
        }
        title[i] = 0;
      }
      fio_seek(f, FIO_SEEK_CUR, metalen - i); /* skip the rest, if we had to truncate the string */
      break;
    case 4: /* instrument name */
      fio_seek(f, FIO_SEEK_CUR, metalen);
#ifdef DBGFILE
      if (logfd != NULL) fprintf(logfd, "%lu: INSTRUMENT EVENT (ignored)\n", *tracklen);
#endif
      break;
    case 5: /* lyric */
      fio_seek(f, FIO_SEEK_CUR, metalen);
#ifdef DBGFILE
      if (logfd != NULL) fprintf(logfd, "%lu: LYRIC EVENT (ignored)\n", *tracklen);
#endif
      break;
    case 0x21:  /* MIDI port -- no support for multi-MIDI files, I just ignore it */
      fio_seek(f, FIO_SEEK_CUR, metalen);
#ifdef DBGFILE
      if (logfd != NULL) fprintf(logfd, "%lu: MIDI PORT EVENT (ignored)\n", *tracklen);
#endif
      break;
    case 0x2F: /* end of track */
#ifdef DBGFILE
      if (logfd != NULL) fprintf(logfd, "%lu: END OF TRACK\n", *tracklen);
#endif
      result = 1;
      break;
    case 0x51:  /* set tempo */
      if (metalen != 3) {
#ifdef DBGFILE
        if (logfd != NULL) fprintf(logfd, "%lu: TEMPO ERROR\n", *tracklen);
#endif
        return(-1);
      } else {
        unsigned char b[3];
        event->type = EVENT_TEMPO;
        fio_read(f, b, 3);
        event->data.tempoval = b[0];
        event->data.tempoval <<= 8;
        event->data.tempoval |= b[1];
        event->data.tempoval <<= 8;
        event->data.tempoval |= b[2];
#ifdef DBGFILE
        if (logfd != NULL) fprintf(logfd, "%lu: TEMPO -> %lu\n", *tracklen, event->data.tempoval);
#endif
      }
      break;
    case 0x54:  /* SMPTE offset -> since I expect only format 0/1 files, I ignore this because I want to start playing asap anyway */
      fio_seek(f, FIO_SEEK_CUR, metalen);
#ifdef DBGFILE
      if (logfd != NULL) fprintf(logfd, "%lu: SMPTE OFFSET (ignored)\n", *tracklen);
#endif
      break;
    case 0x58:  /* Time signature */
      if (metalen != 4) {
#ifdef DBGFILE
        if (logfd != NULL) fprintf(logfd, "%lu: INVALID TIME SIGNATURE!\n", *tracklen);
#endif
        return(-1);
      } else {
        fio_seek(f, FIO_SEEK_CUR, metalen);
#ifdef DBGFILE
        if (logfd != NULL) fprintf(logfd, "%lu: TIME SIGNATURE (ignored)\n", *tracklen);
#endif
      }
      break;
    case 0x59:  /* key signature */
      if (metalen != 2) {
#ifdef DBGFILE
        if (logfd != NULL) fprintf(logfd, "%lu: INVALID KEY SIGNATURE!\n", *tracklen);
#endif
        return(-1);
      } else {
        fio_seek(f, FIO_SEEK_CUR, metalen);
#ifdef DBGFILE
        if (logfd != NULL) fprintf(logfd, "%lu: KEY SIGNATURE (ignored)\n", *tracklen);
#endif
      }
      break;
    case 0x7F:  /* proprietary event -> this is non-standard stuff, I ignore it */
      fio_seek(f, FIO_SEEK_CUR, metalen);
#ifdef DBGFILE
      if (logfd != NULL) fprintf(logfd, "%lu: PROPRIETARY EVENT (ignored)\n", *tracklen);
#endif
      break;
    default:
      fio_seek(f, FIO_SEEK_CUR, metalen); /* skip the meta data */
#ifdef DBGFILE
      if (logfd != NULL) fprintf(logfd, "%lu: UNHANDLED META EVENT [0x%02Xh] (ignored)\n", *tracklen, subtype);
#endif
      break;
  }
  return(result);
}


/* returns a negative value on error, 0 on success, 1 on end of track */
#ifdef DBGFILE
static int ld_sysex(struct midi_event_t *event, struct fiofile_t *f, FILE *logfd, unsigned char statusbyte, unsigned long *tracklen) {
#else
static int ld_sysex(struct midi_event_t *event, struct fiofile_t *f, unsigned char statusbyte, unsigned long *tracklen) {
#endif
  unsigned long sysexlen;
  int sysexleneven; /* can be int, guaranteed to be less than 4K */
  unsigned char *sysexbuff;
  midi_fetch_variablelen_fromfile(f, &sysexlen); /* get length */
  sysexlen += 1; /* add one byte for the status byte that is not counted, but that we will add to the top of the buffer later */
#ifdef DBGFILE
  if (logfd != NULL) fprintf(logfd, "%lu: SYSEX EVENT OF %ld BYTES ON CHAN #%d\n", *tracklen, sysexlen, statusbyte & 0x0F);
#endif
  if (sysexlen > 4096) { /* skip SYSEX events that are more than 4K big */
    fio_seek(f, FIO_SEEK_CUR, sysexlen);
    return(0);
  }
  /* read the sysex string */
  sysexleneven = sysexlen + 2; /* add two bytes for the sysex length that I will add in front of the actual sysex string */
  if ((sysexleneven & 1) != 0) sysexleneven++; /* make sysexleneven an even number (XMS moves MUST occur on even numbers of bytes) */
  sysexbuff = wbuff;
  event->type = EVENT_SYSEX;

  ((unsigned short *)sysexbuff)[0] = sysexlen;
  sysexbuff[2] = statusbyte; /* I store the entire sysex string in memory */
  fio_read(f, sysexbuff + 3, sysexlen - 1); /* read sysexlen-1 because I have already read the status byte */
  event->data.sysex.sysexptr = mem_alloc(sysexleneven);
  if (event->data.sysex.sysexptr >= 0) {
    mem_push(sysexbuff, event->data.sysex.sysexptr, sysexleneven);
  } else {
    event->type = EVENT_NONE;
#ifdef DBGFILE
    if (logfd != NULL) fprintf(logfd, "%lu: SYSEX MEM_ALLOC FAILED FOR %ld BYTES\n", *tracklen, sysexlen);
#endif
    return(MIDI_OUTOFMEM);
  }
  return(0);
}


#ifdef DBGFILE
static int ld_note(struct midi_event_t *event, struct fiofile_t *f, FILE *logfd, unsigned char statusbyte, unsigned long *tracklen, unsigned short *channelsusage, void *reqpatches) {
#else
static int ld_note(struct midi_event_t *event, struct fiofile_t *f, unsigned char statusbyte, unsigned long *tracklen, unsigned short *channelsusage, void *reqpatches) {
#endif
  unsigned char ubuff[2]; /* micro buffer for loading data */
  switch (statusbyte & 0xF0) { /* I care only about NoteOn/NoteOff events */
    case 0x80:  /* Note OFF */
      fio_read(f, ubuff, 2);
      event->type = EVENT_NOTEOFF;
      event->data.note.chan = statusbyte & 0x0F;
      event->data.note.note = ubuff[0] & 127; /* a note must be in range 0..127 */
      event->data.note.velocity = ubuff[1];
      break;
    case 0x90:  /* Note ON */
      fio_read(f, ubuff, 2);
      event->type = EVENT_NOTEON;
      event->data.note.chan = statusbyte & 0x0F;
      event->data.note.note = ubuff[0] & 127;
      event->data.note.velocity = ubuff[1];
      if (event->data.note.velocity == 0) {
        event->type = EVENT_NOTEOFF; /* if no velocity, it's in fact a note OFF */
      } else {
        *channelsusage |= (1 << event->data.note.chan); /* update the channel usage flags */
      }
      /* if it's percussion, mark the required patch */
      if (event->data.note.chan == 9) BIT_SET(reqpatches, event->data.note.note | 128);
      break;
    case 0xA0:  /* key after-touch */
      fio_read(f, ubuff, 2);
      event->type = EVENT_KEYPRESSURE;
      event->data.keypressure.chan = statusbyte & 0x0F;
      event->data.keypressure.note = ubuff[0];
      event->data.keypressure.pressure = ubuff[1];
      break;
    case 0xB0:  /* control change */
      fio_read(f, ubuff, 2);
      event->type = EVENT_CONTROL;
      event->data.control.chan = statusbyte & 0x0F;
      event->data.control.id = ubuff[0];
      event->data.control.val = ubuff[1];
      break;
    case 0xC0:  /* program (patch) change */
      fio_read(f, ubuff, 1);
      event->type = EVENT_PROGCHAN;
      event->data.prog.chan = statusbyte & 0x0F;
      event->data.prog.prog = ubuff[0] & 127;
      BIT_SET(reqpatches, event->data.prog.prog);
      break;
    case 0xD0:  /* channel after-touch (aka "channel pressure") */
      fio_read(f, ubuff, 1);
      event->type = EVENT_CHANPRESSURE;
      event->data.chanpressure.chan = statusbyte & 0x0F;
      event->data.chanpressure.pressure = ubuff[0];
      break;
    case 0xE0:  /* pitch wheel change */
      fio_read(f, ubuff, 2);
      event->type = EVENT_PITCH;
      event->data.pitch.chan = statusbyte & 0x0F;
      event->data.pitch.wheel = ubuff[1];
      event->data.pitch.wheel <<= 7;
      event->data.pitch.wheel |= ubuff[0];
      break;
    default:
#ifdef DBGFILE
      if (logfd != NULL) fprintf(logfd, "%lu: Unknown note data\n", *tracklen);
#endif
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
#ifdef DBGFILE
long midi_track2events(struct fiofile_t *f, char *title, int titlemaxlen, char *copyright, int copyrightmaxlen, char *text, int textmaxlen, unsigned short *channelsusage, FILE *logfd, unsigned long *tracklen, void *reqpatches) {
#else
long midi_track2events(struct fiofile_t *f, char *title, int titlemaxlen, char *copyright, int copyrightmaxlen, char *text, int textmaxlen, unsigned short *channelsusage, unsigned long *tracklen, void *reqpatches) {
#endif
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
    unsigned char bytebuff;
    /* read the delta time first - variable length */
    midi_fetch_variablelen_fromfile(f, &deltatime);
    *tracklen += deltatime;
    /* check the type of the event */
    /* if it's a byte with MSB set, we are dealing with running status (so it's same status as last time */
    if (fio_read(f, &bytebuff, 1) == 0) return(MIDI_TRACKERROR);
    if ((bytebuff & 128) != 0) {
      statusbyte = bytebuff;
    } else { /* get back one byte */
      fio_seek(f, FIO_SEEK_CUR, -1);
    }
    event.type = EVENT_NONE;
    event.deltatime = deltatime;
    event.next = -1;
    if (statusbyte == 0xFF) { /* META event */
#ifdef DBGFILE
      r = ld_meta(&event, f, logfd, tracklen, title, titlemaxlen, copyright, copyrightmaxlen, text, textmaxlen);
#else
      r = ld_meta(&event, f, tracklen, title, titlemaxlen, copyright, copyrightmaxlen, text, textmaxlen);
#endif
      if (r < 0) return(MIDI_TRACKERROR);
      if (r == 1) break; /* end of track */
    } else if ((statusbyte >= 0xF0) && (statusbyte <= 0xF7)) { /* SYSEX event */
#ifdef DBGFILE
      r = ld_sysex(&event, f, logfd, statusbyte, tracklen);
#else
      r = ld_sysex(&event, f, statusbyte, tracklen);
#endif
      if (r != 0) return(MIDI_TRACKERROR);
    } else if ((statusbyte >= 0x80) && (statusbyte <= 0xEF)) { /* else it's a note-related command */
#ifdef DBGFILE
      r = ld_note(&event, f, logfd, statusbyte, tracklen, channelsusage, reqpatches);
#else
      r = ld_note(&event, f, statusbyte, tracklen, channelsusage, reqpatches);
#endif
      if (r != 0) return(MIDI_TRACKERROR);
    } else { /* else it's an error - free memory we allocated and return NULL */
#ifdef DBGFILE
      if (logfd != NULL) fprintf(logfd, "Err. at offset %04lX (bytebuff = 0x%02X)\n", fio_seek(f, FIO_SEEK_CUR, 0), statusbyte);
#endif
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
