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

/* open file fname and set fhandle with the associated file handle. returns 0 on success, non-zero otherwise */
int fio_open(char far *fname, int mode, int *fhandle);

/* reads count bytes from file pointed at by fhandle, and writes the data into buff. returns the number of bytes actually read */
int fio_read(int fhandle, void far *buff, int count);

/* seek to offset position of file pointed at by fhandle. returns current file position on success, a negative error otherwise */
signed long fio_seek(unsigned short fhandle, unsigned short origin, signed long offset);

/* reads a line from file pointed at by fhandle, fill buff up to buflen bytes. returns the line length (possibly longer than buflen) */
int fio_getline(int fhandle, void far *buff, short buflen);

/* close file handle. returns 0 on success, non-zero otherwise */
int fio_close(int fhandle);

#endif
