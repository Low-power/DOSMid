/****************************************************************************\
*                                                                            *
* CTAWEAPI.H SB AWE32 DOS API header                                         *
*                                                                            *
* (C) Copyright Creative Technology Ltd. 1992-94. All rights reserved        *
* worldwide.                                                                 *
*                                                                            *
* THIS CODE AND INFORMATION IS PROVIDED "AS IS" WITHOUT WARRANTY OF ANY      *
* KIND, EITHER EXPRESSED OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE        *
* IMPLIED WARRANTIES OF MERCHANTABILITY AND/OR FITNESS FOR A PARTICULAR      *
* PURPOSE.                                                                   *
*                                                                            *
* You have a royalty-free right to use, modify, reproduce and                *
* distribute the Sample Files (and/or any modified version) in               *
* any way you find useful, provided that you agree to                        *
* the Creative's Software Licensing Aggreement and you also agree that       *
* Creative has no warranty obligations or liability for any Sample Files.    *
*                                                                            *
\****************************************************************************/

/****************************************************************************\
*      File name       : CTAWEAPI.H                                          *
*                                                                            *
*      Programmer      : Creative SB AWE32 Team                              *
*                        Creative Technology Ltd, 1994. All rights reserved. *
*                                                                            *
*      Version         : 2.1b                                                *
*                                                                            *
\****************************************************************************/

#ifndef _CTAWEAPI
#define _CTAWEAPI


#pragma pack(1)


#define MAXBANKS            64      /* maximum number of banks */
#define MAXNRPN             32      /* maximum number of NRPN */


#if defined(__FLAT__) || defined(__HIGHC__) || defined(DOS386) || defined(__NDPC__)
    #if defined(FAROBJ) && !defined(__NDPC__)
        #ifdef __HIGHC__
            #define _FAR_   _Far
        #else
            #define _FAR_   _far
        #endif
    #else
        #define _FAR_
    #endif
    #define PACKETSIZE      8192        /* packet size for 32bit libraries */
#else
    #define PACKETSIZE      512         /* packet size for real mode libraries */
#endif


#if defined(__NDPC__)
    #define _PASCAL_
#endif


#if defined(__HIGHC__)
    #pragma Push_align_members(1)
    #pragma Global_aliasing_convention("_%r")
    #define _PASCAL_    _DCC((_DEFAULT_CALLING_CONVENTION|_CALLEE_POPS_STACK) & \
                             ~ (_REVERSE_PARMS|_OVERLOADED))
#endif


#ifdef __WATCOMC__
    /* pragma to tell the compiler to put all data elements into CODE segment. */
    #ifdef _DATA_IN_CODE
        #pragma data_seg("_CODE", "");
    #endif
#endif


typedef unsigned char           BYTE;
typedef unsigned short          WORD;
typedef unsigned long           DWORD;

#ifndef _FAR_
#define _FAR_                   _far
#endif

#ifndef _PASCAL_
#define _PASCAL_                _pascal
#endif


#if defined(__cplusplus)
extern "C" {
#endif


/* Start of modules */
extern int* __midieng_code(void);
extern int* __sysex_code(void);
extern int* __sfhelp1_code(void);
extern int* __sfhelp2_code(void);
extern int* __sbkload_code(void);
extern int* __wavload_code(void);
extern int* __hardware_code(void);
extern int* __nrpn_code(void);
extern int __midivar_data;
extern int __nrpnvar_data;
extern int __embed_data;

#ifdef FAROBJ
typedef char SCRATCH[676];
#else
typedef char SCRATCH[548];
#endif
typedef char SOUNDFONT[134];
typedef char GCHANNEL[20];
typedef char MIDICHANNEL[28];
typedef char NRPNCHANNEL[90];

typedef struct {
    WORD bank_no;               /* Slot number being used */
    WORD total_banks;           /* Total number of banks */
    long _FAR_* banksizes;      /* Pointer to a list of bank sizes */
    long file_size;             /* Unused */
    char _FAR_* data;           /* Address of buffer of size >= PACKETSIZE */
    char _FAR_* presets;        /* Allocated memory for preset data */

    long total_patch_ram;       /* Total patch ram available */
    WORD no_sample_packets;     /* Number of packets of sound sample to stream */
    long sample_seek;           /* Start file location of sound sample */
    long preset_seek;           /* Address of preset_seek location */
    long preset_read_size;      /* Number of bytes from preset_seek to allocate and read */
    long preset_size;           /* Preset actual size */
} SOUND_PACKET;

typedef struct {
    WORD tag;                   /* Must be 0x100 or 0x101 */
    WORD preset_size;           /* Preset table of this size is required */
    WORD no_wave_packets;       /* Number of packets of Wave sample to stream. */
    long reserved;

    WORD bank_no;               /* Bank number */
    char _FAR_* data;           /* Address of packet of size PACKETSIZE */
    char _FAR_* presets;        /* Allocated memory for preset data */
    long sample_size;           /* Sample size, i.e. number of samples */
    long samples_per_sec;       /* Samples per second */
    WORD bits_per_sample;       /* Bits per sample, 8 or 16 */
    WORD no_channels;           /* Number of channels, 1=mono, 2=stereo */
    WORD looping;               /* Looping? 0=no, 1=yes */
    long startloop;             /* If looping, then these are the addresses */
    long endloop;
    WORD release;               /* Release time, 0=24ms, 8191=23.78s */
} WAVE_PACKET;

typedef struct {
    BYTE _FAR_* SPad1;
    BYTE _FAR_* SPad2;
    BYTE _FAR_* SPad3;
    BYTE _FAR_* SPad4;
    BYTE _FAR_* SPad5;
    BYTE _FAR_* SPad6;
    BYTE _FAR_* SPad7;
} SOUNDPAD;

/* AWE32 variables */
extern WORD         awe32NumG;
extern WORD         awe32BaseAddx[3];
extern DWORD        awe32DramSize;

/* MIDI variables */
extern SCRATCH      awe32Scratch;
extern SOUNDFONT    awe32SFont[4];
extern GCHANNEL     awe32GChannel[32];
extern MIDICHANNEL  awe32MIDIChannel[16];
extern SOUNDPAD     awe32SoundPad;

/* NRPN variables */
extern NRPNCHANNEL  awe32NRPNChannel[16];

/* SoundFont objects */
extern BYTE awe32SPad1Obj[];
extern BYTE awe32SPad2Obj[];
extern BYTE awe32SPad3Obj[];
extern BYTE awe32SPad4Obj[];
extern BYTE awe32SPad5Obj[];
extern BYTE awe32SPad6Obj[];
extern BYTE awe32SPad7Obj[];

/* AWE register functions */
extern void _PASCAL_ awe32RegW(WORD, WORD);
extern WORD _PASCAL_ awe32RegRW(WORD);
extern void _PASCAL_ awe32RegDW(WORD, DWORD);
extern DWORD _PASCAL_ awe32RegRDW(WORD);

/* MIDI support functions */
extern WORD _PASCAL_ awe32InitMIDI(void);
extern WORD _PASCAL_ awe32NoteOn(WORD, WORD, WORD);
extern WORD _PASCAL_ awe32NoteOff(WORD, WORD, WORD);
extern WORD _PASCAL_ awe32ProgramChange(WORD, WORD);
extern WORD _PASCAL_ awe32Controller(WORD, WORD, WORD);
extern WORD _PASCAL_ awe32PolyKeyPressure(WORD, WORD, WORD);
extern WORD _PASCAL_ awe32ChannelPressure(WORD, WORD);
extern WORD _PASCAL_ awe32PitchBend(WORD, WORD, WORD);
extern WORD _PASCAL_ awe32Sysex(WORD, BYTE _FAR_*, WORD);
extern WORD _PASCAL_ __awe32NoteOff(WORD, WORD, WORD, WORD);
extern WORD _PASCAL_ __awe32IsPlaying(WORD, WORD, WORD, WORD);

/* NRPN support functions */
extern WORD _PASCAL_ awe32InitNRPN(void);

/* Hardware support functions */
extern WORD _PASCAL_ awe32Detect(WORD);
extern WORD _PASCAL_ awe32InitHardware(void);
extern WORD _PASCAL_ awe32Terminate(void);
extern WORD _PASCAL_ awe32Check(WORD, DWORD*, DWORD*, DWORD*);

/* SoundFont support functions */
extern WORD _PASCAL_ awe32TotalPatchRam(SOUND_PACKET _FAR_*);
extern WORD _PASCAL_ awe32DefineBankSizes(SOUND_PACKET _FAR_*);
extern WORD _PASCAL_ awe32SFontLoadRequest(SOUND_PACKET _FAR_*);
extern WORD _PASCAL_ awe32StreamSample(SOUND_PACKET _FAR_*);
extern WORD _PASCAL_ awe32SetPresets(SOUND_PACKET _FAR_*);
extern WORD _PASCAL_ awe32SetPresetsEx(SOUND_PACKET _FAR_*);
extern WORD _PASCAL_ awe32ReleaseBank(SOUND_PACKET _FAR_*);
extern WORD _PASCAL_ awe32ReleaseAllBanks(SOUND_PACKET _FAR_*);
extern WORD _PASCAL_ awe32WPLoadRequest(WAVE_PACKET _FAR_*);
extern WORD _PASCAL_ awe32WPLoadWave(WAVE_PACKET _FAR_*);
extern WORD _PASCAL_ awe32WPStreamWave(WAVE_PACKET _FAR_*);
extern WORD _PASCAL_ awe32WPBuildSFont(WAVE_PACKET _FAR_*);

/* End of modules */
extern int* __midieng_ecode(void);
extern int* __hardware_ecode(void);
extern int* __sysex_ecode(void);
extern int* __sfhelp1_ecode(void);
extern int* __sfhelp2_ecode(void);
extern int* __sbkload_ecode(void);
extern int* __wavload_ecode(void);
extern int* __nrpn_ecode(void);
extern int __midivar_edata;
extern int __nrpnvar_edata;
extern int __embed_edata;


#if defined(__cplusplus)
}
#endif


#if defined(__SC__)
    #pragma pack()
#endif


#if defined(__HIGHC__)
    #pragma Pop_align_members
    #pragma Global_aliasing_convention()
    #pragma Alias(awe32RegW,"AWE32REGW")
    #pragma Alias(awe32RegRW,"AWE32REGRW")
    #pragma Alias(awe32RegDW,"AWE32REGDW")
    #pragma Alias(awe32RegRDW,"AWE32REGRDW")
    #pragma Alias(awe32InitMIDI,"AWE32INITMIDI")
    #pragma Alias(awe32NoteOn,"AWE32NOTEON")
    #pragma Alias(awe32NoteOff,"AWE32NOTEOFF")
    #pragma Alias(awe32ProgramChange,"AWE32PROGRAMCHANGE")
    #pragma Alias(awe32Controller,"AWE32CONTROLLER")
    #pragma Alias(awe32PolyKeyPressure,"AWE32POLYKEYPRESSURE")
    #pragma Alias(awe32ChannelPressure,"AWE32CHANNELPRESSURE")
    #pragma Alias(awe32PitchBend,"AWE32PITCHBEND")
    #pragma Alias(awe32Sysex,"AWE32SYSEX")
    #pragma Alias(__awe32NoteOff,"__AWE32NOTEOFF")
    #pragma Alias(__awe32IsPlaying,"__AWE32ISPLAYING")
    #pragma Alias(awe32InitNRPN,"AWE32INITNRPN")
    #pragma Alias(awe32Detect,"AWE32DETECT")
    #pragma Alias(awe32InitHardware,"AWE32INITHARDWARE")
    #pragma Alias(awe32Terminate,"AWE32TERMINATE")
    #pragma Alias(awe32TotalPatchRam,"AWE32TOTALPATCHRAM")
    #pragma Alias(awe32DefineBankSizes,"AWE32DEFINEBANKSIZES")
    #pragma Alias(awe32SFontLoadRequest,"AWE32SFONTLOADREQUEST")
    #pragma Alias(awe32StreamSample,"AWE32STREAMSAMPLE")
    #pragma Alias(awe32SetPresets,"AWE32SETPRESETS")
    #pragma Alias(awe32SetPresetsEx,"AWE32SETPRESETSEX")
    #pragma Alias(awe32ReleaseBank,"AWE32RELEASEBANK")
    #pragma Alias(awe32ReleaseAllBanks,"AWE32RELEASEALLBANKS")
    #pragma Alias(awe32WPLoadRequest,"AWE32WPLOADREQUEST")
    #pragma Alias(awe32WPLoadWave,"AWE32WPLOADWAVE")
    #pragma Alias(awe32WPStreamWave,"AWE32WPSTREAMWAVE")
    #pragma Alias(awe32WPBuildSFont,"AWE32WPBUILDSFONT")
    #pragma Alias(awe32Check,"AWE32CHECK")
#endif


#if defined(__WATCOMC__)
    #pragma pack()
    #pragma aux awe32NumG "_*"
    #pragma aux awe32BaseAddx "_*"
    #pragma aux awe32DramSize "_*"
    #pragma aux awe32Scratch "_*"
    #pragma aux awe32SFont "_*"
    #pragma aux awe32GChannel "_*"
    #pragma aux awe32MIDIChannel "_*"
    #pragma aux awe32SoundPad "_*"
    #pragma aux awe32NRPNChannel "_*"
    #pragma aux awe32SPad1Obj "_*"
    #pragma aux awe32SPad2Obj "_*"
    #pragma aux awe32SPad3Obj "_*"
    #pragma aux awe32SPad4Obj "_*"
    #pragma aux awe32SPad5Obj "_*"
    #pragma aux awe32SPad6Obj "_*"
    #pragma aux awe32SPad7Obj "_*"
    #pragma aux __midieng_code "_*"
    #pragma aux __midieng_ecode "_*"
    #pragma aux __hardware_code "_*"
    #pragma aux __hardware_ecode "_*"
    #pragma aux __sbkload_code "_*"
    #pragma aux __sbkload_ecode "_*"
    #pragma aux __nrpn_code "_*"
    #pragma aux __nrpn_ecode "_*"
    #pragma aux __midivar_data "_*"
    #pragma aux __midivar_edata "_*"
    #pragma aux __nrpnvar_data "_*"
    #pragma aux __nrpnvar_edata "_*"
    #pragma aux __embed_data "_*"
    #pragma aux __embed_edata "_*"
#endif


#pragma pack()


#endif      /* _CTAWEAPI */
