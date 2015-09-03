
# memory segmentation mode (s = small ; c = compact ; m = medium ; l = large)
MODE = c

all: dosmid.exe

dosmid.exe: dosmid.c mem.c midi.c mpu401.c mus.c opl.c outdev.c rs232.c sbdsp.c timer.c ui.c xms.c
	wcl -lr -we -d0 -0 -s -m$(MODE) -wx -fe=dosmid.exe *.c awe32\rawe32$(MODE).lib
	upx --8086 -9 dosmid.exe

clean: .symbolic
	del *.obj
	del dosmid.exe

pkg: dosmid.exe .symbolic
	mkdir progs
	mkdir progs\dosmid
	mkdir source
	mkdir source\dosmid
	mkdir source\dosmid\awe32
	mkdir appinfo
	if exist dosmid.zip del dosmid.zip
	copy dosmid.exe progs\dosmid
	copy dosmid.txt progs\dosmid
	copy history.txt progs\dosmid
	copy *.txt source\dosmid
	copy *.c source\dosmid
	copy *.h source\dosmid
	copy awe32\*.* source\dosmid\awe32
	copy Makefile source\dosmid
	copy dosmid.lsm source\dosmid
	copy dosmid.lsm appinfo
	zip -m -q -k -r -9 dosmid.zip progs source appinfo
