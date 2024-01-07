/*
 * User interface routines of DOSMid
 * Copyright (C) 2014-2016 Mateusz Viste
 * Copyright 2015-2024 Rivoreo
 */

#include <stdint.h>
#ifdef MSDOS
#include <dos.h>     /* REGS */
#else
#ifdef WCHAR
#define NCURSES_WIDECHAR 1
#endif
#include <curses.h>
#endif
#include <stdlib.h>  /* ultoa() */
#include <stdio.h>   /* sprintf() */
#include <string.h>  /* strlen() */
#ifdef MSDOS
#include "mem.h" /* MEM_XMS */
#endif
#include "ui.h"  /* include self for control */
#include "version.h"
#include <assert.h>

/* color scheme 0xBF00 (Background/Foreground/0/0): mono, color */
const unsigned short COLOR_TUI[2]       = {0x0700u, 0x1700u};
const unsigned short COLOR_NOTES[2]     = {0x0700u, 0x1E00u};
const unsigned short COLOR_NOTES_HI[2]  = {0x0000u, 0x8000u}; /* a bit mask for highlighten columns */
const unsigned short COLOR_TEXT[2]      = {0x0700u, 0x1700u};
const unsigned short COLOR_TEMPO[2]     = {0x0700u, 0x1300u};
const unsigned short COLOR_CHANS[2]     = {0x0700u, 0x1200u};
const unsigned short COLOR_CHANS_DIS[2] = {0x0000u, 0x1800u};
const unsigned short COLOR_PROGRESS1[2] = {0x7000u, 0x2000u}; /* elapsed time */
const unsigned short COLOR_PROGRESS2[2] = {0x0700u, 0x8000u}; /* not yet elapsed */
const unsigned short COLOR_ERRMSG[2]    = {0x7000u, 0x4700u};

#ifdef MSDOS
#define HSEPCHAR 0xcd
#define VSEPCHAR 0xba
#define VSSEPCHAR 0xb3
#define DHDBVSCROSS 0xd1
#define DHDTVSCROSS 0xcf
#define RHDBVDCROSS 0xc9
#define LHDBVDCROSS 0xbb
#define RHDTVDCROSS 0xc8
#define LHDTVDCROSS 0xbc
#define FULLBLOCK 0xdb
#define LEFTHALFBLOCK 0xdd
#define RIGHTHALFBLOCK 0xde
#else
#define HSEPCHAR '='
#define VSEPCHAR '|'
#define VSSEPCHAR '|'
#define DHDBVSCROSS '='
#define DHDTVSCROSS '='
#define RHDBVDCROSS '+'
#define LHDBVDCROSS '+'
#define RHDTVDCROSS '+'
#define LHDTVDCROSS '+'
#define FULLBLOCK '#'
#define LEFTHALFBLOCK ']'
#define RIGHTHALFBLOCK '['
#endif

static int colorflag = 0;
#ifdef MSDOS
unsigned short far *screenptr = NULL;
static int oldmode = 0;
static unsigned int oldcursor = 0;
extern unsigned short mem_mode; /* memory mode (MEM_XMS or MEM_MALLOC) */
#elif defined WCHAR
static int use_wchar = 0;
#endif

extern size_t mem_allocated_size;

/* prints a character on screen, at position [x,y]. ch is a 16 bit value where
   higher 8 bits contain the attributes (colors) and lower 8 bit contain the
   actual character to draw. */
static void ui_printchar(int y, int x, uint16_t ch) {
#ifdef MSDOS
  screenptr[(y << 6) + (y << 4) + x] = ch;
//#elif defined WCHAR
//  ui_printwchar(y, x, ch & 0xff, ch);
#else
  chtype cch = (ch & 0xff) | COLOR_PAIR((ch >> 8) & 0x77);
  if((ch >> 11) & 1) cch |= A_BOLD;
  if(ch >> 12 == 0xf) cch |= A_REVERSE;
  else if(ch >> 15) cch |= A_BLINK;
  mvaddch(y, x, cch);
#endif
}

#if !defined MSDOS && defined WCHAR
static void ui_printwchar(int y, int x, wchar_t ch, uint16_t attr) {
  cchar_t cch = { COLOR_PAIR((attr >> 8) & 0x77), { ch } };
  if((attr >> 11) & 1) cch.attr |= A_BOLD;
  if(attr >> 12 == 0xf) cch.attr |= A_REVERSE;
  else if(attr >> 15) cch.attr |= A_BLINK;
  mvadd_wch(y, x, &cch);
}
#endif

static void ui_printstr(int y, int x, const char *string, int staticlen, uint16_t attrib) {
  int xs;
  /* print out the string first */
  for (xs = 0; string[xs] != 0; xs++) ui_printchar(y, x++, string[xs] | attrib);
  /* if staticlen is provided, fill the rest with spaces */
  for (; xs < staticlen; xs++) ui_printchar(y, x++, ' ' | attrib);
}

void ui_init(int flags) {
#ifdef MSDOS
  union REGS regs;
  /* remember the current mode and cursor shape */
  regs.h.ah = 0x0F; /* get current video mode */
  int86(0x10, &regs, &regs);
  oldmode = regs.h.al;
  regs.h.ah = 0x03; /* get cursor shape */
  int86(0x10, &regs, &regs);
  oldcursor = regs.w.cx;
  /* set text mode 80x25 */
  regs.h.ah = 0x00;  /* set video mode */
  screenptr = MK_FP(0xB800, 0);
  colorflag = 0;
  switch (oldmode) {
    case 0: /* 40x25 BW */
    case 2: /* 80x25 BW */
      regs.h.al = 0x02;  /* 80x25 BW */
      break;
    case 7: /* 80x25 BW HGC */
      regs.h.al = 0x07;  /* 80x25 BW HGC */
      screenptr = MK_FP(0xB000, 0);
      break;
    default:
      if(!(flags & UI_NOCOLOR)) colorflag = 1;
      regs.h.al = 0x03;  /* 80x25, 16 colors */
      break;
  }
  int86(0x10, &regs, &regs);
  /* disable blinking effect (enables the use of high-intensity backgrounds).
   * This doesn't change anything on DOSemu nor VBox, but DOSbox is unable to
   * display high intensity backgrounds otherwise. */
  regs.x.ax = 0x1003;  /* toggle intensity/blinking */
  regs.h.bl = 0;       /* enable intensive colors (1 would enable blinking) */
  regs.h.bh = 0;       /* to avoid problems on some adapters */
  int86(0x10, &regs, &regs);
#else
  initscr();
  noecho();
  nodelay(stdscr, 1);
  keypad(stdscr, 1);
  if(!(flags & UI_NOCOLOR) && has_colors()) {
    start_color();
    init_pair(0x17, COLOR_WHITE, COLOR_BLUE);
    init_pair(0x16, COLOR_YELLOW, COLOR_BLUE);
    init_pair(0x00, COLOR_BLACK, COLOR_BLACK);
    init_pair(0x13, COLOR_CYAN, COLOR_BLUE);
    init_pair(0x12, COLOR_GREEN, COLOR_BLUE);
    init_pair(0x10, COLOR_BLACK, COLOR_BLUE);
    init_pair(0x20, COLOR_BLACK, COLOR_GREEN);
    init_pair(0x47, COLOR_WHITE, COLOR_RED);
    colorflag = 1;
#ifdef WCHAR
    use_wchar = flags & UI_WCHAR;
#endif
  }
#endif
}

void ui_close(void) {
#ifdef MSDOS
  union REGS regs;
  regs.h.ah = 0x00;  /* set video mode */
  regs.h.al = oldmode;
  int86(0x10, &regs, &regs);
  if(oldcursor) {
    regs.h.ah = 0x01;
    regs.w.cx = oldcursor;
    int86(0x10, &regs, &regs);
  }
#else
  curs_set(1);
  endwin();
#endif
}

void ui_hidecursor(void) {
#ifdef MSDOS
  union REGS regs;
  regs.h.ah = 0x01;
  regs.x.cx = 0x2000;
  int86(0x10, &regs, &regs);
#else
  curs_set(0);
#endif
}

/* outputs an error message onscreen (title can be NULL) */
void ui_puterrmsg(const char *title, const char *errmsg) {
  int x, y;
  int msglen, titlelen, maxlen;
  int xstart;
  msglen = strlen(errmsg);
  maxlen = msglen;
  if (title != NULL) {
    titlelen = strlen(title);
    if (titlelen > msglen) maxlen = titlelen;
  }
  xstart = 40 - (maxlen >> 1);
  /* draw a red 'box' first */
  for (y = 8; y < 13; y++) {
    for (x = maxlen + 3; x >= 0; x--) {
      ui_printchar(y, xstart + x - 2, ' ' | COLOR_ERRMSG[colorflag]);
    }
  }
  /* print out the title (if any), and the actual error string */
  if (title != NULL) ui_printstr(8, 40 - (titlelen >> 1), title, titlelen, COLOR_ERRMSG[colorflag]);
  ui_printstr(10, 40 - (msglen >> 1), errmsg, msglen, COLOR_ERRMSG[colorflag]);
#ifndef MSDOS
  refresh();
#endif
}

/* draws the UI screen */
void ui_draw(const struct trackinfodata *trackinfo, unsigned short int *refreshflags, unsigned short int *refreshchans, const char *devtypename,
#ifndef MSDOS
const char *devname,
#endif
unsigned int port, int onlpt, int volume) {
  #include "gm.h"  /* GM instruments names */
  int x, y;
  /* draw ascii graphic frames, etc */
  if (*refreshflags & UI_REFRESH_TUI) {
    int len;
    char buffer[32];
    for (x = 0; x < 80; x++) {
#if !defined MSDOS && defined WCHAR
      if(use_wchar) {
        ui_printwchar(0, x, L'═', COLOR_TUI[colorflag]);
        ui_printwchar(17, x, L'═', COLOR_TUI[colorflag]);
        ui_printwchar(24, x, L'═', COLOR_TUI[colorflag]);
      } else
#endif
      {
        ui_printchar(0, x, HSEPCHAR | COLOR_TUI[colorflag]);
        ui_printchar(17, x, HSEPCHAR | COLOR_TUI[colorflag]);
        ui_printchar(24, x, HSEPCHAR | COLOR_TUI[colorflag]);
      }
    }
    for (y = 1; y < 17; y++) {
#if !defined MSDOS && defined WCHAR
      if(use_wchar) ui_printwchar(y, 15, L'│', COLOR_TUI[colorflag]);
      else
#endif
      ui_printchar(y, 15, VSSEPCHAR | COLOR_TUI[colorflag]);
    }
    for (y = 18; y < 24; y++) {
#if !defined MSDOS && defined WCHAR
      if(use_wchar) {
        ui_printwchar(y, 0, L'║', COLOR_TUI[colorflag]);
        ui_printwchar(y, 79, L'║', COLOR_TUI[colorflag]);
      } else
#endif
      {
        ui_printchar(y, 0, VSEPCHAR | COLOR_TUI[colorflag]);
        ui_printchar(y, 79, VSEPCHAR | COLOR_TUI[colorflag]);
      }
    }
#if !defined MSDOS && defined WCHAR
    if(use_wchar) {
      ui_printwchar(0, 15, L'╤', COLOR_TUI[colorflag]);
      ui_printwchar(17, 15, L'╧', COLOR_TUI[colorflag]);
      ui_printwchar(17, 0, L'╔', COLOR_TUI[colorflag]);
      ui_printwchar(17, 79, L'╗', COLOR_TUI[colorflag]);
      ui_printwchar(24, 0, L'╚', COLOR_TUI[colorflag]);
      ui_printwchar(24, 79, L'╝', COLOR_TUI[colorflag]);
    } else
#endif
    {
      ui_printchar(0, 15, DHDBVSCROSS | COLOR_TUI[colorflag]);
      ui_printchar(17, 15, DHDTVSCROSS | COLOR_TUI[colorflag]);
      ui_printchar(17, 0, RHDBVDCROSS | COLOR_TUI[colorflag]);
      ui_printchar(17, 79, LHDBVDCROSS | COLOR_TUI[colorflag]);
      ui_printchar(24, 0, RHDTVDCROSS | COLOR_TUI[colorflag]);
      ui_printchar(24, 79, LHDTVDCROSS | COLOR_TUI[colorflag]);
    }
    ui_printstr(24, 79 - sizeof "[ DOSMid " PVER " ]", "[ DOSMid " PVER " ]", -1, COLOR_TUI[colorflag]);
    /* clear out the background on the 'messages' part of the screen */
    for (y = 18; y < 23; y++) {
      ui_printstr(y, 1, "", 78, COLOR_TEXT[colorflag]);
    }
    /* print static strings */
#ifndef MSDOS
    if(devname) {
#if 0
      len = snprintf(buffer, sizeof buffer, "%s %8s", devtypename, devname);
      if(len >= sizeof buffer) len = sizeof buffer - 1;
#else
      size_t type_name_len = strlen(devtypename);
      assert(type_name_len < sizeof buffer);
      assert(type_name_len < 7);
      size_t name_len = strlen(devname);
      if(name_len > sizeof buffer - type_name_len - 1) name_len = sizeof buffer - type_name_len - 1;
      memcpy(buffer, devtypename, type_name_len);
      len = type_name_len;
      do {
        buffer[len++] = ' ';
      } while(len < 8 && (size_t)len < 12 - name_len);
      memcpy(buffer + len, devname, name_len);
      len += name_len;
      buffer[len] = 0;
      if(len < 12) len = 12;
#endif
    } else
#endif
    if(onlpt) len = sprintf(buffer, "%s port LPT%c", devtypename, onlpt + '0');
    else len = sprintf(buffer, "%s port %03Xh", devtypename, port);
    x = 79 - len;
    ui_printstr(18, x, buffer, -1, COLOR_TEMPO[colorflag]);
    ui_printstr(19, x, "Volume", 6, COLOR_TEMPO[colorflag]);
    ui_printstr(20, x, "Format", 6, COLOR_TEMPO[colorflag]);
    ui_printstr(21, x, "Tracks", 6, COLOR_TEMPO[colorflag]);
    ui_printstr(22, x, "Tempo", 5, COLOR_TEMPO[colorflag]);
#ifdef MSDOS
    ui_printstr(22, 50, mem_mode == MEM_XMS ? "XMS" : "MEM", 4, COLOR_TEMPO[colorflag]);
#else
    ui_printstr(22, 50, "MEM", 4, COLOR_TEMPO[colorflag]);
#endif
    ui_printstr(22, 54, "used", 9, COLOR_TEMPO[colorflag]);
  }
  /* print notes states on every channel */
  if (*refreshflags & UI_REFRESH_NOTES) {
    for (y = 0; y < 16; y++) {
      if ((*refreshchans & (1 << y)) == 0) continue; /* skip channels that haven't changed */
      for (x = 0; x < 64; x++) {
        int noteflag = 0;
        if (trackinfo->notestates[x << 1] & (1 << y)) noteflag = 2;
        if (trackinfo->notestates[1 + (x << 1)] & (1 << y)) noteflag |= 1;
/*
#ifndef MSDOS
        attrset(A_BOLD | COLOR_PAIR(0x16));
#endif
*/
        if(noteflag) {
#if !defined MSDOS && defined WCHAR
          if(use_wchar) {
            wchar_t c = ((wchar_t []){L'▐',L'▌',L'█'})[noteflag - 1];
            ui_printwchar(1 + y, 16 + x, c,
              COLOR_NOTES[colorflag] | ((~x << 13) & COLOR_NOTES_HI[colorflag]));
          } else
#endif
          {
            static const unsigned char block_chars[] = { RIGHTHALFBLOCK, LEFTHALFBLOCK, FULLBLOCK };
            uint16_t c = block_chars[noteflag - 1];
            ui_printchar(1 + y, 16 + x, c |
              COLOR_NOTES[colorflag] | ((~x << 13) & COLOR_NOTES_HI[colorflag]));
          }
        } else {
          ui_printchar(1 + y, 16 + x, ' ' | COLOR_NOTES[colorflag] | ((~x << 13) & COLOR_NOTES_HI[colorflag]));
        }
      }
    }
    *refreshchans = 0;
  }
  /* filename and props (format, tracks) */
  if (*refreshflags & UI_REFRESH_FNAME) {
    char buffer[8], *sptr;
    /* print filename (unless NULL - might happen early at playlist load) */
    if (trackinfo->filename != NULL) {
      ui_printstr(18, 50, trackinfo->filename, 12, COLOR_TEMPO[colorflag]);
    } else {
      ui_printstr(18, 50, "", 12, COLOR_TEMPO[colorflag]);
    }
    /* total allocated memory */
    snprintf(buffer, sizeof buffer, "%zuK", mem_allocated_size >> 10);
    ui_printstr(22, 59, buffer, 6, COLOR_TEMPO[colorflag]);

    /* print format */
    switch ((trackinfo->fileformat << 1) | trackinfo->midiformat) {
      case FORMAT_MIDI << 1:
        sptr = "MID0";
        break;
      case (FORMAT_MIDI << 1) | 1:
        sptr = "MID1";
        break;
      case FORMAT_RMID << 1:
        sptr = "RMI0";
        break;
      case (FORMAT_RMID << 1) | 1:
        sptr = "RMI1";
        break;
      case FORMAT_MUS << 1:
        sptr = "MUS";
        break;
      default:
        sptr = "-";
        break;
    }
    ui_printstr(20, 75, sptr, 4, COLOR_TEMPO[colorflag]);
    /* print number of tracks */
#ifdef MSDOS
    utoa(trackinfo->trackscount, buffer, 10);
#else
    snprintf(buffer, sizeof buffer, "%hu", trackinfo->trackscount);
#endif
    ui_printstr(21, 75, buffer, 4, COLOR_TEMPO[colorflag]);
  }
  /* tempo */
  if (*refreshflags & UI_REFRESH_TEMPO) {
    char tempstr[16];
    unsigned long miditempo;
    /* print tempo */
    if (trackinfo->tempo > 0) {
      miditempo = 60000000lu / trackinfo->tempo;
    } else {
      miditempo = 0;
    }
#ifdef MSDOS
    ultoa(miditempo, tempstr, 10);
#else
    snprintf(tempstr, sizeof tempstr, "%lu", miditempo);
#endif
    ui_printstr(22, 75, tempstr, 4, COLOR_TEMPO[colorflag]);
  }
  /* volume */
  if (*refreshflags & UI_REFRESH_VOLUME) {
    char tempstr[8];
    sprintf(tempstr, "%d%%", volume);
    ui_printstr(19, 75, tempstr, 4, COLOR_TEMPO[colorflag]);
  }
  /* elapsed/total time */
  if (*refreshflags & UI_REFRESH_TIME) {
    char tmpstr1[24];
    char tmpstr2[16];
    unsigned long int perc;
    int rpos;
    if (trackinfo->totlen > 0) {
      perc = (trackinfo->elapsedsec * 100) / trackinfo->totlen;
    } else {
      perc = 0;
    }
    snprintf(tmpstr1, sizeof tmpstr1, " %lu:%02lu (%lu%%)     ", trackinfo->elapsedsec / 60, trackinfo->elapsedsec % 60, perc);
    rpos = 78 - snprintf(tmpstr2, sizeof tmpstr2, "%lu:%02lu ", trackinfo->totlen / 60, trackinfo->totlen % 60);
    /* draw the progress bar */
    if (trackinfo->totlen > 0) {
      perc = (trackinfo->elapsedsec * 78) / trackinfo->totlen;
    } else {
      perc = 0;
    }
    for (x = 0; x < 78; x++) {
      unsigned int curcol =
        ((unsigned long int)x < perc ? COLOR_PROGRESS1 : COLOR_PROGRESS2)[colorflag];
      if (x < 15) {
        ui_printchar(23, 1 + x, tmpstr1[x] | curcol);
      } else if (x >= rpos) {
        ui_printchar(23, 1 + x, tmpstr2[x - rpos] | curcol);
      } else {
        ui_printchar(23, 1 + x, ' ' | curcol);
      }
    }
    /* if we have more title nodes than fits on screen, scroll them down now */
    if (trackinfo->titlescount > 5) *refreshflags |= UI_REFRESH_TITLECOPYR;
  }
  /* title and copyright notice */
  if (*refreshflags & UI_REFRESH_TITLECOPYR) {
    int scrolloffset = 0, i;
    if ((trackinfo->titlescount <= 5) || (trackinfo->elapsedsec < 8)) {
      /* simple case */
      for (i = 0; i < 5; i++) {
        ui_printstr(18 + i, 1, trackinfo->title[i], UI_TITLEMAXLEN, COLOR_TEXT[colorflag]);
      }
    } else { /* else scroll down one line every 2s */
      scrolloffset = (trackinfo->elapsedsec >> 1) % (trackinfo->titlescount + 4);
      scrolloffset -= 4;
      for (i = 0; i < 5; i++) {
        if ((i + scrolloffset >= 0) && (i + scrolloffset < trackinfo->titlescount)) {
          ui_printstr(18 + i, 1, trackinfo->title[i + scrolloffset], UI_TITLEMAXLEN, COLOR_TEXT[colorflag]);
        } else {
          ui_printstr(18 + i, 1, "", UI_TITLEMAXLEN, COLOR_TEXT[colorflag]);
        }
      }
    }
  }
  /* programs (patches) names */
  if (*refreshflags & UI_REFRESH_PROGS) {
    for (y = 0; y < 16; y++) {
      uint16_t color = ((trackinfo->channelsusage & (1 << y)) ? COLOR_CHANS : COLOR_CHANS_DIS)[colorflag];
      if (y == 9) {
        ui_printstr(y + 1, 0, "Percussion", 15, color);
      } else {
        ui_printstr(y + 1, 0, gmset[trackinfo->chanprogs[y]], 15, color);
      }
    }
  }
#ifndef MSDOS
  refresh();
#endif
  /* all refreshed now */
  *refreshflags = 0;
}


/* waits for a keypress and return it. Returns 0 for extended keystroke, then
   function must be called again to return scan code. */
int getkey(void) {
  int k;
#ifdef MSDOS
  union REGS regs;
  regs.h.ah = 0x08;
  int86(0x21, &regs, &regs);
  k = regs.h.al;
  if (k == 0) { /* extended key - poll again */
    regs.h.ah = 0x08;
    int86(0x21, &regs, &regs);
    k = regs.h.al | 0x100;
  }
#else
  nocbreak();
  cbreak();
  nodelay(stdscr, 0);
  k = getch();
  nodelay(stdscr, 1);
#endif
  return k;
}


/* poll the keyboard, and return the next input key if any, or -1 */
int getkey_ifany(void) {
#ifdef MSDOS
  union REGS regs;
  regs.h.ah = 0x0B;
  int86(0x21, &regs, &regs);
  if (regs.h.al == 0xFF) {
    return(getkey());
  } else {
    return(-1);
  }
#else
  return getch();
#endif
}
