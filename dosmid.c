/*
   DOSMID - a low-requirement MIDI player for DOS

   Copyright (c) 2014,2015, Mateusz Viste
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

#include <dos.h>
#include <conio.h>  /* kbhit() */
#include <stdio.h>  /* printf(), puts(), fopen()... */
#include <limits.h> /* ULONG_MAX */
#include <stdlib.h> /* malloc(), free() */
#include <string.h> /* strcmp() */

#include "mem.h"
#include "midi.h"
#include "mpu401.h"
#include "timer.h"
#include "ui.h"

#define PVER "0.6"
#define PDATE "2014-2015"

#define MAXTRACKS 64
#define EVENTSCACHESIZE 64 /* *must* be a power of 2 !!! */
#define EVENTSCACHEMASK 63 /* used by the circular events buffer */


struct clioptions {
  int memmode;      /* type of memory to use: MEM_XMS or MEM_MALLOC */
  int workmem;      /* amount of work memory to allocate, in KiBs */
  int nodelay;
  int mpuport;
  char *midifile;   /* MIDI filename to play */
};


/* copies the base name of a file (ie without directory path) into a string */
static void filename2basename(char *fromname, char *tobasename, int maxlen) {
  int x, x2, firstchar = 0;
  /* find the first character of the base name */
  for (x = 0; fromname[x] != 0; x++) {
    switch (fromname[x]) {
      case '/':
      case '\\':
        firstchar = x + 1;
        break;
    }
  }
  /* copy the basename into tobasename */
  x2 = 0;
  for (x = firstchar; fromname[x] != 0; x++) {
    if ((fromname[x] == 0) || (x2 >= maxlen)) break;
    tobasename[x2++] = fromname[x];
  }
  tobasename[x2] = 0;
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


static void noteon(int mpuport, int chan, int n, int velocity) {
  mpu401_waitwrite(mpuport);      /* Wait for port ready */
  outp(mpuport, 0x90 | chan); /* Send note ON code on selected channel */

  mpu401_waitwrite(mpuport);      /* Wait for port ready */
  outp(mpuport, n);           /* Send note Number to turn ON */

  mpu401_waitwrite(mpuport);      /* Wait for port ready */
  outp(mpuport, velocity);    /* Send velocity */
}

static void noteoff(int mpuport, int chan, int n) {
  mpu401_waitwrite(mpuport);      /* Wait for port ready */
  outp(mpuport, 0x80 | chan);     /* Send note OFF code on selected channel */

  mpu401_waitwrite(mpuport);      /* Wait for port ready */
  outp(mpuport, n);               /* Send note number to turn OFF */

  mpu401_waitwrite(mpuport);      /* Wait for port ready */
  outp(mpuport, 64);              /* Send velocity */
}

static void rawevent(int mpuport, unsigned char far *rawdata, int rawlen) {
  int i;
  for (i = 0; i < rawlen; i++) {
    mpu401_waitwrite(mpuport);     /* Wait for port ready */
    outp(mpuport, rawdata[i]);     /* Send the raw byte over the wire */
  }
}

/* high resolution sleeping routing. sleeps for us microseconds.
   this function remembers the time slept, and automatically adjusts next
   sleeps. If you want absolute sleeping, use udelayabs() */
static void udelay(unsigned long us) {
  static unsigned long last = 0;
  unsigned long t;
  for (;;) {
    timer_read(&t);
    if (t < last) { /* detect timer wraparound */
        last = t;
        break;
      } else if (t - last >= us) {
        last += us;
        break;
    }
  }
}

/* high resolution sleeping routine */
static void udelayabs(unsigned long us) {
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


/* parse command line params and fills the params struct accordingly. returns
   NULL on sucess, or a pointer to an error string otherwise. */
static char *parseargv(int argc, char **argv, struct clioptions *params) {
  int i;
  /* first load defaults */
  params->midifile = NULL;
  params->memmode = MEM_XMS;
  params->workmem = 16384;  /* try to use 16M of XMS memory by default */
  params->nodelay = 0;
  /* if no params at all, don't waste time */
  if (argc == 0) return("");
  /* now read params */
  for (i = 1; i < argc; i++) {
    if (strcmp(argv[i], "/noxms") == 0) {
      params->memmode = MEM_MALLOC;
      params->workmem = 63; /* using 'normal' malloc, fallback to 63K */
    } else if (strcmp(argv[i], "/nodelay") == 0) {
      params->nodelay = 1;
    } else if (stringstartswith(argv[i], "/mpu=") == 0) {
      char *hexstr;
      hexstr = argv[i] + 5;
      params->mpuport = 0;
      while (*hexstr != 0) {
        int c;
        c = hexchar2int(*hexstr);
        if (c < 0) return("Invalid MPU port provided. Example: /mpu=330");
        params->mpuport <<= 4;
        params->mpuport |= c;
        hexstr++;
      }
      if (params->mpuport < 1) return("Invalid MPU port provided. Example: /mpu=330");
    } else if ((strcmp(argv[i], "/?") == 0) || (strcmp(argv[i], "/h") == 0) || (strcmp(argv[i], "/help") == 0)) {
      return("");
    } else if ((argv[i][0] != '/') && (params->midifile == NULL)) {
      params->midifile = argv[i];
    } else {
      return("Unknown option.");
    }
  }
  /* check if at least a MIDI filename have been provided */
  if (params->midifile == NULL) {
    return("You have to provide the path to a MIDI file to play.");
  }
  /* all good */
  return(NULL);
}

static void setchanprog(int mpuport, unsigned char chan, unsigned char prog) {
  mpu401_waitwrite(mpuport);   /* Wait for port ready */
  outp(mpuport, 0xC0 | chan);  /* Send channel */
  mpu401_waitwrite(mpuport);   /* Wait for port ready */
  outp(mpuport, prog);         /* Send patch id */
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

/* check the event cache for a given event */
static struct midi_event_t *getnexteventfromcache(struct midi_event_t *eventscache, long trackpos, int nodelay) {
  static unsigned int itemsincache = 0;
  static unsigned int curcachepos = 0;
  struct midi_event_t *res = NULL;
  long nextevent;
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
        if (nodelay == 0) udelayabs(2000);
        nextslot = curcachepos + itemsincache;
        nextevent = eventscache[nextslot & EVENTSCACHEMASK].next;
        while ((itemsincache < EVENTSCACHESIZE - 1) && (nextevent >= 0)) {
          nextslot++;
          nextslot &= EVENTSCACHEMASK;
          pullres = pullevent(nextevent, &eventscache[nextslot]);
          if (pullres != 0) {
            printf("pullevent() ERROR: %u (eventid = %ld)\n", pullres, trackpos);
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
      if (nodelay == 0) udelayabs(2000);
      nextevent = trackpos;
      curcachepos = 0;
      for (refillcount = 0; refillcount < EVENTSCACHESIZE; refillcount++) {
        pullres = pullevent(nextevent, &eventscache[refillcount]);
        if (pullres != 0) {
          printf("pullevent() ERROR: %u (eventid = %ld)\n", pullres, trackpos);
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


/* reads the BLASTER variable for MPU port. If not found, fallbacks to 0x330 */
static int preloadmpuport(void) {
  char *blaster;
  /* check if a blaster variable is present */
  blaster = getenv("BLASTER");
  /* if so, read it looking for a 'P' parameter */
  if (blaster != NULL) {
    while ((*blaster != 'P') && (*blaster != 0)) {
      blaster++;
    }
    /* have we found a 'P' param? */
    if (*blaster == 'P') {
      int p = 0;
      /* read the P param into a variable */
      blaster++;
      while ((*blaster != 0) && (*blaster != ' ')) {
        int c = hexchar2int(*blaster);
        blaster++;
        if (c < 0) {
          p = -1;
          break;
        }
        p <<= 4;
        p |= c;
      }
      /* if what we have read looks sane, return it */
      if (p > 0) return(p);
    }
  }
  /* if nothing else worked, fallback to 0x330 */
  return(0x330);
}


int main(int argc, char **argv) {
  int midiformat, miditracks;
  unsigned int miditimeunitdiv;
  int i;
  int refreshflags = 0xFF;
  struct midi_chunkmap_t *chunkmap;
  FILE *fd;
  long newtrack, trackpos = -1;
  unsigned long midiplaybackstart;
  char *errstr;
  struct midi_event_t *curevent, *eventscache;
  struct clioptions params;
  struct trackinfodata *trackinfo;

  /* preload the mpu port to be used (might be forced later via **argv) */
  params.mpuport = preloadmpuport();

  errstr = parseargv(argc, argv, &params);
  if (errstr != NULL) {
    if (*errstr != 0) {
      printf("Error: %s\nRun DOSMID /? for some help", errstr);
    } else {
      puts("DOSMid v" PVER " Copyright (C) " PDATE " Mateusz Viste\n"
           "\n"
           "DOSMid is a MIDI player that reads *.MID or *.RMI files and drives a MPU401"
           "synthesizer.\n"
           "\n"
           "Usage: dosmid [/noxms] [/nodelay] [/mpu=XXX] file.mid\n"
           "\n"
           " /noxms    use conventional memory instead of XMS\n"
           " /nodelay  don't wait after writing to MPU\n"
           " /mpu:XXX  use MPU port 0xXXX (by default it follows the BLASTER\n"
           "           environment variable, and if not found, uses 0x330)\n"
      );
    }
    return(1);
  }

  fd = fopen(params.midifile, "rb");
  if (fd == NULL) {
    puts("Error: Failed to open the midi file");
    return(1);
  }

  chunkmap = malloc(sizeof(struct midi_chunkmap_t) * MAXTRACKS);
  if (chunkmap == NULL) {
    puts("Error: Out of memory");
    return(1);
  }

  if (midi_readhdr(fd, &midiformat, &miditracks, &miditimeunitdiv, chunkmap, MAXTRACKS) != 0) {
    puts("Invalid MIDI format file");
    fclose(fd);
    free(chunkmap);
    return(1);
  }

  if ((midiformat != 0) && (midiformat != 1)) {
    printf("Unsupported MIDI format, sorry. So far only formats #0 and #1 are supported. Detected format: %d\n", midiformat);
    fclose(fd);
    free(chunkmap);
    return(1);
  }

  if (miditracks > MAXTRACKS) {
    printf("Too many tracks. Sorry. File contains: %d. Max. limit: %d.\n", miditracks, MAXTRACKS);
    fclose(fd);
    free(chunkmap);
    return(1);
  }

  if (mem_init(params.workmem, params.memmode) == 0) {
    puts("XMS init failed!");
    fclose(fd);
    free(chunkmap);
    return(1);
  }

  trackinfo = calloc(sizeof(struct trackinfodata), 1);
  if (trackinfo == NULL) {
    puts("Out of memory! Free some conventional memory and try again.");
    mem_close();
    return(1);
  }
  /* set default params for trackinfo variables */
  trackinfo->midiformat = midiformat;
  trackinfo->tempo = 500000l;
  trackinfo->volume = 100;
  for (i = 0; i < 16; i++) trackinfo->chanprogs[i] = i;
  for (i = 0; i < UI_TITLENODES; i++) trackinfo->title[i] = trackinfo->titledat[i];
  filename2basename(params.midifile, trackinfo->filename, UI_FILENAMEMAXLEN);

  for (i = 0; i < miditracks; i++) {
    printf("Loading track %d/%d...\n", i + 1, miditracks);
    /* is it really a track we got here? */
    if (strcmp(chunkmap[i].id, "MTrk") != 0) {
      fclose(fd);
      printf("Unexpected chunk: was expecting track #%d...\n", i);
      mem_close();
      free(trackinfo);
      return(1);
    }
    /*printf("Track %d starts at offset 0x%04X\n", i, chunkmap[i].offset);*/
    fseek(fd, chunkmap[i].offset, SEEK_SET);
    if (i == 0) {
        newtrack = midi_track2events(fd, trackinfo->title, UI_TITLENODES, UI_TITLEMAXLEN, trackinfo->copyright, UI_COPYRIGHTMAXLEN, &(trackinfo->channelsusage));
      } else {
        newtrack = midi_track2events(fd, NULL, 0, UI_TITLEMAXLEN, NULL, 0, &(trackinfo->channelsusage));
    }
    /* printf("merging track #%d (%ld)\n", i, track[i]); */
    trackpos = midi_mergetrack(trackpos, newtrack, &(trackinfo->totlen), miditimeunitdiv);
    /* printf("merged track starts with %ld\n", trackpos); */
  }
  fclose(fd);
  free(chunkmap);

  eventscache = malloc(sizeof(struct midi_event_t) * EVENTSCACHESIZE);
  if (eventscache == NULL) {
    puts("Out of memory! Free some conventional memory and try again.");
    mem_close();
    return(1);
  }


  puts("RESET MPU-401...");
  if (mpu401_rst(params.mpuport) != 0) {
    printf("Error: failed to reset the MPU-401 synthesizer via port %03Xh\n", params.mpuport);
    mem_close();
    return(1);
  }

  puts("SET UART MODE..");
  mpu401_uart(params.mpuport);

  /* initialize the high resolution timer */
  timer_init();

  /* init ui and hide the blinking cursor */
  ui_init();
  ui_hidecursor();

  /* save start time so we can compute elapsed time later */
  timer_read(&midiplaybackstart);

  while (trackpos >= 0) {

    /* fetch next event */
    curevent = getnexteventfromcache(eventscache, trackpos, params.nodelay);

    /* puts("flush MPU"); */
    mpu401_flush(params.mpuport);
    /* printf("Action: %d / Note: %d / Vel: %d / t=%lu / next->%ld\n", curevent->type, curevent->data.note.note, curevent->data.note.velocity, curevent->deltatime, curevent->next); */
    if (curevent->deltatime > 0) {
      if (compute_elapsed_time(midiplaybackstart, &(trackinfo->elapsedsec)) != 0) refreshflags |= UI_REFRESH_TIME;
      ui_draw(trackinfo, &refreshflags, PVER, params.mpuport); /* draw the UI only between non-zero gaps */
      udelay(curevent->deltatime * trackinfo->tempo / miditimeunitdiv);
    }
    switch (curevent->type) {
      case EVENT_NOTEON:
        /* puts("NOTE ON"); */
        noteon(params.mpuport, curevent->data.note.chan, curevent->data.note.note, (trackinfo->volume * curevent->data.note.velocity) / 100);
        trackinfo->notestates[curevent->data.note.note] |= (1 << curevent->data.note.chan);
        refreshflags |= UI_REFRESH_NOTES;
        break;
      case EVENT_NOTEOFF:
        /* puts("NOTE OFF"); */
        noteoff(params.mpuport, curevent->data.note.chan, curevent->data.note.note);
        trackinfo->notestates[curevent->data.note.note] &= (0xFFFF ^ (1 << curevent->data.note.chan));
        refreshflags |= UI_REFRESH_NOTES;
        break;
      case EVENT_TEMPO:
        trackinfo->tempo = curevent->data.tempoval;
        refreshflags |= UI_REFRESH_TEMPO;
        break;
      case EVENT_PROGCHAN:
        trackinfo->chanprogs[curevent->data.prog.chan] = curevent->data.prog.prog;
        setchanprog(params.mpuport, curevent->data.prog.chan, curevent->data.prog.prog);
        refreshflags |= UI_REFRESH_PROGS;
        break;
      default: /* probably a raw event - bit 7 should be set (>127) */
        if (curevent->type & 128) {
          /* puts("RAW EVENT"); */
          rawevent(params.mpuport, curevent->data.raw, curevent->type & 127);
          break;
        }
    }

    trackpos = curevent->next;

    /* read keypresses */
    switch (getkey_ifany()) {
      case 0x1B: /* escape */
        trackpos = -1;
        break;
      case '+':  /* volume up */
        trackinfo->volume += 5;
        if (trackinfo->volume > 100) trackinfo->volume = 100;
        break;
      case '-':  /* volume down */
        trackinfo->volume -= 5;
        if (trackinfo->volume < 0) trackinfo->volume = 0;
        break;
    }

  }

  /* reset screen */
  ui_init();

  puts("Clear notes...");

  /* Look for notes that are still ON and turn them OFF */
  for (i = 0; i < 128; i++) {
    if (trackinfo->notestates[i] != 0) {
      int c;
      for (c = 0; c < 16; c++) {
        if (trackinfo->notestates[i] & (1 << c)) {
          /* printf("note #%d is still playing on channel %d\n", i, c); */
          noteoff(params.mpuport, c, i);
        }
      }
    }
  }

  puts("Reset MPU...");

  /* reset the MPU back into intelligent mode */
  mpu401_rst(params.mpuport);

  puts("Free memory...");
  mem_close();
  free(trackinfo);
  free(eventscache);

  return(0);
}
