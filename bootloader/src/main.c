#include <efi.h>
#include <efilib.h>

#include <config.h>

#include <boot.h>
#include <efi_wrappers.h>

EFI_STATUS
EFIAPI
efi_main (EFI_HANDLE ImageHandle, EFI_SYSTEM_TABLE *SystemTable)
{
    EFI_STATUS Status;
    EFI_INPUT_KEY Key;

    EFI_GRAPHICS_OUTPUT_PROTOCOL *gop;

    InitializeLib(ImageHandle, SystemTable);

    Status = LibLocateProtocol(&GraphicsOutputProtocol, (void **)&gop);
	if (EFI_ERROR(Status)) {
		Print(L"Could not locate GOP: %r\r\n", Status);
		return Status;
	}

	if (!gop) {
		Print(L"LocateProtocol(GOP, &gop) returned %r but GOP is NULL\r\n", Status);
		return EFI_UNSUPPORTED;
	}

    uefi_call_wrapper(gop->SetMode, 2, gop, gop->Mode->Mode);

    uefi_call_wrapper(ST->ConIn->Reset, 2, ST->ConIn, FALSE);
    uefi_call_wrapper(ST->ConOut->Reset, 2, ST->ConOut, FALSE);

    Print(L"Started %s v%s\r\n", BOOTLOADER_NAME, BOOTLOADER_VERSION);

    bootinfo_t* bootinfo          = AllocateZeroPool (sizeof(bootinfo_t));

    bootinfo->framebuffer         = AllocatePool (sizeof(framebuffer_info_t));
    bootinfo->framebuffer->addr   = gop->Mode->FrameBufferBase;
    bootinfo->framebuffer->width  = gop->Mode->Info->HorizontalResolution;
    bootinfo->framebuffer->height = gop->Mode->Info->VerticalResolution;
    bootinfo->framebuffer->size   = gop->Mode->FrameBufferSize;

    Print(L"Framebuffer address: 0x%x (mode %d %dx%d)\r\n", gop->Mode->FrameBufferBase, gop->Mode->Mode, gop->Mode->Info->HorizontalResolution, gop->Mode->Info->VerticalResolution);

    UINTN entriesAmount = 0;
    UINTN mapKey = 0;
    UINTN descSize = 0;
    UINT32 descVer = 0;

    EFI_MEMORY_DESCRIPTOR * desc = LibMemoryMap (&entriesAmount, &mapKey, &descSize, &descVer);

    bootinfo->mmap              = AllocatePool (sizeof(mmap_info_t));
    bootinfo->mmap->size        = entriesAmount;
    bootinfo->mmap->descriptors = AllocatePool(sizeof(mmap_descriptor_t) * entriesAmount);
    
    Print(L"Found %d mmap entries. Usable:\r\n", entriesAmount);
    for(int i=0; i<entriesAmount; i++){
        if(desc->Type == EfiConventionalMemory){
            Print(L"-- 0x%08X - 0x%08X\r\n", desc->PhysicalStart, desc->PhysicalStart + desc->NumberOfPages * 0x1000);
        }
        bootinfo->mmap->descriptors[i].type       = desc->Type;
        bootinfo->mmap->descriptors[i].phys_start = desc->PhysicalStart;
        bootinfo->mmap->descriptors[i].size       = desc->NumberOfPages;
        desc = NextMemoryDescriptor (desc, descSize);
    }

    UINT8* Buffer;
    UINT64 Size;

    Status = ReadFile(ImageHandle, L"\\EFI\\BOOT\\boot.cfg", &Buffer, &Size);
    if (EFI_ERROR(Status)) {
		Print(L"Failed to read: %d", Status);
	}else{
        Print(L"Read: %d", Size);
    }

    FreePool(Buffer);

    Pause();

    Status = uefi_call_wrapper(BS->ExitBootServices, 2, ImageHandle, mapKey);
    if (EFI_ERROR(Status)) {
		Print(L"Failed to exit boot services: %d", Status);
		return Status;
	}

    return EFI_SUCCESS;
}