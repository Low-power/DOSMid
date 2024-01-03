/*
 * DOSMID - a low-requirement MIDI and MUS player for DOS
 *
 * Copyright (C) 2014-2022, Mateusz Viste
 * All rights reserved.
 * Copyright 2015-2024 Rivoreo
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

#include <dos.h>    /* REGS */
#include <stdio.h>  /* printf() */
#include <limits.h> /* ULONG_MAX */
#include <stdlib.h> /* rand() */
#include <string.h> /* memset(), strcpy(), strncat(), memcpy() */
#if defined __WATCOMC__ && __WATCOMC__ >= 1240
#define HAVE_STRCASECMP
#include <strings.h>
#endif
#include "bitfield.h"
#include "fio.h"
#include "gus.h"
#include "mem.h"
#include "midi.h"
#include "mus.h"
#include "outdev.h"
#include "rs232.h"
#include "syx.h"
#include "timer.h"
#include "ui.h"
#include "version.h"

#define MAXTRACKS 64
#define EVENTSCACHESIZE 64 /* *must* be a power of 2 !!! */
#define EVENTSCACHEMASK 63 /* used by the circular events buffer */

#define PRESET_GM   0 /* default */
#define PRESET_GS   1
#define PRESET_XG   2
#define PRESET_NONE 3

/* define a work buffer that will be used instead of malloc() calls whenever a temporary buffer is required */
unsigned char wbuff[8192];

enum playaction {
  ACTION_NONE = 0,
  ACTION_NEXT = 1,
  ACTION_PREV = 2,
  ACTION_ERR_SOFT = 3,
  ACTION_ERR_HARD = 4,
  ACTION_EXIT = 64
};


struct clioptions {
  int memmode;          /* type of memory to use: MEM_XMS or MEM_MALLOC */
  unsigned short devport;
  unsigned short port_mpu;
  unsigned short port_awe;
  unsigned short port_sb;
  enum outdev_type device;
  int devicesubtype;
  char *devname;    /* the human name of the out device (MPU, AWE..) */
  char *midifile;   /* MIDI filename to play */
  char *syxrst;     /* syx file to use for MIDI resets */
  int delay;        /* additional delay to apply before playing a file */
  char *playlist;   /* the playlist to read files from */
  char *sbnk;       /* optional sound bank to use (IBK file or so) */
#ifdef DBGFILE
  FILE *logfile;      /* an open debug log file */
#endif
  int dev_clear_flags;
  unsigned char onlpt;
  unsigned char volume;
  /* 'flags' */
  unsigned char xmsdelay;
  char nockdev;
  unsigned char nopowersave;
  unsigned char dontstop;
  unsigned char random;       /* randomize playlist order */
  unsigned char gmgspreset;   /* PRESET_GM, PRESET_GS, PRESET_XG, PRESET_NONE */
};


/* fetch directory where the program resides, and return its length. result
 * string is never longer than 128 (incl. the null terminator), and it is
 * always terminated with a backslash separator, unless it is an empty string */
static int exepath(char *result) {
  char far *psp, far *env;
  unsigned int envseg, pspseg, x, i;
  int lastsep;
  union REGS regs;
  /* get the PSP segment */
  regs.h.ah = 0x62;
  int86(0x21, &regs, &regs),
  pspseg = regs.x.bx;
  /* compute a far pointer that points to the top of PSP */
  psp = MK_FP(pspseg, 0);
  /* fetch the segment address of the environment */
  envseg = psp[0x2D];
  envseg <<= 8;
  envseg |= psp[0x2C];
  /* compute the env pointer */
  env = MK_FP(envseg, 0);
  /* skip all environment variables */
  x = 0;
  for (;;) {
    x++;
    if (env[x] == 0) { /* end of variable */
      x++;
      if (env[x] == 0) break; /* end of list */
    }
  }
  x++;
  /* read the WORD that indicates string that follow */
  if (env[x] < 1) {
    result[0] = 0;
    return(0);
  }
  x += 2;
  /* else copy the EXEPATH to our return variable, and truncate after last '\' */
  lastsep = -1;
  for (i = 0;; i++) {
    result[i] = env[x++];
    if (result[i] == '\\') lastsep = i;
    if (result[i] == 0) break; /* end of string */
    if (i >= 126) break;       /* this DOS string should never go beyond 127 chars! */
  }
  result[lastsep + 1] = 0;
  return(lastsep + 1);
}


static void dos_puts(char *s) {
  /* DOS 1+ - WRITE STRING TO STANDARD OUTPUT
     AH = 09h
     DS:DX -> '$'-terminated string */
  unsigned short segm, offs;
  segm = FP_SEG(s);
  offs = FP_OFF(s);
  __asm {
    mov ah, 9
    push ds
    mov cx, segm
    push cx
    pop ds
    mov dx, offs
    int 21h
    pop ds /* restore DS */
    /* print out a CR/LF using INT21h AH=2 */
    mov ah, 2
    mov dl, 0dh
    int 21h
    mov ah, 2
    mov dl, 0ah
    int 21h
  }
}

/* returns a pseudo-random number, based on the DOS system timer */
static unsigned long rnd(void) {
  unsigned long res;
  union REGS regs;
  regs.h.ah = 0;
  int86(0x1A, &regs, &regs);
  res = regs.x.cx; /* number of clock ticks since midnight (high word) */
  res <<= 16;
  res |= regs.x.dx; /* number of clock ticks since midnight (low word) */
  return(res);
}

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
      if ((fromname[x] == 0) || (x2+1 >= maxlen)) break;
      tobasename[x2++] = fromname[x];
    }
    tobasename[x2] = 0;
  }
  /* copy dirname to todirname */
  if (todirname != NULL) {
    x2 = 0;
    for (x = 0; x < firstchar; x++) {
      if ((fromname[x] == 0) || (x2+1 >= maxlen)) break;
      todirname[x2++] = fromname[x];
    }
    todirname[x2] = 0;
  }
}


/* switch a string to upper case */
static void ucasestr(char *s) {
  for (; *s != 0; s++) if ((*s >= 'a') && (*s <= 'z')) *s -= 32;
}


/* returns the lower-case version of c char, if applicable */
static int lcase(char c) {
  if ((c >= 'A') && (c <= 'Z')) return(c + 32);
  return(c);
}


#ifndef HAVE_STRCASECMP
/* a case-insensitive version of strcmp() */
static int strcasecmp(const char *s1, const char *s2) {
  for (;;) {
    if (lcase(*s1) != lcase(*s2)) return(1);
    if (*s1 == 0) return(0);
    s1++;
    s2++;
  }
}
#endif


/* case-insensitively check whether 'str' starts with 'start' */
static int stringstartswith(const char *str, const char *start) {
  while (*start != 0) {
    if (lcase(*start) != lcase(*str)) return(0);
    str++;
    start++;
  }
  return(1);
}


static int hexchar2int(char c) {
  if ((c >= '0') && (c <= '9')) return(c - '0');
  if ((c >= 'a') && (c <= 'f')) return(10 + c - 'a');
  if ((c >= 'A') && (c <= 'F')) return(10 + c - 'A');
  return(-1);
}


/* converts a hex string to unsigned int. stops at first null terminator or
 * space. returns zero on error. */
static unsigned int hexstr2uint(const char *hexstr) {
  unsigned int v = 0;
  while (*hexstr && *hexstr != ' ') {
    int c;
    c = hexchar2int(*hexstr);
    if (c < 0) return(0);
    v <<= 4;
    v |= c;
    hexstr++;
  }
  return(v);
}


static char *devtoname(enum outdev_type device, int devicesubtype) {
  switch (device) {
    case DEV_NONE:   return("NONE");
    case DEV_MPU401: return("MPU");
#ifdef SBAWE
    case DEV_AWE:    return("AWE");
#endif
#ifdef OPL
    case DEV_OPL:    return("OPL");
    case DEV_OPL2:   return("OPL2");
    case DEV_OPL3:   return("OPL3");
#endif
    case DEV_RS232:
      if (devicesubtype == 1) return("COM1");
      if (devicesubtype == 2) return("COM2");
      if (devicesubtype == 3) return("COM3");
      if (devicesubtype == 4) return("COM4");
      return("COM");
    case DEV_SBMIDI: return("SB");
    case DEV_GUS:    return("GUS");
#ifdef CMS
    case DEV_CMS:    return("CMS");
#endif
    default:         return("UNK");
  }
}


/* analyzes a 16 bytes file header and guess the file format */
static enum fileformat header2fileformat(unsigned char *hdr) {
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

#if defined CMSLPT || defined OPLLPT
static unsigned short int get_lpt_port(unsigned int i) {
  return *(unsigned short int __far *)MK_FP(0x40, 6 + 2*i);
}
#endif

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


#define REQUEST_HELP ((char *)-1)
#define REQUEST_VERSION ((char *)-2)

/* interpret a single config argument, returns NULL on succes, or a pointer to
 * an error string otherwise */
static char *feedarg(char *arg, struct clioptions *params, int option_allowed, int file_allowed) {
  if(option_allowed && (arg[0] == '/' || arg[0] == '-')) {
    char *o = arg + 1;
    if (strcasecmp(o, "noxms") == 0) {
      params->memmode = MEM_MALLOC;
    } else if (strcasecmp(o, "xmsdelay") == 0) {
      params->xmsdelay = 1;
    } else if (strcasecmp(o, "fullcpu") == 0) {
      params->nopowersave = 1;
    } else if (strcasecmp(o, "dontstop") == 0) {
      params->dontstop = 1;
    } else if (strcasecmp(o, "random") == 0) {
      params->random = 1;
    } else if (strcasecmp(o, "nosound") == 0) {
      params->device = DEV_NONE;
      params->devport = 0;
#ifdef SBAWE
    } else if (strcasecmp(o, "awe") == 0) {
      params->device = DEV_AWE;
      params->devport = params->port_awe;
      /* if AWE port not found in BLASTER, use the default 0x620 */
      if (params->devport == 0) params->devport = 0x620;
    } else if (stringstartswith(o, "awe=")) {
      params->device = DEV_AWE;
      params->devport = hexstr2uint(o + 4);
    if (params->devport < 1) return("Invalid AWE port provided. Example: /awe=620$");
#endif
    } else if (strcasecmp(o, "mpu") == 0) {
      params->device = DEV_MPU401;
      params->devport = params->port_mpu;
      /* if MPU port not found in BLASTER, use the default 0x330 */
      if (params->devport == 0) params->devport = 0x330;
    } else if (strcasecmp(o, "preset=gm") == 0) {
      params->gmgspreset = PRESET_GM;
    } else if (strcasecmp(o, "preset=gs") == 0) {
      params->gmgspreset = PRESET_GS;
    } else if (strcasecmp(o, "preset=xg") == 0) {
      params->gmgspreset = PRESET_XG;
    } else if (strcasecmp(o, "preset=none") == 0) {
      params->gmgspreset = PRESET_NONE;
    } else if (strcasecmp(o, "gus") == 0) {
      params->device = DEV_GUS;
      params->devport = gus_find();
      if (params->devport < 1) return("GUS error: No ULTRAMID driver found$");
#ifdef OPL
    } else if (strcasecmp(o, "opl") == 0) {
      params->device = DEV_OPL;
      params->devport = 0x388;
    } else if (stringstartswith(o, "opl=")) {
      params->device = DEV_OPL;
      params->devport = hexstr2uint(o + 4);
      if (params->devport < 1) return("Invalid OPL port provided. Example: /opl=388$");
    } else if (stringstartswith(o, "opl") && (o[3] == '2' || o[3] == '3') && (!o[4] || o[4] == '=')) {
      params->device = o[3] == '3' ? DEV_OPL3 : DEV_OPL2;
      if(o[4]) {
#ifdef OPLLPT
        if(stringstartswith(o + 5, "lpt")) {
          char i = o[8];
          if(i < '1' || i > '4' || o[9]) return "Invalid LPT index provided. Example: /opl3=lpt2$";
          params->devicesubtype = params->onlpt = i - '0';
        } else
#endif
        {
          params->devport = hexstr2uint(o + 5);
          if (params->devport < 1) {
            return params->device == DEV_OPL3 ?
              "Invalid OPL3 port provided. Example: /opl3=388$" :
              "Invalid OPL2 port provided. Example: /opl2=388$";
          }
        }
        params->nockdev = 1;
      } else {
        params->devport = 0x388;
      }
#endif
#ifdef CMS
    } else if (strcasecmp(o, "cms") == 0) {
      params->device = DEV_CMS;
      params->devport = 0x220;
    } else if (stringstartswith(o, "cms=")) {
      params->device = DEV_CMS;
#ifdef CMSLPT
      if(stringstartswith(o + 4, "lpt")) {
        char i = o[7];
        if(i < '1' || i > '4' || o[8]) return "Invalid LPT index provided. Example: /cms=lpt2$";
        params->devicesubtype = params->onlpt = i - '0';
      } else
#endif
      {
        params->devport = hexstr2uint(o + 4);
        if (params->devport < 1) return("Invalid CMS port provided. Example: /cms=220$");
      }
#endif
    } else if (stringstartswith(o, "sbnk=")) {
      if (params->sbnk != NULL) free(params->sbnk); /* drop last sbnk if already present, so a CLI sbnk would take precedence over a config-file sbnk */
      params->sbnk = strdup(o + 5);
    } else if (stringstartswith(o, "mpu=")) {
      params->device = DEV_MPU401;
      params->devport = hexstr2uint(o + 4);
      if (params->devport < 1) return("Invalid MPU port provided. Example: /mpu=330$");
    } else if (stringstartswith(o, "com=")) {
      params->device = DEV_RS232;
      params->devport = hexstr2uint(o + 4);
      if (params->devport < 10) return("Invalid COM port provided. Example: /com=3f8$");
    } else if (stringstartswith(o, "com")) { /* must be compared AFTER "com=" */
      params->device = DEV_RS232;
      params->devicesubtype = o[3] - '0';
      if ((params->devicesubtype < 1) || (params->devicesubtype > 4)) return("Invalid COM port provided. Example: /com1$");
      params->devport = rs232_getport(params->devicesubtype);
      if (params->devport < 1) return("Failed to autodetect the I/O address of this COM port. Try using the /com=XXX option.$");
    } else if (strcasecmp(o, "sbmidi") == 0) {
      params->device = DEV_SBMIDI;
      params->devport = params->port_sb;
      /* if SB port not found in BLASTER, use the default 0x220 */
      if (params->devport == 0) params->devport = 0x220;
    } else if (stringstartswith(o, "sbmidi=")) {
      params->device = DEV_SBMIDI;
      params->devport = hexstr2uint(o + 7);
      if (params->devport < 1) return("Invalid SBMIDI port provided. Example: /sbmidi=220$");
#ifdef DBGFILE
    } else if (stringstartswith(o, "log=")) {
      if (params->logfile) fclose(params->logfile);
      params->logfile = fopen(o + 4, "wb");
      if (params->logfile == NULL) return("Failed to open the debug log file.$");
#endif
    } else if (stringstartswith(o, "syx=")) {
      params->syxrst = strdup(o + 4);
    } else if (stringstartswith(o, "delay=")) {
      params->delay = atoi(o + 6);
      if ((params->delay < 1) || (params->delay > 9000)) {
        return("Invalid delay value: must be in the range 1..9000$");
      }
    } else if (stringstartswith(o, "volume=")) {
      int v = atoi(o + 7);
      if(v < 1) return "Invalid volume setting.$";
      if(v > 100) v = 100;
      params->volume = v;
    } else if(stringstartswith(o, "quirk=")) {
      char *comma;
      o += 6;
      do {
        comma = strchr(o, ',');
        if(comma) *comma++ = 0;
        if(strcmp(o, "norstctrl") == 0) params->dev_clear_flags |= DOSMID_DEV_NORSTCTRL;
        else return "Unrecognized quirk name.$";
      } while(comma && *(o = comma));
    } else if (strcmp(o, "?") == 0 || strcasecmp(o, "h") == 0 || strcasecmp(o, "help") == 0) {
      return REQUEST_HELP;
    } else if (strcasecmp(o, "version") == 0) {
      return REQUEST_VERSION;
    } else {
      return("Unknown option.$");
    }
  } else if (file_allowed && !params->midifile && !params->playlist) {
    char ext[4];
    getfileext(ext, arg, 4);
    if (strcasecmp(ext, "m3u") == 0) {
      params->playlist = arg;
    } else {
      params->midifile = arg;
    }
  }
  return(NULL);
}


/* trims any white-space and line feeds occuring at the right of the string */
static void rtrim(char *s) {
  char *lastchar = s;
  while (*s != 0) {
    switch (*s) {
      case ' ':
      case '\t':
      case '\r':
      case '\n':
        s++;
        break;
      default:
        lastchar = ++s;
    }
  }
  *lastchar = 0;
}


static char *loadconfigfile(struct clioptions *params) {
  char buff[128 + 12]; /* 128 for exepath plus 8+3 for the config file */
  int r;
  char *res = NULL;
  struct fiofile f;
  /* prepare config file's full path */
  r = exepath(buff);
  if (r < 1) return(NULL);
  /* append the config file itself */
  sprintf(buff + r, "dosmid.cfg");
  /* open file */
  if (fio_open(buff, FIO_OPEN_RD, &f) != 0) return(NULL);
  for (;;) {
    /* read line & trim */
    r = fio_getline(&f, buff, sizeof(buff));
    if (r < 0) break; /* stop on EOF */
    if (*buff == '#') continue; /* skip comments */
    rtrim(buff);
    if (*buff == 0) continue; /* skip empty lines */
    /* push arg to feedarg() (files not allowed because filename not allocated in persistent memory) */
    res = feedarg(buff, params, 1, 0);
    if (res != NULL) break;
  }
  /* close file */
  fio_close(&f);
  return(res);
}


/* parse command line params and fills the params struct accordingly. returns
   NULL on sucess, or a pointer to an error string otherwise. */
static char *parseargv(int argc, char **argv, struct clioptions *params) {
  int end_of_options = 0;
  int i;
  /* if no params at all, don't waste time */
  if (argc == 0) return("");
  /* now read params */
  for (i = 1; i < argc; i++) {
    char *r;
    if(!end_of_options && strcmp(argv[i], "--") == 0) {
      end_of_options = 1;
      continue;
    }
    r = feedarg(argv[i], params, !end_of_options, 1);
    if (r != NULL) return(r);
  }
  /* check if at least a MIDI filename have been provided */
  if ((params->midifile == NULL) && (params->playlist == NULL)) {
    return("You have to provide the path to a MIDI file or a playlist to play.$");
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
static struct midi_event *getnexteventfromcache(struct midi_event *eventscache, long int trackpos, int xmsdelay) {
  static unsigned int itemsincache = 0;
  static unsigned int curcachepos = 0;
  struct midi_event *res = NULL;
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
        if (xmsdelay != 0) udelay(2000);
        nextslot = curcachepos + itemsincache;
        nextevent = eventscache[nextslot & EVENTSCACHEMASK].next;
        while ((itemsincache < EVENTSCACHESIZE - 1) && (nextevent >= 0)) {
          nextslot++;
          nextslot &= EVENTSCACHEMASK;
          pullres = mem_pull(nextevent, &eventscache[nextslot], sizeof(struct midi_event));
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
      if (xmsdelay != 0) udelay(2000);
      nextevent = trackpos;
      curcachepos = 0;
      for (refillcount = 0; refillcount < EVENTSCACHESIZE; refillcount++) {
        pullres = mem_pull(nextevent, &eventscache[refillcount], sizeof(struct midi_event));
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

  params->port_mpu = 0;
  params->port_awe = 0;
  params->port_sb  = 0;

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
      unsigned short *portptr;
      blaster = blasterptr[blastercount];
      /* have we found an interesting param? */
      if ((*blaster != 'P') && (*blaster != 'E') && (*blaster != 'A')) continue;
      if (*blaster == 'E') {
        portptr = &(params->port_awe);
      } else if (*blaster == 'P') {
        portptr = &(params->port_mpu);
      } else {
        portptr = &(params->port_sb);
      }
      /* read the param value into a variable */
      p = hexstr2uint(blaster + 1);
      /* if what we have read looks sane, keep it */
      if (p > 0) *portptr = p;
    }
  }

  /* look at what we got, and choose in order of preference */

#if defined DOSMID_DEFAULT_DEVICE_TYPE && defined DOSMID_DEFAULT_DEVICE_PORT
  params->device = DOSMID_DEFAULT_DEVICE_TYPE;
  params->devport = DOSMID_DEFAULT_DEVICE_PORT;
#else
  /* set NONE, just so we have anything set */
  params->device = DEV_NONE;
  params->devport = 0;
#endif

  /* never try using SBMIDI: it's unlikely anything's connected to it anyway */

  /* is there an MPU? if so, take it */
  if (params->port_mpu > 0) {
    params->device = DEV_MPU401;
    params->devport = params->port_mpu;
  }

  /* if a GUS seems to be installed, let's try it */
  if (getenv("ULTRADIR") != 0) {
    int gusp = gus_find();
    if (gusp > 0) {
      params->device = DEV_GUS;
      params->devport = gusp;
    }
  }

  /* AWE is the most desirable, if present (and compiled in) */
#ifdef SBAWE
  if (params->port_awe > 0) { /* AWE is the most desirable, if present */
    params->device = DEV_AWE;
    params->devport = params->port_awe;
  }
#endif
}

enum order {
  FORWARD_ORDER, REVERSE_ORDER, RANDOM_ORDER
};
/* reads a position from an M3U file and returns a ptr from static mem */
static char *getnextm3uitem(char *playlist, enum order order) {
  static char fnamebuf[256];
  char tempstr[256];
  long fsize;
  static long pos = 0;
  int slen;
  struct fiofile f;
  /* open the playlist and read its size */
  if (fio_open(playlist, FIO_OPEN_RD, &f) != 0) return(NULL);
  fsize = fio_seek(&f, FIO_SEEK_END, 0);
  if (fsize < 3) { /* a one-entry m3u would be at least 3 bytes long */
    fio_close(&f);
    return(NULL);
  }
  if (order == RANDOM_ORDER) {
    /* go to a random position (avoid last bytes, could be an empty \r\n record) */
    pos = (rnd() << 1) % (fsize - 2); /* mul rnd by 2 to speed up 'randomness' at the cost of getting only even offsets */
  } else if (order == REVERSE_ORDER) pos -= 3;
  GOHEREFORPREV:
  if (pos < 0) pos = 0;
  fio_seek(&f, FIO_SEEK_START, pos);
  /* rewind back to nearest \n or 0 position */
  while (pos > 0) {
    fio_read(&f, tempstr, 1);
    if (tempstr[0] != '\n') {
      fio_seek(&f, FIO_SEEK_CUR, -2);
      pos--;
    } else {
      pos++;
      break;
    }
  }
  if (order == REVERSE_ORDER) {
    pos -= 3;
    order = FORWARD_ORDER;
    goto GOHEREFORPREV;
  }

  /* read the string into fnamebuf */
  slen = 0;
  fnamebuf[0] = 0;
  for (;;) {
    char c;
    if ((fio_read(&f, &c, 1) != 1) || (c == '\r') || (c == '\n')) break;
    pos++;
    fnamebuf[slen++] = c;
    if (slen == sizeof(fnamebuf)) { /* overflow! */
      fnamebuf[0] = 0;
      break;
    }
    fnamebuf[slen] = 0;
  }

  /* if sequential reading of the playlist, then jump to next entry */
  if (order == FORWARD_ORDER) {
    for (;;) {
      char c;
      if (fio_read(&f, &c, 1) != 1) {
        pos = 0;
        break;
      } else if ((c == '\r') || (c == '\n')) {
        pos++;
      } else {
        break;
      }
    }
  }

  /* close the file */
  fio_close(&f);
  /* trim any leading spaces, if any */
  rtrim(fnamebuf);
  /* if empty, something went wrong */
  if (fnamebuf[0] == 0) return(NULL);
  /* if the file is a relative path, then prepend it with the path of the playlist */
  if (fnamebuf[1] != ':') {
    strcpy(tempstr, fnamebuf);
    filename2basename(playlist, NULL, fnamebuf, sizeof(fnamebuf) - 1);
    strncat(fnamebuf, tempstr, sizeof(fnamebuf) - 1);
  }
  /* return the result */
  return(fnamebuf);
}


/* returns a pointer to the next line of s, or NULL if no more lines */
static char *nextlinefrombuf(char *s) {
  for (;; s++) {
    if (*s == 0) return(NULL);
    if (*s == '\n') {
      s++;
      if (*s == 0) return(NULL);
      return(s);
    }
  }
}


/* copy the first line of s into d, up to l characters (incl. null term.) */
static void copyline(char *d, int l, char *s) {
  for (;;) {
    *d = *s;
    if (*d == 0) return;
    if ((*d == '\r') || (*d == '\n') || (--l == 0)) {
      *d = 0;
      return;
    }
    d++;
    s++;
  }
}


static enum playaction loadfile_midi(struct fiofile *f, const struct clioptions *params, struct trackinfodata *trackinfo, long int *trackpos) {
  static unsigned long trackmap[MAXTRACKS];
  int miditracks;
  int i;
  long newtrack;
  char copystring[UI_TITLEMAXLEN];
  char text[256];

  *trackpos = -1;

  miditracks = midi_readhdr(f, &(trackinfo->midiformat), &(trackinfo->miditimeunitdiv), trackmap, MAXTRACKS);
  if (miditracks < 1) {
    char errstr[64];
    sprintf(errstr, "Error: Invalid MIDI file format (ERR %d)", miditracks);
    ui_puterrmsg(params->midifile, errstr);
    return(ACTION_ERR_SOFT);
  }
  trackinfo->trackscount = miditracks;

#ifdef DBGFILE
  if (params->logfile) {
    fprintf(params->logfile, "LOADED FILE '%s': format=%d tracks=%d timeunitdiv=%u\n", params->midifile, trackinfo->midiformat, miditracks, trackinfo->miditimeunitdiv);
  }
#endif

  if ((trackinfo->midiformat != 0) && (trackinfo->midiformat != 1)) {
    char errstr[64];
    sprintf(errstr, "Error: Unsupported MIDI format (%d)", trackinfo->midiformat);
    ui_puterrmsg(params->midifile, errstr);
    return(ACTION_ERR_SOFT);
  }

  if (miditracks > MAXTRACKS) {
    char errstr[64];
    sprintf(errstr, "Error: Too many tracks (%d, max: %d)", miditracks, MAXTRACKS);
    ui_puterrmsg(params->midifile, errstr);
    return(ACTION_ERR_SOFT);
  }

  for (i = 0; i < miditracks; i++) {
    char tracktitle[UI_TITLEMAXLEN];
    unsigned long tracklen;

#ifdef DBGFILE
    if (params->logfile) fprintf(params->logfile, "LOADING TRACK %d FROM OFFSET 0x%04X\n", i, trackmap[i]);
#endif
    fio_seek(f, FIO_SEEK_START, trackmap[i]);
    if (i == 0) { /* copyright and text events are fetched from track 0 only */
      newtrack = midi_track2events(f, tracktitle, UI_TITLEMAXLEN, copystring,
                                   UI_TITLEMAXLEN, text, sizeof(text),
                                   &(trackinfo->channelsusage),
#ifdef DBGFILE
                                   params->logfile,
#endif
                                   &tracklen, trackinfo->reqpatches);
    } else {
      newtrack = midi_track2events(f, tracktitle, UI_TITLEMAXLEN, NULL, 0,
                                   NULL, 0, &(trackinfo->channelsusage),
#ifdef DBGFILE
                                   params->logfile,
#endif
                                   &tracklen, trackinfo->reqpatches);
    }
    /* look for error conditions */
    if (newtrack == MIDI_OUTOFMEM) {
      ui_puterrmsg(params->midifile, "Error: Out of memory");
      return(ACTION_ERR_SOFT);
    } else if (newtrack == MIDI_TRACKERROR) {
      ui_puterrmsg(params->midifile, "Error: Malformed MIDI file");
      return(ACTION_ERR_SOFT);
    }
    /* there is a non-written rule saying that useful text is written into
     * titles of empty tracks - push data into next available title node */
    if (((tracklen == 0) || (i == 0)) && (trackinfo->titlescount < UI_TITLENODES) && (tracktitle[0] != 0)) {
      /* ignore empty titles, though, if no valid title was found before */
      rtrim(tracktitle);
      if ((trackinfo->titlescount > 0) || (tracktitle[0] != 0)) {
        memcpy(trackinfo->title[trackinfo->titlescount++], tracktitle, UI_TITLEMAXLEN);
      }
    }
    /* merge the track now */
    if (newtrack >= 0) {
      *trackpos = midi_mergetrack(*trackpos, newtrack, &(trackinfo->totlen), trackinfo->miditimeunitdiv);
#ifdef DBGFILE
      if (params->logfile) fprintf(params->logfile, "TRACK %d MERGED (start id=%ld) -> TOTAL TIME: %ld\n", i, *trackpos, trackinfo->totlen);
#endif
    }
  }
  /* if we got any 'text', but no 'titles', then push the text into titles */
  if ((text[0] != 0) && (trackinfo->titlescount == 0)) {
    char *l;
    for (l = text; (l != NULL) && (trackinfo->titlescount < UI_TITLENODES); l = nextlinefrombuf(l)) {
      copyline(trackinfo->title[trackinfo->titlescount++], UI_TITLEMAXLEN, l);
    }
  }
  /* if we have room in title nodes, copy the copyright string there */
  if ((trackinfo->titlescount < UI_TITLENODES) && (copystring[0] != 0)) {
    memcpy(trackinfo->title[trackinfo->titlescount++], copystring, UI_TITLEMAXLEN);
  }
  return(ACTION_NONE);
}


static enum playaction loadfile(const struct clioptions *params, struct trackinfodata *trackinfo, long int *trackpos) {
  struct fiofile f;
  unsigned char hdr[16];
  enum playaction res;

  /* (try to) open the music file */
  if (fio_open(params->midifile, FIO_OPEN_RD, &f) != 0) {
    ui_puterrmsg(params->midifile, "Error: Failed to open the file");
    return(ACTION_ERR_SOFT);
  }

  /* read first few bytes of the file to detect its format, and rewind */
  if (fio_read(&f, hdr, 16) != 16) {
    fio_close(&f);
    ui_puterrmsg(params->midifile, "Error: Unknown file format");
    return(ACTION_ERR_SOFT);
  }
  fio_seek(&f, FIO_SEEK_START, 0);

  /* analyze the header to guess the format of the file */
  trackinfo->fileformat = header2fileformat(hdr);

  /* load file if format recognized */
  switch (trackinfo->fileformat) {
    case FORMAT_MIDI:
    case FORMAT_RMID:
      res = loadfile_midi(&f, params, trackinfo, trackpos);
      break;
    case FORMAT_MUS:
      *trackpos = mus_load(&f, &(trackinfo->totlen), &(trackinfo->miditimeunitdiv), &(trackinfo->channelsusage), trackinfo->reqpatches);
      if (*trackpos == MUS_OUTOFMEM) { /* detect out of memory */
        res = ACTION_ERR_SOFT;
        ui_puterrmsg(params->midifile, "Error: Out of memory");
      } else if (*trackpos < 0) { /* detect any other problems */
        char msg[64];
        res = ACTION_ERR_SOFT;
        snprintf(msg, 64, "Error: Failed to load the MUS file (%ld)", *trackpos);
        ui_puterrmsg(params->midifile, msg);
      } else { /* all right, now we're talking */
        trackinfo->trackscount = 1;
        res = ACTION_NONE;
      }
      break;
    default:
      res = ACTION_ERR_SOFT;
      ui_puterrmsg(params->midifile, "Error: Unknown file format");
      break;
  }
  fio_close(&f);

  /* if no text data could be found at all, add a note about that */
  if ((res == ACTION_NONE) && (trackinfo->titlescount == 0)) {
    strcpy(trackinfo->title[trackinfo->titlescount++], "<no title>");
  }

  return(res);
}


static void pauseplay(unsigned long *starttime, unsigned long *nexteventtime, struct trackinfodata *trackinfo) {
  unsigned long beforepause, afterpause, deltaremainder;
  int i;
  /* save timing information */
  timer_read(&beforepause);
  deltaremainder = *nexteventtime - beforepause;
  /* print a pause message on screen */
  ui_puterrmsg("PAUSE", "[ Press any key ]");
  /* turn off all notes before pausing */
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
  /* wait for a key press */
  getkey();
  /* restore play timing */
  /* FIXME if paused for a long time (over a hour and some), the timer might wrap, leading to very bad things */
  timer_read(&afterpause);
  *nexteventtime = afterpause + deltaremainder;  /* set nexteventtime to resync the song */
  /* adapt starttime to keep the progress bar in sync */
  *starttime += (afterpause - beforepause);
}


static void init_trackinfo(struct trackinfodata *trackinfo, const struct clioptions *params) {
  /* zero out the entire structure */
  memset(trackinfo, 0, sizeof(struct trackinfodata));
  /* preload piano into channels and set initial tempo */
  /* for (i = 0; i < 16; i++) trackinfo->chanprogs[i] = 0; */ /* no need, already zeroed by memset() */
  trackinfo->tempo = 500000l;
  /* put a something into the 'filename' field - midi or playlist, anything */
  if (params->midifile != NULL) {
    filename2basename(params->midifile, trackinfo->filename, NULL, UI_FILENAMEMAXLEN);
  } else if (params->playlist != NULL) {
    filename2basename(params->playlist, trackinfo->filename, NULL, UI_FILENAMEMAXLEN);
  }
  ucasestr(trackinfo->filename);
}


/* plays a file. returns 0 on success, non-zero if the program must exit */
static enum playaction playfile(struct clioptions *params, struct trackinfodata *trackinfo, struct midi_event *eventscache, enum order playlist_order) {
  static int volume = -1; /* volume is static because it needs to be retained between songs */
  int i;
  enum playaction exitaction;
  unsigned long nexteventtime;
  unsigned short refreshflags = UI_REFRESH_ALL;
  unsigned short refreshchans = 0xffffu;
  long trackpos;
  unsigned long midiplaybackstart;
  struct midi_event *curevent;
#ifdef DBGFILE
  unsigned long elticks = 0; /* used only to count clock ticks in debug mode */
#endif
  unsigned char *sysexbuff;

  /* abort early if there is a pending ESC or 'q' input (user wants out) */
  switch(getkey_ifany()) {
    case 0x1B:
    case 'q':
      return(ACTION_EXIT);
  }

  if(volume < 0) volume = params->volume;

  /* flush all MIDI events from memory for new events to have where to load */
  mem_clear();

  /* init trackinfo & cache data */
  init_trackinfo(trackinfo, params);
  getnexteventfromcache(eventscache, -1, 0);

  /* update screen with the next operation */
  sprintf(trackinfo->title[0], "Loading file...");
  ui_draw(trackinfo, &refreshflags, &refreshchans, params->devname, params->devport, params->onlpt, volume);
  refreshflags = UI_REFRESH_ALL;

  /* if running on a playlist, load next song */
  if (params->playlist != NULL) {
    params->midifile = getnextm3uitem(params->playlist, playlist_order);
    if (params->midifile == NULL) {
      ui_puterrmsg("Playlist error", "Failed to fetch an entry from the playlist");
      return(ACTION_ERR_HARD); /* this must be a hard error otherwise DOSMid might be trapped in a loop */
    }
  }

  /* reset the timer, to make sure it doesn't wrap around during playback */
  timer_reset();
  timer_read(&nexteventtime); /* save current time, to schedule when the song shall start */

#ifdef DBGFILE
  if (params->logfile) fprintf(params->logfile, "Reset MPU\n");
#endif

  /* load piano to all channels (even real MIDI synths do not always reset
   * those properly) - this could just as well happen during dev_clear(), but
   * there are users that happen to use DOSMid to init their MPU hardware,
   * and resetting patches *after* the midi file played would break that
   * usage for them */
  for (i = 0; i < 16; i++) {
    if (i != 9) dev_setprog(i, 0); /* do not set program on percussion */
    /* set pitch bend to a default preset */
    dev_controller(i, 100, 0);  /* RPN MSB 0 */
    dev_controller(i, 101, 0);  /* RPN LSB 0 -> RPN 0x0000 = "pitch bend" */
    dev_controller(i, 6, 2);    /* Pitch Bend Sensitivity MSB */
    dev_controller(i, 38, 0);   /* Pitch Bend Sensitivity LSB */
  }

  /* reset the device's master volume via sysex */
  //dev_sysex(0x7F, "\xF0\x7F\x7F\x04\x01\x7F\x7F\xF7", 8);

  /* preset the midi device to GM/GS/XG mode (or nothing) */
  switch (params->gmgspreset) {
    case PRESET_GM: /* GM RESET */
      dev_sysex(0, "\xF0\x7E\x7F\x09\x01\xF7", 6);
      break;
    case PRESET_GS: /* ROLAND GS RESET */
      dev_sysex(0, "\xF0\x41\x10\x42\x12\x40\x00\x7F\x00\x41\xF7", 11);
      break;
    case PRESET_XG: /* YAMAHA XG RESET */
      dev_sysex(0, "\xF0\x7E\x7F\x09\x01\xF7", 6); /* prefixed with GM reset */
      dev_sysex(0, "\xF0\x43\x10\x4C\x00\x00\x7E\x00\xF7", 9);
      break;
  }

  /* if a SYX init file is provided, feed it to the MIDI synth now */
  if (params->syxrst != NULL) {
    int syxlen;
    struct fiofile syxfh;
#ifdef DBGFILE
  if (params->logfile) fprintf(params->logfile, "loading SYSEX file %s\n", params->syxrst);
#endif
    /* open the syx file */
    if (fio_open(params->syxrst, FIO_OPEN_RD, &syxfh) != 0) {
      ui_puterrmsg(params->syxrst, "Error: Failed to open the SYX file");
      return(ACTION_ERR_HARD);
    }
    /* alloc a temporary buffer to hold sysex messages */
    sysexbuff = (void *)wbuff;
    /* read SYSEX messages until EOF */
    for (;;) {
      syxlen = syx_fetchnext(&syxfh, sysexbuff, 8192);
#ifdef DBGFILE
      if (params->logfile) fprintf(params->logfile, "sys_fetchnext() returned %d\n", syxlen);
#endif
      if (syxlen == 0) break; /* EOF */
      if (syxlen < 0) { /* error condition */
        fio_close(&syxfh); /* close the syx file */
        ui_puterrmsg(params->syxrst, "Error: Failed to process the SYX file");
        return(ACTION_ERR_HARD);
      }
      /* send the SYSEX message to the MIDI device, and wait a short moment
       * between messages so the device has time to digest them */
      dev_sysex(sysexbuff[0] & 0x0F, sysexbuff, syxlen);
      udelay(40000); /* 40ms should be enough time for the MPU interface   */
                     /* note that MT32 rev00 are *very* sensitive to this! */
      if (sysexbuff[0] == 0x7F) {  /* the 'all parameters reset' sysex takes */
        udelay(250000lu);          /* a very long time to be processed on    */
      }                            /* MT32 rev00 gears.                      */
    }
    fio_close(&syxfh); /* close the syx file */
  }

  /* load the file into memory */
  sprintf(trackinfo->title[0], "Loading...");
  filename2basename(params->midifile, trackinfo->filename, NULL, UI_FILENAMEMAXLEN);
  ucasestr(trackinfo->filename);
  ui_draw(trackinfo, &refreshflags, &refreshchans, params->devname, params->devport, params->onlpt, volume);
  memset(trackinfo->title[0], 0, 16);
  refreshflags = UI_REFRESH_ALL;

  if ((params->playlist != NULL) && (params->delay < 2000)) nexteventtime += (2000 - params->delay) * 1000L; /* playback starts no sooner than in 2s (for playlist listening comfort) */
  nexteventtime += params->delay * 1000L; /* add the extra custom delay */
  exitaction = loadfile(params, trackinfo, &trackpos);
  if (exitaction != ACTION_NONE) return(exitaction);
  /* if driving a GUS, preload needed MIDI patches up front */
  if (params->device == DEV_GUS) {
    int i;
    dev_preloadpatch(params->device, 0); /* always load grand piano, since it's the default instrument and the midi track might omit loading it explicitely */
    for (i = 0; i < 256; i++) {
      if (BIT_GET(trackinfo->reqpatches, i) != 0) {
        dev_preloadpatch(params->device, i);
#ifdef DBGFILE
        if (params->logfile) fprintf(params->logfile, "preloading patch %d\n", i);
#endif
      }
    }
  }
  /* draw the gui with track's data */
  ui_draw(trackinfo, &refreshflags, &refreshchans, params->devname, params->devport, params->onlpt, volume);
  for (;;) {
    timer_read(&midiplaybackstart); /* save start time so we can compute elapsed time later */
    if (midiplaybackstart >= nexteventtime) break; /* wait until the scheduled start time is met */
  }
  nexteventtime = midiplaybackstart;

  while (trackpos >= 0) {

    /* fetch next event */
    curevent = getnexteventfromcache(eventscache, trackpos, params->xmsdelay);
    if (curevent == NULL) { /* abort on error */
      ui_puterrmsg(params->midifile, "Error: Memory access fault");
      exitaction = ACTION_ERR_HARD;
      break;
    }

    /* give some time to the outdev driver for doing its things */
    dev_tick();

    /* printf("Action: %d / Note: %d / Vel: %d / t=%lu / next->%ld\n", curevent->type, curevent->data.note.note, curevent->data.note.velocity, curevent->deltatime, curevent->next); */
    if (curevent->deltatime > 0) { /* if I have some time ahead, I can do a few things */
      nexteventtime += DELTATIME2US(curevent->deltatime, trackinfo->tempo, trackinfo->miditimeunitdiv);
#ifdef DBGFILE
      elticks += curevent->deltatime;
#endif
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
          case 'q':
            exitaction = ACTION_EXIT;
            break;
          case 0x0D: /* return */
            exitaction = ACTION_NEXT;
            break;
          case 0x08: /* bkspc */
            exitaction = ACTION_PREV;
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
          case ' ':  /* pause */
            pauseplay(&midiplaybackstart, &nexteventtime, trackinfo);
            refreshflags = UI_REFRESH_ALL; /* force a full-screen refresh to wipe */
            refreshchans = 0xffffu;        /* the pause message out of the screen */
            break;
        }
        /* do I need to refresh the screen now? if not, just call INT28h */
        if (refreshflags != 0) {
          ui_draw(trackinfo, &refreshflags, &refreshchans, params->devname, params->devport, params->onlpt, volume);
        } else if (params->nopowersave == 0) { /* if no screen refresh is     */
          union REGS regs;                     /* needed, and power saver not */
          int86(0x28, &regs, &regs);           /* disabled, then call INT 28h */
        }                                      /* for some power saving       */
      }
      if (exitaction != ACTION_NONE) break;
    }

    switch (curevent->type) {
      case EVENT_NOTEON:
#ifdef DBGFILE
        if (params->logfile) {
          fprintf(params->logfile, "%lu: NOTE ON chan: %d / note: %d / vel: %d\n", trackinfo->elapsedsec, curevent->data.note.chan, curevent->data.note.note, curevent->data.note.velocity);
        }
#endif
        dev_noteon(curevent->data.note.chan, curevent->data.note.note, (volume * curevent->data.note.velocity) / 100);
        trackinfo->notestates[curevent->data.note.note] |= (1 << curevent->data.note.chan);
        refreshflags |= UI_REFRESH_NOTES;
        refreshchans |= (1 << curevent->data.note.chan);
        break;
      case EVENT_NOTEOFF:
#ifdef DBGFILE
        if (params->logfile) {
          fprintf(params->logfile, "%lu: NOTE OFF chan: %d / note: %d\n", trackinfo->elapsedsec, curevent->data.note.chan, curevent->data.note.note);
        }
#endif
        dev_noteoff(curevent->data.note.chan, curevent->data.note.note);
        trackinfo->notestates[curevent->data.note.note] &= (0xFFFF ^ (1 << curevent->data.note.chan));
        refreshflags |= UI_REFRESH_NOTES;
        refreshchans |= (1 << curevent->data.note.chan);
        break;
      case EVENT_TEMPO:
#ifdef DBGFILE
        if (params->logfile) {
          fprintf(params->logfile, "%lu (%lu): TEMPO change from %lu to %lu\n", trackinfo->elapsedsec, elticks, trackinfo->tempo, curevent->data.tempoval);
        }
#endif
        trackinfo->tempo = curevent->data.tempoval;
        refreshflags |= UI_REFRESH_TEMPO;
        break;
      case EVENT_PROGCHAN:
#ifdef DBGFILE
        if (params->logfile) {
          fprintf(params->logfile, "%lu: CHANNEL #%d PROG: %d\n", trackinfo->elapsedsec, curevent->data.prog.chan, curevent->data.prog.prog);
        }
#endif
        trackinfo->chanprogs[curevent->data.prog.chan] = curevent->data.prog.prog;
        dev_setprog(curevent->data.prog.chan, curevent->data.prog.prog);
        refreshflags |= UI_REFRESH_PROGS;
        break;
      case EVENT_PITCH:
#ifdef DBGFILE
        if (params->logfile) {
          fprintf(params->logfile, "%lu: PITCH WHEEL ON CHAN #%d: %d\n", trackinfo->elapsedsec, curevent->data.pitch.chan, curevent->data.pitch.wheel);
        }
#endif
        dev_pitchwheel(curevent->data.pitch.chan, curevent->data.pitch.wheel);
        break;
      case EVENT_CONTROL:
#ifdef DBGFILE
        if (params->logfile) {
          fprintf(params->logfile, "%lu: CONTROLLER %d ON CHAN #%d -> val %d\n", trackinfo->elapsedsec, curevent->data.control.id, curevent->data.control.chan, curevent->data.control.val);
        }
#endif
        dev_controller(curevent->data.control.chan, curevent->data.control.id, curevent->data.control.val);
        break;
      case EVENT_CHANPRESSURE:
#ifdef DBGFILE
        if (params->logfile) {
          fprintf(params->logfile, "%lu: CHANNEL PRESSURE %d ON CHAN #%d\n", trackinfo->elapsedsec, curevent->data.chanpressure.pressure, curevent->data.chanpressure.chan);
        }
#endif
        dev_chanpressure(curevent->data.chanpressure.chan, curevent->data.chanpressure.pressure);
        break;
      case EVENT_KEYPRESSURE:
#ifdef DBGFILE
        if (params->logfile) {
          fprintf(params->logfile, "%lu: KEY PRESSURE %d ON CHAN #%d, KEY %d\n", trackinfo->elapsedsec, curevent->data.keypressure.pressure, curevent->data.keypressure.chan, curevent->data.keypressure.note);
        }
#endif
        dev_keypressure(curevent->data.keypressure.chan, curevent->data.keypressure.note, curevent->data.keypressure.pressure);
        break;
      case EVENT_SYSEX:
      {
        unsigned short sysexlen;
        /* read two bytes from sysexptr so I know how long the thing is */
        mem_pull(curevent->data.sysex.sysexptr, &sysexlen, 2);
#ifdef DBGFILE
        if (params->logfile) fprintf(params->logfile, "%lu: SYSEX is %d bytes long", trackinfo->elapsedsec, sysexlen);
#endif
        /* */
        i = sysexlen;
        if ((i & 1) != 0) i++; /* XMS moves MUST occur on even-aligned data only */
        sysexbuff = (void *)wbuff;
        mem_pull(curevent->data.sysex.sysexptr, sysexbuff, i + 2);
        dev_sysex(sysexbuff[2] & 0x0F, sysexbuff + 2, sysexlen);
#ifdef DBGFILE
        if (params->logfile) {
          for (i = 0; i < sysexlen; i++) {
            fprintf(params->logfile, " %02Xh", sysexbuff[i + 2]);
          }
          fputc('\n', params->logfile);
        }
#endif
        break;
      }
      default:
#ifdef DBGFILE
        if (params->logfile) fprintf(params->logfile, "%lu: ILLEGAL COMMAND: 0x%02X\n", trackinfo->elapsedsec, curevent->type);
#endif
        break;
    }

    if (trackpos < 0) break;
    trackpos = curevent->next;

  }

#ifdef DBGFILE
  if (params->logfile) fprintf(params->logfile, "Clear notes\n");
#endif

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

  /* reset the device (all notes off, reset master volume, etc) */
  dev_clear(params->dev_clear_flags);

  return(exitaction);
}


int main(int argc, char **argv) {
  char *errstr;
  enum playaction action = ACTION_NONE;
  /* below objects are declared static so they land in the data segment and not in stack */
  static struct trackinfodata trackinfo;
  static struct midi_event eventscache[EVENTSCACHESIZE];
  static struct clioptions params;
  enum order playlistdir;

  params.volume = 100;

  /* preload the mpu port to be used (might be forced later via **argv) */
  preload_outdev(&params);

  errstr = loadconfigfile(&params);
  if (errstr == NULL) errstr = parseargv(argc, argv, &params);
  //switch(errstr) {
  if(errstr == REQUEST_HELP) {
      dos_puts("Usage: dosmid [<options>] <file> ...\r\n"
               "File can be m3u playlist.\r\n"
               "Options:\r\n"
               " /noxms     use conventional memory instead of XMS (loads small files only)\r\n"
               " /xmsdelay  wait 2ms before accessing XMS memory (AWEUTIL compatibility)\r\n"
               " /mpu[=<X>] use MPU-401 on I/O port <X>; will read BLASTER for port if omitted$");
      dos_puts(
#ifdef SBAWE
               " /awe[=<X>] use the EMU8K on SB AWE card; will read BLASTER for port if omitted\r\n"
#endif
#ifdef OPL
               " /opl[=<X>] use an FM synthesis OPL2/OPL3 chip for sound output\r\n"
               " /opl2[=<X>] use an OPL2-compatible chip for output even it is OPL3-compatible\r\n"
               " /opl3[=<X>] use an OPL3-compatible chip for output without fallback\r\n"
#endif
#ifdef CMS
               " /cms[=<X>] use Creative Music System / Game Blaster for sound output\r\n"
#endif
               " /sbmidi[=<X>] outputs MIDI to the SoundBlaster MIDI port at I/O port <X>$"
      );
      dos_puts(" /com=<X>   output MIDI messages to the RS-232 port at I/O port <X>\r\n"
               " /comX      same as /com=<X>, but takes a COM port instead (example: /com1)\r\n"
               " /gus       use the Gravis UltraSound card (requires ULTRAMID)\r\n"
               " /syx=<FILE> use SYSEX instructions from FILE for MIDI initialization\r\n"
               " /sbnk=<FILE> load a custom sound bank file(s) (IBK on OPL, SBK on AWE)$");
      dos_puts(
#ifdef DBGFILE
               " /log=<FILE> write highly verbose logs about DOSMid's activity to FILE\r\n"
#endif
               " /preset={GM|GS|XG|NONE} preset midi device to specified mode (default GM)\r\n"
               " /fullcpu   do not let DOSMid try to be CPU-friendly\r\n"
               " /dontstop  never wait for a keypress on error and continue the playlist\r\n"
               " /random    randomize playlist order\r\n"
               " /nosound   disable sound output\r\n"
               " /version   print version and optional features of this build\r\n"
               "Options can begin with either '-' or '/'.$"
      );
      return 0;
  }
  if(errstr == REQUEST_VERSION) {
      dos_puts("DOSMid " PVER "\r\n"
               "Copyright (C) " PDATE " Mateusz Viste\r\n"
               "$");
      dos_puts("Enabled optional features:"
#ifdef OPL
               "\r\n  OPL"
#ifdef OPLLPT
               "\r\n  OPLLPT"
#endif
#endif
#ifdef CMS
               "\r\n  CMS"
#ifdef CMSLPT
               "\r\n  CMSLPT"
#endif
#endif
#ifdef SBAWE
               "\r\n  AWE"
#endif
               "$");
      return 0;
  }
  if(errstr) {
    //default:
      dos_puts(errstr);
      dos_puts("Run DOSMID /? for additional help.$");
      return(1);
  }

#if defined CMSLPT || defined OPLLPT
  if(params.onlpt) {
    params.devport = get_lpt_port(params.onlpt);
    if(!params.devport) {
      fprintf(stderr, "LPT%c not found\r\n", params.onlpt + '0');
      return 1;
    }
  }
#endif
  params.devname = devtoname(params.device, params.devicesubtype);

  /* populate trackinfo with initial data */
  init_trackinfo(&trackinfo, &params);

  /* initialize the high resolution timer */
  timer_init();

  /* init ui and hide the blinking cursor */
  ui_init();
  ui_hidecursor();

  /* init the sound device */
  sprintf(trackinfo.title[0], "Sound hardware initialization...");
  {
    unsigned short rflags = 0xffffu, rchans = 0xffffu;
    ui_draw(&trackinfo, &rflags, &rchans, params.devname, params.devport, params.onlpt, 100);
  }
#ifdef DBGFILE
  if (params.logfile) fprintf(params.logfile, "INIT SOUND HARDWARE\n");
#endif
  errstr = dev_init(params.device, params.devport, params.onlpt, params.nockdev, params.sbnk);
  if (errstr != NULL) {
    ui_puterrmsg("Hardware initialization failure", errstr);
    getkey();
    goto hardwarefailure;
  }
  /* refresh outdev and its name (might have been changed due to OPL autodetection) */
  params.device = dev_getcurdev();
  params.devname = devtoname(params.device, params.devicesubtype);

  /* allocate the work memory */
  if (mem_init(params.memmode) == 0) {
    if (params.memmode == MEM_XMS) {
      ui_puterrmsg("ERROR: Memory init failed! No XMS maybe? Try /noxms.", NULL);
    } else {
      ui_puterrmsg("ERROR: Memory init failed!", NULL);
    }
    getkey();
    goto memallocfail;
  }

  /* playlist loop */
  while (action != ACTION_EXIT) {
    playlistdir = params.random ? RANDOM_ORDER : FORWARD_ORDER;
    action = playfile(&params, &trackinfo, eventscache, playlistdir);
    switch (action) {
      case ACTION_EXIT:
        /* do nothing, we will exit at the end of the loop anyway */
        break;
      case ACTION_PREV:
        playlistdir = REVERSE_ORDER;
        break;
      case ACTION_NEXT:
        /* no explicit action - we will do a 'next' action by default */
        break;
      case ACTION_ERR_HARD: /* wait for a keypress and quit */
        udelay(2000000lu);
        getkey();
        action = ACTION_EXIT;
        break;
      case ACTION_ERR_SOFT:         /* wait for a keypress so the user */
        if (params.dontstop == 0) { /* acknowledges the error message, */
          if (getkey() == 0x1B) {
            action = ACTION_EXIT;  /* abort on ESC */
            break;
          }
        } else {
          udelay(2000000lu);
        }
        /* FALLTHRU */
      case ACTION_NONE: /* choose an action depending on the mode we are in */
        if (params.playlist == NULL) {
          /* wait 1s before quit, so it doesn't feel 'brutal', but don't if */
          if (action == ACTION_NONE) udelay(1000000lu); /* an error occured */
          action = ACTION_EXIT;
        } else {
          action = ACTION_NEXT;
        }
        break;
    }
  }

  /* unload XMS memory */
  mem_close();

  memallocfail: /* jump here if mem_init() fails */

  /* close sound hardware */
  dev_close();

  hardwarefailure: /* this label I jump to when sound hardware init fails */

  /* reset screen (clears the screen and makes the cursor visible again) */
  ui_close();

  /* free the allocated strings, if any */
  if (params.sbnk != NULL) free(params.sbnk);
  if (params.syxrst != NULL) free(params.syxrst);

  /* if a verbose log file was used, close it now */
#ifdef DBGFILE
  if (params.logfile) {
    fprintf(params.logfile, "Closing the log file\n");
    fclose(params.logfile);
  }
#endif

  dos_puts("Exiting...\r\n$");
  dos_puts("DOSMid " PVER "\r\nCopyright (C) " PDATE " Mateusz Viste$");

  return(0);
}
