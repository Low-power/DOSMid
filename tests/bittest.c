/*
 * simple test for bitfield operations
 * Copyright (C) 2018 Mateusz Viste
 */

#include <stdio.h>

#include "../bitfield.h"

int main(void) {
  unsigned long instmap[8] = {0};

  int bits[] = {0, 4, 5, 6, 63, 99, 143, 250, -1};
  int i;

  printf("Setting bits..:");
  for (i = 0; bits[i] >= 0; i++) {
    printf(" %d", bits[i]);
    BIT_SET(instmap, bits[i]);
  }

  printf("\nRead bits.....:");
  for (i = 0; i < 256; i++) {
    if (BIT_GET(instmap, i) != 0) printf(" %d", i);
  }

  return(0);
}
