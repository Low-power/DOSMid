/*
 * File I/O, based on DOS int 21h calls
 * This file is part of the DOSMid project
 * http://dosmid.sourceforge.net
 * Modified by Rivoreo
 * https://sourceforge.net/p/rivoreo/dosmid-code/
 * Copyright (C) 2018 Mateusz Viste
 * Copyright 2015-2024 Rivoreo
 */

#include "defines.h"
#ifdef MSDOS
#include <dos.h>    /* REGS */
#else
#include <unistd.h>
#include <fcntl.h>
#include <errno.h>
#define _fmemcpy memcpy
#endif
#include <string.h> /* _fmemcpy() */
#include "fio.h" /* include self for control */

#ifndef MSDOS
static int sync_read(int fd, void *buffer, size_t count) {
  char *p = buffer;
  do {
    int s = read(fd, p, count);
    if(s < 0) {
      if(errno == EINTR) continue;
      if(p == (char *)buffer) return -1;
      break;
    }
    if(!s) break;
    count -= s;
    p += s;
  } while(count > 0);
  return p - (char *)buffer;
}
#endif

static void fio_seek_sync(struct fiofile *f) {
#ifdef MSDOS
/* DOS 2+ - LSEEK - SET CURRENT FILE POSITION
   AH = 42h
   AL = origin of move
     00h start of file
     01h current file position
     02h end of file
   BX = file handle
   CX:DX = (signed) offset from origin of new file position
   Return on success:
     CF clear
     DX:AX = new file position in bytes from start of file
   Return on error:
     CF set
     AX = error code */
  union REGS regs;
  unsigned short *off;
  long offset;
#endif
  if ((f->flags & FIO_FLAG_SEEKSYNC) == 0) return;
#ifdef MSDOS
  offset = f->curpos;
  off = (unsigned short *)(&offset);
  regs.x.ax = 0x4200u;
  regs.x.bx = f->fh;
  regs.x.cx = off[1];
  regs.x.dx = off[0];
  int86(0x21, &regs, &regs);
  f->flags &= ~FIO_FLAG_SEEKSYNC;
  /* if (regs.x.cflag != 0) return(0 - regs.x.ax);
  return(((long)regs.x.dx << 16) | regs.x.ax); */
#else
  lseek(f->fh, f->curpos, SEEK_SET);
#endif
}

/* seek to offset position of file pointed at by fhandle. returns current file position on success, a negative error otherwise */
signed long int fio_seek(struct fiofile *f, unsigned short int origin, signed long int offset) {
  switch (origin) {
    case FIO_SEEK_START:
      if (offset < 1) {
        f->curpos = 0;
      } else {
        f->curpos = offset;
      }
      break;
    case FIO_SEEK_END:
      f->curpos = f->flen;
    case FIO_SEEK_CUR:
      f->curpos += offset;
      break;
  }
  f->flags |= FIO_FLAG_SEEKSYNC;
  return(f->curpos);
}

/* reads a line from file pointed at by fhandle, fill buff up to buflen bytes. returns the line length (possibly longer than buflen), or -1 on EOF */
int fio_getline(struct fiofile *f, void far *buff, short int buflen) {
  unsigned char bytebuf;
  short linelen = 0;
  buflen--; /* leave space for the zero terminator */
  for (;;) {
    if (fio_read(f, &bytebuf, 1) == 0) { /* EOF */
      if (linelen == 0) linelen = -1;
      break;
    }
    if (bytebuf == '\n') break;
    if (bytebuf == '\r') continue;
    linelen++;
    if (buflen > 0) {
      buflen--;
      *((unsigned char far *)buff) = bytebuf;
      buff = ((unsigned char far *)buff) + 1;
    }
  }
  *((unsigned char far *)buff) = 0;
  return(linelen);
}

static void loadcache(struct fiofile *f) {
#ifdef MSDOS
  union REGS regs;
  struct SREGS sregs;
#endif
  fio_seek_sync(f);
  f->flags |= FIO_FLAG_SEEKSYNC;
  f->bufoffs = f->curpos;
#ifdef MSDOS
  regs.h.ah = 0x3f;
  regs.x.bx = f->fh;
  regs.x.cx = FIO_CACHE;
#ifdef _QC
  segread(&sregs);
  regs.x.dx = (unsigned int)f->buff;
#else
  sregs.ds = FP_SEG(f->buff);
  regs.x.dx = FP_OFF(f->buff);
#endif
  int86x(0x21, &regs, &regs, &sregs);
#else
  sync_read(f->fh, f->buff, FIO_CACHE);
#endif
}

/* open file fname and set fhandle with the associated file handle. returns 0 on success, non-zero otherwise */
int fio_open(const char far *fname, int mode, struct fiofile *f) {
#ifdef MSDOS
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
  /* */
  regs.h.ah = 0x3d;
  regs.h.al = mode;
  sregs.ds = FP_SEG(fname);
  regs.x.dx = FP_OFF(fname);
  int86x(0x21, &regs, &regs, &sregs);
  f->fh = regs.x.ax;
  if (regs.x.cflag != 0) return(-1);
#else
  int flags;
  switch(mode) {
    case FIO_OPEN_RD:
      flags = O_RDONLY;
      break;
    case FIO_OPEN_WR:
      flags = O_WRONLY;
      break;
    case FIO_OPEN_RW:
      flags = O_RDWR;
      break;
    default:
      return -1;
  }
  int fd = open(fname, flags);
  if(fd == -1) return -1;
  f->fh = fd;
#endif
  f->curpos = 0;
  loadcache(f);
  /* fseek to end so I know the file length */
#ifdef MSDOS
  regs.x.ax = 0x4202u;
  regs.x.bx = f->fh;
  regs.x.cx = 0;
  regs.x.dx = 0;
  int86(0x21, &regs, &regs);
  f->flen = (((long)regs.x.dx << 16) | regs.x.ax);
  /* fseek to start */
  regs.x.ax = 0x4200u;
  regs.x.bx = f->fh;
  regs.x.cx = 0;
  regs.x.dx = 0;
  int86(0x21, &regs, &regs);
#else
  off_t len = lseek(fd, 0, SEEK_END);
  if(len != (off_t)-1) f->flen = len;
  lseek(fd, 0, SEEK_SET);
#endif
  f->flags = FIO_FLAG_SEEKSYNC;
  return(0);
}

/* reads count bytes from file pointed at by fhandle, and writes the data into buff. returns the number of bytes actually read, or a negative number on error */
int fio_read(struct fiofile *f, void far *buff, int count) {
#ifdef MSDOS
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
#endif
  if (f->curpos + count > f->flen) count = f->flen - f->curpos;
  if (count == 0) return(0);
  if (count <= FIO_CACHE) {
    if ((f->curpos < f->bufoffs) || (f->curpos + count > f->bufoffs + FIO_CACHE)) {
      loadcache(f);
    }
    _fmemcpy(buff, f->buff + (f->curpos - f->bufoffs), count);
    f->curpos += count;
    return(count);
  }
  fio_seek_sync(f);
#ifdef MSDOS
  regs.h.ah = 0x3f;
  regs.x.bx = f->fh;
  regs.x.cx = count;
  sregs.ds = FP_SEG(buff);
  regs.x.dx = FP_OFF(buff);
  int86x(0x21, &regs, &regs, &sregs);
  if (regs.x.cflag != 0) return(0 - regs.x.ax);
  f->curpos += regs.x.ax;
  return(regs.x.ax);
#else
  return sync_read(f->fh, buff, count);
#endif
}

/* close file handle. returns 0 on success, non-zero otherwise */
int fio_close(struct fiofile *f) {
#ifdef MSDOS
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
  regs.x.bx = f->fh;
  int86(0x21, &regs, &regs);
  if (regs.x.cflag != 0) return(0 - regs.x.ax);
  return(0);
#else
  return close(f->fh);
#endif
}
