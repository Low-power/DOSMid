
#include <unistd.h>

#include "..\opl.h"

#define PORT 0x388



int main(void) {
  opl_init(PORT);

  opl_midi_changeprog(0, 0);
  opl_midi_changeprog(1, 0);
  opl_midi_changeprog(2, 0);

  opl_midi_noteon(PORT, 0, 50, 120);
  sleep(1);
  opl_midi_noteoff(PORT, 0, 50);
  opl_midi_noteon(PORT, 1, 55, 120);
  sleep(1);
  opl_midi_noteoff(PORT, 0, 55);
  opl_midi_noteon(PORT, 2, 60, 120);
  sleep(1);
  opl_midi_noteoff(PORT, 0, 60);
  opl_midi_noteon(PORT, 2, 65, 120);
  sleep(1);

  opl_close(PORT);
  return(0);
}
