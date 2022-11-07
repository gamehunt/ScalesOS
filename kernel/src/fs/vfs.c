#include <fs/vfs.h>
#include "kernel.h"
#include "mem/heap.h"
#include "util/path.h"
#include "util/types/tree.h"
#include <string.h>
#include <util/log.h>

typedef struct{
    char alias[128];
    mount_callback mnt;
}fs_descriptor_t;

static tree_t*           vfs_tree    = 0;
static fs_descriptor_t*  filesystems = 0;
uint32_t                 fs_count    = 0;

static vfs_entry_t* __k_fs_vfs_root_entry(){
    return ((vfs_entry_t*)vfs_tree->root->value);
}

static fs_node_t* __k_fs_vfs_root_node(){
    return __k_fs_vfs_root_entry()->node;
}

static fs_node_t* __k_fs_vfs_find_node(const char* path){
    tree_node_t* cur_node  = vfs_tree->root;
    uint32_t len = k_util_path_length(path);
    char* filename = k_util_path_filename(path);
    for(uint32_t i = 0; i < len; i++){
        char* part = k_util_path_segment(path, i);
        if(!part){
            continue;
        }
        uint8_t f = 0;
        for(uint32_t j = 0; j < cur_node->child_count; j++){
            vfs_entry_t* entry = (vfs_entry_t*)cur_node->childs[j]->value;
            if(!strcmp(entry->name, part)){
                cur_node = cur_node->childs[j];
                f = 1;
                break;
            }
        }
        if(!f){
            fs_node_t* fsnode = ((vfs_entry_t*)cur_node->value)->node;
            while(i < len && fsnode){
                k_free(part);
                char* part = k_util_path_segment(path, i);
                if(!part){
                    break;
                }
                fsnode = k_fs_vfs_finddir(fsnode, part);
                i++;
            }
            if(strcmp(filename, fsnode->name)){
                fsnode = 0;
            }
            k_free(filename);
            return fsnode;
        }
        k_free(part);
    }
    if(filename){
        k_free(filename);
    }
    return ((vfs_entry_t*)cur_node->value)->node;
}

static vfs_entry_t* __k_fs_vfs_get_entry(const char* path, uint8_t create){
    tree_node_t* cur_node  = vfs_tree->root;
    uint32_t len = k_util_path_length(path);
    for(uint32_t i = 0; i < len; i++){
        char* part = k_util_path_segment(path, i);
        if(!part){
            continue;
        }
        uint8_t f = 0;
        for(uint32_t i = 0; i < cur_node->child_count; i++){
            vfs_entry_t* entry = (vfs_entry_t*)cur_node->childs[i]->value;
            if(!strcmp(entry->name, part)){
                cur_node = cur_node->childs[i];
                f = 1;
                break;
            }
        }
        if(!f){
            if(!create){
                k_free(part);
                return 0;
            }
            vfs_entry_t* new_entry = k_fs_vfs_create_entry(part);
            tree_node_t* node      = tree_create_node(new_entry);
            tree_insert_node(vfs_tree, node, cur_node);
            cur_node = node;
        }
        k_free(part);
    }
    return cur_node->value;
}

void  k_fs_vfs_register_fs(const char* alias, mount_callback fs){
    EXTEND(filesystems, fs_count, sizeof(fs_descriptor_t));
    strcpy(filesystems[fs_count - 1].alias, alias);
    filesystems[fs_count - 1].mnt = fs;
}

static mount_callback __k_fs_vfs_get_mount_callback(const char* alias){
    for(uint32_t i = 0; i < fs_count; i++){
        if(!strcmp(filesystems[i].alias, alias)){
            return filesystems[i].mnt;
        }
    }
    return 0;
}

vfs_entry_t* k_fs_vfs_map_path(const char* path){
    return __k_fs_vfs_get_entry(path, 1);
}

K_STATUS k_fs_vfs_init(){
    vfs_tree = tree_create();
    vfs_entry_t* root = k_fs_vfs_create_entry("[root]");
    tree_node_t* node = tree_create_node(root);
    tree_set_root(vfs_tree, node);
    return K_STATUS_OK;
}

vfs_entry_t* k_fs_vfs_create_entry(const char* name){
    vfs_entry_t* e = k_calloc(1, sizeof(vfs_entry_t));
    strcpy(e->name, name);
    return e;
}

fs_node_t* k_fs_vfs_create_node(const char* name){
    fs_node_t* node = k_calloc(1, sizeof(fs_node_t));
    strcpy(node->name, name);
    return node;
}

uint32_t    k_fs_vfs_read(fs_node_t* node, uint32_t offset, uint32_t size, uint8_t* buffer){
    if(!node->fs.read){
        return 0;
    }

    return node->fs.read(node, offset, size, buffer);
}
uint32_t    k_fs_vfs_write(fs_node_t* node, uint32_t offset, uint32_t size, uint8_t* buffer){
    if(!node->fs.write){
        return 0;
    }

    return node->fs.write(node, offset, size, buffer);
}

fs_node_t*  k_fs_vfs_open(const char* path){
    if(!vfs_tree){
        return 0;
    }

    fs_node_t* node = __k_fs_vfs_find_node(path);
    if(node && node->fs.open){
        node->fs.open(node);
    }
    return node;
}

void k_fs_vfs_close(fs_node_t* node){
    if(node){
        if(node->fs.close){
            node->fs.close(node);
        }
        k_free(node);
    }
}

struct dirent*   k_fs_vfs_readdir(fs_node_t* node, uint32_t index){
    if(!node->fs.readdir){
        return 0;
    }

    return node->fs.readdir(node, index);
}

fs_node_t*   k_fs_vfs_finddir(fs_node_t* node, const char* path){
    if(!node->fs.finddir){
        return 0;
    }

    return node->fs.finddir(node, path);
}

K_STATUS k_fs_vfs_mount_node(const char* path, fs_node_t* fsroot){
    if(!vfs_tree){
        return K_STATUS_ERR_GENERIC;
    }

    vfs_entry_t* mountpoint = __k_fs_vfs_get_entry(path, 1);
    if(mountpoint){
        mountpoint->node = fsroot;
        return K_STATUS_OK;
    }

    return K_STATUS_ERR_GENERIC;
}

K_STATUS  k_fs_vfs_mount(const char* path, const char* device, const char* type){
    mount_callback mount = __k_fs_vfs_get_mount_callback(type);
    if(mount){
        fs_node_t* node = mount(path, device);
        if(node){
            return k_fs_vfs_mount_node(path, node);
        }
    }
    return K_STATUS_ERR_GENERIC;
}

void __k_d_fs_vfs_print_entry(vfs_entry_t* entry, uint8_t depth){
    char buffer[1024];
    char prefix[512];
    for(int i = 0; i < depth*2 - 1; i++){
        prefix[i] = '-';
        prefix[i + 1] = '-';
    }
    prefix[depth*2 - 1] = '>';
    prefix[depth*2] = '\0';
    sprintf(buffer, "%s%s", prefix, entry->name);
    k_debug(buffer);
}

void __k_d_fs_vfs_print_node(tree_node_t* node, uint8_t depth){
    __k_d_fs_vfs_print_entry(node->value, depth);
    for(uint32_t i = 0; i < node->child_count; i++){
        __k_d_fs_vfs_print_node(node->childs[i], depth + 1);
    }
}

void  k_d_fs_vfs_print(){
    __k_d_fs_vfs_print_node(vfs_tree->root, 0);
}