#include <efi.h>
#include <efilib.h>

#include <config.h>


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

    UINTN entriesAmount = 0;
    UINTN mapKey = 0;
    UINTN descSize = 0;
    UINT32 descVer = 0;

    EFI_MEMORY_DESCRIPTOR * desc = LibMemoryMap (&entriesAmount, &mapKey, &descSize, &descVer);
    
    Print(L"Found %d mmap entries. Usable:\r\n", entriesAmount);
    for(int i=0; i<entriesAmount; i++){
        if(desc->Type == EfiConventionalMemory){
            Print(L"-- 0x%x - 0x%x\r\n", desc->PhysicalStart, desc->PhysicalStart + desc->NumberOfPages * 0x1000);
        }
        desc = NextMemoryDescriptor (desc, descSize);
    }
    
    Print(L"\r\nFramebuffer address: 0x%x (mode %d %dx%d)\r\n", gop->Mode->FrameBufferBase, gop->Mode->Mode, gop->Mode->Info->HorizontalResolution, gop->Mode->Info->VerticalResolution);

    uefi_call_wrapper(BS->Stall, 1, 2000000);

    Print(L"Press any key for reboot.\r\n");

    while (uefi_call_wrapper(ST->ConIn->ReadKeyStroke, 2, ST->ConIn, &Key) == EFI_NOT_READY) ;
 
    return EFI_SUCCESS;
}