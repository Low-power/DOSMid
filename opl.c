/*
 * Library to access OPL2/OPL3 hardware (YM3812 / YMF262)
 *
 * Copyright (C) 2015-2016 Mateusz Viste
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

#ifdef OPL

#include "defines.h"
#ifdef MSDOS
#include <conio.h> /* inp(), out() */
#else
#include "unixpio.h"
#endif
#include <stdint.h>
#include <stdlib.h> /* calloc() */
#include <string.h> /* strdup() */
#include "opl.h"
#include "fio.h"
#include "opl-gm.h"
#include "timer.h"
#ifdef OPLLPT
#include "lpt.h"
#endif

struct voicealloc {
  unsigned short priority;
  signed short timbreid;
  signed char channel;
  signed char note;
};

struct oplstate {
  signed char notes2voices[16][128];    /* keeps the map of channel:notes -> voice allocations */
  unsigned short channelpitch[16];      /* per-channel pitch level */
  unsigned short channelvol[16];        /* per-channel pitch level */
  struct voicealloc voices2notes[18]; /* keeps the map of what voice is playing what note/channel currently */
  unsigned char channelprog[16];        /* programs (patches) assigned to channels */
  unsigned char *channelenable;
#if !defined MSDOS && defined OPLLPT
  int fd;
#endif
  uint16_t port;
  char opl3; /* flag indicating whether or not the sound module is OPL3-compatible or only OPL2 */
#ifdef OPLLPT
  char opllpt;
#endif
};

struct oplstate *oplmem = NULL; /* memory area holding all OPL's current states */

const unsigned short freqtable[128] = {                          /* note # */
        345, 365, 387, 410, 435, 460, 488, 517, 547, 580, 615, 651,  /*  0 */
        690, 731, 774, 820, 869, 921, 975, 517, 547, 580, 615, 651,  /* 12 */
        690, 731, 774, 820, 869, 921, 975, 517, 547, 580, 615, 651,  /* 24 */
        690, 731, 774, 820, 869, 921, 975, 517, 547, 580, 615, 651,  /* 36 */
        690, 731, 774, 820, 869, 921, 975, 517, 547, 580, 615, 651,  /* 48 */
        690, 731, 774, 820, 869, 921, 975, 517, 547, 580, 615, 651,  /* 60 */
        690, 731, 774, 820, 869, 921, 975, 517, 547, 580, 615, 651,  /* 72 */
        690, 731, 774, 820, 869, 921, 975, 517, 547, 580, 615, 651,  /* 84 */
        690, 731, 774, 820, 869, 921, 975, 517, 547, 580, 615, 651,  /* 96 */
        690, 731, 774, 820, 869, 921, 975, 517, 547, 580, 615, 651, /* 108 */
        690, 731, 774, 820, 869, 921, 975, 517};                    /* 120 */

const unsigned char octavetable[128] = {                         /* note # */
        0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,                          /*  0 */
        0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,                          /* 12 */
        0, 0, 0, 0, 0, 0, 0, 1, 1, 1, 1, 1,                          /* 24 */
        1, 1, 1, 1, 1, 1, 1, 2, 2, 2, 2, 2,                          /* 36 */
        2, 2, 2, 2, 2, 2, 2, 3, 3, 3, 3, 3,                          /* 48 */
        3, 3, 3, 3, 3, 3, 3, 4, 4, 4, 4, 4,                          /* 60 */
        4, 4, 4, 4, 4, 4, 4, 5, 5, 5, 5, 5,                          /* 72 */
        5, 5, 5, 5, 5, 5, 5, 6, 6, 6, 6, 6,                          /* 84 */
        6, 6, 6, 6, 6, 6, 6, 7, 7, 7, 7, 7,                          /* 96 */
        7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7,                         /* 108 */
        7, 7, 7, 7, 7, 7, 7, 7};                                    /* 120 */

const unsigned short pitchtable[256] = {                    /* pitch wheel */
         29193U,29219U,29246U,29272U,29299U,29325U,29351U,29378U,  /* -128 */
         29405U,29431U,29458U,29484U,29511U,29538U,29564U,29591U,  /* -120 */
         29618U,29644U,29671U,29698U,29725U,29752U,29778U,29805U,  /* -112 */
         29832U,29859U,29886U,29913U,29940U,29967U,29994U,30021U,  /* -104 */
         30048U,30076U,30103U,30130U,30157U,30184U,30212U,30239U,  /*  -96 */
         30266U,30293U,30321U,30348U,30376U,30403U,30430U,30458U,  /*  -88 */
         30485U,30513U,30541U,30568U,30596U,30623U,30651U,30679U,  /*  -80 */
         30706U,30734U,30762U,30790U,30817U,30845U,30873U,30901U,  /*  -72 */
         30929U,30957U,30985U,31013U,31041U,31069U,31097U,31125U,  /*  -64 */
         31153U,31181U,31209U,31237U,31266U,31294U,31322U,31350U,  /*  -56 */
         31379U,31407U,31435U,31464U,31492U,31521U,31549U,31578U,  /*  -48 */
         31606U,31635U,31663U,31692U,31720U,31749U,31778U,31806U,  /*  -40 */
         31835U,31864U,31893U,31921U,31950U,31979U,32008U,32037U,  /*  -32 */
         32066U,32095U,32124U,32153U,32182U,32211U,32240U,32269U,  /*  -24 */
         32298U,32327U,32357U,32386U,32415U,32444U,32474U,32503U,  /*  -16 */
         32532U,32562U,32591U,32620U,32650U,32679U,32709U,32738U,  /*   -8 */
         32768U,32798U,32827U,32857U,32887U,32916U,32946U,32976U,  /*    0 */
         33005U,33035U,33065U,33095U,33125U,33155U,33185U,33215U,  /*    8 */
         33245U,33275U,33305U,33335U,33365U,33395U,33425U,33455U,  /*   16 */
         33486U,33516U,33546U,33576U,33607U,33637U,33667U,33698U,  /*   24 */
         33728U,33759U,33789U,33820U,33850U,33881U,33911U,33942U,  /*   32 */
         33973U,34003U,34034U,34065U,34095U,34126U,34157U,34188U,  /*   40 */
         34219U,34250U,34281U,34312U,34343U,34374U,34405U,34436U,  /*   48 */
         34467U,34498U,34529U,34560U,34591U,34623U,34654U,34685U,  /*   56 */
         34716U,34748U,34779U,34811U,34842U,34874U,34905U,34937U,  /*   64 */
         34968U,35000U,35031U,35063U,35095U,35126U,35158U,35190U,  /*   72 */
         35221U,35253U,35285U,35317U,35349U,35381U,35413U,35445U,  /*   80 */
         35477U,35509U,35541U,35573U,35605U,35637U,35669U,35702U,  /*   88 */
         35734U,35766U,35798U,35831U,35863U,35895U,35928U,35960U,  /*   96 */
         35993U,36025U,36058U,36090U,36123U,36155U,36188U,36221U,  /*  104 */
         36254U,36286U,36319U,36352U,36385U,36417U,36450U,36483U,  /*  112 */
         36516U,36549U,36582U,36615U,36648U,36681U,36715U,36748U}; /*  120 */


/* tables below provide register offsets for each voice. note, that these are
 * NOT the registers IDs, but their direct offsets instead - this for simpler
 * and faster computations. */
const unsigned short op1offsets[18] = {0x00,0x01,0x02,0x08,0x09,0x0a,0x10,0x11,0x12,0x100,0x101,0x102,0x108,0x109,0x10a,0x110,0x111,0x112};
const unsigned short op2offsets[18] = {0x03,0x04,0x05,0x0b,0x0c,0x0d,0x13,0x14,0x15,0x103,0x104,0x105,0x10b,0x10c,0x10d,0x113,0x114,0x115};

/* number of melodic voices: 9 by default (OPL2), can go up to 18 (OPL3) */
static int voicescount = 9;

#ifdef HAVE_PORT_IO
#define pdelay(port, ncycles) do { int i = (ncycles); while(i-- > 0) inp(port); } while(0)
#else
#define pdelay(port, ncycles) abort()
#endif

/* function used to write into a register 'reg' of the OPL chip located at
 * port 'port', writing byte 'data' into it. this function supports also OPL3.
 * to write into the secondary address of an OPL3, just OR your register with
 * the 0x100 value (the 0x100 flag will be removed then, and the data will be
 * written into port+3). */
static void write_opl(
#if !defined MSDOS && defined OPLLPT
 int fd,
#endif
 unsigned short int port, int is_opl3,
#ifdef OPLLPT
 int is_opllpt,
#endif
 unsigned short int reg, unsigned char data) {
#ifdef OPLLPT
  if(is_opllpt) {
#ifndef MSDOS
    if(fd != 1) {
      write_lpt_fd(fd, reg & 0xff, (reg & 0x100) ? 0x5 : 0xd);
      udelay(4);
      write_lpt_fd(fd, data, 0xc);
    } else
#endif
    {
#ifdef HAVE_PORT_IO
      write_lpt(port, reg & 0xff, (reg & 0x100) ? 0x5 : 0xd);
      pdelay(port, 6);
      write_lpt(port, data, 0xc);
#endif
    }
  } else
#endif
  {
#ifdef HAVE_PORT_IO
    /* remap 'high' registers to second port (OPL3) */
    if ((reg & 0x100) != 0) {
      reg &= 0xff;
      port += 2;
    }
    /* select the register we want to write to, via the index register */
    outp(port, reg);
    /* OPL2 requires 3.3us to pass before writing to the data port. AdLib
     * recommends reading 6 times from the index register to make time pass. */
    pdelay(port, 6);
    /* write the data to the data register */
    outp(port+1, data);
#endif
  }
  /* OPL2 requires 23us to pass before writing to the data port. AdLib
   * recommends reading 35 times from the index register to make time pass. */
#ifndef MSDOS
  if(fd != 1) udelay(is_opl3 ? 4 : 23);
  else
#endif
  pdelay(port, is_opl3 ? 6 : 35);
}
#ifdef OPLLPT
#ifdef MSDOS
#define WRITE_OPL(H,REG,VALUE) write_opl((H)->port, (H)->opl3, (H)->opllpt, (REG), (VALUE))
#define WRITE_OPL_PROBE(PORT,REG,VALUE) write_opl((PORT), 0, 0, (REG), (VALUE))
#else
#define WRITE_OPL(H,REG,VALUE) write_opl((H)->fd, (H)->port, (H)->opl3, (H)->opllpt, (REG), (VALUE))
#define WRITE_OPL_PROBE(PORT,REG,VALUE) write_opl(-1, (PORT), 0, 0, (REG), (VALUE))
#endif
#else
#define WRITE_OPL(H,REG,VALUE) write_opl((H)->port, (H)->opl3, (REG), (VALUE))
#define WRITE_OPL_PROBE(PORT,REG,VALUE) write_opl((PORT), 0, (REG), (VALUE))
#endif

/* 'volume' is in range 0..127 - take care to change only the 'attenuation'
 * part of the register, and never touch the KSL bits */
static void calc_vol(unsigned char *regbyte, int volume) {
  int level;
  /* invert bits and strip out the KSL header */
  level = ~(*regbyte);
  level &= 0x3f;

  /* adjust volume */
  level = (level * volume) / 127;

  /* boundaries check */
  if (level > 0x3f) level = 0x3f;
  if (level < 0) level = 0;

  /* invert the bits, as expected by the OPL registers */
  level = ~level;
  level &= 0x3f;

  /* final result computation */
  *regbyte &= 0xC0;  /* zero out all attentuation bits */
  *regbyte |= level; /* fill in the new attentuation value */
}


/* get the id of the instrument that relates to channel/note pair */
static int getinstrument(int channel, int note) {
  if ((note < 0) || (note > 127) || (channel > 15)) return(-1);
  if (channel == 9) { /* the percussion channel requires special handling */
    return(128 | note);
  }
  return(oplmem->channelprog[channel]);
}


/* Initialize hardware upon startup
 * Possible values for '*gen':
 * -1	Auto-detect OPL2 and OPL3, detection result will be stored back
 * 2	Use the device as OPL2, even if it is OPL3-compatible
 * 3	Use the device as OPL3, fail if it isn't OPL3-compatible
 * 'flags' can be 0 or bit-wise ored value of following flags:
 * OPL_SKIP_CHECKING	Skip device presence checking
 * OPL_ON_LPT		Specify the device is an OPL2LPT or OPL3LPT; because
 *			this kind of device is write-only, the exact chip type
 *			must be specified via '*gen'
 * OPL_PORT_IS_FD	Specify 'port' is actually a file descriptor to a
 *			high-level device node for writing LPT; requires
 *			OPL_ON_LPT; this flag is valid for UNIX only
 * Possible return values:
 * 0	Success
 * -1	Device presence check failed (possible only if OPL_SKIP_CHECKING isn't
 *	specified)
 * -2	Request OPL3 but device is OPL2
 * -3	Out of memory
 * -4	Invalid value in '*gen'
 * -5	Already initialized
 * -6	Invalid 'flags'
 */
int opl_init(unsigned int port, int *gen, int flags) {
  int x;

  /* make sure we're not inited yet */
  if (oplmem != NULL) return(-5);

  //if(flags >> 3) return -6;
#if !defined MSDOS && defined OPLLPT
  if((flags & OPL_PORT_IS_FD) && !(flags & OPL_ON_LPT)) return -6;
#endif

#ifdef HAVE_PORT_IO
  if(!(flags & OPL_SKIP_CHECKING) && !(flags & OPL_ON_LPT)) {
    int y;
    /* detect the hardware and return error if not found */
    WRITE_OPL_PROBE(port, 0x04, 0x60); /* reset both timers by writing 60h to register 4 */
    WRITE_OPL_PROBE(port, 0x04, 0x80); /* enable interrupts by writing 80h to register 4 (must be a separate write from the 1st one) */
    x = inp(port) & 0xE0; /* read the status register (port 388h) and store the result */
    WRITE_OPL_PROBE(port, 0x02, 0xff); /* write FFh to register 2 (Timer 1) */
    WRITE_OPL_PROBE(port, 0x04, 0x21); /* start timer 1 by writing 21h to register 4 */
    udelay(500); /* Creative Labs recommends a delay of at least 80 microseconds
                    I delay for 500us just to be sure. DO NOT perform inp()
                    calls for delay here, some cards do not initialize well then
                    (reported for CT2760) */
    y = inp(port) & 0xE0;  /* read the upper bits of the status register */
    WRITE_OPL_PROBE(port, 0x04, 0x60); /* reset both timers and interrupts (see steps 1 and 2) */
    WRITE_OPL_PROBE(port, 0x04, 0x80); /* reset both timers and interrupts (see steps 1 and 2) */
    /* test the stored results of steps 3 and 7 by ANDing them with E0h. The result of step 3 should be */
    if (x != 0) return(-1);    /* 00h, and the result of step 7 should be C0h. If both are     */
    if (y != 0xC0) return(-1); /* ok, an AdLib-compatible board is installed in the computer   */
  }
#endif

  /* is it an OPL3 or just an OPL2? */
  switch(*gen) {
    case -1:
#ifdef OPLLPT
      if(flags & OPL_ON_LPT) return -1;
#endif
#ifdef HAVE_PORT_IO
      *gen = (inp(port) & 0x06) == 0 ? 3 : 2;
      break;
#else
      return -1;
#endif
    case 2:
      break;
    case 3:
#ifdef HAVE_PORT_IO
#ifdef OPLLPT
      if(!(flags & OPL_ON_LPT) && (inp(port) & 0x06)) return -2;
#else
      if(inp(port) & 0x06) return -2;
#endif
#endif
      break;
    default:
      return -4;
  }

  /* init memory */
  oplmem = calloc(1, sizeof(struct oplstate));
  if (oplmem == NULL) return(-3);

#if !defined MSDOS && defined OPLLPT
  if(flags & OPL_ON_LPT) {
    oplmem->fd = port;
    oplmem->port = 0;
  } else {
    oplmem->fd = -1;
    oplmem->port = port;
  }
#else
  oplmem->port = port;
#endif
  oplmem->opl3 = *gen == 3;
#ifdef OPLLPT
  oplmem->opllpt = flags & OPL_ON_LPT;
#endif

  /* init the hardware */
  voicescount = 9; /* OPL2 provides 9 melodic voices */

  /* enable OPL3 (if detected) and put it into 36 operators mode */
  if (oplmem->opl3 != 0) {
    WRITE_OPL(oplmem, 0x105, 1);  /* enable OPL3 mode (36 operators) */
    WRITE_OPL(oplmem, 0x104, 0);  /* disable four-operator voices */
    voicescount = 18;          /* OPL3 provides 18 melodic channels */

    /* Init the secondary OPL chip
     * NOTE: this I don't do anymore, it turns my Aztech Waverider mute! */
    /* WRITE_OPL(oplmem, 0x101, 0x20); */ /* enable Waveform Select */
    /* WRITE_OPL(oplmem, 0x108, 0x40); */ /* turn off CSW mode and activate FM synth mode */
    /* WRITE_OPL(oplmem, 0x1BD, 0x00); */ /* set vibrato/tremolo depth to low, set melodic mode */

    oplmem->channelenable = malloc(16);
  }

  WRITE_OPL(oplmem, 0x01, 0x20);  /* enable Waveform Select */
  WRITE_OPL(oplmem, 0x04, 0x00);  /* turn off timers IRQs */
  WRITE_OPL(oplmem, 0x08, 0x40);  /* turn off CSW mode and activate FM synth mode */
  WRITE_OPL(oplmem, 0xBD, 0x00);  /* set vibrato/tremolo depth to low, set melodic mode */

  for (x = 0; x < voicescount; x++) {
    WRITE_OPL(oplmem, 0x20 + op1offsets[x], 0x1);     /* set the modulator's multiple to 1 */
    WRITE_OPL(oplmem, 0x20 + op2offsets[x], 0x1);     /* set the modulator's multiple to 1 */
    WRITE_OPL(oplmem, 0x40 + op1offsets[x], 0x10);    /* set volume of all channels to about 40 dB */
    WRITE_OPL(oplmem, 0x40 + op2offsets[x], 0x10);    /* set volume of all channels to about 40 dB */
  }

  opl_clear();

  /* all done */
  return 0;
}


/* close OPL device */
void opl_close() {
  int x;

  /* turns all notes 'off' */
  opl_clear();

  /* set volume to lowest level on all voices */
  for (x = 0; x < voicescount; x++) {
    WRITE_OPL(oplmem, 0x40 + op1offsets[x], 0x1f);
    WRITE_OPL(oplmem, 0x40 + op2offsets[x], 0x1f);
  }

  /* if OPL3, switch the chip back into its default OPL2 mode */
  if (oplmem->opl3 != 0) WRITE_OPL(oplmem, 0x105, 0);

  /* free state memory */
  free(oplmem->channelenable);
  free(oplmem);
  oplmem = NULL;
}


/* turns off all notes */
void opl_clear() {
  int x, y;
  for (x = 0; x < voicescount; x++) opl_noteoff(x);

  /* reset the percussion bits at the 0xBD register */
  WRITE_OPL(oplmem, 0xBD, 0);

  /* mark all voices as unused */
  for (x = 0; x < voicescount; x++) {
    oplmem->voices2notes[x].channel = -1;
    oplmem->voices2notes[x].note = -1;
    oplmem->voices2notes[x].timbreid = -1;
  }

  /* mark all notes as unallocated */
  for (x = 0; x < 16; x++) {
    for (y = 0; y < 128; y++) oplmem->notes2voices[x][y] = -1;
  }

  /* pre-set emulated channel patches to default GM ids and reset all
   * per-channel volumes */
  for (x = 0; x < 16; x++) {
    opl_midi_changeprog(x, x);
    oplmem->channelvol[x] = 127;
    if(oplmem->channelenable) oplmem->channelenable[x] = 0x30;
  }
}


void opl_noteoff(unsigned short int voice) {
  /* if voice is one of the OPL3 set, adjust it and route over secondary OPL port */
  if (voice >= 9) {
    WRITE_OPL(oplmem, 0x1B0 + voice - 9, 0);
  } else {
    WRITE_OPL(oplmem, 0xB0 + voice, 0);
  }
}


void opl_noteon(unsigned short int voice, unsigned int note, int pitch) {
  unsigned int freq = freqtable[note];
  unsigned int octave = octavetable[note];

  if (pitch != 0) {
    if (pitch > 127) {
      pitch = 127;
    } else if (pitch < -128) {
      pitch = -128;
    }
    freq = ((unsigned long)freq * pitchtable[pitch + 128]) >> 15;
    if (freq >= 1024) {
      freq >>= 1;
      octave++;
    }
  }
  if (octave > 7) octave = 7;

  /* if voice is one of the OPL3 set, adjust it and route over secondary OPL port */
  if (voice >= 9) {
    voice -= 9;
    voice |= 0x100;
  }

  WRITE_OPL(oplmem, 0xA0 + voice, freq & 0xff); /* set lowfreq */
  WRITE_OPL(oplmem, 0xB0 + voice, (freq >> 8) | (octave << 2) | 32); /* KEY ON + hifreq + octave */
}


void opl_midi_pitchwheel(int channel, int pitchwheel) {
  /*int x;*/
  /* update the new pitch value for channel (used by newly played notes) */
  oplmem->channelpitch[channel] = pitchwheel;
  /* check all active voices to see who is playing on given channel now, and
   * recompute all playing notes for this channel with the new pitch TODO */
  /*for (x = 0; x < voicescount; x++) {
    if (oplmem->voices2notes[x].channel != channel) continue;
    opl_noteon(x, oplmem->voices2notes[x].note, pitchwheel + gmtimbres[oplmem->voices2notes[x].timbreid].finetune);
  }*/
}


void opl_midi_controller(int channel, int id, int value) {
  int x;
  switch (id) {
      unsigned char *enable;
    case 7:
    case 11: /* "Expression" (meaning "channel volume") */
      oplmem->channelvol[channel] = value;
      break;
    case 10: /* Pan */
      if(!oplmem->opl3) break;
      if(!oplmem->channelenable) break;
      enable = oplmem->channelenable + channel;
      if(value < 32) *enable = 0x20;
      else if(value > 96) *enable = 0x10;
      else *enable = 0x30;
      break;
    case 121: /* Reset all controllers */
      for(x = 0; x < 16; x++) {
        oplmem->channelvol[x] = 127;
        if(oplmem->channelenable) oplmem->channelenable[x] = 0x30;
      }
      break;
    case 123: /* 'all notes off' */
    case 120: /* 'all sound off' - I map it to 'all notes off' for now, not perfect but better than not handling it at all */
      for (x = 0; x < voicescount; x++) {
        if (oplmem->voices2notes[x].channel != channel) continue;
        opl_midi_noteoff(x, oplmem->voices2notes[x].note);
      }
      break;
  }
}


/* assign a new instrument to emulated MIDI channel */
void opl_midi_changeprog(int channel, int program) {
  if (channel == 9) return; /* do not allow to change channel 9, it is for percussions only */
  oplmem->channelprog[channel] = program;
}


static void opl_loadinstrument(unsigned short int voice, const struct timbre *timbre, unsigned char channelenable) {
  /* KSL (key level scaling) / attenuation */
  WRITE_OPL(oplmem, 0x40 + op1offsets[voice], timbre->modulator_40);
  WRITE_OPL(oplmem, 0x40 + op2offsets[voice], timbre->carrier_40 | 0x3f); /* force volume to 0, it will be reajusted during 'note on' */

  /* select waveform on both operators */
  WRITE_OPL(oplmem, 0xE0 + op1offsets[voice], timbre->modulator_E862 >> 24);
  WRITE_OPL(oplmem, 0xE0 + op2offsets[voice], timbre->carrier_E862 >> 24);

  /* sustain / release */
  WRITE_OPL(oplmem, 0x80 + op1offsets[voice], (timbre->modulator_E862 >> 16) & 0xff);
  WRITE_OPL(oplmem, 0x80 + op2offsets[voice], (timbre->carrier_E862 >> 16) & 0xff);

  /* attack rate / decay */
  WRITE_OPL(oplmem, 0x60 + op1offsets[voice], (timbre->modulator_E862 >> 8) & 0xff);
  WRITE_OPL(oplmem, 0x60 + op2offsets[voice], (timbre->carrier_E862 >> 8) & 0xff);

  /* AM / vibrato / envelope */
  WRITE_OPL(oplmem, 0x20 + op1offsets[voice], timbre->modulator_E862 & 0xff);
  WRITE_OPL(oplmem, 0x20 + op2offsets[voice], timbre->carrier_E862 & 0xff);

  /* feedback / connection */
  if (voice >= 9) {
    voice -= 9;
    voice |= 0x100;
  }
  WRITE_OPL(oplmem, 0xC0 + voice, timbre->feedconn | channelenable);
}


/* adjust the volume of the voice (in the usual MIDI range of 0..127) */
static void voicevolume(unsigned short voice, int program, int volume) {
  unsigned char carrierval = gmtimbres[program].carrier_40;
  if (volume == 0) {
    carrierval |= 0x3f;
  } else {
    calc_vol(&carrierval, volume);
  }
  WRITE_OPL(oplmem, 0x40 + op2offsets[voice], carrierval);
}


void opl_midi_noteon(int channel, int note, int velocity) {
  int x, voice = -1;
  int lowestpriorityvoice = 0;
  int instrument;

  /* get the instrument to play */
  instrument = getinstrument(channel, note);
  if (instrument < 0) return;

  /* if note already playing, then reuse its voice to avoid leaving a stuck voice */
  if (oplmem->notes2voices[channel][note] >= 0) {
    voice = oplmem->notes2voices[channel][note];
  } else {
    /* else find a free voice, possibly with the right timbre, or at least locate the oldest note */
    for (x = 0; x < voicescount; x++) {
      if (oplmem->voices2notes[x].channel < 0) {
        voice = x; /* preselect this voice, but continue looking */
        /* if the instrument is right, do not look further */
        if (oplmem->voices2notes[x].timbreid == instrument) {
          break;
        }
      }
      if (oplmem->voices2notes[x].priority < oplmem->voices2notes[lowestpriorityvoice].priority) lowestpriorityvoice = x;
    }
    /* if no free voice available, then abort the oldest one */
    if (voice < 0) {
      voice = lowestpriorityvoice;
      opl_midi_noteoff(oplmem->voices2notes[voice].channel, oplmem->voices2notes[voice].note);
    }
  }

  /* load the proper instrument, if not already good */
  if (oplmem->voices2notes[voice].timbreid != instrument) {
    unsigned char enable = oplmem->opl3 ?
      (oplmem->channelenable ? oplmem->channelenable[channel] : 0x30) : 0;
    oplmem->voices2notes[voice].timbreid = instrument;
    opl_loadinstrument(voice, gmtimbres + instrument, enable);
  }

  /* update states */
  oplmem->voices2notes[voice].channel = channel;
  oplmem->voices2notes[voice].note = note;
  oplmem->voices2notes[voice].priority = ((16 - channel) << 8) | 0xff; /* lower channels must have priority */
  oplmem->notes2voices[channel][note] = voice;

  /* set the requested velocity on the voice */
  voicevolume(voice, oplmem->voices2notes[voice].timbreid, velocity * oplmem->channelvol[channel] / 127);

  /* trigger NOTE_ON on the OPL, take care to apply the 'finetune' pitch correction, too */
  if (channel == 9) { /* percussion channel doesn't provide a real note, so I */
                      /* use a static one (MUSPLAYER uses C-5 (60), why not.  */
    opl_noteon(voice, 60, oplmem->channelpitch[channel] + gmtimbres[instrument].finetune);
  } else {
    opl_noteon(voice, note, oplmem->channelpitch[channel] + gmtimbres[instrument].finetune);
  }

  /* reajust all priorities */
  for (x = 0; x < voicescount; x++) {
    if (oplmem->voices2notes[x].priority > 0) oplmem->voices2notes[x].priority -= 1;
  }
}


void opl_midi_noteoff(int channel, int note) {
  if(note >= 0) {
    int voice = oplmem->notes2voices[channel][note];
    if (voice >= 0) {
      opl_noteoff(voice);
      oplmem->voices2notes[voice].channel = -1;
      oplmem->voices2notes[voice].note = -1;
      oplmem->voices2notes[voice].priority = -1;
      oplmem->notes2voices[channel][note] = -1;
    }
  }
}


static int opl_loadbank_internal(char *file, int offset) {
  unsigned char buff[16];
  int i;
  struct fiofile f;
  /* open the IBK file */
  if (fio_open(file, FIO_OPEN_RD, &f) != 0) return(-1);
  /* file must be exactly 3204 bytes long */
  if (fio_seek(&f, FIO_SEEK_END, 0) != 3204) {
    fio_close(&f);
    return(-2);
  }
  fio_seek(&f, FIO_SEEK_START, 0);
  /* file must start with an IBK header */
  if ((fio_read(&f, buff, 4) != 4) || (buff[0] != 'I') || (buff[1] != 'B') || (buff[2] != 'K') || (buff[3] != 0x1A)) {
    fio_close(&f);
    return(-3);
  }
  /* load 128 instruments from the IBK file */
  for (i = offset; i < 128 + offset; i++) {
    /* load instruments */
    if (fio_read(&f, buff, 16) != 16) {
      fio_close(&f);
      return(-4);
    }
    /* load modulator */
    gmtimbres[i].modulator_E862 = buff[8]; /* wave select */
    gmtimbres[i].modulator_E862 <<= 8;
    gmtimbres[i].modulator_E862 |= buff[6]; /* sust/release */
    gmtimbres[i].modulator_E862 <<= 8;
    gmtimbres[i].modulator_E862 |= buff[4]; /* attack/decay */
    gmtimbres[i].modulator_E862 <<= 8;
    gmtimbres[i].modulator_E862 |= buff[0]; /* AM/VIB... flags */
    /* load carrier */
    gmtimbres[i].carrier_E862 = buff[9]; /* wave select */
    gmtimbres[i].carrier_E862 <<= 8;
    gmtimbres[i].carrier_E862 |= buff[7]; /* sust/release */
    gmtimbres[i].carrier_E862 <<= 8;
    gmtimbres[i].carrier_E862 |= buff[5]; /* attack/decay */
    gmtimbres[i].carrier_E862 <<= 8;
    gmtimbres[i].carrier_E862 |= buff[1]; /* AM/VIB... flags */
    /* load KSL */
    gmtimbres[i].modulator_40 = buff[2];
    gmtimbres[i].carrier_40 = buff[3];
    /* feedconn & finetune */
    gmtimbres[i].feedconn = buff[10];
    gmtimbres[i].finetune = buff[12]; /* used only in some IBK files */
  }
  /* close file and return success */
  fio_close(&f);
  return(0);
}

/*
static void dump2file(void) {
  FILE *f;
  int i;
  f = fopen("dump.txt", "wb");
  if (f == NULL) return;
  for (i = 0; i < 256; i++) {
    char *comma = "";
    if (i < 255) comma = ",";
    fprintf(f, "{0x%07lX,0x%07lX,0x%02X,0x%02X,0x%02X,%d}%s\r\n", gmtimbres[i].modulator_E862, gmtimbres[i].carrier_E862, gmtimbres[i].modulator_40, gmtimbres[i].carrier_40, gmtimbres[i].feedconn, gmtimbres[i].finetune, comma);
  }
  fclose(f);
}*/

int opl_loadbank(char *file) {
  char *instruments = NULL, *percussion = NULL;
  int i, res;
  instruments = strdup(file); /* duplicate the string so we can modify it */
  if (instruments == NULL) return(-64); /* out of mem */
  /* if a second file is provided, it's for percussion */
  for (i = 0; instruments[i] != 0; i++) {
    if (instruments[i] == ',') {
      instruments[i] = 0;
      percussion = instruments + i + 1;
      break;
    }
  }
  /* load the file(s) */
  res = opl_loadbank_internal(instruments, 0);
  if ((res == 0) && (percussion != NULL)) {
    res = opl_loadbank_internal(percussion, 128);
  }
  free(instruments);
  /*dump2file();*/ /* dump instruments to a 'dump.txt' file */
  return(res);
}

#endif /* #ifdef OPL */
