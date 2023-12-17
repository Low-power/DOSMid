/*
 * test for fio routines
 */

#include <io.h>
#include <stdio.h>

#include "../fio.h"

#define FNAME "fiotest.dat"

int main(void) {
  FILE *f;
  int i;
  struct fiofile_t f;
  long pos;
  static unsigned char buff[1024];

  /* delete the test file, just in case it would exist from a previous test */
  unlink(FNAME);

  /* try opening a non-existing file - should fail */
  if (fio_open(FNAME, FIO_OPEN_RD, &f) == 0) {
    printf("ERR: opening " FNAME " should have failed (supposed not to exist)\n");
    fio_close(&f);
    return(1);
  }

  /* create a test file of 2000 bytes */
  f = fopen(FNAME, "wb");
  if (f == NULL) {
    printf("ERR: fopen()\n");
    return(1);
  }
  for (i = 0; i < 2000; i++) {
    fprintf(f, "%c", i & 0xff);
  }
  fclose(f);

  /* open the file */
  if (fio_open(FNAME, FIO_OPEN_RD, &f) != 0) {
    printf("ERR: opening " FNAME " should have failed (supposed not to exist)\n");
    return(1);
  }

  /* read 200 bytes 1 byte a time, and verify them */
  for (i = 0; i < 200; i++) {
    fio_read(&f, buff, 1);
    if (*buff != (i & 0xff)) {
      fio_close(&f);
      printf("ERR: unexpected data at offset %d (1 by 1 test, %d != %d)\n", i, *buff, i);
      return(1);
    }
  }

  /* read 1024 bytes again and verify them (this time in one go) */
  fio_seek(&f, FIO_SEEK_START, 0);
  i = fio_read(&f, buff, 1024);
  if (i != 1024) {
    printf("ERR: tried to read 1024 bytes but got something else (%d)\n", i);
    fio_close(&f);
    return(1);
  }
  for (i = 0; i < 1024; i++) {
    if (buff[i] != (i & 0xff)) {
      fio_close(&f);
      printf("ERR: unexpected data\n");
      return(1);
    }
  }

  /* seek to offset 999 */
  pos = fio_seek(&f, FIO_SEEK_START, 999);
  if (pos != 999) {
    printf("ERR: forward seek failure (%ld)\n", pos);
    fio_close(&f);
    return(1);
  }

  /* seek to last offset */
  pos = fio_seek(&f, FIO_SEEK_END, 0);
  if (pos != 2000) {
    printf("ERR: eof seek failure (%ld)\n", pos);
    fio_close(&f);
    return(1);
  }

  /* seek back 500 bytes - should land at 1500 */
  pos = fio_seek(&f, FIO_SEEK_END, -500);
  if (pos != 1500) {
    printf("ERR: backward seek failure (%ld)\n", pos);
    fio_close(&f);
    return(1);
  }

  /* read 100 bytes and seek to 0 - should return offset 1600 */
  fio_read(&f, buff, 100);
  pos = fio_seek(&f, FIO_SEEK_CUR, 0);
  if (pos != 1600) {
    printf("ERR: ftell-like seek failure (%ld)\n", pos);
    fio_close(&f);
    return(1);
  }

  /* try reading 500 bytes - should succeed only for 400 */
  pos = fio_read(&f, buff, 500);
  if (pos != 400) {
    printf("ERR: fio_read() read an unexpected amount of bytes (%ld instead of 400)\n", pos);
    fio_close(&f);
    return(1);
  }

  /* go back 3 bytes, and read 4 bytes (only 3 should succeed) */
  fio_seek(&f, FIO_SEEK_CUR, -3);
  pos = fio_read(&f, buff, 4);
  if (pos != 3) {
    printf("ERR: fio_read() read an unexpected amount of bytes (%ld instead of 3)\n", pos);
    fio_close(&f);
    return(1);
  }

  fio_close(&f);

  /* delete the test file */
  unlink(FNAME);

  printf("OK\n");
  return(0);
}
