/*
 * File I/O, based on DOS int 21h calls
 * This file is part of the DOSMid project
 * http://dosmid.sourceforge.net
 *
 * Copyright (C) 2018 Mateusz Viste
 */

#ifndef fio_h_sentinel
#define fio_h_sentinel

#define FIO_SEEK_START 0
#define FIO_SEEK_CUR   1
#define FIO_SEEK_END   2

#define FIO_OPEN_RD 0
#define FIO_OPEN_WR 1
#define FIO_OPEN_RW 2

#define FIO_FLAG_SEEKSYNC 1

#define FIO_CACHE 32

struct fiofile {
  unsigned short int fh;         /* file handle (as used by DOS) */
  unsigned long int flen;        /* file length */
  unsigned long int curpos;      /* current offset position (ftell) */
  unsigned char buff[FIO_CACHE]; /* buffer storage   */
  unsigned long int bufoffs;     /* offset of buffer */
  unsigned char flags;           /* flags */
};

/* open file fname and set fhandle with the associated file handle. returns 0 on success, non-zero otherwise */
int fio_open(const char far *fname, int mode, struct fiofile *f);

/* reads count bytes from file pointed at by fhandle, and writes the data into buff. returns the number of bytes actually read */
int fio_read(struct fiofile *f, void far *buff, int count);

/* seek to offset position of file pointed at by fhandle. returns current file position on success, a negative error otherwise */
signed long int fio_seek(struct fiofile *f, unsigned short int origin, signed long int offset);

/* reads a line from file pointed at by fhandle, fill buff up to buflen bytes. returns the line length (possibly longer than buflen) */
int fio_getline(struct fiofile *f, void far *buff, short int buflen);

/* close file handle. returns 0 on success, non-zero otherwise */
int fio_close(struct fiofile *f);

#endif
