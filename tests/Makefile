
all: opltest.exe syxdump.exe

opltest.exe: opltest.c ..\opl.c
	wcl -0 -we -lr opltest.c ..\opl.c

syxdump.exe: syxdump.c ..\syx.c
	wcl -0 -we -lr syxdump.c ..\syx.c

clean: .symbolic
	del *.obj
	del *.exe
