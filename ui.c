/*
 * User interface routines of DOSMid.
 * Copyright (C) Mateusz Viste
 */

#include <dos.h>
#include <stdlib.h>  /* ultoaf() */
#include <stdio.h>   /* sprintf() */
#include <string.h>  /* strlen() */

#include "ui.h"  /* include self for control */

/* color scheme 0xBF00 (Background/Foreground/0/0) */
#define COLOR_TUI       0x1700u
#define COLOR_NOTES     0x1E00u
#define COLOR_TEXT      0x1700u
#define COLOR_TEMPO     0x1300u
#define COLOR_CHANS     0x1200u
#define COLOR_CHANS_DIS 0x1800u
#define COLOR_PROGRESS1 0x2000u /* elapsed time */
#define COLOR_PROGRESS2 0x8000u /* not yet elapsed */
#define COLOR_ERRMSG    0x4700u

unsigned short far *screenptr = NULL;
static int oldmode = 0;
static int colorflag = 0;

/* prints a character on screen, at position [x,y]. charbyte is a 16bit
   value where higher 8 bits contain the attributes (colors) and lower
   8bits contain the actual character to draw. */
static void ui_printchar(int y, int x, unsigned short charbyte) {
  screenptr[(y << 6) + (y << 4) + x] = charbyte;
}

static void ui_printstr(int y, int x, char *string, int staticlen, unsigned short attrib) {
  int xs;
  /* print out the string first */
  for (xs = 0; string[xs] != 0; xs++) ui_printchar(y, x++, string[xs] | attrib);
  /* if staticlen is provided, fill the rest with spaces */
  for (; xs < staticlen; xs++) ui_printchar(y, x++, ' ' | attrib);
}

void ui_init(void) {
  union REGS regs;
  /* remember the current mode */
  regs.h.ah = 0x0F; /* get current video mode */
  int86(0x10, &regs, &regs);
  oldmode = regs.h.al;
  /* set text mode 80x25 */
  regs.h.ah = 0x00;  /* set video mode */
  if ((oldmode == 0) || (oldmode == 7)) { /* 0=40x25 BW ; 7=80x25 BW */
    colorflag = 0;
    regs.h.al = 0x07;  /* 80x25, mono */
    screenptr = MK_FP(0xB000, 0);
  } else {
    colorflag = 1;
    regs.h.al = 0x03;  /* 80x25, 16 colors */
    screenptr = MK_FP(0xB800, 0);
  }
  int86(0x10, &regs, &regs);
  /* disable blinking effect (enables the use of high-intensity backgrounds).
   * This doesn't change anything on DOSemu nor VBox, but DOSbox is unable to
   * display high intensity backgrounds otherwise. */
  regs.x.ax = 0x1003;  /* toggle intensity/blinking */
  regs.h.bl = 0;       /* enable intensive colors (1 would enable blinking) */
  regs.h.bh = 0;       /* to avoid problems on some adapters */
  int86(0x10, &regs, &regs);
}

void ui_close(void) {
  union REGS regs;
  regs.h.ah = 0x00;  /* set video mode */
  regs.h.al = oldmode;
  int86(0x10, &regs, &regs);
}

void ui_hidecursor(void) {
  union REGS regs;
  regs.h.ah = 0x01;
  regs.x.cx = 0x2000;
  int86(0x10, &regs, &regs);
}

/* outputs an error message onscreen (title can be NULL) */
void ui_puterrmsg(char *title, char *errmsg) {
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
      ui_printchar(y, xstart + x - 2, ' ' | COLOR_ERRMSG);
    }
  }
  /* print out the title (if any), and the actual error string */
  if (title != NULL) ui_printstr(8, 40 - (titlelen >> 1), title, titlelen, COLOR_ERRMSG);
  ui_printstr(10, 40 - (msglen >> 1), errmsg, msglen, COLOR_ERRMSG);
}

/* draws the UI screen */
void ui_draw(struct trackinfodata *trackinfo, int *refreshflags, char *pver, char *devname, unsigned int mpuport, int volume) {
  #include "gm.h"  /* GM instruments names */
  int x, y;
  /* draw ascii graphic frames, etc */
  if (*refreshflags & UI_REFRESH_TUI) {
    char tempstr[32];
    for (x = 0; x < 80; x++) {
      ui_printchar(0, x, 205 | COLOR_TUI);
      ui_printchar(17, x, 205 | COLOR_TUI);
      ui_printchar(24, x, 205 | COLOR_TUI);
    }
    for (y = 1; y < 17; y++) ui_printchar(y, 15, 179 | COLOR_TUI);
    for (y = 18; y < 24; y++) {
      ui_printchar(y, 0, 186 | COLOR_TUI);
      ui_printchar(y, 79, 186 | COLOR_TUI);
    }
    ui_printchar(0, 15, 209 | COLOR_TUI);
    ui_printchar(17, 15, 207 | COLOR_TUI);
    ui_printchar(17, 0, 201 | COLOR_TUI);
    ui_printchar(17, 79, 187 | COLOR_TUI);
    ui_printchar(24, 0, 200 | COLOR_TUI);
    ui_printchar(24, 79, 188 | COLOR_TUI);
    sprintf(tempstr, "[ DOSMid v%s ]", pver);
    ui_printstr(24, 78 - strlen(tempstr), tempstr, -1, COLOR_TUI);
    sprintf(tempstr, "%s port: %03Xh", devname, mpuport);
    ui_printstr(18, 79 - strlen(tempstr), tempstr, -1, COLOR_TEMPO);
  }
  /* print notes states on every channel */
  if (*refreshflags & UI_REFRESH_NOTES) {
    for (y = 0; y < 16; y++) {
      for (x = 0; x < 64; x++) {
        int noteflag = 0;
        if (trackinfo->notestates[x << 1] & (1 << y)) noteflag = 2;
        if (trackinfo->notestates[1 + (x << 1)] & (1 << y)) noteflag |= 1;
        switch (noteflag) {
          case 0:
            ui_printchar(1 + y, 16 + x, ' ' | COLOR_NOTES | ((~x << 13) & 0x8000u));
            break;
          case 1:
            ui_printchar(1 + y, 16 + x, 0xde | COLOR_NOTES | ((~x << 13) & 0x8000u));
            break;
          case 2:
            ui_printchar(1 + y, 16 + x, 0xdd | COLOR_NOTES | ((~x << 13) & 0x8000u));
            break;
          case 3:
            ui_printchar(1 + y, 16 + x, 0xdb | COLOR_NOTES | ((~x << 13) & 0x8000u));
            break;
        }
      }
    }
  }
  /* tempo */
  if (*refreshflags & UI_REFRESH_TEMPO) {
    char tempstr[16];
    unsigned long miditempo;
    /* print filename */
    ui_printstr(18, 1, trackinfo->filename, 16, COLOR_TEMPO);
    /* print format */
    itoa(trackinfo->midiformat, tempstr, 10);
    ui_printstr(18, 17, "Format:", 8, COLOR_TEMPO);
    ui_printstr(18, 25, tempstr, 4, COLOR_TEMPO);
    /* print tempo */
    if (trackinfo->tempo > 0) {
      miditempo = 60000000lu / trackinfo->tempo;
    } else {
      miditempo = 0;
    }
    ultoa(miditempo, tempstr, 10);
    strcat(tempstr, " bpm");
    ui_printstr(18, 29, "Tempo:", 7, COLOR_TEMPO);
    ui_printstr(18, 36, tempstr, 9, COLOR_TEMPO);
  }
  /* volume */
  if (*refreshflags & UI_REFRESH_VOLUME) {
    char tempstr[16];
    sprintf(tempstr, "Volume: %d%%", volume);
    ui_printstr(18, 45, tempstr, 23 - strlen(devname), COLOR_TEMPO);
  }
  /* title and copyright notice */
  if (*refreshflags & UI_REFRESH_TITLECOPYR) {
    ui_printstr(19, 1, trackinfo->title[0], 78, COLOR_TEXT);
    ui_printstr(20, 1, trackinfo->title[1], 78, COLOR_TEXT);
    ui_printstr(21, 1, trackinfo->title[2], 78, COLOR_TEXT);
    ui_printstr(22, 1, trackinfo->copyright, 78, COLOR_TEXT);
  }
  /* programs (patches) names */
  if (*refreshflags & UI_REFRESH_PROGS) {
    unsigned int color;
    for (y = 0; y < 16; y++) {
      color = COLOR_CHANS_DIS;
      if (trackinfo->channelsusage & (1 << y)) color = COLOR_CHANS;
      if (y == 9) {
          ui_printstr(y + 1, 0, "Percussion", 15, color);
        } else {
          ui_printstr(y + 1, 0, gmset[trackinfo->chanprogs[y]], 15, color);
      }
    }
  }
  /* elapsed/total time */
  if (*refreshflags & UI_REFRESH_TIME) {
    char tempstr1[24];
    char tempstr2[16];
    unsigned long perc;
    unsigned int curcol;
    int rpos;
    if (trackinfo->totlen > 0) {
      perc = (trackinfo->elapsedsec * 100) / trackinfo->totlen;
    } else {
      perc = 0;
    }
    sprintf(tempstr1, " %lu:%02lu (%lu%%)     ", trackinfo->elapsedsec / 60, trackinfo->elapsedsec % 60, perc);
    rpos = 78 - sprintf(tempstr2, "%lu:%02lu ", trackinfo->totlen / 60, trackinfo->totlen % 60);
    /* draw the progress bar */
    if (trackinfo->totlen > 0) {
      perc = (trackinfo->elapsedsec * 78) / trackinfo->totlen;
    } else {
      perc = 0;
    }
    for (x = 0; x < 78; x++) {
      if (x < perc) {
        curcol = COLOR_PROGRESS1;
      } else {
        curcol = COLOR_PROGRESS2;
      }
      if (x < 15) {
        ui_printchar(23, 1 + x, tempstr1[x] | curcol);
      } else if (x >= rpos) {
        ui_printchar(23, 1 + x, tempstr2[x - rpos] | curcol);
      } else {
        ui_printchar(23, 1 + x, ' ' | curcol);
      }
    }
  }
  /* all refreshed now */
  *refreshflags = 0;
}


/* waits for a keypress and return it. Returns 0 for extended keystroke, then
   function must be called again to return scan code. */
int getkey(void) {
  union REGS regs;
  int res;
  regs.h.ah = 0x08;
  int86(0x21, &regs, &regs);
  res = regs.h.al;
  if (res == 0) { /* extended key - poll again */
    regs.h.ah = 0x08;
    int86(0x21, &regs, &regs);
    res = regs.h.al | 0x100;
  }
  return(res);
}


/* poll the keyboard, and return the next input key if any, or -1 */
int getkey_ifany(void) {
  union REGS regs;
  regs.h.ah = 0x0B;
  int86(0x21, &regs, &regs);
  if (regs.h.al == 0xFF) {
      return(getkey());
    } else {
      return(-1);
  }
}
