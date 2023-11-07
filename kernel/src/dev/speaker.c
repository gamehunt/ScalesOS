#include "errno.h"
#include "fs/vfs.h"
#include "mem/heap.h"
#include "proc/process.h"
#include "util/asm_wrappers.h"
#include "util/log.h"
#include <dev/speaker.h>

void k_dev_speaker_start(uint32_t nFrequence) {
 	uint32_t div;
 	uint8_t tmp;
 
        //Set the PIT to the desired frequency
 	div = 1193180 / nFrequence;
 	outb(0x43, 0xb6);
 	outb(0x42, (uint8_t) (div) );
 	outb(0x42, (uint8_t) (div >> 8));

        //And play the sound using the PC speaker
 	tmp = inb(0x61);
  	if (tmp != (tmp | 3)) {
 		outb(0x61, tmp | 3);
 	}
 }
 
void k_dev_speaker_end(){
 	uint8_t tmp = inb(0x61) & 0xFC;
 	outb(0x61, tmp);
}

void k_dev_speaker_beep(uint32_t freq, uint32_t dur){
    k_dev_speaker_start(freq);
	k_proc_process_sleep(k_proc_process_current(), dur);
    k_dev_speaker_end();
}

typedef struct {
	uint32_t freq;
	uint32_t dur;
} __beep;

void __beep_routine(__beep* beep) {
	uint32_t freq = beep->freq;
	uint32_t dur  = beep->dur;
	k_free(beep);
	k_dev_speaker_beep(freq, dur);
}

int __k_dev_speaker_ioctl(fs_node_t* node UNUSED, unsigned int req, void* arg) {
	__beep* beep = NULL;
	switch(req) {
		case KIOCSOUND:
			if(!arg) {
				k_dev_speaker_end();
			} else {
				k_dev_speaker_start((uint32_t) arg);
			}
			return 0;
		case KDMKTONE:
			beep = malloc(sizeof(__beep));
			beep->dur  = (uint32_t) arg;
			beep->freq = 300;
			k_proc_process_create_tasklet("pcspkr_tone", (uintptr_t) &__beep_routine, beep);
			return 0;
		default:
			return -EINVAL;
	}
}

fs_node_t* __k_dev_speaker_create_device() {
	fs_node_t* node = k_fs_vfs_create_node("pcspkr");
	node->fs.ioctl = __k_dev_speaker_ioctl;
	return node;
}

void k_dev_speaker_init() {
	k_fs_vfs_mount_node("/dev/pcspkr", __k_dev_speaker_create_device());
}
