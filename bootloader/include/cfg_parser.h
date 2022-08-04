#ifndef _S_BOOTL_CFG_PARSER_H
#define _S_BOOTL_CFG_PARSER_H

typedef struct{
    char* name;
    char* value;
} cfg_node_t;

cfg_node_t* ParseCfg(EFI_HANDLE handle, BOOTL_WCHAR* path, BOOTL_UINT* size);

#endif