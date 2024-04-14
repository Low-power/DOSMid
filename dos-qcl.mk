#
# DOSMid Makefile for Microsoft NMake and Microsoft QuickC
# Copyright (C) 2014-2023 Mateusz Viste
# Copyright 2015-2024 Rivoreo
#

# you can control the availability of some features with the FEATURES string:
#  -DSBAWE    enables SoundBlaster AWE drivers (+36K)
#  -DOPL      enables MIDI emulation over OPL output (+9K)
#  -DOPLLPT   enables OPL2LPT and OPL3LPT output, requires OPL
#  -DCMS      enables Creative Music System / Game Blaster output (+5K)
#  -DCMSLPT   enables CMSLPT output, requires CMS
#  -DDBGFILE  enables debug output to file (+10K)
FEATURES   = -DSBAWE -DOPL -DOPLLPT -DCMS -DCMSLPT
FEATURESLT = -DOPL -DOPLLPT -DCMS -DCMSLPT

# Configure the default output device for use when no device option specified
# and no GUS, MPU and AWE configured via environment
#DEFAULT_DEVICE = -DDOSMID_DEFAULT_DEVICE_TYPE=DEV_OPL -DDOSMID_DEFAULT_DEVICE_PORT=0x388

# memory segmentation mode (s = small ; c = compact ; m = medium ; l = large)
#             code | data
#  small      64K  | 64K
#  compact    64K  | 64K+
#  medium     64K+ | 64K
#  large      64K+ | 64K+
#MODE = s

CC = qcl
CFLAGS = -Os -Gs -AS -W1 -I compat

SOURCES = cms.c dosmid.c fio.c gus.c lpt.c mem.c midi.c mpu401.c mus.c opl.c outdev.c rs232.c sbdsp.c syx.c timer.c ui.c xms.c

all:	dosmid.exe dosmidlt.exe

dosmid.exe:	$(SOURCES)
	$(CC) $(CFLAGS) $(FEATURES) $(DEFAULT_DEVICE) -Fe$@ -Fmdosmid.map *.c awe32\rawe32s.lib

dosmidlt.exe:	$(SOURCES)
	$(CC) $(CFLAGS) $(FEATURESLT) $(DEFAULT_DEVICE) -Fe$@ -Fmdosmidlt.map *.c

clean:
	del *.obj
	del *.map
	del dosmid.exe
	del dosmidlt.exe
