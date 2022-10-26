#include "dev/timer.h"
#include "util/asm_wrappers.h"
#include <dev/speaker.h>

 static void __k_dev_speaker__start(uint32_t nFrequence) {
 	uint32_t Div;
 	uint8_t tmp;
 
        //Set the PIT to the desired frequency
 	Div = 1193180 / nFrequence;
 	outb(0x43, 0xb6);
 	outb(0x42, (uint8_t) (Div) );
 	outb(0x42, (uint8_t) (Div >> 8));
 
        //And play the sound using the PC speaker
 	tmp = inb(0x61);
  	if (tmp != (tmp | 3)) {
 		outb(0x61, tmp | 3);
 	}
 }
 
void __k_dev_speaker_end(){
 	uint8_t tmp = inb(0x61) & 0xFC;
 	outb(0x61, tmp);
}

void k_dev_speaker_beep(uint32_t freq, uint32_t dur){
    __k_dev_speaker__start(freq);
    k_sleep(dur);
    __k_dev_speaker_end();
}

