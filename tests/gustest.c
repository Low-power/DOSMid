#include <stdio.h>
#include <unistd.h>
#include "../gus.h"


int main(void) {
  int vect;
  vect = gus_find();
  if (vect < 0) {
    printf("Err: ultramid not found\n");
    return(1);
  }
  printf("ultramid FOUND at vector 0x%02X\n", vect);
  gus_open(vect);
  gus_allnotesoff();

  /* make sure the GUS loads a piano (0) patch */
  printf("load patch #0\n");
  gus_loadpatch(0);
  gus_write(0xC0 | 0);
  gus_write(0);

  /* note on */
  printf("note on\n");
  gus_write(0x90 | 0); /* note on */
  gus_write(32);       /* note */
  gus_write(63);      /* velocity */

  sleep(1);

  /* note off */
  printf("note off\n");
  gus_write(0x80 | 0); /* note off */
  gus_write(32);       /* note */
  gus_write(64);       /* velocity */

  gus_close();

  return(0);
}
