#
# DOSMid Makefile for GNU Make
# Based on the Open Watcom Make version
# Copyright (C) 2014-2022 Mateusz Viste
# Copyright 2015-2023 Rivoreo
#

# You can control the availability of some features with following macros:
#  SBAWE    enables SoundBlaster AWE drivers (+36K)
#  OPL      enables MIDI emulation over OPL output (+9K)
#  OPLLPT   enables OPL2LPT and OPL3LPT output, requires OPL
#  CMS      enables Creative Music System / Game Blaster output (+5K)
#  CMSLPT   enables CMSLPT output, requires CMS
#  DBGFILE  enables debug output to file (+10K)
FEATURES   := -D SBAWE=1 -D OPL=1 -D OPLLPT=1 -D CMS=1 -D CMSLPT=1
FEATURESLT := -D OPL=1 -D OPLLPT=1 -D CMS=1 -D CMSLPT=1

# Configure the default output device for use when no device option specified
# and no GUS, MPU and AWE configured via environment
DEFAULT_DEVICE := \
	-D DOSMID_DEFAULT_DEVICE_TYPE=DEV_OPL \
	-D DOSMID_DEFAULT_DEVICE_PORT=0x388

# memory segmentation mode (s = small ; c = compact ; m = medium ; l = large)
#             code | data
#  small      64K  | 64K
#  compact    64K  | 64K+
#  medium     64K+ | 64K
#  large      64K+ | 64K+
CMODEL := s

UPX ?= upx
UPX_FLAGS ?= -9
CFLAGS += -b dos -fpack-struct=2 -Wall -Werror -Wc,-0 -fno-stack-check -mcmodel=$(CMODEL) -Os
#LDFLAGS += -s

# upx(1) chokes on UPX environment variable
unexport UPX

SOURCES := cms.c dosmid.c fio.c gus.c lpt.c mem.c midi.c mpu401.c mus.c opl.c outdev.c rs232.c sbdsp.c syx.c timer.c ui.c xms.c

all:	dosmid.exe dosmidlt.exe

dosmid.exe:	$(SOURCES)
	owcc $(CFLAGS) $(FEATURES) $(DEFAULT_DEVICE) $(LDFLAGS) $(SOURCES) -fm=$(basename $@).map -o $@ awe32/rawe32$(CMODEL).lib
	$(UPX) $(UPX_FLAGS) --8086 $@

dosmidlt.exe:	$(SOURCES)
	owcc $(CFLAGS) $(FEATURESLT) $(DEFAULT_DEVICE) $(LDFLAGS) $(SOURCES) -fm=$(basename $@).map -o $@
	$(UPX) $(UPX_FLAGS) --8086 $@

clean:
	rm -f *.o dosmid.map dosmid.exe dosmidlt.map dosmidlt.exe

.PHONY:	all clean
