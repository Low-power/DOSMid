#
# DOSMid Makefile for Open Watcom Make
# Copyright (C) 2014-2023 Mateusz Viste
# DOSMid Makefile for OpenWatcom
# Copyright (C) 2014-2023 Mateusz Viste
#

# you can control the availability of some features with the FEATURES string:
#  -DSBAWE    enables SoundBlaster AWE drivers (+36K)
#  -DOPL      enables MIDI emulation over OPL output (+9K)
#  -DDBGFILE  enables debug output to file (+10K)
#  -DCMS      enables Creative Music System / Game Blaster output
#  -DCMSLPT   enables CMSLPT output, requires CMS
FEATURES   = -DSBAWE -DOPL -DCMS -DCMSLPT
FEATURESLT = -DOPL -DCMS -DCMSLPT

# memory segmentation mode (s = small ; c = compact ; m = medium ; l = large)
#             code | data
#  small      64K  | 64K
#  compact    64K  | 64K+
#  medium     64K+ | 64K
#  large      64K+ | 64K+
MODE = s

CFLAGS = -zp2 -lr -we -d0 -y -0 -s -m$(MODE) -wx

all: dosmid.exe

dosmid.exe: cms.c dosmid.c fio.c gus.c mem.c midi.c mpu401.c mus.c opl.c outdev.c rs232.c sbdsp.c syx.c timer.c ui.c xms.c
	wcl $(CFLAGS) $(FEATURES) -fe=dosmid.exe -fm=dosmid.map *.c awe32\rawe32$(MODE).lib
	wcl $(CFLAGS) $(FEATURESLT) -fe=dosmidlt.exe -fm=dosmidlt.map *.c
	upx --8086 -9 dosmid.exe
	upx --8086 -9 dosmidlt.exe

clean: .symbolic
	del *.obj
	del *.map
	del dosmid.exe
	del dosmidlt.exe

release: dosmid.exe .symbolic
	if exist dosmid.svp del dosmid.svp
	if exist dosmid.zip del dosmid.zip
	if exist source.zip del source.zip
	mkdir progs
	mkdir progs\dosmid
	mkdir appinfo
	copy dosmid.lsm appinfo\
	copy dosmid.exe progs\dosmid\
	copy dosmidlt.exe progs\dosmid\
	copy dosmid.txt progs\dosmid\
	copy dosmid.cfg progs\dosmid\
	copy history.txt progs\dosmid\
	zip -k9rqDX -j dosmid.zip progs\dosmid
	zip -k9rqDX -m dosmid.svp progs appinfo
	zip -k9rqDX source.zip awe32 *.txt *.cfg *.c *.h Makefile dosmid.lsm
	rmdir appinfo
	rmdir progs\dosmid
	rmdir progs
