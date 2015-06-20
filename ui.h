/*
 * User interface routines of DOSMid.
 * Copyright (C) Mateusz Viste
 */

#ifndef ui_h_sentinel
#define ui_h_sentinel

#define UI_REFRESH_TUI        1
#define UI_REFRESH_NOTES      2
#define UI_REFRESH_TEMPO      4
#define UI_REFRESH_TITLECOPYR 8
#define UI_REFRESH_PROGS     16
#define UI_REFRESH_TIME      32
#define UI_REFRESH_VOLUME    64

#define UI_TITLEMAXLEN 63
#define UI_TITLENODES 3
#define UI_COPYRIGHTMAXLEN 63
#define UI_FILENAMEMAXLEN 15

struct trackinfodata {
  unsigned long tempo;
  unsigned long totlen; /* total length, in seconds */
  unsigned long elapsedsec; /* time elapsed (in seconds) */
  unsigned short notestates[128]; /* here I record the state of every note on every channel, to turn all notes OFF in case of program termination */
  unsigned short channelsusage;  /* a bit field indicating what channels are used */
  unsigned short miditimeunitdiv;
  unsigned char chanprogs[16];
  int titlescount;
  int midiformat;
  char titledat[UI_TITLENODES][UI_TITLEMAXLEN + 1];
  char *title[UI_TITLENODES];
  char copyright[UI_COPYRIGHTMAXLEN + 1];
  char filename[UI_FILENAMEMAXLEN + 1];
};

/* inits ui */
void ui_init(void);

/* hides the blinking cursor */
void ui_hidecursor(void);

/* outputs an error message onscreen (title can be NULL) */
void ui_puterrmsg(char *title, char *errmsg);

/* draws the UI screen */
void ui_draw(struct trackinfodata *trackinfo, int *refreshflags, char *pver, int mpuport, int volume);

/* waits for a keypress and return it. Returns 0 for extended keystroke, then
   function must be called again to return scan code. */
int getkey(void);

/* poll the keyboard, and return the next input key if any, or -1 */
int getkey_ifany(void);

#endif
