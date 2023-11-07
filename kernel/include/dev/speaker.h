#ifndef __K_DEV_SPEAKER_H
#define __K_DEV_SPEAKER_H

#include <stdint.h>

#define KDMKTONE  0
#define KIOCSOUND 1

void k_dev_speaker_beep(uint32_t freq, uint32_t dur);
void k_dev_speaker_init();
void k_dev_speaker_start(uint32_t nFrequence);
void k_dev_speaker_end();

#endif
