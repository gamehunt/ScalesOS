#include "dev/vt.h"
#include "fs/pipe.h"
#include "fs/vfs.h"
#include "int/irq.h"
#include "int/isr.h"
#include "int/pic.h"
#include "kernel.h"
#include "stdio.h"
#include <dev/ps2.h>
#include <util/asm_wrappers.h>
#include <util/log.h>

#define PS2_DATA 0x60
#define PS2_STATUS 0x64
#define PS2_COMMAND 0x64

#define PS2_DISABLE_PORT1 0xA7
#define PS2_DISABLE_PORT2 0xAD

#define PS2_ENABLE_PORT1 0xAE
#define PS2_ENABLE_PORT2 0xA8

#define PS2_READ_CONFIG 0x20
#define PS2_WRITE_CONFIG 0x60

#define PS2_MOUSE_WRITE 0xD4

#define PS2_CFG_PORT1_INTERRUPT (1 << 0)
#define PS2_CFG_PORT2_INTERRUPT (1 << 1)
#define PS2_CFG_PORT1_CLOCK (1 << 4)
#define PS2_CFG_PORT2_CLOCK (1 << 5)
#define PS2_CFG_TRANSLATION (1 << 6)

#define PS2_KBD_SET_SCANCODE 0xF0

#define PS2_MOUSE_SET_REMOTE 0xF0
#define PS2_MOUSE_DEVICE_ID 0xF2
#define PS2_MOUSE_SAMPLE_RATE 0xF3
#define PS2_MOUSE_DATA_ON 0xF4
#define PS2_MOUSE_DATA_OFF 0xF5
#define PS2_MOUSE_SET_DEFAULTS 0xF6

#define PS2_MOUSE_PACKETS_PER_PIPE 128

static fs_node_t* mouse_pipe;
static fs_node_t* kbd_pipe;

static uint8_t __k_dev_ps2_wait_input(void) {
    uint64_t timeout = 100000UL;
    while (--timeout) {
        if (!(inb(PS2_STATUS) & (1 << 1)))
            return 0;
    }
    return 1;
}

static uint8_t __k_dev_ps2_wait_output(void) {
    uint64_t timeout = 100000UL;
    while (--timeout) {
        if (inb(PS2_STATUS) & (1 << 0))
            return 0;
    }
    return 1;
}

static void __k_dev_ps2_write_command(uint8_t cmd) {
    __k_dev_ps2_wait_input();
    outb(PS2_COMMAND, cmd);
}

static void __k_dev_ps2_write_byte(uint8_t b) {
    __k_dev_ps2_wait_input();
    outb(PS2_DATA, b);
}

static uint8_t __k_dev_ps2_read_byte(void) {
    __k_dev_ps2_wait_output();
    return inb(PS2_DATA);
}

static void __k_dev_ps2_handle_keyboard(uint8_t byte) {
    k_fs_vfs_write(kbd_pipe, 0, 1, &byte);
	k_dev_vt_handle_scancode(byte);
}

static uint8_t mouse_packet_counter = 0;

typedef struct ps2_mouse_packet{
    uint8_t data;
    uint8_t mx;
    uint8_t my;
}ps_mouse_packet_t;

static ps_mouse_packet_t current_mouse_packet;

static void __k_dev_ps2_handle_mouse(uint8_t byte) {
    switch(mouse_packet_counter){
        case 0:
            current_mouse_packet.data = byte;
        case 1:
            current_mouse_packet.mx   = byte;
        case 2:
            current_mouse_packet.my   = byte;
    }
    mouse_packet_counter++;
    if(mouse_packet_counter >= 3){
        mouse_packet_counter = 0;
        k_fs_vfs_write(mouse_pipe, 0, sizeof(ps_mouse_packet_t), (uint8_t*) &current_mouse_packet);
    }
}

static interrupt_context_t* __irq1_handler(interrupt_context_t* ctx) {
    uint8_t data_byte = inb(PS2_DATA);
    k_int_pic_eoi(1);
    __k_dev_ps2_handle_keyboard(data_byte);
    return ctx;
}

static interrupt_context_t* __irq12_handler(interrupt_context_t* ctx) {
    uint8_t data_byte = inb(PS2_DATA);
    k_int_pic_eoi(12);
    __k_dev_ps2_handle_mouse(data_byte);
    return ctx;
}

static uint8_t __k_dev_ps2_write_kbd(uint8_t data) {
    __k_dev_ps2_write_byte(data);
    return __k_dev_ps2_read_byte();
}

static uint8_t __k_dev_ps2_write_mouse(uint8_t data) {
    __k_dev_ps2_write_command(PS2_MOUSE_WRITE);
    __k_dev_ps2_write_byte(data);
    return __k_dev_ps2_read_byte();
}

static void __k_dev_ps2_clear_buffer(uint32_t timeout) {
    while ((inb(PS2_STATUS) & 1) && timeout > 0) {
        timeout--;
        inb(PS2_DATA);
    }
}

K_STATUS k_dev_ps2_init() {
    kbd_pipe   = k_fs_pipe_create(128);
    mouse_pipe = k_fs_pipe_create(PS2_MOUSE_PACKETS_PER_PIPE * sizeof(ps_mouse_packet_t));

	kbd_pipe->mode   |= O_NOBLOCK;
	mouse_pipe->mode |= O_NOBLOCK;

    k_fs_vfs_mount_node("/dev/kbd", kbd_pipe);
    k_fs_vfs_mount_node("/dev/mouse", mouse_pipe);

    __k_dev_ps2_write_command(PS2_DISABLE_PORT1);
    __k_dev_ps2_write_command(PS2_DISABLE_PORT2);

    __k_dev_ps2_write_command(PS2_READ_CONFIG);
    uint8_t cfg = __k_dev_ps2_read_byte();
    cfg |= (PS2_CFG_PORT1_INTERRUPT | PS2_CFG_PORT2_INTERRUPT |
            PS2_CFG_TRANSLATION);
    __k_dev_ps2_write_command(PS2_WRITE_CONFIG);
    __k_dev_ps2_write_byte(cfg);

    __k_dev_ps2_write_command(PS2_ENABLE_PORT1);
    __k_dev_ps2_write_command(PS2_ENABLE_PORT2);

    __k_dev_ps2_write_kbd(PS2_KBD_SET_SCANCODE);
    __k_dev_ps2_write_kbd(2);

    __k_dev_ps2_write_mouse(PS2_MOUSE_SET_DEFAULTS);
    __k_dev_ps2_write_mouse(PS2_MOUSE_DATA_ON);

    k_int_irq_setup_handler(1,  __irq1_handler);
    k_int_irq_setup_handler(12, __irq12_handler);

    __k_dev_ps2_clear_buffer(0x1000);

    k_int_pic_unmask_irq(1);
    k_int_pic_unmask_irq(12);

    return K_STATUS_OK;
}
