/*
 * DOSMID - a low-requirement MIDI and MUS player for DOS
 *
 * Copyright (c) 2014, 2015, Mateusz Viste
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

#include <dos.h>
#include <stdio.h>  /* printf(), puts(), fopen()... */
#include <limits.h> /* ULONG_MAX */
#include <stdlib.h> /* malloc(), free(), rand() */
#include <string.h> /* strcmp() */
#include <time.h>

#include "mem.h"
#include "midi.h"
#include "mus.h"
#include "outdev.h"
#include "timer.h"
#include "ui.h"

#define PVER "0.7"
#define PDATE "2014-2015"

#define MAXTRACKS 64
#define EVENTSCACHESIZE 64 /* *must* be a power of 2 !!! */
#define EVENTSCACHEMASK 63 /* used by the circular events buffer */

enum playactions {
  ACTION_NONE = 0,
  ACTION_NEXT = 1,
  ACTION_PREV = 2,
  ACTION_ERR_SOFT = 3,
  ACTION_ERR_HARD = 4,
  ACTION_EXIT = 64
};

enum fileformats {
  FORMAT_UNKNOWN,
  FORMAT_MIDI,
  FORMAT_RMID,
  FORMAT_MUS
};

struct clioptions {
  int memmode;      /* type of memory to use: MEM_XMS or MEM_MALLOC */
  int workmem;      /* amount of work memory to allocate, in KiBs */
  int delay;
  int devport;
  int nopowersave;
  int dontstop;
  enum outdev_types device;
  char *devname;    /* the human name of the out device (MPU, AWE..) */
  char *midifile;   /* MIDI filename to play */
  char *playlist;   /* the playlist to read files from */
  FILE *logfd;      /* an open file descriptor to the debug log file */
};


/* copies the base name of a file (ie without directory path) into a string */
static void filename2basename(char *fromname, char *tobasename, char *todirname, int maxlen) {
  int x, x2, firstchar = 0;
  /* find the first character of the base name */
  for (x = 0; fromname[x] != 0; x++) {
    switch (fromname[x]) {
      case '/':
      case '\\':
      case ':':
        firstchar = x + 1;
        break;
    }
  }
  /* copy basename to tobasename */
  if (tobasename != NULL) {
    x2 = 0;
    for (x = firstchar; fromname[x] != 0; x++) {
      if ((fromname[x] == 0) || (x2 >= maxlen)) break;
      tobasename[x2++] = fromname[x];
    }
    tobasename[x2] = 0;
  }
  /* copy dirname to todirname */
  if (todirname != NULL) {
    x2 = 0;
    for (x = 0; x < firstchar; x++) {
      if ((fromname[x] == 0) || (x2 >= maxlen)) break;
      todirname[x2++] = fromname[x];
    }
    todirname[x2] = 0;
  }
}


/* checks whether str starts with start or not. returns 0 if so, non-zero otherwise */
static int stringstartswith(char *str, char *start) {
  if ((str == NULL) || (start == NULL)) return(-1);
  while (*start != 0) {
    if (*start != *str) return(-1);
    str++;
    start++;
  }
  return(0);
}


static int hexchar2int(char c) {
  if ((c >= '0') && (c <= '9')) return(c - '0');
  if ((c >= 'a') && (c <= 'f')) return(c - 'a');
  if ((c >= 'A') && (c <= 'F')) return(c - 'A');
  return(-1);
}


/* converts a hex string to unsigned int. stops at first null terminator or
 * space. returns zero on error. */
static unsigned int hexstr2uint(char *hexstr) {
  unsigned int v = 0;
  while ((*hexstr != 0) && (*hexstr != ' ')) {
    int c;
    c = hexchar2int(*hexstr);
    if (c < 0) return(0);
    v <<= 4;
    v |= c;
    hexstr++;
  }
  return(v);
}


/* high resolution sleeping routine, waits n microseconds */
static void udelay(unsigned long us) {
  unsigned long t1, t2;
  timer_read(&t1);
  for (;;) {
    timer_read(&t2);
    if (t2 < t1) { /* detect timer wraparound */
        break;
      } else if (t2 - t1 >= us) {
        break;
    }
  }
}


static char *devtoname(enum outdev_types device) {
  switch (device) {
    case DEV_MPU401: return("MPU");
    case DEV_AWE:    return("AWE");
    case DEV_OPL2:   return("OPL");
    default:         return("UNK");
  }
}


/* analyzes a 16 bytes file header and guess the file format */
enum fileformats header2fileformat(unsigned char *hdr) {
  /* Classic MIDI */
  if ((hdr[0] == 'M') && (hdr[1] == 'T') && (hdr[2] == 'h') && (hdr[3] == 'd')) {
    return(FORMAT_MIDI);
  }
  /* RMID inside a RIFF container */
  if ((hdr[0] == 'R') && (hdr[1] == 'I') && (hdr[2] == 'F') && (hdr[3] == 'F')
   && (hdr[8] == 'R') && (hdr[9] == 'M') && (hdr[10] == 'I') && (hdr[11] == 'D')) {
    return(FORMAT_RMID);
  }
  /* MUS (as used in Doom, from Id Software) */
  if ((hdr[0] == 'M') && (hdr[1] == 'U') && (hdr[2] == 'S') && (hdr[3] == 0x1A)) {
    return(FORMAT_MUS);
  }
  /* else I don't know */
  return(FORMAT_UNKNOWN);
}


/* loads the file's extension into ext (limited to limit characters) */
static void getfileext(char *ext, char *filename, int limit) {
  int x;
  char *extptr = NULL;
  ext[0] = 0;
  /* find the last dot first */
  while (*filename != 0) {
    if (*filename == '.') {
      extptr = filename + 1;
    }
    filename++;
  }
  if (extptr == NULL) return;
  /* copy the extension to ext, up to limit bytes */
  limit--; /* make room for the null char */
  for (x = 0; extptr[x] != 0; x++) {
    if (x >= limit) break;
    /* make sure the extension is all-lowercase */
    if ((extptr[x] >= 'A') && (extptr[x] <= 'Z')) {
      ext[x] = extptr[x] + 32;
    } else {
      ext[x] = extptr[x];
    }
  }
  ext[x] = 0; /* terminate the ext string */
}


/* parse command line params and fills the params struct accordingly. returns
   NULL on sucess, or a pointer to an error string otherwise. */
static char *parseargv(int argc, char **argv, struct clioptions *params) {
  int i;
  /* first load defaults */
  params->midifile = NULL;
  params->playlist = NULL;
  params->memmode = MEM_XMS;
  params->workmem = 16384;  /* try to use 16M of XMS memory by default */
  params->delay = 1;
  params->logfd = NULL;
  params->nopowersave = 0;
  params->dontstop = 0;
  /* if no params at all, don't waste time */
  if (argc == 0) return("");
  /* now read params */
  for (i = 1; i < argc; i++) {
    if (strcmp(argv[i], "/noxms") == 0) {
      params->memmode = MEM_MALLOC;
      params->workmem = 63; /* using 'normal' malloc, fallback to 63K */
    } else if (strcmp(argv[i], "/nodelay") == 0) {
      params->delay = 0;
    } else if (strcmp(argv[i], "/fullcpu") == 0) {
      params->nopowersave = 1;
    } else if (strcmp(argv[i], "/dontstop") == 0) {
      params->dontstop = 1;
    } else if (strcmp(argv[i], "/nosound") == 0) {
      params->device = DEV_NONE;
    } else if (strcmp(argv[i], "/awe") == 0) {
      params->device = DEV_AWE;
      params->devport = 0x620; /* EMU8000 is usually under 0x620 */
    } else if (stringstartswith(argv[i], "/mpu=") == 0) {
      char *hexstr;
      hexstr = argv[i] + 5;
      params->devport = 0;
      params->device = DEV_MPU401;
      while (*hexstr != 0) {
        int c;
        c = hexchar2int(*hexstr);
        if (c < 0) return("Invalid MPU port provided. Example: /mpu=330");
        params->devport <<= 4;
        params->devport |= c;
        hexstr++;
      }
      if (params->devport < 1) return("Invalid MPU port provided. Example: /mpu=330");
    } else if (stringstartswith(argv[i], "/log=") == 0) {
      params->logfd = fopen(argv[i] + 5, "wb");
      if (params->logfd == NULL) {
        return("Failed to open the debug log file.");
      }
    } else if ((strcmp(argv[i], "/?") == 0) || (strcmp(argv[i], "/h") == 0) || (strcmp(argv[i], "/help") == 0)) {
      return("");
    } else if ((argv[i][0] != '/') && (params->midifile == NULL) && (params->playlist == NULL)) {
      char ext[4];
      getfileext(ext, argv[i], 4);
      if (strcmp(ext, "m3u") == 0) {
        params->playlist = argv[i];
      } else {
        params->midifile = argv[i];
      }
    } else {
      return("Unknown option.");
    }
  }
  /* check if at least a MIDI filename have been provided */
  if ((params->midifile == NULL) && (params->playlist == NULL)) {
    return("You have to provide the path to a MIDI file or a playlist to play.");
  }
  /* all good */
  return(NULL);
}


/* computes the time elapsed since the song started (in secs). Returns 0 if
 * elapsed time didn't changed since last time, non-zero otherwise */
static int compute_elapsed_time(unsigned long starttime, unsigned long *elapsed) {
  unsigned long curtime, res;
  timer_read(&curtime);
  if (curtime < starttime) { /* wraparound detected */
      res = (ULONG_MAX - starttime) + curtime;
    } else {
      res = curtime - starttime;
  }
  res /= 1000000lu; /* microseconds to seconds */
  if (res == *elapsed) return(0);
  *elapsed = res;
  return(1);
}


/* check the event cache for a given event. to reset the cache, issue a single
 * call with trackpos < 0. */
static struct midi_event_t *getnexteventfromcache(struct midi_event_t *eventscache, long trackpos, int delay) {
  static unsigned int itemsincache = 0;
  static unsigned int curcachepos = 0;
  struct midi_event_t *res = NULL;
  long nextevent;
  /* if trackpos < 0 then this is only about flushing cache */
  if (trackpos < 0) {
    memset(eventscache, 0, sizeof(*eventscache));
    itemsincache = 0;
    curcachepos = 0;
    return(NULL);
  }
  /* if we have available cache */
  if (itemsincache > 0) {
      curcachepos++;
      curcachepos &= EVENTSCACHEMASK;
      itemsincache--;
      res = &eventscache[curcachepos];
      /* if we have some free time, refill the cache proactively */
      if (res->deltatime > 0) {
        int nextslot, pullres;
        /* sleep 2ms after a MIDI OUT write, and before accessing XMS.
           This is especially important for SoundBlaster "AWE" cards with the
           AWEUTIL TSR midi emulation enabled, without this AWEUTIL crashes. */
        if (delay != 0) udelay(2000);
        nextslot = curcachepos + itemsincache;
        nextevent = eventscache[nextslot & EVENTSCACHEMASK].next;
        while ((itemsincache < EVENTSCACHESIZE - 1) && (nextevent >= 0)) {
          nextslot++;
          nextslot &= EVENTSCACHEMASK;
          pullres = pullevent(nextevent, &eventscache[nextslot]);
          if (pullres != 0) {
            /* printf("pullevent() ERROR: %u (eventid = %ld)\n", pullres, trackpos); */
            return(NULL);
          }
          nextevent = eventscache[nextslot].next;
          itemsincache++;
        }
      }
    } else { /* need to refill the cache NOW */
      int refillcount, pullres;
      /* sleep 2ms after a MIDI OUT write, and before accessing XMS.
         this is especially important for SoundBlaster "AWE" cards with the
         AWEUTIL TSR midi emulation enabled, without this AWEUTIL crashes. */
      if (delay != 0) udelay(2000);
      nextevent = trackpos;
      curcachepos = 0;
      for (refillcount = 0; refillcount < EVENTSCACHESIZE; refillcount++) {
        pullres = pullevent(nextevent, &eventscache[refillcount]);
        if (pullres != 0) {
          /* printf("pullevent() ERROR: %u (eventid = %ld)\n", pullres, trackpos); */
          return(NULL);
        }
        nextevent = eventscache[refillcount].next;
        itemsincache++;
        if (nextevent < 0) break;
      }
      itemsincache--;
      res = eventscache;
  }
  return(res);
}


/* reads the BLASTER variable for best guessing of current hardware and port.
 * If nothing found, fallbacks to MPU and 0x330 */
static void preload_outdev(struct clioptions *params) {
  char *blaster;
  unsigned short mpu = 0;
  unsigned short awe = 0;
  unsigned short *portptr;

  /* check if a blaster variable is present */
  blaster = getenv("BLASTER");
  /* if so, read it looking for 'P' and 'E' parameters */
  if (blaster != NULL) {
    char *blasterptr[16];
    int blastercount = 0;

    /* read the variable in a first pass to collect all starting points */
    if (*blaster != 0) {
      blasterptr[blastercount++] = blaster++;
    }
    for (;;) {
      if (*blaster == ' ') {
        blasterptr[blastercount++] = ++blaster;
      } else if ((*blaster == 0) || (blastercount >= 16)) {
        break;
      } else {
        blaster++;
      }
    }

    while (blastercount-- > 0) {
      unsigned short p;
      blaster = blasterptr[blastercount];
      /* have we found an interesting param? */
      if ((*blaster != 'P') && (*blaster != 'E')) continue;
      if (*blaster == 'E') {
        portptr = &awe;
      } else {
        portptr = &mpu;
      }
      /* read the param value into a variable */
      p = hexstr2uint(blaster + 1);
      /* if what we have read looks sane, keep it */
      if (p > 0) *portptr = p;
    }
  }

  /* look at what we got, and choose in order of preference */
  if (awe > 0) { /* AWE is the most desirable, if present */
    params->device = DEV_AWE;
    params->devport = awe;
  } else if (mpu > 0) { /* then look for MPU */
    params->device = DEV_MPU401;
    params->devport = mpu;
  } else { /* if all else fails, fallback to MPU401 on 0x330 */
    params->device = DEV_MPU401;
    params->devport = 0x330;
  }
}


/* reads a position from an M3U file and returns a ptr to it from a static mem */
static char *getnextm3uitem(char *playlist) {
  static char fnamebuf[256];
  char tempstr[256];
  long fsize;
  long pos;
  int slen;
  char *ptr;
  FILE *fd;
  /* open the playlist and read its size */
  fd = fopen(playlist, "rb");
  if (fd == NULL) return(NULL);
  fseek(fd, 0, SEEK_END);
  fsize = ftell(fd);
  /* go to a random position */
  pos = rand() % (fsize - 2);
  fseek(fd, pos, SEEK_SET);
  /* rewind back to nearest \n or 0 position */
  while ((pos > 0) && (fgetc(fd) != '\n')) {
    fseek(fd, -2, SEEK_CUR);
    pos--;
  }
  /* read the string into fnamebuf */
  slen = fread(fnamebuf, 1, sizeof(fnamebuf), fd);
  if (slen > 0) {
    fnamebuf[slen - 1] = 0;
  } else {
    fnamebuf[0] = 0;
  }
  /* close the file descriptor */
  fclose(fd);
  /* trim out the first line */
  for (ptr = fnamebuf; *ptr != '\r' && *ptr != '\n' && *ptr != 0; ptr++);
  *ptr = 0;
  /* if the file is a relative path, then prepend it with the path of the playlist */
  if ((fnamebuf[0] == 0) || (fnamebuf[1] != ':')) {
    strcpy(tempstr, fnamebuf);
    filename2basename(playlist, NULL, fnamebuf, sizeof(fnamebuf) - 1);
    strncat(fnamebuf, tempstr, sizeof(fnamebuf) - 1);
  }
  /* return the result */
  return(fnamebuf);
}


static enum playactions loadfile_midi(FILE *fd, struct clioptions *params, struct trackinfodata *trackinfo, long *trackpos) {
  struct midi_chunkmap_t *chunkmap;
  int miditracks;
  int i;
  long newtrack;

  *trackpos = -1;

  if (fd == NULL) return(ACTION_ERR_HARD);

  chunkmap = malloc(sizeof(struct midi_chunkmap_t) * MAXTRACKS);
  if (chunkmap == NULL) {
    ui_puterrmsg(params->midifile, "Error: Out of memory");
    return(ACTION_ERR_HARD);
  }

  if (midi_readhdr(fd, &(trackinfo->midiformat), &miditracks, &(trackinfo->miditimeunitdiv), chunkmap, MAXTRACKS) != 0) {
    ui_puterrmsg(params->midifile, "Error: Invalid MIDI file format");
    free(chunkmap);
    return(ACTION_ERR_SOFT);
  }

  if (params->logfd != NULL) fprintf(params->logfd, "LOADED FILE '%s': format=%d tracks=%d timeunitdiv=%u\n", params->midifile, trackinfo->midiformat, miditracks, trackinfo->miditimeunitdiv);

  if ((trackinfo->midiformat != 0) && (trackinfo->midiformat != 1)) {
    char errstr[64];
    sprintf(errstr, "Error: Unsupported MIDI format (%d)", trackinfo->midiformat);
    ui_puterrmsg(params->midifile, errstr);
    free(chunkmap);
    return(ACTION_ERR_SOFT);
  }

  if (miditracks > MAXTRACKS) {
    char errstr[64];
    sprintf(errstr, "Error: Too many tracks (%d, max: %d)", miditracks, MAXTRACKS);
    ui_puterrmsg(params->midifile, errstr);
    free(chunkmap);
    return(ACTION_ERR_SOFT);
  }

  for (i = 0; i < miditracks; i++) {
    /* is it really a track we got here? */
    if (strcmp(chunkmap[i].id, "MTrk") != 0) {
      char errstr[64];
      sprintf(errstr, "Error: Unexpected chunk (expecting mtrk #%d)", i);
      ui_puterrmsg(params->midifile, errstr);
      free(chunkmap);
      return(ACTION_ERR_SOFT);
    }

    if (params->logfd != NULL) fprintf(params->logfd, "LOADING TRACK %d FROM OFFSET 0x%04X\n", i, chunkmap[i].offset);
    fseek(fd, chunkmap[i].offset, SEEK_SET);
    if (i == 0) {
        newtrack = midi_track2events(fd, trackinfo->title, UI_TITLENODES, UI_TITLEMAXLEN, trackinfo->copyright, UI_COPYRIGHTMAXLEN, &(trackinfo->channelsusage), params->logfd);
      } else {
        newtrack = midi_track2events(fd, NULL, 0, UI_TITLEMAXLEN, NULL, 0, &(trackinfo->channelsusage), params->logfd);
    }
    if (params->logfd != NULL) fprintf(params->logfd, "TRACK %d LOADED -> MERGING NOW\n", i);
    *trackpos = midi_mergetrack(*trackpos, newtrack, &(trackinfo->totlen), trackinfo->miditimeunitdiv);
    /* printf("merged track starts with %ld\n", trackpos); */
  }
  free(chunkmap);

  return(ACTION_NONE);
}


static enum playactions loadfile(struct clioptions *params, struct trackinfodata *trackinfo, long *trackpos) {
  FILE *fd;
  unsigned char hdr[16];
  enum playactions res;
  enum fileformats fileformat;

  /* flush all MIDI events from memory for new events to have where to load */
  flushevents();

  /* (try to) open the music file */
  fd = fopen(params->midifile, "rb");
  if (fd == NULL) {
    ui_puterrmsg(params->midifile, "Error: Failed to open the file");
    return(ACTION_ERR_SOFT);
  }

  /* read first few bytes of the file to detect its format, and rewind */
  if (fread(hdr, 1, 16, fd) != 16) {
    fclose(fd);
    ui_puterrmsg(params->midifile, "Error: Unknown file format");
    return(ACTION_ERR_SOFT);
  }
  rewind(fd);

  /* analyze the header to guess the format of the file */
  fileformat = header2fileformat(hdr);

  /* load file if format recognized */
  switch (fileformat) {
    case FORMAT_MIDI:
    case FORMAT_RMID:
      res = loadfile_midi(fd, params, trackinfo, trackpos);
      break;
    case FORMAT_MUS:
      /* memset(trackinfo, 0, sizeof(struct trackinfodata)); */ /* should I ? */
      *trackpos = mus_load(fd, &(trackinfo->totlen), &(trackinfo->miditimeunitdiv), &(trackinfo->channelsusage));
      sleep(1);
      if (*trackpos < 0) {
        char msg[64];
        res = ACTION_ERR_SOFT;
        snprintf(msg, 64, "Error: Failed to load the MUS file (%ld)", *trackpos);
        ui_puterrmsg(params->midifile, msg);
      } else {
        res = ACTION_NONE;
      }
      break;
    default:
      res = ACTION_ERR_SOFT;
      ui_puterrmsg(params->midifile, "Error: Unknown file format");
      break;
  }

  fclose(fd);
  return(res);
}


/* plays a file. returns 0 on success, non-zero if the program must exit */
static enum playactions playfile(struct clioptions *params, struct trackinfodata *trackinfo, struct midi_event_t *eventscache) {
  static int volume = 100; /* volume is static because it needs to be retained between songs */
  int i;
  enum playactions exitaction;
  unsigned long nexteventtime;
  int refreshflags;
  long trackpos;
  unsigned long midiplaybackstart;
  struct midi_event_t *curevent;
  unsigned long elticks = 0;

  /* clear out trackinfo & cache data */
  memset(trackinfo, 0, sizeof(*trackinfo));
  getnexteventfromcache(eventscache, -1, 0);

  /* set default params for trackinfo variables */
  trackinfo->tempo = 500000l;
  for (i = 0; i < 16; i++) trackinfo->chanprogs[i] = i;
  for (i = 0; i < UI_TITLENODES; i++) trackinfo->title[i] = trackinfo->titledat[i];

  /* if running on a playlist, load next song */
  if (params->playlist != NULL) {
    params->midifile = getnextm3uitem(params->playlist);
    if (params->midifile == NULL) {
      ui_puterrmsg(params->playlist, "Error: Failed to fetch an entry from the playlist");
      return(ACTION_ERR_SOFT);
    }
  }

  /* draw the UI a first time, without data yet */
  refreshflags = 0xff;
  sprintf(trackinfo->title[0], "Loading...");
  filename2basename(params->midifile, trackinfo->filename, NULL, UI_FILENAMEMAXLEN);
  ui_draw(trackinfo, &refreshflags, PVER, params->devname, params->devport, volume);
  memset(trackinfo->title[0], 0, 16);
  refreshflags = 0xff;

  exitaction = loadfile(params, trackinfo, &trackpos);
  if (exitaction != ACTION_NONE) return(exitaction);

  if (params->logfd != NULL) fputs("RESET MPU-401", params->logfd);
  if (dev_init(params->device, params->devport) != 0) {
    ui_puterrmsg("Hardware error", "Error: Failed to initialize the sound device");
    return(ACTION_ERR_HARD);
  }

  /* save start time so we can compute elapsed time later */
  timer_read(&midiplaybackstart);
  nexteventtime = midiplaybackstart;

  while (trackpos >= 0) {

    /* fetch next event */
    curevent = getnexteventfromcache(eventscache, trackpos, params->delay);
    if (curevent == NULL) { /* abort on error */
      ui_puterrmsg(params->midifile, "Error: Memory access fault");
      exitaction = ACTION_ERR_HARD;
      break;
    }

    /* give some time to the outdev driver for doing its things */
    dev_tick();

    /* printf("Action: %d / Note: %d / Vel: %d / t=%lu / next->%ld\n", curevent->type, curevent->data.note.note, curevent->data.note.velocity, curevent->deltatime, curevent->next); */
    if (curevent->deltatime > 0) { /* if I have some time ahead, I can do a few things */
      nexteventtime += (curevent->deltatime * trackinfo->tempo / trackinfo->miditimeunitdiv);
      elticks += curevent->deltatime;
      while (exitaction == ACTION_NONE) {
        unsigned long t;
        /* is time for next event yet? */
        timer_read(&t);
        if (t >= nexteventtime) break;
        /* detect wraparound of the timer counter */
        if (nexteventtime - t > ULONG_MAX / 2) break;
        /* if next event not due yet, do some keyboard/screen processing */
        if (compute_elapsed_time(midiplaybackstart, &(trackinfo->elapsedsec)) != 0) refreshflags |= UI_REFRESH_TIME;
        /* read keypresses */
        switch (getkey_ifany()) {
          case 0x1B: /* escape */
            exitaction = ACTION_EXIT;
            break;
          case 0x0D: /* return */
            exitaction = ACTION_NEXT;
            break;
          case '+':  /* volume up */
            volume += 5;                       /* note: I could also use a MIDI command to */
            if (volume > 100) volume = 100;    /* adjust the MPU's global volume but this  */
            refreshflags |= UI_REFRESH_VOLUME; /* is messy because the MIDI file might use */
            break;                             /* such message, too. Besides, some MPUs do */
          case '-':  /* volume down */         /* not support volume control (eg. my SB64) */
            volume -= 5;
            if (volume < 0) volume = 0;
            refreshflags |= UI_REFRESH_VOLUME;
            break;
        }
        /* do I need to refresh the screen now? if not, just call INT28h */
        if (refreshflags != 0) {
          ui_draw(trackinfo, &refreshflags, PVER, params->devname, params->devport, volume);
        } else if (params->nopowersave == 0) { /* if no screen refresh is     */
          union REGS regs;                     /* needed, and power saver not */
          int86(0x28, &regs, &regs);           /* disabled, then call INT 28h */
        }                                      /* for some power saving       */
      }
      if (exitaction != ACTION_NONE) break;
    }

    switch (curevent->type) {
      case EVENT_NOTEON:
        if (params->logfd != NULL) fprintf(params->logfd, "%lu: NOTE ON chan: %d / note: %d / vel: %d\n", trackinfo->elapsedsec, curevent->data.note.chan, curevent->data.note.note, curevent->data.note.velocity);
        dev_noteon(curevent->data.note.chan, curevent->data.note.note, (volume * curevent->data.note.velocity) / 100);
        trackinfo->notestates[curevent->data.note.note] |= (1 << curevent->data.note.chan);
        refreshflags |= UI_REFRESH_NOTES;
        break;
      case EVENT_NOTEOFF:
        if (params->logfd != NULL) fprintf(params->logfd, "%lu: NOTE OFF chan: %d / note: %d\n", trackinfo->elapsedsec, curevent->data.note.chan, curevent->data.note.note);
        dev_noteoff(curevent->data.note.chan, curevent->data.note.note);
        trackinfo->notestates[curevent->data.note.note] &= (0xFFFF ^ (1 << curevent->data.note.chan));
        refreshflags |= UI_REFRESH_NOTES;
        break;
      case EVENT_TEMPO:
        if (params->logfd != NULL) fprintf(params->logfd, "%lu (%lu): TEMPO change from %lu to %lu\n", trackinfo->elapsedsec, elticks, trackinfo->tempo, curevent->data.tempoval);
        trackinfo->tempo = curevent->data.tempoval;
        refreshflags |= UI_REFRESH_TEMPO;
        break;
      case EVENT_PROGCHAN:
        if (params->logfd != NULL) fprintf(params->logfd, "%lu: CHANNEL #%d PROG: %d\n", trackinfo->elapsedsec, curevent->data.prog.chan, curevent->data.prog.prog);
        trackinfo->chanprogs[curevent->data.prog.chan] = curevent->data.prog.prog;
        dev_setprog(curevent->data.prog.chan, curevent->data.prog.prog);
        refreshflags |= UI_REFRESH_PROGS;
        break;
      case EVENT_PITCH:
        if (params->logfd != NULL) fprintf(params->logfd, "%lu: PITCH WHEEL ON CHAN #%d: %d\n", trackinfo->elapsedsec, curevent->data.pitch.chan, curevent->data.pitch.wheel);
        dev_pitchwheel(curevent->data.pitch.chan, curevent->data.pitch.wheel);
        break;
      case EVENT_CONTROL:
        if (params->logfd != NULL) fprintf(params->logfd, "%lu: CONTROLLER %d ON CHAN #%d -> val %d\n", trackinfo->elapsedsec, curevent->data.control.id, curevent->data.control.chan, curevent->data.control.val);
        dev_controller(curevent->data.control.chan, curevent->data.control.id, curevent->data.control.val);
        break;
      case EVENT_CHANPRESSURE:
        if (params->logfd != NULL) fprintf(params->logfd, "%lu: CHANNEL PRESSURE %d ON CHAN #%d\n", trackinfo->elapsedsec, curevent->data.chanpressure.pressure, curevent->data.chanpressure.chan);
        dev_chanpressure(curevent->data.chanpressure.chan, curevent->data.chanpressure.pressure);
        break;
      case EVENT_KEYPRESSURE:
        if (params->logfd != NULL) fprintf(params->logfd, "%lu: KEY PRESSURE %d ON CHAN #%d, KEY %d\n", trackinfo->elapsedsec, curevent->data.keypressure.pressure, curevent->data.keypressure.chan, curevent->data.keypressure.note);
        dev_keypressure(curevent->data.keypressure.chan, curevent->data.keypressure.note, curevent->data.keypressure.pressure);
        break;
      default: /* probably a raw event - bit 7 should be set (>127) */
        if (curevent->type & 128) {
          if (params->logfd != NULL) {
            fprintf(params->logfd, "%lu: RAW EVENT TYPE OF LEN %d: [0x%02X 0x%02X 0x%02X]\n", trackinfo->elapsedsec, curevent->type & 127, curevent->data.raw[0], curevent->data.raw[1], curevent->data.raw[2]);
          }
          dev_rawmidi(curevent->data.raw, curevent->type & 127);
          break;
        } else {
          if (params->logfd != NULL) {
            fprintf(params->logfd, "%lu: ILLEGAL COMMAND: 0x%02X\n", trackinfo->elapsedsec, curevent->type);
          }
        }
        break;
    }

    if (trackpos < 0) break;
    trackpos = curevent->next;

  }

  if (params->logfd != NULL) fputs("Clear notes", params->logfd);

  /* Look for notes that are still ON and turn them OFF */
  for (i = 0; i < 128; i++) {
    if (trackinfo->notestates[i] != 0) {
      int c;
      for (c = 0; c < 16; c++) {
        if (trackinfo->notestates[i] & (1 << c)) {
          /* printf("note #%d is still playing on channel %d\n", i, c); */
          dev_noteoff(c, i);
        }
      }
    }
  }

  if (params->logfd != NULL) fputs("Reset MPU", params->logfd);
  dev_clear(); /* reinit the device */
  dev_close();

  return(exitaction);
}


int main(int argc, char **argv) {
  struct clioptions params;
  char *errstr;
  enum playactions action = ACTION_NONE;
  struct trackinfodata *trackinfo;
  struct midi_event_t *eventscache;

  /* preload the mpu port to be used (might be forced later via **argv) */
  preload_outdev(&params);

  errstr = parseargv(argc, argv, &params);
  if (errstr != NULL) {
    if (*errstr != 0) {
      printf("Error: %s\nRun DOSMID /? for some help", errstr);
    } else {
      puts("DOSMid v" PVER " Copyright (C) " PDATE " Mateusz Viste\n"
           "\n"
           "DOSMid is a MIDI player that reads *.MID or *.RMI files and drives a MPU401\n"
           "synthesizer.\n"
           "\n"
           "Usage: dosmid [options] file.mid|playlist.m3u\n"
           "\n"
           "options:\n"
           " /noxms    use conventional memory instead of XMS\n"
           " /nodelay  do not wait 2ms before accessing XMS memory (makes AWEUTIL crash)\n"
           " /mpu=XXX  use MPU port 0xXXX (by default it follows the BLASTER\n"
           "           environment variable, and if not found, uses 0x330)\n"
           " /awe      use the EMU8000 synth on AWE32/AWE64 SoundBlaster cards\n"
           " /log=FILE write highly verbose logs about DOSMid's activity to FILE\n"
           " /fullcpu  do not let DOSMid trying to be CPU-friendly\n"
           " /dontstop never wait for a keypress on error and continue the playlist\n"
           " /nosound  disable sound output\n"
      );
    }
    return(1);
  }

  params.devname = devtoname(params.device);

  /* allocate the work memory */
  if (mem_init(params.workmem, params.memmode) == 0) {
    puts("ERROR: Memory init failed!");
    return(1);
  }

  /* allocate memory segments to be used later */
  trackinfo = malloc(sizeof(struct trackinfodata));
  eventscache = malloc(sizeof(struct midi_event_t) * EVENTSCACHESIZE);

  if ((trackinfo == NULL) || (eventscache == NULL)) {
    puts("ERROR: Out of memory! Free some conventional memory.");
    mem_close();
    if (trackinfo != NULL) free(trackinfo);
    if (eventscache != NULL) free(eventscache);
    return(1);
  }

  /* init random numbers */
  srand((unsigned int)time(NULL));

  /* initialize the high resolution timer */
  timer_init();

  /* init ui and hide the blinking cursor */
  ui_init();
  ui_hidecursor();

  /* playlist loop */
  while (action != ACTION_EXIT) {

    action = playfile(&params, trackinfo, eventscache);
    switch (action) {
      case ACTION_EXIT:
        /* do nothing, we will exit at the end of the loop anyway */
        break;
      case ACTION_PREV:
        /* TODO to be implemented */
        break;
      case ACTION_NEXT:
        /* no explicit action - we will do a 'next' action by default */
        break;
      case ACTION_ERR_HARD: /* wait for a keypress and quit */
        if (params.dontstop == 0) {
          getkey();
        } else {
          sleep(2);
        }
        action = ACTION_EXIT;
        break;
      case ACTION_ERR_SOFT:         /* wait for a keypress so the user */
        if (params.dontstop == 0) { /* acknowledges the error message, */
          getkey();                 /* then continue as usual          */
        } else {
          sleep(2);
        }
      case ACTION_NONE: /* choose an action depending of the mode we are in */
        if (params.playlist == NULL) {
          action = ACTION_EXIT;
        } else {
          action = ACTION_NEXT;
        }
        break;
    }
  }

  /* reset screen (clears the screen and makes the cursor visible again) */
  ui_init();

  /* unload XMS memory */
  mem_close();

  /* free allocated heap memory */
  if (params.logfd != NULL) fputs("Free heap memory", params.logfd);
  free(trackinfo);
  free(eventscache);

  /* if a verbose log file was used, close it now */
  if (params.logfd != NULL) {
    fputs("Closing the log file", params.logfd);
    fclose(params.logfd);
  }

  puts("DOSMid v" PVER " Copyright (C) Mateusz Viste " PDATE);

  return(0);
}
