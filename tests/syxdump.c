
#include <stdio.h>
#include <stdlib.h>

#include "..\syx.c"


int main(int argc, char **argv) {
  unsigned char *buff;
  int syxlen;
  FILE *fd;
  int i;

  if (argc != 2) {
    puts("Usage: syxdump file.syx");
    return(1);
  }

  buff = malloc(8192);
  if (buff == NULL) {
    puts("Out of memory");
    return(1);
  }

  fd = fopen(argv[1], "rb");
  if (fd == NULL) {
    free(buff);
    puts("Failed to open file");
    return(1);
  }

  for (;;) {
    syxlen = syx_fetchnext(fd, buff, 8192);
    if (syxlen < 0) {
      printf("Error: syx_fetchnext() returned %d\n", syxlen);
      break;
    }
    if (syxlen == 0) {
      puts("[EOF]");
      break;
    }
    for (i = 0; i < syxlen; i++) {
      printf("%02X ", buff[i]);
    }
    puts("");
  }

  fclose(fd);
  free(buff);

  return(0);
}
