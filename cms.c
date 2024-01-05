/*
 * Creative Music System device output support for DOSMid
 * Copyright 2021 Tronix
 * Copyright 2024 Rivoreo
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

#ifdef CMS

//#define CMS_DEBUG		// verbose to CMSLOG debug file
//#define DRUMS_ONLY		// for percussion debug purpouses

#include "cms.h"
#ifdef CMSLPT
#include "lpt.h"
#endif
#include <conio.h>
#include <stddef.h>
#include <assert.h>
#ifdef CMS_DEBUG
#include <stdio.h>
#include <stdarg.h>
#endif

#ifdef CMS_DEBUG
static void debug_log(const char *fmt, ...)
{
   FILE *log_f;
   va_list ap;

	log_f = fopen("cmslog", "a");
	if(!log_f) return;
	va_start(ap, fmt);
	vfprintf(log_f, fmt, ap);
	va_end(ap);
	fclose(log_f);
}
#endif

struct mid_channel {
	unsigned char note;
	unsigned char priority;
	unsigned char ch;
	unsigned char voice;
	unsigned char velocity;
};

static struct mid_channel cms_synth[MAX_CMS_CHANNELS];

static const unsigned short freqtable[128] = {
    8,    9,    9,   10,   10,   11,   12,   12,   13,   14,   15,   15,
   16,   17,   18,   19,   21,   22,   23,   24,   26,   28,   29,   31,
   33,   35,   37,   39,   41,   44,   46,   49,   52,   55,   58,   62,
   65,   69,   73,   78,   82,   87,   92,   98,  104,  110,  117,  123,
  131,  139,  147,  156,  165,  175,  185,  196,  208,  220,  233,  247,
  262,  277,  294,  311,  330,  349,  370,  392,  415,  440,  466,  494,
  523,  554,  587,  622,  659,  698,  740,  784,  831,  880,  932,  988,
 1047, 1109, 1175, 1245, 1319, 1397, 1480, 1568, 1661, 1760, 1865, 1976,
 2093, 2217, 2349, 2489, 2637, 2794, 2960, 3136, 3322, 3520, 3729, 3951,
 4186, 4435, 4699, 4978, 5274, 5588, 5920, 6272, 6645, 7040, 7459, 7902,
 8372, 8870, 9397, 9956,10548,11175,11840,12544};

static const unsigned short pitchtable[256] = {                    /* pitch wheel */
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

static const unsigned char CMSFreqMap[128] = {
		0  ,  3,  7, 11, 15, 19, 23, 27,
		31 , 34, 38, 41, 45, 48, 51, 55,
		58 , 61, 64, 66, 69, 72, 75, 77,
		80 , 83, 86, 88, 91, 94, 96, 99,
		102,104,107,109,112,114,116,119,
		121,123,125,128,130,132,134,136,
		138,141,143,145,147,149,151,153,
		155,157,159,161,162,164,166,168,
		170,172,174,175,177,179,181,182,
		184,186,188,189,191,193,194,196,
		197,199,200,202,203,205,206,208,
		209,210,212,213,214,216,217,218,
		219,221,222,223,225,226,227,228,
		229,231,232,233,234,235,236,237,
		239,240,241,242,243,244,245,246,
		247,249,250,251,252,253,254,255
	};

// CMS I/O port address
static unsigned short int cms_port;
#ifdef CMSLPT
static int is_cmslpt;
#endif

#if 0
// The 12 note-within-an-octave values for the SAA1099, starting at B
static unsigned char noteAdr[] = {5, 32, 60, 85, 110, 132, 153, 173, 192, 210, 227, 243};
#endif

// Volume
static const unsigned char atten[128] = {
                 0,1,1,1,1,1,1,1,2,2,2,2,2,2,2,2,
                 3,3,3,3,3,3,3,3,4,4,4,4,4,4,4,4,
                 5,5,5,5,5,5,5,5,6,6,6,6,6,6,6,6,
                 7,7,7,7,7,7,7,7,8,8,8,8,8,8,8,8,
                 9,9,9,9,9,9,9,9,10,10,10,10,10,10,10,10,
                 11,11,11,11,11,11,11,11,12,12,12,12,12,12,12,12,
                 13,13,13,13,13,13,13,13,14,14,14,14,14,14,14,14,
        	 15,15,15,15,15,15,15,15,15,15,15,15,15,15,15,15,
        };

// Logic channel - first chip/second chip
static const unsigned char ChanReg[12] =  {000,001,002,003,004,005,000,001,002,003,004,005};

// Set octave command
static const unsigned char OctavReg[12] = {0x10,0x10,0x11,0x11,0x12,0x12,0x10,0x10,0x11,0x11,0x12,0x12};

static unsigned char octave_store[12];
static unsigned char voice_enable[2];
static unsigned char note_priority;
static unsigned short int channel_pitch[16];
static unsigned char channel_volume[16];
static signed char pan[16];

static void write_cms(unsigned char chip_i, unsigned char reg, unsigned char value) {
#ifdef CMS_DEBUG
	debug_log("function: write_cms(%hhu, 0x%hhx, 0x%hhx)\n", chip_i, reg, value);
#endif
	assert(chip_i == 0 || chip_i == 1);
#ifdef CMSLPT
	if(is_cmslpt) {
		unsigned int ctrl = chip_i ? 6 : 12;
		write_lpt(cms_port, reg, ctrl);
		write_lpt(cms_port, value, ctrl | 1);
	} else
#endif
	{
		unsigned int port = cms_port + (chip_i ? 2 : 0);
		outp(port + 1, reg);	/* Select register */
		outp(port, value);	/* Set value of the register */
	}
}

#if 1
static void __declspec(naked) asm_write_cms() {
	__asm {
		push	ax
		push	dx
		push	bx
		xchg	ax, dx
		xor	bx, bx
		mov	bl, dl
		mov	dl, dh
		xor	dh, dh
		cmp	ax, 0
		je	first
		dec	ax
	first:
		call	write_cms
		pop	bx
		pop	dx
		pop	ax
		ret
	}
}
#endif

void cms_reset(unsigned short int port, int is_on_lpt)
{
   int i;
   cms_port = port;
#ifdef CMSLPT
   is_cmslpt = is_on_lpt;
#endif
	for(i = 0; i < 2; i++) {
		int j;
		for(j = 0; j < 32; j++) write_cms(i, j, 0);
		write_cms(i, 0x1c, 0x2);
		write_cms(i, 0x1c, 0x1);
	}
	for (i=0; i<12; i++) octave_store[i] = 0;
	voice_enable[0] = 0;
	voice_enable[1] = 0;
	for (i=0; i<MAX_CMS_CHANNELS; i++) {
		struct mid_channel *mch = cms_synth + i;
		mch->note = 0;
		mch->priority = 0;
		mch->ch = 0;
		mch->voice = 0;
		mch->velocity = 0;
	}
	for (i=0; i<16; i++) {
		channel_pitch[i] = 8192;
		channel_volume[i] = 127;
		pan[i] = 0;
	}
	note_priority = 0;
}

#if 1
static void cmsDisableVoice(unsigned char voice)
{
	if (voice > 5) {
		voice_enable[1] &= ~(1 << ChanReg[voice]);
		write_cms(1, 0x14, voice_enable[1]);
	} else {
		voice_enable[0] &= ~(1 << ChanReg[voice]);
		write_cms(0, 0x14, voice_enable[0]);
	}
}
#endif

#if 0
static void cmsSound(unsigned char voice, unsigned char freq, unsigned char octave, unsigned char amplitudeLeft, unsigned char amplitudeRight)
{
	if (voice > 5) {
		if (ChanReg[voice]&0x1 == 0) {
			octave_store[OctavReg[voice]-0x10+3] = (octave_store[OctavReg[voice]-0x10+3] & 0xF0) | octave;
		} else {
			octave_store[OctavReg[voice]-0x10+3] = ((octave_store[OctavReg[voice]-0x10+3] & 0xF) << 4) | octave;
		}
		write_cms(1, OctavReg[voice], octave_store[OctavReg[voice]-0x10+3]);
		write_cms(1, ChanReg[voice], (amplitudeLeft << 4) | amplitudeRight);
		write_cms(1, ChanReg[voice] | 0x8, freq);
		voice_enable[1] |= 1 << ChanReg[voice];
		write_cms(1, 0x14, voice_enable[1]);
	} else {
		if (ChanReg[voice]&0x1 == 0) {
			octave_store[OctavReg[voice]-0x10] = (octave_store[OctavReg[voice]-0x10] & 0xF0) | octave;
		} else {
			octave_store[OctavReg[voice]-0x10] = ((octave_store[OctavReg[voice]-0x10] & 0xF) << 4) | octave;
		}
		write_cms(0, OctavReg[voice], octave_store[OctavReg[voice]-0x10]);
		write_cms(0, ChanReg[voice], (amplitudeLeft << 4) | amplitudeRight);
		write_cms(0, ChanReg[voice] | 0x8, freq);
		voice_enable[0] |= 1 << ChanReg[voice];
		write_cms(0, 0x14, voice_enable[0]);
	}
}
#endif

#if 0
static void cmsDisableVoice(unsigned char voice)
{
_asm
   {
		xor   bh,bh
		mov   bl,voice

		mov   dx,cms_port	// FIXME !!!

		mov   bl,ChanReg[bx]	; bl = true channel (0 - 5)

		xor   di,di
		mov   cl,voice
		cmp   cl,06h
                jl    skip_inc
		inc   di
                add   dx,2
skip_inc:
		mov   al,14h
		inc   dx
		out   dx,al
		dec   dx

		mov   al,voice_enable[di]
		mov   ah,01h
		mov   cl,bl
		shl   ah,cl
		not   ah
		and   al,ah		; al = voice enable reg

		out   dx,al
		mov   voice_enable[di],al
    }
}
#endif

static void cms_set_volume(unsigned char voice, unsigned char left_amplitude, unsigned char right_amplitude) {
	__asm {
		xor	bh,bh
		mov	bl, voice
		xor	dx, dx
		cmp	bl, 6			; check channel num > 5?
		jl	skip_inc		; yes - set port = port + 2
		add	dx, 2
	skip_inc:
		mov	bl, ChanReg[bx]		; bx = true channel (0 - 5)
		mov	al, byte ptr left_amplitude
		mov	ah, byte ptr right_amplitude
		mov	cl, 4
		shl	ah, cl
		or	al, ah
		mov	ah, bl
		call	asm_write_cms
	}
}

#if 1
static void cmsSound(unsigned char voice, unsigned char freq, unsigned char octave, unsigned char amplitudeLeft, unsigned char amplitudeRight)
{
_asm
   {
		xor   bh,bh
		mov   bl,voice

		xor	dx, dx
		cmp   bl,06h		; check channel num > 5?
		jl    setOctave	; yes - set port = port + 2
		add   dx,2
setOctave:
		mov   bl,ChanReg[bx]	; bx = true channel (0 - 5) //FIXME swap down
		mov   ah,OctavReg[bx]   ; ah = Set octave command   //FIXME swap up
;
;	ah now = register
;		0,1,6,7=$10
;		2,3,8,9=$11
;		4,5,10,11=$12
;
;	CMS octave regs are write only, so we have to track
;	the values in adjoining voices manually

		mov   al,ah
		xor   ah,ah		; ax = set octave cmd (10h - 12h)
		mov   di,ax		; di = ax
		sub   di,010h		; di = octave cmd - 10h (0..2 index)
		mov   cl,voice
		cmp   cl,06h
                jl    skip_inc
		add   di,3
skip_inc:
		mov   ah,al		; set ah back to octave cmd

		mov   al,byte ptr octave_store[di]
		mov   bh,octave
		test  bl,01h
		jnz   shiftOctave
		and   al,0F0h
		jmp   outOctave
shiftOctave:
		and   al,0Fh
		mov   cl,4
		shl   bh,cl
outOctave:
		or    al,bh
		mov   byte ptr octave_store[di],al
		call	asm_write_cms	; set octave to CMS
setAmp:
		mov   al,byte ptr amplitudeLeft
		mov   ah,byte ptr amplitudeRight
		;and   al,0Fh
		mov   cl,4
		shl   ah,cl
		or    al,ah
		mov   ah,bl
		call	asm_write_cms
setFreq:
		mov   al,byte ptr freq
		or    ah,08h


		call	asm_write_cms
voiceEnable:
		xor   di,di
		mov   cl,voice
		cmp   cl,06h
                jl    skip_inc2
		inc   di
skip_inc2:
		mov   cl,bl
		mov   bl,voice_enable[di]
		mov   bh,01h
		shl   bh,cl
		or    bl,bh
		mov   voice_enable[di],bl
		mov	ax, dx
		mov	dx, 14h
		cmp	ax, 0
		je	first
		dec	ax
first:
		call	write_cms
    }
}
#endif

static int scale_velocity(int velocity, unsigned char volume, signed char pan) {
	if(volume < 127) velocity = velocity * volume / 127;
	if(!pan) return velocity;
	velocity += velocity * pan / 47;
	if(velocity < 0) return 0;
	if(velocity > 127) return 127;
	return velocity;
}

// ****
// High-level CMS synth procedures
// ****

void cms_pitchwheel(unsigned short oplport, int channel, int pitchwheel)
{
  int i;
  unsigned short notefreq;
  unsigned char note;
  int pitch;
  unsigned char octave;

  channel_pitch[channel] = pitchwheel;
  for(i=0; i<MAX_CMS_CHANNELS; i++) {
    const struct mid_channel *mch = cms_synth + i;
    if (mch->ch == channel && mch->note != 0) {
         note = cms_synth[i].note;

         pitch = pitchwheel;
         if (pitch != 0) {
           if (pitch > 127) {
              pitch = 127;
           } else if (pitch < -128) {
              pitch = -128;
           }
         }

        notefreq = ((unsigned long int)freqtable[note] * pitchtable[pitch + 128]) >> 15;

	if (notefreq > 31 && notefreq < 7824) {
		int left_velocity, right_velocity;

		octave = 4;
		while (notefreq < 489) {
			notefreq=notefreq * 2;
			octave--;
		}
		while (notefreq > 977) {
			notefreq=notefreq / 2;
			octave++;
		}

#ifndef DRUMS_ONLY
		left_velocity = scale_velocity(mch->velocity, channel_volume[channel], -pan[channel]);
		right_velocity = scale_velocity(mch->velocity, channel_volume[channel], pan[channel]);
		cmsSound(mch->voice, CMSFreqMap[((notefreq-489)*128) / 489], octave, atten[left_velocity], atten[right_velocity]); 
#endif

        }
    }
  }
}

void cms_noteoff(unsigned char channel, unsigned char note)
{
	int i, j;

	if (channel == 9) {
#ifdef CMS_DEBUG
		debug_log("DRUM OFF note %u\n", note);
#endif
		write_cms(1, 0x19, 0x0);
		write_cms(1, 0x15, 0x0); // noise ch 11
		cmsDisableVoice(11);
	} else {
		for(i=0; i<MAX_CMS_CHANNELS; i++) {
			struct mid_channel *mch = cms_synth + i;
			if(mch->note != note) continue;
			if(mch->ch != channel) continue;

			// decrease priority for all notes greater than current
			for (j=0; j<MAX_CMS_CHANNELS; j++) {
				if (cms_synth[j].priority <= mch->priority) continue;
				cms_synth[j].priority--;
			}

			if (note_priority != 0) note_priority--;

			cmsDisableVoice(i);
			mch->note = 0;
			mch->priority = 0;
			mch->ch = 0;
			mch->voice = 0;
			mch->velocity = 0;
			return;
		}

		// Note not found, ignore note off command
#ifdef CMS_DEBUG
		debug_log("note %u channel %u not found\n", (unsigned int)channel, (unsigned int)note);
#endif
	}
}

void cms_noteon(unsigned char channel, unsigned char note, unsigned char velocity)
{
  int left_velocity, right_velocity;
  int i;
  unsigned char voice;
  int pitch;

  if(velocity) {
    left_velocity = scale_velocity(velocity, channel_volume[channel], -pan[channel]);
    right_velocity = scale_velocity(velocity, channel_volume[channel], pan[channel]);
  }

  if (channel == 9)
  {
    if (velocity != 0)
       {
#ifdef CMS_DEBUG
       debug_log("DRUM ON note= %u\n",note);
#endif
	write_cms(1, 0x19, 0x84); // single dacay
	switch (note) {
		case 37: // Side Stick
			write_cms(1, 0x16, 0x00); // noise gen 1 31.3kHz
			write_cms(1, 0x15, 0x20); // noise ch 11
			cmsSound(11, 0, 2, atten[left_velocity], atten[right_velocity]);
			break;
		case 38: // Acoustic Snare
			write_cms(1, 0x16, 0x00); // noise gen 1 31.3kHz
			write_cms(1, 0x15, 0x20); // noise ch 11
			cmsSound(11, 0, 0, atten[left_velocity], atten[right_velocity]);
			break;
		case 39: // Hand Clap
			write_cms(1, 0x16, 0x10); // noise gen 1 15.6kHz
			write_cms(1, 0x15, 0x20); // noise ch 11
			cmsSound(11, 0, 3, atten[left_velocity], atten[right_velocity]);
			break;
		case 40: // Electric Snare
			write_cms(1, 0x16, 0x10); // noise gen 1 15.6kHz
			write_cms(1, 0x15, 0x20); // noise ch 11
			cmsSound(11, 0, 1, atten[left_velocity], atten[right_velocity]);
			break;
		case 42: // Closed Hi Hat
			write_cms(1, 0x16, 0x10); // noise gen 1 15.6kHz
			write_cms(1, 0x15, 0x20); // noise ch 11
			cmsSound(11, 0, 2, atten[left_velocity], atten[right_velocity]);
			break;
		case 44: // Pedal Hi-Hat
			write_cms(1, 0x16, 0x10); // noise gen 1 15.6kHz
			write_cms(1, 0x15, 0x20); // noise ch 11
			cmsSound(11, 0, 0, atten[left_velocity], atten[right_velocity]);
			break;
		case 52: // Chinese Cymbal
			write_cms(1, 0x16, 0x20); // noise gen 1 7.6kHz
			write_cms(1, 0x15, 0x20); // noise ch 11
			cmsSound(11, 0, 2, atten[left_velocity], atten[right_velocity]);
			break;
		case 55: // Splash Cymbal
			write_cms(1, 0x16, 0x20); // noise gen 1 7.6kHz
			write_cms(1, 0x15, 0x20); // noise ch 11
			cmsSound(11, 0, 0, atten[left_velocity], atten[right_velocity]);
			break;
		case 71: // Short Whistle
		case 72: // Long Whistle
			cmsSound(11, 0, 6, atten[left_velocity], atten[right_velocity]);
			break;
		default:
			cmsSound(11, 0, 1, atten[left_velocity], atten[right_velocity]);
        }
       }
    else
       {
	write_cms(1, 0x19, 0x0);
	write_cms(1, 0x15, 0x0); // noise ch 11
	cmsDisableVoice(11);
       }
  } else if (velocity != 0) {
/*
	unsigned char note_cms = note+1;
	unsigned char octave = (note_cms / 12) - 1; //Some fancy math to get the correct octave
	unsigned char noteVal = note_cms - ((octave + 1) * 12); //More fancy math to get the correct note
*/
	struct mid_channel *mch = NULL;
	unsigned int notefreq;

	note_priority++;

        voice = MAX_CMS_CHANNELS;
	for(i=0; i<MAX_CMS_CHANNELS; i++) {
      		if(cms_synth[i].note == 0) {
        		voice = i;
			mch = cms_synth + i;
        		break;
      		}
    	}


  	// We run out of voices, find low priority voice
  	if(!mch) {
		unsigned char min_prior = cms_synth[0].priority;

#ifdef CMS_DEBUG
		debug_log("out of voice. note priority %u\n", note_priority);
		for (i=0; i<MAX_CMS_CHANNELS; i++) debug_log("%u ", cms_synth[i].priority);
		debug_log("\n");
#endif
		// find note with min prioryty
		mch = cms_synth;
		voice = 0;
		for (i=1; i<MAX_CMS_CHANNELS; i++) {
			if (cms_synth[i].priority >= min_prior) continue;
			mch = cms_synth + i;
			voice = i;
			min_prior = mch->priority;
		}

		// decrease all notes priority by one
		for (i=0; i<MAX_CMS_CHANNELS; i++) {
			if (cms_synth[i].priority != 0) cms_synth[i].priority--;
		}

		// decrease current priority
		if (note_priority != 0) note_priority--;

#ifdef CMS_DEBUG
		debug_log("find low priority voice %u, note priority %u\n", voice, note_priority);
		for (i=0; i<MAX_CMS_CHANNELS; i++) debug_log("%u ", cms_synth[i].priority);
		debug_log("\n");
#endif
  	}

        pitch = channel_pitch[channel];
        if (pitch != 0) {
           if (pitch > 127) {
              pitch = 127;
           } else if (pitch < -128) {
              pitch = -128;
           }
        }

        notefreq = ((unsigned long int)freqtable[note] * pitchtable[pitch + 128]) >> 15;
	if (notefreq > 31 && notefreq < 7824) {
		unsigned char octave = 4;
		while (notefreq < 489) {
			notefreq = notefreq * 2;
			octave--;
		}
		while (notefreq > 977) {
			notefreq = notefreq / 2;
			octave++;
		}

#ifndef DRUMS_ONLY
		cmsSound(voice, CMSFreqMap[((notefreq-489)*128) / 489], octave, atten[left_velocity], atten[right_velocity]); 
#endif

		mch->note = note;
		mch->priority = note_priority;
		mch->velocity = velocity;
        	mch->ch = channel;
        	mch->voice = voice;
	}

  } else {
        cms_noteoff(channel,note);
  }
}

#if 0
void cms_tick(void)
{
}
#endif

void cms_controller(unsigned char channel, unsigned char id, unsigned char val)
{
	int i;
	switch(id) {
		case 7:
			// Volume
			if(val > 127) val = 127;
			channel_volume[channel] = val;
			for(i=0; i<MAX_CMS_CHANNELS; i++) {
				int left_velocity, right_velocity;
				const struct mid_channel *mch = cms_synth + i;
				if(!mch->note) continue;
				if(mch->ch != channel) continue;
				left_velocity = scale_velocity(mch->velocity, val, -pan[channel]);
				right_velocity = scale_velocity(mch->velocity, val, pan[channel]);
				cms_set_volume(i, left_velocity, right_velocity);
			}
			break;
		case 10:
			// Pan
			if(val > 127) val = 127;
			pan[channel] = (signed char)val - 64;
			break;
		case 121:
			// Reset controllers
			for (i=0; i<16; i++) {
				channel_pitch[i] = 8192;
				channel_volume[i] = 127;
				pan[i] = 0;
			}
			// Fallthrough
		case 120:
		case 123:
			// All notes off
			for (i=0; i<MAX_CMS_CHANNELS; i++) {
				struct mid_channel *mch = cms_synth + i;
				if (mch->note != 0) {
					cmsDisableVoice(i);
					mch->note = 0;
					mch->priority = 0;
					mch->ch = 0;
					mch->voice = 0;
					mch->velocity = 0;
				}
			}
			break;
	}
}

#endif
