/*
 * test performances of tim's solution to the (delta * tempo / unitdiv) problem
 */

#include <stdio.h>
#include <stdlib.h>

static unsigned long gettime(void) {
  unsigned short hw = 0, lw = 0;
  unsigned long r;
  _asm {
    xor ah, ah
    int 0x1a
    mov hw, cx
    mov lw, dx
  }
  r = ((unsigned long)hw << 16) | lw;
  return(r);
}

/* run with arguments 500 1090909 15360 */
int main(int argc, char **argv) {
  unsigned long ticks = atoi(argv[1]);
  unsigned long tempo = atoi(argv[2]);
  unsigned short grouplen = atoi(argv[3]);
  unsigned short i;
  unsigned long r;
  unsigned long t0, t1;

  if (argc != 4) return(1);

  t0 = gettime();
  for (i = 1; i++; ) {
    unsigned long a = ticks,   b = tempo,   c = grouplen;
    unsigned long aq = a/c,    ar = a%c,    bq = b/c,    br = b%c;
    r = aq*b + ar*bq + ar*br/c;
  }
  t1 = gettime();
  printf("Tim = %lu after %lu clocks\n", r, t1 - t0);

  t0 = gettime();
  for (i = 1; i++; ) {
    r = ((ticks / grouplen) * tempo + (ticks % grouplen) * (tempo / grouplen) + (ticks % grouplen) * (tempo % grouplen) / grouplen);
  }
  t1 = gettime();
  printf("Tim2= %lu after %lu clocks\n", r, t1 - t0);

  t0 = gettime();
  for (i = 1; i++; ) {
    r = (((unsigned long long)ticks * tempo) / grouplen);
  }
  t1 = gettime();
  printf("Cast= %lu after %lu clocks\n", r, t1 - t0);

  return(0);
}
