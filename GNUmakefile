#
# DOSMid Makefile for GNU Make
# Based on the Open Watcom Make version
# Copyright (C) 2014-2022 Mateusz Viste
# Copyright 2015-2023 Rivoreo
#

# You can control the availability of some features with following macros:
#  SBAWE    enables SoundBlaster AWE drivers (+36K)
#  OPL      enables MIDI emulation over OPL output (+9K)
#  DBGFILE  enables debug output to file (+10K)
#  CMS      enables Creative Music System / Game Blaster output
#  CMSLPT   enables CMSLPT output, requires CMS
FEATURES   := -D SBAWE=1 -D OPL=1 -D CMS=1 -D CMSLPT=1
FEATURESLT := -D OPL=1 -D CMS=1 -D CMSLPT=1

# memory segmentation mode (s = small ; c = compact ; m = medium ; l = large)
#             code | data
#  small      64K  | 64K
#  compact    64K  | 64K+
#  medium     64K+ | 64K
#  large      64K+ | 64K+
CMODEL := s

UPX ?= upx
CFLAGS := -b dos -fpack-struct=2 -Wall -Werror -Wc,-0 -fno-stack-check -mcmodel=$(CMODEL) -Os

SOURCES := cms.c dosmid.c fio.c gus.c mem.c midi.c mpu401.c mus.c opl.c outdev.c rs232.c sbdsp.c syx.c timer.c ui.c xms.c

all:	dosmid.exe dosmidlt.exe

dosmid.exe:	$(SOURCES)
	owcc $(CFLAGS) $(FEATURES) $(SOURCES) -fm=$(basename $@).map -o $@ awe32/rawe32$(CMODEL).lib
	$(UPX) --8086 -9 $@

dosmidlt.exe:	$(SOURCES)
	owcc $(CFLAGS) $(FEATURESLT) $(SOURCES) -fm=$(basename $@).map -o $@
	$(UPX) --8086 -9 $@

clean:
	rm -f *.o dosmid.map dosmid.exe dosmidlt.map dosmidlt.exe
