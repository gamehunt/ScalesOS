#ifndef _S_BOOT_H
#define _S_BOOT_H

typedef uintptr_t BOOTL_ADDR;
typedef uint32_t  BOOTL_UINT;
typedef uint16_t  BOOTL_USHORT;
typedef uint8_t   BOOTL_BYTE;

enum BOOTL_STATUS
{
    BOOTL_SUCCESS,
    BOOTL_FAIL
};

#define S_BOOTLOADER_MAGIC 0x52415752 // RAWR

typedef struct{
    BOOTL_ADDR    addr;
    BOOTL_USHORT  width;
    BOOTL_USHORT  height;
    BOOTL_UINT    size;
}framebuffer_info_t;

typedef struct{
    BOOTL_BYTE  type;
    BOOTL_ADDR   phys_start;
    BOOTL_UINT   size;
}mmap_descriptor_t;

typedef struct{
    mmap_descriptor_t* descriptors;
    BOOTL_UINT         size;
}mmap_info_t;

typedef struct{
    BOOTL_UINT          magic;
    char*               cmdline;
    mmap_info_t*        mmap;
    framebuffer_info_t* framebuffer;
}bootinfo_t;

#endif