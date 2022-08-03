#include <efi.h>
#include <efilib.h>

#include <efi_wrappers.h>

EFI_FILE_HANDLE OpenVolume(EFI_HANDLE image)
{
  EFI_LOADED_IMAGE *loaded_image = NULL;                  
  EFI_GUID lipGuid = EFI_LOADED_IMAGE_PROTOCOL_GUID;                            
  uefi_call_wrapper(BS->HandleProtocol, 3, image, &lipGuid, (void **) &loaded_image);
  return LibOpenRoot(loaded_image->DeviceHandle);
}

EFI_STATUS  CloseHandle(EFI_FILE_HANDLE handle){
  return uefi_call_wrapper(handle->Close, 1, handle);
}

EFI_STATUS ReadFile(EFI_HANDLE root, CHAR16* Path, UINT8** Buffer, UINT64* ReadTotal){
    EFI_STATUS Status;

    EFI_FILE_HANDLE Vol = OpenVolume(root);
    EFI_FILE_HANDLE FileHandle;
    EFI_FILE_INFO   *FileInfo;

    Status = uefi_call_wrapper(Vol->Open, 5, Vol, &FileHandle, Path, EFI_FILE_MODE_READ, EFI_FILE_READ_ONLY | EFI_FILE_HIDDEN | EFI_FILE_SYSTEM);
    if(EFI_ERROR(Status)){
      return Status;
    }

    FileInfo = LibFileInfo(FileHandle);
    *ReadTotal  = FileInfo->FileSize;
    FreePool(FileInfo);

    *Buffer = AllocatePool(*ReadTotal);

    Status = uefi_call_wrapper(FileHandle->Read, 3, FileHandle, ReadTotal, *Buffer);
    if(EFI_ERROR(Status)){
      return Status;
    }

    Status = CloseHandle(FileHandle);
    if(EFI_ERROR(Status)){
      return Status;
    }
    
    Status = CloseHandle(Vol);
    if(EFI_ERROR(Status)){
      return Status;
    }

    return EFI_SUCCESS;
}