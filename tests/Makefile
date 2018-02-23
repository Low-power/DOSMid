
all: bittest.exe fiotest.exe gustest.exe sysex.exe opltest.exe syxdump.exe

bittest.exe: bittest.c ..\bitfield.h
	wcl -ms -0 -we -lr bittest.c

fiotest.exe: fiotest.c ..\fio.c
	wcl -ms -0 -we -lr fiotest.c ..\fio.c

sysex.exe: sysex.c ..\outdev.c ..\fio.c
	wcl -ml -0 -we -lr -DSBAWE sysex.c ..\outdev.c ..\fio.c ..\gus.c ..\rs232.c ..\mpu401.c ..\sbdsp.c ..\timer.c ..\awe32\rawe32l.lib

opltest.exe: opltest.c ..\opl.c ..\fio.c
	wcl -mc -0 -we -lr -DOPL opltest.c ..\opl.c ..\fio.c

gustest.exe: gustest.c ..\gus.c
	wcl -mc -0 -we -lr gustest.c ..\gus.c

syxdump.exe: syxdump.c ..\syx.c
	wcl -ms -0 -we -lr syxdump.c ..\syx.c

clean: .symbolic
	del *.obj
	del *.exe
