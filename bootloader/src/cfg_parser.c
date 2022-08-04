#include <boot.h>
#include <efi_wrappers.h>
#include <cfg_parser.h>

static BOOTL_UINT ascii_strlen(BOOTL_CHAR* buffer){
    BOOTL_UINT res = 0;
    while(buffer[res] != '\0'){
        res++;
    }
    return res;
}

static void ascii_strcpy_e(BOOTL_CHAR* a, BOOTL_CHAR* b, BOOTL_UINT start_pos, BOOTL_UINT size){
    for(BOOTL_UINT i = 0; i < size; i++){
        a[i] = b[start_pos + i];
    }
    a[start_pos + size] = '\0';
}

static BOOTL_UINT ascii_substr_len(BOOTL_CHAR* buffer, BOOTL_UINT buff_size, BOOTL_UINT start_pos, BOOTL_CHAR delim){
    BOOTL_UINT res = start_pos;
    while(res < buff_size && buffer[res] != delim){
        res++;
    }
    return res - start_pos;
}

BOOTL_CHAR** ascii_split(BOOTL_CHAR* Buffer, BOOTL_UINT* Amount, BOOTL_CHAR delimiter){
    BOOTL_UINT size = ascii_strlen(Buffer);
    BOOTL_UINT start_pos = 0;
    BOOTL_UINT line = 0;
    BOOTL_CHAR** result = 0;
    BOOTL_UINT substr_size = 0;
    while((substr_size = ascii_substr_len(Buffer, size, start_pos, delimiter))){
        line++;
        if(line == 1){
            result = AllocatePool(sizeof(BOOTL_CHAR*));
        }else{
            result = ReallocatePool(result, sizeof(BOOTL_CHAR*) * (line - 1), sizeof(BOOTL_CHAR*) * line);
        }
        result[line - 1] = AllocateZeroPool(substr_size + 1);
        ascii_strcpy_e(result[line - 1], Buffer, start_pos, substr_size);
        start_pos += substr_size + 1;
    }
    if(!line){
        line = 1;
        result = AllocatePool(sizeof(BOOTL_CHAR*));
        result[0] = AllocateZeroPool(size + 1);
        ascii_strcpy_e(result[0], Buffer, 0, size);
    }
    *Amount = line;
    return result;
}

cfg_node_t* ParseCfg(EFI_HANDLE handle, BOOTL_WCHAR* path, BOOTL_UINT* size) {
    EFI_STATUS Status;
    UINT8* Buffer;
    UINT64 Size;

    SAFE_CALL_R(ReadFile(handle, path, &Buffer, &Size), 0);

    BOOTL_UINT _size;
    BOOTL_CHAR** lines = ascii_split(Buffer, &_size, '\n');

    FreePool(Buffer);

    cfg_node_t* res = 0;

    BOOTL_UINT tmp_size = 0;

    for(BOOTL_UINT i=0;i<_size;i++){
        BOOTL_UINT tmpsz = 0;
        BOOTL_CHAR** values = ascii_split(lines[i], &tmpsz, '=');

        if(values[0][0] == '#'){
            for(BOOTL_UINT j=0; j<tmpsz; j++){
                FreePool(values[j]);
            }
            FreePool(values);
            continue;
        }
        
        if(tmpsz != 2){
            if(res){
                FreePool(res);
            }
            for(int j=0; j<tmpsz; j++){
                FreePool(values[j]);
            }
            FreePool(values);
            for(int z=0; z<_size;z++){
                FreePool(lines[z]);
            }
            FreePool(lines);
            return 0;
        }

        if(res){
            res = ReallocatePool(res, sizeof(cfg_node_t) * tmp_size, sizeof(cfg_node_t) * (tmp_size + 1));
        }else{
            res = AllocatePool(sizeof(cfg_node_t));
        }

        BOOTL_UINT name_size = ascii_strlen(values[0]);
        res[tmp_size].name = AllocateZeroPool(name_size + 1);
        ascii_strcpy_e(res[tmp_size].name, values[0], 0, name_size);

        BOOTL_UINT val_size = ascii_strlen(values[1]);
        res[tmp_size].value = AllocateZeroPool(val_size + 1);
        ascii_strcpy_e(res[tmp_size].value, values[1], 0, val_size);

        tmp_size++;

        for(int j=0; j<tmpsz; j++){
            FreePool(values[j]);
        }
        FreePool(values);
    }

    for(int z=0; z<_size;z++){
        FreePool(lines[z]);
    }
    FreePool(lines);

    *size = tmp_size;
    return res;
}