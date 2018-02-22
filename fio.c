/*
 * File I/O, based on DOS int 21h calls
 * This file is part of the DOSMid project
 * http://dosmid.sourceforge.net
 *
 * Copyright (C) 2018 Mateusz Viste
 */

#include <dos.h>
#include <fcntl.h>
#include <share.h>

#include "fio.h" /* include self for control */

/* seek to offset position of file pointed at by fhandle. returns 0 on success, non-zero otherwise */
unsigned short fio_seek(unsigned short fhandle, unsigned short origin, signed long offset) {
/* DOS 2+ - LSEEK - SET CURRENT FILE POSITION
   AH = 42h
   AL = origin of move
     00h start of file
     01h current file position
     02h end of file
   BX = file handle
   CX:DX = (signed) offset from origin of new file position */
  union REGS regs;
  unsigned short *off = (unsigned short *)(&offset);
  regs.h.ah = 0x42;
  regs.h.al = origin;
  regs.x.bx = fhandle;
  regs.x.cx = off[1];
  regs.x.dx = off[0];
  int86(0x21, &regs, &regs);
  if (regs.x.cflag != 0) return(0 - regs.x.ax);
  return(0);
}

/* open file fname and set fhandle with the associated file handle. returns 0 on success, non-zero otherwise */
int fio_open(char far *fname, int mode, int *fhandle) {
  /* DOS 2+ - OPEN - OPEN EXISTING FILE
     AH = 3Dh
     AL = access and sharing modes
     DS:DX -> ASCIZ filename
     CL = attribute mask of files to look for (server call only)
     Return:
     CF clear if successful
     AX = file handle or error code
     CF set on error */
  union REGS regs;
  struct SREGS sregs;
  regs.h.ah = 0x3d;
  regs.h.al = mode;
  sregs.ds = FP_SEG(fname);
  regs.x.dx = FP_OFF(fname);
  int86x(0x21, &regs, &regs, &sregs);
  *fhandle = regs.x.ax;
  if (regs.x.cflag != 0) return(-1);
  return(0);
}

/* reads count bytes from file pointed at by fhandle, and writes the data into buff. returns the number of bytes actually read, or a negative number on error */
int fio_read(int fhandle, void far *buff, int count) {
/* DOS 2+ - READ - READ FROM FILE OR DEVICE
 * AH = 3Fh
 * BX = file handle
 * CX = number of bytes to read
 * DS:DX -> buffer for data
 * Return:
 * CF clear if successful
 * AX = number of bytes actually read (0 if at EOF before call)
 * CF set on error
 * AX = error code (05h,06h) (see #01680 at AH=59h/BX=0000h) */
  union REGS regs;
  struct SREGS sregs;
  regs.h.ah = 0x3f;
  regs.x.bx = fhandle;
  regs.x.cx = count;
  sregs.ds = FP_SEG(buff);
  regs.x.dx = FP_OFF(buff);
  int86x(0x21, &regs, &regs, &sregs);
  if (regs.x.cflag != 0) return(0 - regs.x.ax);
  return(regs.x.ax);
}

/* close file handle. returns 0 on success, non-zero otherwise */
int fio_close(int fhandle) {
  /* DOS 2+ - CLOSE - CLOSE FILE
     AH = 3Eh
     BX = file handle
     Return on success:
       CF clear
       AX destroyed
     On error:
       CF set
       AX = error code */
  union REGS regs;
  regs.h.ah = 0x3e;
  regs.x.bx = fhandle;
  int86(0x21, &regs, &regs);
  if (regs.x.cflag != 0) return(0 - regs.x.ax);
  return(0);
}
