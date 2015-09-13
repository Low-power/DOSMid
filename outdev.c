/*
 * Wrapper for outputing MIDI commands to different devices.
 *
 * Copyright (c) 2015, Mateusz Viste
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

#include <conio.h> /* outp(), inp() */
#include <dos.h>

#ifdef OPL
#include "opl.h"
#endif

#include "mpu401.h"
#include "rs232.h"
#include "sbdsp.h"

#ifdef SBAWE
#include "awe32/ctaweapi.h"
#endif

#include "outdev.h" /* include self for control */

/* force the compiler to load valid DS segment value before calling
 * the AWE32 API functions (in far data models, where DS is floating) */
#pragma aux __pascal "^" parm loadds reverse routine [] \
                         value struct float struct caller [] \
                         modify [ax bx cx dx es];


static enum outdev_types outdev = DEV_NONE;
static unsigned short outport = 0;


/* inits the out device, also selects the out device, from one of these:
 *  DEV_MPU401
 *  DEV_AWE
 *  DEV_OPL
 *  DEV_RS232
 *  DEV_SBMIDI
 *  DEV_NONE
 *
 * This should be called only ONCE, when program starts.
 * Returns 0 on success, non-zero otherwise. */
int dev_init(enum outdev_types dev, unsigned short port) {
  outdev = dev;
  outport = port;
  switch (outdev) {
    case DEV_MPU401:
      /* reset the MPU401 */
      if (mpu401_rst(outport) != 0) return(-1);
      /* put it into UART mode */
      mpu401_uart(outport);
      break;
    case DEV_AWE:
#ifdef SBAWE
      if (awe32Detect(outport) != 0) return(-1);
      if (awe32InitHardware() != 0) return(-2);
      /* load GM samples from AWE's ROM */
      awe32SoundPad.SPad1 = awe32SPad1Obj;
      awe32SoundPad.SPad2 = awe32SPad2Obj;
      awe32SoundPad.SPad3 = awe32SPad3Obj;
      awe32SoundPad.SPad4 = awe32SPad4Obj;
      awe32SoundPad.SPad5 = awe32SPad5Obj;
      awe32SoundPad.SPad6 = awe32SPad6Obj;
      awe32SoundPad.SPad7 = awe32SPad7Obj;
      if (awe32InitMIDI() != 0) return(-3);
#endif
      break;
    case DEV_OPL:
    case DEV_OPL2:
    case DEV_OPL3:
#ifdef OPL
    {
      int res;
      res = opl_init(outport);
      if (res < 0) return(res);
      /* change the outdev device depending on OPL autodetection */
      if (res == 0) {
        outdev = DEV_OPL2;
      } else {
        outdev = DEV_OPL3;
      }
    }
#endif
      break;
    case DEV_RS232:
      if (rs232_check(outport) != 0) return(-1);
      break;
    case DEV_SBMIDI:
      /* The DSP has to be reset before it is first programmed. The reset
       * causes it to perform an initialization and returns it to its default
       * state. The DSP reset is done through the Reset port. */
      if (dsp_reset(outport) != 0) return(-1);
      dsp_write(outport, 0x30); /* switch the MIDI I/O into polling mode */
      break;
    case DEV_NONE:
      break;
  }
  dev_clear();
  return(0);
}


/* returns the device that has been inited/selected */
enum outdev_types dev_getcurdev(void) {
  return(outdev);
}


/* close/deinitializes the out device */
void dev_close(void) {
  switch (outdev) {
    case DEV_MPU401:
      mpu401_rst(outport); /* resets it to intelligent mode */
      break;
    case DEV_AWE:
#ifdef SBAWE
      /* Creative recommends to disable interrupts during AWE shutdown */
      _disable();
      awe32Terminate();
      _enable();
#endif
      break;
    case DEV_OPL:
    case DEV_OPL2:
    case DEV_OPL3:
#ifdef OPL
      opl_close(outport);
#endif
      break;
    case DEV_RS232:
      break;
    case DEV_SBMIDI:
      /* To terminate UART mode, send a DSP reset command. The reset command
       * behaves differently while the DSP is in MIDI UART mode. It terminates
       * MIDI UART mode and restores all the DSP parameters to the states
       * prior to entering MIDI UART mode. If your application was run in MIDI
       * UART mode, it important that you send the DSP reset command to exit
       * the MIDI UART mode when your application terminates. */
      dsp_reset(outport); /* I don't use MIDI UART mode because it requires */
      break;              /* DSP v2.x, but reseting the chip seems like a   */
    case DEV_NONE:        /* good thing to do anyway                        */
      break;
  }
}


/* clears/reinits the out device (turns all sounds off...). this can be used
 * often (typically: between each song). */
void dev_clear(void) {
  int i;
  switch (outdev) {
    case DEV_MPU401:
    case DEV_AWE:
    case DEV_RS232:
    case DEV_SBMIDI:
      /* iterate on MIDI channels and send messages */
      for (i = 0; i < 16; i++) {
        dev_controller(i, 123, 0);   /* "all notes off" */
        dev_controller(i, 120, 0);   /* "all sounds off" */
        dev_controller(i, 121, 0);   /* "all controllers off" */
      }
      break;
    case DEV_OPL:
    case DEV_OPL2:
    case DEV_OPL3:
#ifdef OPL
      opl_clear(outport);
#endif
      break;
    case DEV_NONE:
      break;
  }
  /* load default GM instruments (even real MIDI synths do not always reset those properly) */
  for (i = 0; i < 16; i++) dev_setprog(i, i);
}


/* activate note on channel */
void dev_noteon(int channel, int note, int velocity) {
  switch (outdev) {
    case DEV_MPU401:
      mpu401_waitwrite(outport);      /* Wait for port ready */
      outp(outport, 0x90 | channel);  /* Send note ON to selected channel */
      mpu401_waitwrite(outport);      /* Wait for port ready */
      outp(outport, note);            /* Send note number to turn ON */
      mpu401_waitwrite(outport);      /* Wait for port ready */
      outp(outport, velocity);        /* Send velocity */
      break;
    case DEV_OPL:
    case DEV_OPL2:
    case DEV_OPL3:
#ifdef OPL
      opl_midi_noteon(outport, channel, note, velocity);
#endif
      break;
    case DEV_AWE:
#ifdef SBAWE
      awe32NoteOn(channel, note, velocity);
#endif
      break;
    case DEV_RS232:
      rs232_write(outport, 0x90 | channel); /* Send note ON to selected channel */
      rs232_write(outport, note);           /* Send note number to turn ON */
      rs232_write(outport, velocity);       /* Send velocity */
      break;
    case DEV_SBMIDI:
      dsp_write(outport, 0x38);             /* MIDI output */
      dsp_write(outport, 0x90 | channel);   /* Send note ON to selected channel */
      dsp_write(outport, 0x38);             /* MIDI output */
      dsp_write(outport, note);             /* Send note number to turn ON */
      dsp_write(outport, 0x38);             /* MIDI output */
      dsp_write(outport, velocity);         /* Send velocity */
      break;
    case DEV_NONE:
      break;
  }
}


/* disable note on channel */
void dev_noteoff(int channel, int note) {
  switch (outdev) {
    case DEV_MPU401:
      mpu401_waitwrite(outport);      /* Wait for port ready */
      outp(outport, 0x80 | channel);  /* Send note OFF code on selected channel */
      mpu401_waitwrite(outport);      /* Wait for port ready */
      outp(outport, note);            /* Send note number to turn OFF */
      mpu401_waitwrite(outport);      /* Wait for port ready */
      outp(outport, 64);              /* Send velocity */
      break;
    case DEV_OPL:
    case DEV_OPL2:
    case DEV_OPL3:
#ifdef OPL
      opl_midi_noteoff(outport, channel, note);
#endif
      break;
    case DEV_AWE:
#ifdef SBAWE
      awe32NoteOff(channel, note, 64);
#endif
      break;
    case DEV_RS232:
      rs232_write(outport, 0x80 | channel); /* 'note off' + channel selector */
      rs232_write(outport, note);           /* note number */
      rs232_write(outport, 64);             /* velocity */
      break;
    case DEV_SBMIDI:
      dsp_write(outport, 0x38);           /* MIDI output */
      dsp_write(outport, 0x80 | channel); /* Send note OFF code on selected channel */
      dsp_write(outport, 0x38);           /* MIDI output */
      dsp_write(outport, note);           /* Send note number to turn OFF */
      dsp_write(outport, 0x38);           /* MIDI output */
      dsp_write(outport, 64);             /* Send velocity */
      break;
    case DEV_NONE:
      break;
  }
}


/* adjust the pitch wheel of a channel */
void dev_pitchwheel(int channel, int wheelvalue) {
  switch (outdev) {
    case DEV_MPU401:
      mpu401_waitwrite(outport);      /* Wait for port ready */
      outp(outport, 0xE0 | channel);  /* Send selected channel */
      mpu401_waitwrite(outport);      /* Wait for port ready */
      outp(outport, wheelvalue & 127);/* Send the lowest (least significant) 7 bits of the wheel value */
      mpu401_waitwrite(outport);      /* Wait for port ready */
      outp(outport, wheelvalue >> 7); /* Send the highest (most significant) 7 bits of the wheel value */
      break;
    case DEV_OPL:
    case DEV_OPL2:
    case DEV_OPL3:
#ifdef OPL
      opl_midi_pitchwheel(outport, channel, wheelvalue);
#endif
      break;
    case DEV_AWE:
#ifdef SBAWE
      awe32PitchBend(channel, wheelvalue & 127, wheelvalue >> 7);
#endif
      break;
    case DEV_RS232:
      rs232_write(outport, 0xE0 | channel);   /* Send selected channel */
      rs232_write(outport, wheelvalue & 127); /* Send the lowest (least significant) 7 bits of the wheel value */
      rs232_write(outport, wheelvalue >> 7);  /* Send the highest (most significant) 7 bits of the wheel value */
      break;
    case DEV_SBMIDI:
      dsp_write(outport, 0x38);             /* MIDI output */
      dsp_write(outport, 0xE0 | channel);   /* Send selected channel */
      dsp_write(outport, 0x38);             /* MIDI output */
      dsp_write(outport, wheelvalue & 127); /* Send the lowest (least significant) 7 bits of the wheel value */
      dsp_write(outport, 0x38);             /* MIDI output */
      dsp_write(outport, wheelvalue >> 7);  /* Send the highest (most significant) 7 bits of the wheel value */
      break;
    case DEV_NONE:
      break;
  }
}


/* send a 'controller' message */
void dev_controller(int channel, int id, int val) {
  switch (outdev) {
    case DEV_MPU401:
      mpu401_waitwrite(outport);      /* Wait for port ready */
      outp(outport, 0xB0 | channel);  /* Send selected channel */
      mpu401_waitwrite(outport);      /* Wait for port ready */
      outp(outport, id);              /* Send the controller's id */
      mpu401_waitwrite(outport);      /* Wait for port ready */
      outp(outport, val);             /* Send controller's value */
      break;
    case DEV_OPL:
    case DEV_OPL2:
    case DEV_OPL3:
#ifdef OPL
      opl_midi_controller(outport, channel, id, val);
#endif
      break;
    case DEV_AWE:
#ifdef SBAWE
      awe32Controller(channel, id, val);
#endif
      break;
    case DEV_RS232:
      rs232_write(outport, 0xB0 | channel);  /* Send selected channel */
      rs232_write(outport, id);              /* Send the controller's id */
      rs232_write(outport, val);             /* Send controller's value */
      break;
    case DEV_SBMIDI:
      dsp_write(outport, 0x38);            /* MIDI output */
      dsp_write(outport, 0xB0 | channel);  /* Send selected channel */
      dsp_write(outport, 0x38);            /* MIDI output */
      dsp_write(outport, id);              /* Send the controller's id */
      dsp_write(outport, 0x38);            /* MIDI output */
      dsp_write(outport, val);             /* Send controller's value */
      break;
    case DEV_NONE:
      break;
  }
}


void dev_chanpressure(int channel, int pressure) {
  switch (outdev) {
    case DEV_MPU401:
      mpu401_waitwrite(outport);      /* Wait for port ready */
      outp(outport, 0xD0 | channel);  /* Send selected channel */
      mpu401_waitwrite(outport);      /* Wait for port ready */
      outp(outport, pressure);        /* Send the pressure value */
      break;
    case DEV_OPL:
    case DEV_OPL2:
    case DEV_OPL3:
#ifdef OPL
      /* nothing to do */
#endif
      break;
    case DEV_AWE:
#ifdef SBAWE
      awe32ChannelPressure(channel, pressure);
#endif
      break;
    case DEV_RS232:
      rs232_write(outport, 0xD0 | channel);  /* Send selected channel */
      rs232_write(outport, pressure);        /* Send the pressure value */
      break;
    case DEV_SBMIDI:
      dsp_write(outport, 0x38);            /* MIDI output */
      dsp_write(outport, 0xD0 | channel);  /* Send selected channel */
      dsp_write(outport, 0x38);            /* MIDI output */
      dsp_write(outport, pressure);        /* Send the pressure value */
      break;
    case DEV_NONE:
      break;
  }
}


void dev_keypressure(int channel, int note, int pressure) {
  switch (outdev) {
    case DEV_MPU401:
      mpu401_waitwrite(outport);      /* Wait for port ready */
      outp(outport, 0xA0 | channel);  /* Send selected channel */
      mpu401_waitwrite(outport);      /* Wait for port ready */
      outp(outport, note);            /* Send the note we target */
      mpu401_waitwrite(outport);      /* Wait for port ready */
      outp(outport, pressure);        /* Send the pressure value */
      break;
    case DEV_OPL:
    case DEV_OPL2:
    case DEV_OPL3:
#ifdef OPL
      /* nothing to do */
#endif
      break;
    case DEV_AWE:
#ifdef SBAWE
      awe32PolyKeyPressure(channel, note, pressure);
#endif
      break;
    case DEV_RS232:
      rs232_write(outport, 0xA0 | channel);  /* Send selected channel */
      rs232_write(outport, note);            /* Send the note we target */
      rs232_write(outport, pressure);        /* Send the pressure value */
      break;
    case DEV_SBMIDI:
      dsp_write(outport, 0x38);            /* MIDI output */
      dsp_write(outport, 0xA0 | channel);  /* Send selected channel */
      dsp_write(outport, 0x38);            /* MIDI output */
      dsp_write(outport, note);            /* Send the note we target */
      dsp_write(outport, 0x38);            /* MIDI output */
      dsp_write(outport, pressure);        /* Send the pressure value */
      break;
    case DEV_NONE:
      break;
  }
}


/* should be called by the application from time to time */
void dev_tick(void) {
  switch (outdev) {
    case DEV_MPU401:
      mpu401_flush(outport);
      break;
    case DEV_OPL:
    case DEV_OPL2:
    case DEV_OPL3:
      break;
    case DEV_AWE:
      break;
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
    case DEV_NONE:
      break;
  }
}


/* sets a "program" (meaning an instrument) on a channel */
void dev_setprog(int channel, int program) {
  switch (outdev) {
    case DEV_MPU401:
      mpu401_waitwrite(outport);     /* Wait for port ready */
      outp(outport, 0xC0 | channel); /* Send channel */
      mpu401_waitwrite(outport);     /* Wait for port ready */
      outp(outport, program);        /* Send patch id */
      break;
    case DEV_OPL:
    case DEV_OPL2:
    case DEV_OPL3:
#ifdef OPL
      opl_midi_changeprog(channel, program);
#endif
      break;
    case DEV_AWE:
#ifdef SBAWE
      awe32ProgramChange(channel, program);
#endif
      break;
    case DEV_RS232:
      rs232_write(outport, 0xC0 | channel); /* Send channel */
      rs232_write(outport, program);        /* Send patch id */
      break;
    case DEV_SBMIDI:
      dsp_write(outport, 0x38);           /* MIDI output */
      dsp_write(outport, 0xC0 | channel); /* Send channel */
      dsp_write(outport, 0x38);           /* MIDI output */
      dsp_write(outport, program);        /* Send patch id */
      break;
    case DEV_NONE:
      break;
  }
}


/* sends a raw sysex string to the device */
void dev_sysex(int channel, char *buff, int bufflen) {
  int x;
  switch (outdev) {
    case DEV_MPU401:
      mpu401_waitwrite(outport);     /* Wait for port ready */
      outp(outport, 0xF0 | channel); /* Send channel */
      for (x = 0; x < bufflen; x++) {
        mpu401_waitwrite(outport);     /* Wait for port ready */
        outp(outport, buff[x]);        /* Send sysex data byte */
      }
      break;
    case DEV_OPL:
    case DEV_OPL2:
    case DEV_OPL3:
#ifdef OPL
      /* SYSEX is unsupported on OPL output */
#endif
      break;
    case DEV_AWE:
#ifdef SBAWE
      awe32Sysex(channel, buff, bufflen);
#endif
      break;
    case DEV_RS232:
      rs232_write(outport, 0xF0 | channel); /* Send channel */
      for (x = 0; x < bufflen; x++) {
        rs232_write(outport, buff[x]);        /* Send sysex data byte */
      }
      break;
    case DEV_SBMIDI:
      dsp_write(outport, 0x38);           /* MIDI output */
      dsp_write(outport, 0xF0 | channel); /* Send channel */
      for (x = 0; x < bufflen; x++) {
        dsp_write(outport, 0x38);           /* MIDI output */
        dsp_write(outport, buff[x]);        /* Send sysex data byte */
      }
      break;
    case DEV_NONE:
      break;
  }
}
