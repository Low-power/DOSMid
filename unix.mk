#
# DOSMid Makefile targeting UNIX-like systems
# Copyright 2015-2024 Rivoreo
#

# Enable OPL and OPLLPT output supports
FEATURES += -D OPL=1 -D OPLLPT=1

# Enable CMS and CMSLPT output supports
FEATURES += -D CMS=1 -D CMSLPT=1

# Enable wide-character support, requires wide-character-enabled curses
FEATURES += -D WCHAR=1

# Configure the default output device for use when no device option specified
# and no GUS and MPU configured via environment
DEFAULT_DEVICE := \
	-D DOSMID_DEFAULT_DEVICE_TYPE=DEV_OPL \
	-D DOSMID_DEFAULT_DEVICE_PORT=0x388

#CFLAGS += -I /usr/include/ncursesw
CFLAGS += -Wall -Wno-switch -Os -fno-common
CFLAGS += -D __far= -D __near= -D far= -D near= $(CPPFLAGS) $(FEATURES) $(DEFAULT_DEVICE)
#LIBS += -l rt
CURSES_LIBS ?= -l curses
#CURSES_LIBS ?= -l ncurses
#CURSES_LIBS ?= -l ncursesw

OBJECTS := \
	cms.o \
	dosmid.o \
	fio.o \
	lpt.o \
	mem.o \
	midi.o \
	mpu401.o \
	mus.o \
	opl.o \
	outdev.o \
	sbdsp.o \
	syx.o \
	timer.o \
	ui.o \
	unixpio.o

dosmid:	$(OBJECTS)
	$(CC) $(LDFLAGS) $(OBJECTS) -o $@ $(CURSES_LIBS) $(LIBS)

clean:
	rm -f $(OBJECTS) dosmid
