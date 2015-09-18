
all: sysex.exe opltest.exe syxdump.exe

sysex.exe: sysex.c ..\outdev.c
	wcl -ml -0 -we -lr -DSBAWE sysex.c ..\outdev.c ..\rs232.c ..\mpu401.c ..\sbdsp.c ..\timer.c ..\awe32\rawe32l.lib

opltest.exe: opltest.c ..\opl.c
	wcl -mc -0 -we -lr -DOPL opltest.c ..\opl.c

syxdump.exe: syxdump.c ..\syx.c
	wcl -ms -0 -we -lr syxdump.c ..\syx.c

clean: .symbolic
	del *.obj
	del *.exe
