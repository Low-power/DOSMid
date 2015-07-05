
all: dosmid.exe

dosmid.exe: dosmid.c mem.c midi.c mpu401.c mus.c outdev.c timer.c ui.c xms.c
	wcl -lr -we -d0 -0 -s -mc -wx dosmid.c mem.c midi.c mpu401.c mus.c outdev.c timer.c ui.c xms.c
	#upx --8086 --best dosmid.exe

clean: .symbolic
	del *.obj
	del dosmid.exe

pkg: dosmid.exe .symbolic
	mkdir progs
	mkdir progs\dosmid
	mkdir source
	mkdir source\dosmid
	mkdir appinfo
	if exist dosmid.zip del dosmid.zip
	copy dosmid.exe progs\dosmid
	copy dosmid.txt progs\dosmid
	copy history.txt progs\dosmid
	copy *.txt source\dosmid
	copy *.c source\dosmid
	copy *.h source\dosmid
	copy Makefile source\dosmid
	copy dosmid.lsm source\dosmid
	copy dosmid.lsm appinfo
	zip -m -q -k -r -9 dosmid.zip progs source appinfo
