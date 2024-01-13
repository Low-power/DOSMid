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

#ifndef opl_h_sentinel
#define opl_h_sentinel

struct timbre {
  unsigned long int modulator_E862, carrier_E862;
  unsigned char modulator_40, carrier_40;
  unsigned char feedconn;
  signed char finetune;
};

#define OPL_SKIP_CHECKING (1 << 0)
#define OPL_ON_LPT (1 << 1)
#define OPL_PORT_IS_FD (1 << 2)

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
int opl_init(unsigned int port, int *gen, int flags);

/* close OPL device */
void opl_close(void);

/* turns off all notes */
void opl_clear(void);

/* turn note 'on', on emulated MIDI channel */
void opl_midi_noteon(int channel, int note, int velocity);

/* turn note 'off', on emulated MIDI channel */
void opl_midi_noteoff(int channel, int note);

/* adjust the pitch wheel on emulated MIDI channel */
void opl_midi_pitchwheel(int channel, int wheelvalue);

/* emulate MIDI 'controller' messages on the OPL */
void opl_midi_controller(int channel, int id, int value);

/* assign a new instrument to emulated MIDI channel */
void opl_midi_changeprog(int channel, int program);

//void opl_loadinstrument(unsigned short int voice, const struct timbre *timbre, unsigned char channelenable);

/* loads an IBK bank from file into an array of 128 'struct timbre' objects.
 * returns 0 on success, non-zero otherwise */
int opl_loadbank(char *file);

/* the functions below are not necessarily useful - if you want to play MIDI,
 * you will probably prefer to use the opl_midi_* emulation layer instead of
 * dealing with OPL's voices directly - in any case, do NOT mix calls to MIDI
 * emulation functions (opl_midi_*) with any of the functions below! */

/* turn off note on selected voice */
void opl_noteoff(unsigned short int voice);

/* turn on note on selected voice */
void opl_noteon(unsigned short int voice, unsigned int note, int pitch);

#endif
