/*
 * Test DOSMid's sysex handling using reverb manipulation
 * Copyright (C) 2015 Mateusz Viste
 *
 * compatible with Roland devices, and supposedly SoundBlaster AWE cards (but
 * I couldn't get it working using the AWE32 dev pack, so...)
 *
 * *** Reverb Sysex macro ***
 *
 * F0 41 10 42 12 40 01 30 XX CS F7
 *
 * Where XX denotes the Reverb variation to be selected, and CS denotes the
 * Roland checksum value. The valid values for XX are:
 *  0 - Room 1
 *  1 - Room 2
 *  2 - Room 3
 *  3 - Hall 1
 *  4 - Hall 2
 *  5 - Plate
 *  6 - Delay
 *  7 - Panning Delay
 */

#include <stdio.h>   /* puts() */
#include <unistd.h>  /* sleep() */

#include "..\outdev.h"


#define DEVICE     DEV_MPU401
#define DEVICEPORT 0x330


int main(void) {
  int i, x;
  unsigned char sysexbuff[] = {0xf0, 0x41, 0x10, 0x42, 0x12, 0x40, 0x01, 0x30, 0x00, 0x00, 0xf7};

  if (dev_init(DEVICE, DEVICEPORT, NULL) != 0) {
    puts("dev_init() failure");
    return(1);
  }

  for (i = 0; i < 8; i += 1) {
    /* set the desired reverb effect */
    sysexbuff[8] = i;
    /* compute the roland checksum on [5],[6],[7],[8] */
    sysexbuff[9] = 0;
    for (x = 5; x < 9; x++) {
      sysexbuff[9] += sysexbuff[x];
      sysexbuff[9] &= 127;
    }
    sysexbuff[9] = 128 - sysexbuff[9];
    /* */
    printf("step %d/%d: %02X %02X %02X %02X %02X %02X %02X %02X %02X %02X %02X\n", i+1, 8, sysexbuff[0], sysexbuff[1], sysexbuff[2], sysexbuff[3], sysexbuff[4], sysexbuff[5], sysexbuff[6], sysexbuff[7], sysexbuff[8], sysexbuff[9], sysexbuff[10]);
    dev_sysex(0, sysexbuff, 11);
    sleep(2);
    dev_noteon(0, 62, 120);
    sleep(1);
    dev_noteon(0, 63, 120);
    sleep(1);
    dev_noteon(0, 64, 120);
    sleep(1);
    dev_noteoff(0, 62);
    dev_noteoff(0, 63);
    dev_noteoff(0, 64);
  }

  dev_close();

  return(0);
}
