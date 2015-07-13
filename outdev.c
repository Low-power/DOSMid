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

#include "mpu401.h"
#include "awe32/ctaweapi.h"

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
 *  DEV_OPL2
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
      if (awe32Detect(port) != 0) return(-1);
      if (awe32InitHardware() != 0) return(-2);
      /* load GM samples from AWE's ROM */
      awe32SoundPad.SPad1 = awe32SPad1Obj;
      awe32SoundPad.SPad2 = awe32SPad2Obj;
      awe32SoundPad.SPad3 = awe32SPad3Obj;
      awe32SoundPad.SPad4 = awe32SPad4Obj;
      awe32SoundPad.SPad5 = awe32SPad5Obj;
      awe32SoundPad.SPad6 = awe32SPad6Obj;
      awe32SoundPad.SPad7 = awe32SPad7Obj;
      /* make use of all 32 oscillators on the EMU chip (possible as long as I don't want to use DRAM sound fonts) */
      awe32NumG = 32;
      if (awe32InitMIDI() != 0) return(-3);
      break;
    case DEV_OPL2:
      break;
    case DEV_NONE:
      break;
  }
  dev_clear();
  return(0);
}


/* close/deinitializes the out device */
void dev_close(void) {
  switch (outdev) {
    case DEV_MPU401:
      mpu401_rst(outport); /* resets it to intelligent mode */
      break;
    case DEV_AWE:
      /* Creative recommends to disable interrupts during AWE shutdown */
      _disable();
      awe32Terminate();
      _enable();
      break;
    case DEV_OPL2:
      break;
    case DEV_NONE:
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
      /* iterate on MIDI channels and send messages */
      for (i = 0; i < 16; i++) {
        dev_controller(i, 123, 0);   /* "all notes off" */
        dev_controller(i, 120, 0);   /* "all sounds off" */
        dev_controller(i, 121, 0);   /* "all controllers off" */
      }
      break;
    case DEV_OPL2:
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
      outp(outport, 0x90 | channel);  /* Send note ON code on selected channel */
      mpu401_waitwrite(outport);      /* Wait for port ready */
      outp(outport, note);            /* Send note Number to turn ON */
      mpu401_waitwrite(outport);      /* Wait for port ready */
      outp(outport, velocity);        /* Send velocity */
      break;
    case DEV_OPL2:
      break;
    case DEV_AWE:
      awe32NoteOn(channel, note, velocity);
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
    case DEV_OPL2:
      break;
    case DEV_AWE:
      awe32NoteOff(channel, note, 64);
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
    case DEV_OPL2:
      break;
    case DEV_AWE:
      awe32PitchBend(channel, wheelvalue & 127, wheelvalue >> 7);
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
    case DEV_OPL2:
      break;
    case DEV_AWE:
      awe32Controller(channel, id, val);
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
    case DEV_OPL2:
      break;
    case DEV_AWE:
      awe32ChannelPressure(channel, pressure);
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
    case DEV_OPL2:
      break;
    case DEV_AWE:
      awe32PolyKeyPressure(channel, note, pressure);
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
    case DEV_OPL2:
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
    case DEV_OPL2:
      break;
    case DEV_AWE:
      awe32ProgramChange(channel, program);
      break;
    case DEV_NONE:
      break;
  }
}