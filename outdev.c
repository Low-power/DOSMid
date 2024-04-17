/*
 * Wrapper for outputing MIDI commands to different devices.
 *
 * Copyright (C) 2014-2020, Mateusz Viste
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

#include "defines.h"
#include <stddef.h>
#ifdef MSDOS
#include <conio.h>  /* outp(), inp() */
#include <dos.h>    /* _disable(), _enable() */
#include <malloc.h> /* _fmalloc(), _ffree() */
#else
#include "unixpio.h"
#include <unistd.h>
#include <string.h>
#include <stdlib.h>
#include <errno.h>
#endif

#ifdef OPL
#include "opl.h"
#endif

#ifdef CMS
#include "cms.h"
#endif

#include "fio.h"
#include "gus.h"
#include "mpu401.h"
#ifdef MSDOS
#include "rs232.h"
#endif
#include "sbdsp.h"

#ifdef SBAWE
#include "awe32/ctaweapi.h"
static char far *presetbuf = NULL; /* used to allocate presets for custom sound banks */
#endif

#include "outdev.h" /* include self for control */
#include <assert.h>

#ifdef MSDOS
/* force the compiler to load valid DS segment value before calling
 * the AWE32 API functions (in far data models, where DS is floating) */
#pragma aux __pascal "^" parm loadds reverse routine [] \
                         value struct float struct caller [] \
                         modify [ax bx cx dx es];
#endif


static enum outdev_type outdev = DEV_NONE;
static unsigned short int outport = 0;
static int outport_is_lpt = 0;
#ifndef MSDOS
static int out_fd = -1;
#endif

/* loads a SBK sound font to AWE hardware */
#ifdef SBAWE
static int awe_loadfont(char *filename) {
  struct fiofile f;
  SOUND_PACKET sp;
  long banks[1];
  int i;
  char buffer[PACKETSIZE];
  awe32TotalPatchRam(&sp); /* get available patch DRAM */
  if (sp.total_patch_ram < 512*1024) return(-1);
  /* Setup bank sizes with all available RAM */
  banks[0] = sp.total_patch_ram;
  sp.banksizes = banks;
  sp.total_banks = 1; /* total number of banks */
  if (awe32DefineBankSizes(&sp) != 0) return(-1);
  /* Open SoundFont Bank */
  if (fio_open(filename, FIO_OPEN_RD, &f) != 0) return(-1);
  fio_read(&f, buffer, PACKETSIZE); /* read sf header */
  /* prepare stuff */
  sp.bank_no = 0;
  sp.data = buffer;
  if (awe32SFontLoadRequest(&sp) != 0) {
    fio_close(&f);
    return(-1); /* invalid soundfont file */
  }
  /* stream sound samples into the hardware */
  if (sp.no_sample_packets > 0) {
    fio_seek(&f, FIO_SEEK_START, sp.sample_seek); /* move pointer to where instruments begin */
    for (i = 0; i < sp.no_sample_packets; i++) {
      if ((fio_read(&f, sp.data, PACKETSIZE) != PACKETSIZE) || (awe32StreamSample(&sp))) {
        fio_close(&f);
        return(-1);
      }
    }
  }
  /* load presets to memory */
  presetbuf = _fmalloc(sp.preset_read_size);
  if (presetbuf == NULL) { /* out of mem! */
    fio_close(&f);
    return(-1);
  }
  sp.presets = presetbuf;
  fio_seek(&f, FIO_SEEK_START, sp.preset_seek);
  fio_read(&f, sp.presets, sp.preset_read_size);
  /* close the sf file */
  fio_close(&f);
  /* apply presets to hardware */
  if (awe32SetPresets(&sp) != 0) {
    _ffree(presetbuf);
    presetbuf = NULL;
    return(-1);
  }
  return(0);
}
#endif


/* inits the out device, also selects the out device, from one of these:
 *  DEV_MPU401
 *  DEV_AWE
 *  DEV_OPL
 *  DEV_RS232
 *  DEV_SBMIDI
 *  DEV_GUS
 *  DEV_NONE
 *
 * This should be called only ONCE, when program starts.
 * Returns NULL on success, or a pointer to an error string otherwise. */
const char *dev_init(enum outdev_type dev,
#ifdef HAVE_PORT_IO
uint16_t port,
#endif
#ifndef MSDOS
int fd,
#endif
int is_on_lpt, int skip_checking, char *sbank) {
  outdev = dev;
#ifdef MSDOS
  outport = port;
#elif HAVE_PORT_IO
  if(fd == -1) outport = port; else out_fd = fd;
#else
  if(fd == -1) return "Missing file descriptor";
  out_fd = fd;
#endif
  outport_is_lpt = is_on_lpt;
  switch (outdev) {
#ifdef OPL
      unsigned int port_or_fd;
      int gen;
      int flags;
#endif
#ifdef HAVE_PORT_IO
    case DEV_MPU401:
      /* reset the MPU401 */
      if (mpu401_rst(outport) != 0) return("MPU doesn't answer");
      /* put it into UART mode */
      mpu401_uart(outport);
      break;
#ifdef SBAWE
    case DEV_AWE:
      awe32NumG = 30; /* global var used by the AWE lib, must be set to 30 for
                         DRAM sound fonts to work properly */
      if (awe32Detect(outport) != 0) return("No EMU8000 chip detected");
      if (awe32InitHardware() != 0) return("EMU8000 initialization failed");
      /* preload GM samples from AWE's ROM */
      if (sbank == NULL) {
        awe32SoundPad.SPad1 = awe32SPad1Obj;
        awe32SoundPad.SPad2 = awe32SPad2Obj;
        awe32SoundPad.SPad3 = awe32SPad3Obj;
        awe32SoundPad.SPad4 = awe32SPad4Obj;
        awe32SoundPad.SPad5 = awe32SPad5Obj;
        awe32SoundPad.SPad6 = awe32SPad6Obj;
        awe32SoundPad.SPad7 = awe32SPad7Obj;
      } else if (awe_loadfont(sbank) != 0) {
        dev_close();
        return("Sound bank could not be loaded");
      }
      if (awe32InitMIDI() != 0) {
        dev_close();
        return("EMU8000 MIDI processor initialization failed");
      }
      break;
#endif
#endif	/* HAVE_PORT_IO */
#ifdef CMS
    case DEV_CMS:
      cms_reset(
#ifndef MSDOS
        out_fd,
#endif
        outport, is_on_lpt);
      break;
#endif
#ifdef OPL
#ifdef HAVE_PORT_IO
    case DEV_OPL:
      assert(!is_on_lpt);
      gen = -1;
      goto init_opl;
#endif
    case DEV_OPL2:
      gen = 2;
      goto init_opl;
    case DEV_OPL3:
      gen = 3;
    init_opl:
      flags = 0;
      if(skip_checking) flags |= OPL_SKIP_CHECKING;
      if(is_on_lpt) flags |= OPL_ON_LPT;
#ifndef MSDOS
      if(out_fd != -1) {
        port_or_fd = out_fd;
        flags |= OPL_PORT_IS_FD;
      } else
#endif
      {
        port_or_fd = outport;
      }
      switch(opl_init(port_or_fd, &gen, flags)) {
        case 0:
          break;
        case -1:
          return("No OPL2/OPL3 device detected");
        case -2:
          return("No OPL3 device detected");
        case -3:
          return "Out of memory";
        default:
          return "Failed to initialize OPL device due to an internal error";
      }
      /* change the outdev device depending on OPL autodetection */
      if(outdev == DEV_OPL) outdev = gen == 2 ? DEV_OPL2 : DEV_OPL3;
      /* load a custom sound bank, if any provided */
      if (sbank && opl_loadbank(sbank) < 0) {
        dev_close();
        return("OPL sound bank could not be loaded");
      }
      break;
#endif
    case DEV_RS232:
#ifdef MSDOS
      if (rs232_check(outport) != 0) return("RS232 failure");
#else
      if(!isatty(out_fd)) return strerror(errno);
#endif
      break;
#ifdef HAVE_PORT_IO
    case DEV_SBMIDI:
      /* The DSP has to be reset before it is first programmed. The reset
       * causes it to perform an initialization and returns it to its default
       * state. The DSP reset is done through the Reset port. */
      if (dsp_reset(outport) != 0) return("SB DSP initialization failure");
      dsp_write(outport, 0x30); /* switch the MIDI I/O into polling mode */
      break;
#endif
#ifdef MSDOS
    case DEV_GUS:
      gus_open(port);
      break;
#endif
    case DEV_NONE:
      break;
  }
  dev_clear(0);
  return(NULL);
}


/* pre-load a patch (so far needed only for GUS) */
void dev_preloadpatch(enum outdev_type dev, int p) {
  switch (dev) {
    case DEV_MPU401:
#ifdef SBAWE
    case DEV_AWE:
#endif
#ifdef OPL
    case DEV_OPL:
    case DEV_OPL2:
    case DEV_OPL3:
#endif
    case DEV_RS232:
    case DEV_SBMIDI:
#ifdef CMS
    case DEV_CMS:
#endif
      break;
#ifdef MSDOS
    case DEV_GUS:
      gus_loadpatch(p);
      break;
#endif
    case DEV_NONE:
      break;
  }
}


/* returns the device that has been inited/selected */
enum outdev_type dev_getcurdev(void) {
  return(outdev);
}


/* close/deinitializes the out device */
void dev_close(void) {
  switch (outdev) {
#ifdef HAVE_PORT_IO
    case DEV_MPU401:
      mpu401_rst(outport); /* resets it to intelligent mode */
      break;
#ifdef SBAWE
    case DEV_AWE:
      /* Creative recommends to disable interrupts during AWE shutdown */
      _disable();
      awe32Terminate();
      _enable();
      /* free memory used by custom sound banks */
      if (presetbuf != NULL) {
        _ffree(presetbuf);
        presetbuf = NULL;
      }
      break;
#endif
#endif
#ifdef OPL
    case DEV_OPL:
    case DEV_OPL2:
    case DEV_OPL3:
      opl_close();
      break;
#endif
#ifdef CMS
    case DEV_CMS:
      cms_reset(
#ifndef MSDOS
        out_fd,
#endif
        outport, outport_is_lpt);
      break;
#endif
    case DEV_RS232:
      break;
#ifdef HAVE_PORT_IO
    case DEV_SBMIDI:
      /* To terminate UART mode, send a DSP reset command. The reset command
       * behaves differently while the DSP is in MIDI UART mode. It terminates
       * MIDI UART mode and restores all the DSP parameters to the states
       * prior to entering MIDI UART mode. If your application was run in MIDI
       * UART mode, it important that you send the DSP reset command to exit
       * the MIDI UART mode when your application terminates. */
      dsp_reset(outport); /* I don't use MIDI UART mode because it requires */
                          /* DSP v2.x, but reseting the chip seems like a   */
      break;              /* good thing to do anyway                        */
#endif
#ifdef MSDOS
    case DEV_GUS:
      gus_close();
      break;
#endif
    case DEV_NONE:
      break;
  }
#ifndef MSDOS
  out_fd = -1;
#endif
  outport = 0;
}


/* clears the out device (turns all sounds off...). this can be used
 * often (typically: after each song) */
void dev_clear(int flags) {
  int i;
  /* iterate on MIDI channels and send 'off' messages */
  for (i = 0; i < 16; i++) {
    dev_controller(i, 123, 0);   /* "all notes off" */
    dev_controller(i, 120, 0);   /* "all sounds off" */
    if(!(flags & DOSMID_DEV_NORSTCTRL)) dev_controller(i, 121, 0);   /* "all controllers off" */
  }
  /* execute hardware-specific actions */
  switch (outdev) {
    case DEV_MPU401:
#ifdef SBAWE
    case DEV_AWE:
#endif
    case DEV_RS232:
    case DEV_SBMIDI:
      break;
#ifdef OPL
    case DEV_OPL:
    case DEV_OPL2:
    case DEV_OPL3:
      opl_clear();
      break;
#endif
#ifdef CMS
    case DEV_CMS:
      cms_reset(
#ifndef MSDOS
        out_fd,
#endif
        outport, outport_is_lpt);
      break;
#endif
#ifdef MSDOS
    case DEV_GUS:
      gus_allnotesoff();
      gus_unloadpatches();
      break;
#endif
    case DEV_NONE:
      break;
  }
}


/* activate note on channel */
void dev_noteon(int channel, int note, int velocity) {
  switch (outdev) {
#ifndef MSDOS
      unsigned char buffer[3];
#endif
#ifdef HAVE_PORT_IO
    case DEV_MPU401:
      mpu401_waitwrite(outport);      /* Wait for port ready */
      outp(outport, 0x90 | channel);  /* Send note ON to selected channel */
      mpu401_waitwrite(outport);      /* Wait for port ready */
      outp(outport, note);            /* Send note number to turn ON */
      mpu401_waitwrite(outport);      /* Wait for port ready */
      outp(outport, velocity);        /* Send velocity */
      break;
#endif
#ifdef OPL
    case DEV_OPL:
    case DEV_OPL2:
    case DEV_OPL3:
      opl_midi_noteon(channel, note, velocity);
      break;
#endif
#ifdef SBAWE
    case DEV_AWE:
      awe32NoteOn(channel, note, velocity);
      break;
#endif
#ifdef CMS
    case DEV_CMS:
      cms_noteon(channel, note, velocity);
      break;
#endif
    case DEV_RS232:
#ifdef MSDOS
      rs232_write(outport, 0x90 | channel); /* Send note ON to selected channel */
      rs232_write(outport, note);           /* Send note number to turn ON */
      rs232_write(outport, velocity);       /* Send velocity */
#else
      buffer[0] = 0x90 | channel;
      buffer[1] = note;
      buffer[2] = velocity;
      write(out_fd, buffer, 3);
#endif
      break;
#ifdef HAVE_PORT_IO
    case DEV_SBMIDI:
      dsp_write(outport, 0x38);             /* MIDI output */
      dsp_write(outport, 0x90 | channel);   /* Send note ON to selected channel */
      dsp_write(outport, 0x38);             /* MIDI output */
      dsp_write(outport, note);             /* Send note number to turn ON */
      dsp_write(outport, 0x38);             /* MIDI output */
      dsp_write(outport, velocity);         /* Send velocity */
      break;
#endif
#ifdef MSDOS
    case DEV_GUS:
      gus_write(0x90 | channel);            /* Send note ON to selected channel */
      gus_write(note);                      /* Send note number to turn ON */
      gus_write(velocity);                  /* Send velocity */
      break;
#endif
    case DEV_NONE:
      break;
  }
}


/* disable note on channel */
void dev_noteoff(int channel, int note) {
  switch (outdev) {
#ifndef MSDOS
      unsigned char buffer[3];
#endif
#ifdef HAVE_PORT_IO
    case DEV_MPU401:
      mpu401_waitwrite(outport);      /* Wait for port ready */
      outp(outport, 0x80 | channel);  /* Send note OFF code on selected channel */
      mpu401_waitwrite(outport);      /* Wait for port ready */
      outp(outport, note);            /* Send note number to turn OFF */
      mpu401_waitwrite(outport);      /* Wait for port ready */
      outp(outport, 64);              /* Send velocity */
      break;
#endif
#ifdef OPL
    case DEV_OPL:
    case DEV_OPL2:
    case DEV_OPL3:
      opl_midi_noteoff(channel, note);
      break;
#endif
#ifdef SBAWE
    case DEV_AWE:
      awe32NoteOff(channel, note, 64);
      break;
#endif
#ifdef CMS
    case DEV_CMS:
      cms_noteoff(channel, note);
      break;
#endif
    case DEV_RS232:
#ifdef MSDOS
      rs232_write(outport, 0x80 | channel); /* 'note off' + channel selector */
      rs232_write(outport, note);           /* note number */
      rs232_write(outport, 64);             /* velocity */
#else
      buffer[0] = 0x80 | channel;
      buffer[1] = note;
      buffer[2] = 64;
      write(out_fd, buffer, 3);
#endif
      break;
#ifdef HAVE_PORT_IO
    case DEV_SBMIDI:
      dsp_write(outport, 0x38);           /* MIDI output */
      dsp_write(outport, 0x80 | channel); /* Send note OFF code on selected channel */
      dsp_write(outport, 0x38);           /* MIDI output */
      dsp_write(outport, note);           /* Send note number to turn OFF */
      dsp_write(outport, 0x38);           /* MIDI output */
      dsp_write(outport, 64);             /* Send velocity */
      break;
#endif
#ifdef MSDOS
    case DEV_GUS:
      gus_write(0x80 | channel);   /* 'note off' + channel selector */
      gus_write(note);             /* note number */
      gus_write(64);               /* velocity */
      break;
#endif
    case DEV_NONE:
      break;
  }
}


/* adjust the pitch wheel of a channel */
void dev_pitchwheel(int channel, int wheelvalue) {
  switch (outdev) {
#ifndef MSDOS
      unsigned char buffer[3];
#endif
#ifdef HAVE_PORT_IO
    case DEV_MPU401:
      mpu401_waitwrite(outport);      /* Wait for port ready */
      outp(outport, 0xE0 | channel);  /* Send selected channel */
      mpu401_waitwrite(outport);      /* Wait for port ready */
      outp(outport, wheelvalue & 127);/* Send the lowest (least significant) 7 bits of the wheel value */
      mpu401_waitwrite(outport);      /* Wait for port ready */
      outp(outport, wheelvalue >> 7); /* Send the highest (most significant) 7 bits of the wheel value */
      break;
#endif
#ifdef OPL
    case DEV_OPL:
    case DEV_OPL2:
    case DEV_OPL3:
      opl_midi_pitchwheel(channel, wheelvalue);
      break;
#endif
#ifdef CMS
    case DEV_CMS:
      cms_pitchwheel(channel, wheelvalue);
      break;
#endif
#ifdef SBAWE
    case DEV_AWE:
      awe32PitchBend(channel, wheelvalue & 127, wheelvalue >> 7);
      break;
#endif
    case DEV_RS232:
#ifdef MSDOS
      rs232_write(outport, 0xE0 | channel);   /* Send selected channel */
      rs232_write(outport, wheelvalue & 127); /* Send the lowest (least significant) 7 bits of the wheel value */
      rs232_write(outport, wheelvalue >> 7);  /* Send the highest (most significant) 7 bits of the wheel value */
#else
      buffer[0] = 0xE0 | channel;
      buffer[1] = wheelvalue & 127;
      buffer[2] = wheelvalue >> 7;
      write(out_fd, buffer, 3);
#endif
      break;
#ifdef HAVE_PORT_IO
    case DEV_SBMIDI:
      dsp_write(outport, 0x38);             /* MIDI output */
      dsp_write(outport, 0xE0 | channel);   /* Send selected channel */
      dsp_write(outport, 0x38);             /* MIDI output */
      dsp_write(outport, wheelvalue & 127); /* Send the lowest (least significant) 7 bits of the wheel value */
      dsp_write(outport, 0x38);             /* MIDI output */
      dsp_write(outport, wheelvalue >> 7);  /* Send the highest (most significant) 7 bits of the wheel value */
      break;
#endif
#ifdef MSDOS
    case DEV_GUS:
      gus_write(0xE0 | channel);   /* Send selected channel */
      gus_write(wheelvalue & 127); /* Send the lowest (least significant) 7 bits of the wheel value */
      gus_write(wheelvalue >> 7);  /* Send the highest (most significant) 7 bits of the wheel value */
      break;
#endif
    case DEV_NONE:
      break;
  }
}


/* send a 'controller' message */
void dev_controller(int channel, int id, int val) {
  switch (outdev) {
#ifndef MSDOS
      unsigned char buffer[3];
#endif
#ifdef HAVE_PORT_IO
    case DEV_MPU401:
      mpu401_waitwrite(outport);      /* Wait for port ready */
      outp(outport, 0xB0 | channel);  /* Send selected channel */
      mpu401_waitwrite(outport);      /* Wait for port ready */
      outp(outport, id);              /* Send the controller's id */
      mpu401_waitwrite(outport);      /* Wait for port ready */
      outp(outport, val);             /* Send controller's value */
      break;
#endif
#ifdef OPL
    case DEV_OPL:
    case DEV_OPL2:
    case DEV_OPL3:
      opl_midi_controller(channel, id, val);
      break;
#endif
#ifdef CMS
    case DEV_CMS:
      cms_controller(channel, id, val);
      break;
#endif
#ifdef SBAWE
    case DEV_AWE:
      awe32Controller(channel, id, val);
      break;
#endif
    case DEV_RS232:
#ifdef MSDOS
      rs232_write(outport, 0xB0 | channel);  /* Send selected channel */
      rs232_write(outport, id);              /* Send the controller's id */
      rs232_write(outport, val);             /* Send controller's value */
#else
      buffer[0] = 0xB0 | channel;
      buffer[1] = id;
      buffer[2] = val;
      write(out_fd, buffer, 3);
#endif
      break;
#ifdef HAVE_PORT_IO
    case DEV_SBMIDI:
      dsp_write(outport, 0x38);            /* MIDI output */
      dsp_write(outport, 0xB0 | channel);  /* Send selected channel */
      dsp_write(outport, 0x38);            /* MIDI output */
      dsp_write(outport, id);              /* Send the controller's id */
      dsp_write(outport, 0x38);            /* MIDI output */
      dsp_write(outport, val);             /* Send controller's value */
      break;
#endif
#ifdef MSDOS
    case DEV_GUS:
      gus_write(0xB0 | channel);           /* Send selected channel */
      gus_write(id);                       /* Send the controller's id */
      gus_write(val);                      /* Send controller's value */
      break;
#endif
    case DEV_NONE:
      break;
  }
}


void dev_chanpressure(int channel, int pressure) {
  switch (outdev) {
#ifndef MSDOS
      unsigned char buffer[2];
#endif
#ifdef HAVE_PORT_IO
    case DEV_MPU401:
      mpu401_waitwrite(outport);      /* Wait for port ready */
      outp(outport, 0xD0 | channel);  /* Send selected channel */
      mpu401_waitwrite(outport);      /* Wait for port ready */
      outp(outport, pressure);        /* Send the pressure value */
      break;
#endif
#ifdef OPL
    case DEV_OPL:
    case DEV_OPL2:
    case DEV_OPL3:
      /* nothing to do */
      break;
#endif
#ifdef SBAWE
    case DEV_AWE:
      awe32ChannelPressure(channel, pressure);
      break;
#endif
    case DEV_RS232:
#ifdef MSDOS
      rs232_write(outport, 0xD0 | channel);  /* Send selected channel */
      rs232_write(outport, pressure);        /* Send the pressure value */
#else
      buffer[0] = 0xD0 | channel;
      buffer[1] = pressure;
      write(out_fd, buffer, 2);
#endif
      break;
#ifdef HAVE_PORT_IO
    case DEV_SBMIDI:
      dsp_write(outport, 0x38);            /* MIDI output */
      dsp_write(outport, 0xD0 | channel);  /* Send selected channel */
      dsp_write(outport, 0x38);            /* MIDI output */
      dsp_write(outport, pressure);        /* Send the pressure value */
      break;
#endif
#ifdef MSDOS
    case DEV_GUS:
      gus_write(0xD0 | channel);           /* Send selected channel */
      gus_write(pressure);                 /* Send the pressure value */
      break;
#endif
    case DEV_NONE:
      break;
  }
}


void dev_keypressure(int channel, int note, int pressure) {
  switch (outdev) {
#ifndef MSDOS
      unsigned char buffer[3];
#endif
#ifdef HAVE_PORT_IO
    case DEV_MPU401:
      mpu401_waitwrite(outport);      /* Wait for port ready */
      outp(outport, 0xA0 | channel);  /* Send selected channel */
      mpu401_waitwrite(outport);      /* Wait for port ready */
      outp(outport, note);            /* Send the note we target */
      mpu401_waitwrite(outport);      /* Wait for port ready */
      outp(outport, pressure);        /* Send the pressure value */
      break;
#endif
#ifdef OPL
    case DEV_OPL:
    case DEV_OPL2:
    case DEV_OPL3:
      /* nothing to do */
      break;
#endif
#ifdef SBAWE
    case DEV_AWE:
      awe32PolyKeyPressure(channel, note, pressure);
      break;
#endif
    case DEV_RS232:
#ifdef MSDOS
      rs232_write(outport, 0xA0 | channel);  /* Send selected channel */
      rs232_write(outport, note);            /* Send the note we target */
      rs232_write(outport, pressure);        /* Send the pressure value */
#else
      buffer[0] = 0xA0 | channel;
      buffer[1] = note;
      buffer[2] = pressure;
      write(out_fd, buffer, 3);
#endif
      break;
#ifdef HAVE_PORT_IO
    case DEV_SBMIDI:
      dsp_write(outport, 0x38);            /* MIDI output */
      dsp_write(outport, 0xA0 | channel);  /* Send selected channel */
      dsp_write(outport, 0x38);            /* MIDI output */
      dsp_write(outport, note);            /* Send the note we target */
      dsp_write(outport, 0x38);            /* MIDI output */
      dsp_write(outport, pressure);        /* Send the pressure value */
      break;
#endif
#ifdef MSDOS
    case DEV_GUS:
      gus_write(0xA0 | channel);           /* Send selected channel */
      gus_write(note);                     /* Send the note we target */
      gus_write(pressure);                 /* Send the pressure value */
      break;
#endif
    case DEV_NONE:
      break;
  }
}


/* should be called by the application from time to time */
void dev_tick(void) {
  switch (outdev) {
#ifdef CMS
    case DEV_CMS:
      cms_tick();
      break;
#endif
#ifdef HAVE_PORT_IO
    case DEV_MPU401:
      mpu401_flush(outport);
      break;
#endif
#ifdef OPL
    case DEV_OPL:
    case DEV_OPL2:
    case DEV_OPL3:
      break;
#endif
#ifdef SBAWE
    case DEV_AWE:
      break;
#endif
    case DEV_RS232:
      /* I do nothing here - although flushing any incoming bytes would seem
       * to be the 'sane thing to do', it can lead sometimes to freezes on
       * systems where the RS232 UART always reports a 'read ready' status.
       * NOT flushing the UART, on the other hand, doesn't seem to affect
       * anything. */
      /* while (rs232_read(outport) >= 0); */
      break;
    case DEV_SBMIDI:
      break;
#ifdef MSDOS
    case DEV_GUS:
      break;
#endif
    case DEV_NONE:
      break;
  }
}


/* sets a "program" (meaning an instrument) on a channel */
void dev_setprog(int channel, int program) {
  switch (outdev) {
#ifndef MSDOS
      unsigned char buffer[2];
#endif
#ifdef HAVE_PORT_IO
    case DEV_MPU401:
      mpu401_waitwrite(outport);     /* Wait for port ready */
      outp(outport, 0xC0 | channel); /* Send channel */
      mpu401_waitwrite(outport);     /* Wait for port ready */
      outp(outport, program);        /* Send patch id */
      break;
#endif
#ifdef OPL
    case DEV_OPL:
    case DEV_OPL2:
    case DEV_OPL3:
      opl_midi_changeprog(channel, program);
      break;
#endif
#ifdef SBAWE
    case DEV_AWE:
      awe32ProgramChange(channel, program);
      break;
#endif
    case DEV_RS232:
#ifdef MSDOS
      rs232_write(outport, 0xC0 | channel); /* Send channel */
      rs232_write(outport, program);        /* Send patch id */
#else
      buffer[0] = 0xC0 | channel;
      buffer[1] = program;
      write(out_fd, buffer, 2);
#endif
      break;
#ifdef HAVE_PORT_IO
    case DEV_SBMIDI:
      dsp_write(outport, 0x38);           /* MIDI output */
      dsp_write(outport, 0xC0 | channel); /* Send channel */
      dsp_write(outport, 0x38);           /* MIDI output */
      dsp_write(outport, program);        /* Send patch id */
      break;
#endif
#ifdef MSDOS
    case DEV_GUS:
      /* NOTE I might (?) want to call gus_loadpatch() here */
      /*if (channel == 9) program |= 128;
      gus_loadpatch(program);*/
      gus_write(0xC0 | channel);          /* Send channel */
      gus_write(program);                 /* Send patch id */
      break;
#endif
    case DEV_NONE:
      break;
  }
}


/* sends a raw sysex string to the device */
void dev_sysex(int channel, unsigned char *buff, int bufflen) {
  int x;
  switch (outdev) {
#ifdef HAVE_PORT_IO
    case DEV_MPU401:
      for (x = 0; x < bufflen; x++) {
        mpu401_waitwrite(outport);     /* Wait for port ready */
        outp(outport, buff[x]);        /* Send sysex data byte */
      }
      break;
#endif
#ifdef OPL
    case DEV_OPL:
    case DEV_OPL2:
    case DEV_OPL3:
      /* SYSEX is unsupported on OPL output */
      break;
#endif
#ifdef SBAWE
    case DEV_AWE:
      awe32Sysex(channel, (unsigned char far *)buff, bufflen);
      break;
#endif
    case DEV_RS232:
#ifdef MSDOS
      for (x = 0; x < bufflen; x++) {
        rs232_write(outport, buff[x]);      /* Send sysex data byte */
      }
#else
      write(out_fd, buff, bufflen);
#endif
      break;
#ifdef HAVE_PORT_IO
    case DEV_SBMIDI:
      for (x = 0; x < bufflen; x++) {
        dsp_write(outport, 0x38);           /* MIDI output */
        dsp_write(outport, buff[x]);        /* Send sysex data byte */
      }
      break;
#endif
#ifdef MSDOS
    case DEV_GUS:
      for (x = 0; x < bufflen; x++) {
        gus_write(buff[x]);                 /* Send sysex data byte */
      }
#endif
      break;
    case DEV_NONE:
      break;
  }
}
