#ifndef cms_h_sentinel
#define cms_h_sentinel

#define MAX_CMS_CHANNELS 10

typedef struct {
        unsigned char note;
        unsigned char priority;
        unsigned char ch;
        unsigned char voice;
	unsigned char velocity;
} mid_channel;

void cms_reset(unsigned short int port);

void cms_pitchwheel(unsigned short oplport, int channel, int pitchwheel);

void cms_noteon(unsigned char channel, unsigned char note, unsigned char velocity);

void cms_noteoff(unsigned char channel, unsigned char note);

void cms_tick(void);

void cms_controller(unsigned char channel, unsigned char id, unsigned char val);

#endif
