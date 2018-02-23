/*
 * test for fio routines
 */

#include <io.h>
#include <stdio.h>

#include "..\fio.h"

#define FNAME "fiotest.dat"

int main(void) {
  FILE *fd;
  int fh, i;
  long pos;
  unsigned char buff[1024];

  /* try opening a non-existing file - should fail */
  if (fio_open(FNAME, FIO_OPEN_RD, &fh) == 0) {
    printf("ERR: opening " FNAME " should have failed (supposed not to exist)\n");
    fio_close(fh);
    return(1);
  }

  /* create a test file of 2000 bytes */
  fd = fopen(FNAME, "wb");
  if (fd == NULL) {
    printf("ERR: fopen()\n");
    return(1);
  }
  for (i = 0; i < 2000; i++) {
    fprintf(fd, "%c", i & 0xff);
  }
  fclose(fd);

  /* open the file */
  if (fio_open(FNAME, FIO_OPEN_RD, &fh) != 0) {
    printf("ERR: opening " FNAME " should have failed (supposed not to exist)\n");
    return(1);
  }
  /* read 1024 bytes and verify them */
  i = fio_read(fh, buff, 1024);
  if (i != 1024) {
    printf("ERR: tried to read 1024 bytes but got something else (%d)\n", i);
    fio_close(fh);
    return(1);
  }
  for (i = 0; i < 1024; i++) {
    if (buff[i] != (i & 0xff)) {
      fio_close(fh);
      printf("ERR: unexpected data\n");
      return(1);
    }
  }

  /* seek to offset 999 */
  pos = fio_seek(fh, FIO_SEEK_START, 999);
  if (pos != 999) {
    printf("ERR: forward seek failure (%ld)\n", pos);
    fio_close(fh);
    return(1);
  }

  /* seek to last offset */
  pos = fio_seek(fh, FIO_SEEK_END, 0);
  if (pos != 2000) {
    printf("ERR: eof seek failure (%ld)\n", pos);
    fio_close(fh);
    return(1);
  }

  /* seek back 500 bytes - should land at 1500 */
  pos = fio_seek(fh, FIO_SEEK_END, -500);
  if (pos != 1500) {
    printf("ERR: backward seek failure (%ld)\n", pos);
    fio_close(fh);
    return(1);
  }

  /* read 100 bytes and seek to 0 - should return offset 1600 */
  fio_read(fh, buff, 100);
  pos = fio_seek(fh, FIO_SEEK_CUR, 0);
  if (pos != 1600) {
    printf("ERR: ftell-like seek failure (%ld)\n", pos);
    fio_close(fh);
    return(1);
  }

  /* try reading 500 bytes - should succeed only for 400 */
  pos = fio_read(fh, buff, 500);
  if (pos != 400) {
    printf("ERR: fio_read() read an unexpected amount of bytes (%ld)\n", pos);
    fio_close(fh);
    return(1);
  }

  fio_close(fh);

  /* delete the test file */
  unlink(FNAME);

  printf("OK\n");
  return(0);
}
