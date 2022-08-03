#ifndef _S_BOOTL_INTERNALS_H
#define _S_BOOTL_INTERNALS_H

#define SAFE_CALL(x)        \
    Status = x;             \
    if(EFI_ERROR(Status)) { \
        return Status;      \
    }                       

EFI_FILE_HANDLE OpenVolume(EFI_HANDLE image);
EFI_STATUS      CloseHandle(EFI_FILE_HANDLE handle);
EFI_STATUS      ReadFile(EFI_HANDLE root, CHAR16* Path, UINT8** Buffer, UINT64* ReadTotal);

#endif