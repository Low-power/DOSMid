#
# DOSMid Makefile for OpenWatcom
# Copyright (C) 2014-2021 Mateusz Viste
#

# you can control the availability of some features with the FEATURES string:
#  -DSBAWE    enables SoundBlaster AWE drivers (+36K)
#  -DOPL      enables MIDI emulation over OPL output (+9K)
#  -DDBGFILE   enables debug output to file (+10K)
FEATURES   = -DOPL -DSBAWE
FEATURESLT = -DOPL

# memory segmentation mode (s = small ; c = compact ; m = medium ; l = large)
#             code | data
#  small      64K  | 64K
#  compact    64K  | 64K+
#  medium     64K+ | 64K
#  large      64K+ | 64K+
MODE = s

CFLAGS = -zp2 -lr -we -d0 -y -0 -s -m$(MODE) -wx

all: dosmid.exe

dosmid.exe: dosmid.c fio.c gus.c mem.c midi.c mpu401.c mus.c opl.c outdev.c rs232.c sbdsp.c syx.c timer.c ui.c xms.c
	wcl $(CFLAGS) $(FEATURES) -fe=dosmid.exe -fm=dosmid.map *.c awe32\rawe32$(MODE).lib
	wcl $(CFLAGS) $(FEATURESLT) -fe=dosmidlt.exe -fm=dosmidlt.map *.c
	upx --8086 -9 dosmid.exe
	upx --8086 -9 dosmidlt.exe

clean: .symbolic
	del *.obj
	del *.map
	del dosmid.exe
	del dosmidlt.exe

pkg: dosmid.exe .symbolic
	mkdir progs
	mkdir progs\dosmid
	mkdir source
	mkdir source\dosmid
	mkdir source\dosmid\awe32
	mkdir appinfo
	if exist dosmid.zip del dosmid.zip
	copy dosmid.exe progs\dosmid
	copy dosmidlt.exe progs\dosmid
	copy dosmid.txt progs\dosmid
	copy dosmid.cfg progs\dosmid
	copy history.txt progs\dosmid
	copy *.txt source\dosmid
	copy *.cfg source\dosmid
	copy *.c source\dosmid
	copy *.h source\dosmid
	copy awe32\*.* source\dosmid\awe32
	copy Makefile source\dosmid
	copy dosmid.lsm source\dosmid
	copy dosmid.lsm appinfo
	zip -m -q -k -r -9 dosmid.zip progs source appinfo
