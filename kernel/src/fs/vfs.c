#include <fs/vfs.h>
#include "dirent.h"
#include "errno.h"
#include "kernel.h"
#include "mem/heap.h"
#include "mem/paging.h"
#include "util/path.h"
#include "types/list.h"
#include "types/tree.h"
#include <stdio.h>
#include <string.h>
#include <util/log.h>

typedef struct{
    char alias[128];
    mount_callback mnt;
}fs_descriptor_t;

static tree_t*  vfs_tree    = 0;
static list_t*  filesystems = 0;

static vfs_entry_t* __k_fs_vfs_root_entry(){
    return ((vfs_entry_t*)vfs_tree->root->value);
}

static fs_node_t* __k_fs_vfs_root_node(){
    return __k_fs_vfs_root_entry()->node;
}

static fs_node_t* __k_fs_vfs_find_node(const char* path, uint16_t flags);

static fs_node_t* __k_fs_vfs_follow_symlink(fs_node_t* node) {
	if(!(node->flags & VFS_SYMLINK)) {
		return 0;
	}
	char buff[4096];
	k_fs_vfs_readlink(node, buff, 4096);
	return __k_fs_vfs_find_node(buff, 0);
}

static void __k_fs_vfs_maybe_free(fs_node_t* node) {
	if(!node->mountpoint) {
		k_free(node);
	}
}

static fs_node_t* __k_fs_vfs_find_node(const char* path, uint16_t flags){
    tree_node_t* cur_node  = vfs_tree->root;
    uint32_t len   = k_util_path_length(path);
    char* filename = k_util_path_filename(path);
    for(uint32_t i = 0; i < len; i++){
        char* part = k_util_path_segment(path, i);
        if(!part){
            continue;
        }
        uint8_t f = 0;
        for(uint32_t j = 0; j < cur_node->children->size; j++){
            vfs_entry_t* entry = (vfs_entry_t*) ((tree_node_t*)cur_node->children->data[j])->value;
            if(!strcmp(entry->name, part)){
                cur_node = cur_node->children->data[j];
                f = 1;
                break;
            }
        }
        if(!f){
            fs_node_t* fsnode = ((vfs_entry_t*)cur_node->value)->node;
            while(i < len && fsnode){
				fs_node_t* to_free = fsnode;
                k_free(part);
                char* part = k_util_path_segment(path, i);
                if(!part){
                    break;
                }
				if((fsnode->flags & VFS_SYMLINK) && !(flags & O_NOFOLLOW)) {
					fsnode = __k_fs_vfs_follow_symlink(fsnode);
					if(!fsnode) {
						__k_fs_vfs_maybe_free(to_free);
                		k_free(part);
            			k_free(filename);
						return 0;
					}
				}
                fsnode = k_fs_vfs_finddir(fsnode, part);
				__k_fs_vfs_maybe_free(to_free);
                i++;
            }
            if(fsnode && strcmp(filename, fsnode->name)){
				__k_fs_vfs_maybe_free(fsnode);
                fsnode = 0;
            }
            k_free(filename);
			if(fsnode && (fsnode->flags & VFS_SYMLINK) && !(flags & O_NOFOLLOW)) {
				fs_node_t* to_free = fsnode;
				fsnode = __k_fs_vfs_follow_symlink(fsnode);
				__k_fs_vfs_maybe_free(to_free);
			}
            return fsnode;
        }
        k_free(part);
    }
    if(filename){
        k_free(filename);
    }
    return ((vfs_entry_t*)cur_node->value)->node;
}

static struct dirent* __k_fs_virtual_readdir(fs_node_t* node, uint32_t index) {
	if(!IS_VALID_PTR((uint32_t) node->device)) {
		return 0;
	}

	tree_node_t* tnode = ((vfs_entry_t*) node->device)->tree_node;
	if(!IS_VALID_PTR((uint32_t) tnode)) {
		return 0;
	}

	struct dirent* result = malloc(sizeof(struct dirent));

	if(index == 0) {
		strcpy(result->name, ".");
		result->ino  = 0;
		return result;
	}

	if(index == 1) {
		strcpy(result->name, "..");
		result->ino  = 1;
		return result;
	}

	index -= 2;

	if(tnode->children->size > index) {
		vfs_entry_t* child = (vfs_entry_t*) ((tree_node_t*)tnode->children->data[index])->value;
		strcpy(result->name, child->name);
		result->ino = 1;
		return result;
	}

	k_free(result);
	return 0;
}

static fs_node_t* __k_fs_vfs_create_virtual_node(vfs_entry_t* entry) {
	fs_node_t* node = k_fs_vfs_create_node(entry->name);
	node->flags = VFS_DIR | VFS_MAPPER;
	node->device = entry;
	node->mountpoint = entry;
	node->fs.readdir = &__k_fs_virtual_readdir;
	entry->node = node;
	return node;
}

static vfs_entry_t* __k_fs_vfs_get_entry(const char* path, uint8_t create){
	if(!strcmp(path, "/")) {
		return __k_fs_vfs_root_entry();
	}
    tree_node_t* cur_node  = vfs_tree->root;
    uint32_t len = k_util_path_length(path);
    for(uint32_t i = 0; i < len; i++){
        char* part = k_util_path_segment(path, i);
        if(!part){
            continue;
        }
        uint8_t f = 0;
        for(uint32_t i = 0; i < cur_node->children->size; i++){
            vfs_entry_t* entry = (vfs_entry_t*) ((tree_node_t*)cur_node->children->data[i])->value;
            if(!strcmp(entry->name, part)){
                cur_node = cur_node->children->data[i];
                f = 1;
                break;
            }
        }
        if(!f){
            if(!create){
                k_free(part);
                return 0;
            }
            vfs_entry_t* new_entry = k_fs_vfs_create_entry(part, cur_node);
            cur_node = new_entry->tree_node;
        }
        k_free(part);
    }
    return cur_node->value;
}

void  k_fs_vfs_register_fs(const char* alias, mount_callback fs){
	fs_descriptor_t* desc = k_malloc(sizeof(fs_descriptor_t));
	strcpy(desc->alias, alias);
	desc->mnt = fs;
	list_push_back(filesystems, desc);
}

static mount_callback __k_fs_vfs_get_mount_callback(const char* alias){
    for(uint32_t i = 0; i < filesystems->size; i++){
		fs_descriptor_t* desc = filesystems->data[i];
        if(!strcmp(desc->alias, alias)){
            return desc->mnt;
        }
    }
    return 0;
}

vfs_entry_t* k_fs_vfs_map_path(const char* path){
    return __k_fs_vfs_get_entry(path, 1);
}

K_STATUS k_fs_vfs_init(){
    vfs_tree    = tree_create();
	filesystems = list_create();

    vfs_entry_t* root = k_fs_vfs_create_entry("[root]", 0);
    tree_node_t* node = tree_create_node(root);
	root->tree_node = node;
    tree_set_root(vfs_tree, node);

    return K_STATUS_OK;
}


vfs_entry_t* k_fs_vfs_create_entry(const char* name, tree_node_t* parent){
    vfs_entry_t* e = k_calloc(1, sizeof(vfs_entry_t));
    strcpy(e->name, name);
	if(parent) {
		tree_node_t* tree_node = tree_create_node(e);
		e->tree_node = tree_node;
		tree_insert_node(vfs_tree, tree_node, parent);
	}
	__k_fs_vfs_create_virtual_node(e);
    return e;
}

fs_node_t* k_fs_vfs_create_node(const char* name){
    fs_node_t* node = k_calloc(1, sizeof(fs_node_t));
    strcpy(node->name, name);
	node->links = 1;
    return node;
}

fs_node_t* k_fs_vfs_dup(fs_node_t* node) {
	fs_node_t* nnode = k_malloc(sizeof(fs_node_t));
	memcpy(nnode, node, sizeof(fs_node_t));
	return nnode;
}

int32_t k_fs_vfs_read(fs_node_t* node, uint32_t offset, uint32_t size, uint8_t* buffer){
    if(!node->fs.read){
        return -EPERM;
    }

	if(!(node->mode & O_RDONLY)) {
		return -EPERM;
	}

	return node->fs.read(node, offset, size, buffer);
}

int32_t k_fs_vfs_write(fs_node_t* node, uint32_t offset, uint32_t size, uint8_t* buffer){
    if(!node->fs.write){
        return -EPERM;
    }

	if(!(node->mode & O_WRONLY)) {
		return -EPERM;
	}

    return node->fs.write(node, offset, size, buffer);
}

fs_node_t*  k_fs_vfs_open(const char* path, uint16_t mode){
    if(!vfs_tree){
        return NULL;
    }

	char* canon_path = k_util_path_canonize(path, "");

    fs_node_t* root_node = __k_fs_vfs_find_node(canon_path, mode);

	if(!root_node) {
		if(mode & O_CREAT) {
			char* folder = k_util_path_folder(canon_path);
			char* name   = k_util_path_filename(canon_path);
			fs_node_t* root_folder = k_fs_vfs_open(folder, O_WRONLY);
			if(root_folder) {
				root_node = k_fs_vfs_create(root_folder, name, mode);
			}
			k_free(folder);
			k_free(name);
			k_fs_vfs_close(root_folder);
		}
	}

	if(!root_node) {
		k_free(canon_path);
		return NULL;
	}

	fs_node_t* node = NULL;

	if(root_node->mountpoint) {
		node = (fs_node_t*) k_malloc(sizeof(fs_node_t));
		memcpy(node, root_node, sizeof(fs_node_t));
	} else {
		node = root_node;
	}

    if(node->fs.open){
        node->fs.open(node, mode);
    }

	node->mode  = mode;
	node->links = 1;
	node->path  = canon_path;

    return node;
}

void k_fs_vfs_close(fs_node_t* node){
    if(node){
		node->links--;
		if(!node->links) {
        	if(node->fs.close){
        	    node->fs.close(node);
        	}
			if(node->path) {
				k_free(node->path);
			}
        	k_free(node);
    	}
	}
}


struct dirent* k_fs_vfs_readdir(fs_node_t* node, uint32_t index){

	if(!(node->flags & VFS_MAPPER) && 
		 node->mountpoint && 
		 node->mountpoint->tree_node) {
		if(node->mountpoint->tree_node->children->size > index) {
			tree_node_t* tnode = node->mountpoint->tree_node->children->data[index];
			if(tnode && tnode->value) {
				vfs_entry_t* ent = tnode->value;
				struct dirent* dent = k_malloc(sizeof(struct dirent));
				dent->ino = 1;
				strncpy(dent->name, ent->name, 256);
				return dent;
			}
		} else {
			index -= node->mountpoint->tree_node->children->size;
		}
	}

    if(!node->fs.readdir){
        return NULL;
    }

    return node->fs.readdir(node, index);
}

uint32_t  k_fs_vfs_readlink(fs_node_t* node, uint8_t* buf, uint32_t size) {
	if(!node->fs.readlink) {
		return 0;
	}

	return node->fs.readlink(node, buf, size);
}

fs_node_t* k_fs_vfs_finddir(fs_node_t* node, const char* path){
    if(!node->fs.finddir){
        return NULL;
    }

    return node->fs.finddir(node, path);
}

fs_node_t* k_fs_vfs_create(fs_node_t* node, const char* path, uint16_t mode) {
	if(!node->fs.create) {
		return NULL;
	}

	return node->fs.create(node, path, mode);
}

fs_node_t* k_fs_vfs_mkdir(fs_node_t* node, const char* path, uint16_t mode) {
	if(!node->fs.mkdir) {
		return NULL;
	}

	return node->fs.mkdir(node, path, mode);
}

int k_fs_vfs_ioctl(fs_node_t *node, uint32_t req, void *args) {
	if(!node->fs.ioctl) {
		return -EPERM;
	}

	return node->fs.ioctl(node, req, args);
}

uint8_t k_fs_vfs_check(fs_node_t* node, uint8_t mode) {
	if(!node->fs.check) {
		return -EPERM;
	}

	return node->fs.check(node, mode);
}

int k_fs_vfs_wait(fs_node_t* node, uint8_t events, struct process* prc) {
	if(!node->fs.wait) {
		return -EPERM;
	}

	return node->fs.wait(node, events, prc);
}

vfs_entry_t* k_fs_vfs_get_mountpoint(const char* path) {
	return __k_fs_vfs_get_entry(path, 0);
}

K_STATUS k_fs_vfs_mount_node(const char* path, fs_node_t* fsroot){
    if(!vfs_tree){
        return K_STATUS_ERR_GENERIC;
    }

    vfs_entry_t* mountpoint = __k_fs_vfs_get_entry(path, 1);
    if(mountpoint){
		if(mountpoint->node && mountpoint->node->flags & VFS_MAPPER) {
			k_fs_vfs_close(mountpoint->node);
		}
        mountpoint->node = fsroot;
		fsroot->mountpoint = mountpoint;
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

K_STATUS k_fs_vfs_umount(const char* path) {
	vfs_entry_t* entry = __k_fs_vfs_get_entry(path, 0);
	if(entry) {
		if(entry->node) {
			entry->node->mountpoint = NULL;
		}
		if(entry != __k_fs_vfs_root_entry()) {
			tree_remove_node(vfs_tree, entry->tree_node);
			k_free(entry);
		}
		return K_STATUS_OK;
	}
	return K_STATUS_ERR_GENERIC;
}

int32_t k_fs_vfs_link(const char* source, const char* target) {
	char* path = k_util_path_folder(target);
	
	fs_node_t* parent = __k_fs_vfs_find_node(path, 0);
	k_free(path);
	
	if(!parent) {
		return -ENOENT;
	}

	if(!parent->fs.link) {
		return -EINVAL;
	}

	return parent->fs.link(parent, source, target);
}

int32_t k_fs_vfs_rm(fs_node_t* node) {
	if(node->flags & VFS_DIR) {
		return -EISDIR;
	}
	if(node->fs.rm) {
		node->fs.rm(node);
	}
	if(node->mountpoint) {
		k_free(node->mountpoint->node);
		tree_remove_node(vfs_tree, node->mountpoint->tree_node);
		k_free(node->mountpoint);
	}
	k_fs_vfs_close(node);
	return 0;
}

int32_t k_fs_vfs_rmdir(fs_node_t* node) {
	if(!(node->flags & VFS_DIR)) {
		return -ENOTDIR;
	}
	if(node->fs.rmdir) {
		node->fs.rmdir(node);
	}
	if(node->mountpoint) {
		k_free(node->mountpoint->node);
		tree_remove_node(vfs_tree, node->mountpoint->tree_node);
		k_free(node->mountpoint);
	}
	k_fs_vfs_close(node);
	return 0;
}

int  k_fs_vfs_truncate(fs_node_t* node, off_t size) {
	if(!node->fs.truncate) {
		return -EPERM;
	}

	return node->fs.truncate(node, size);
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
    for(uint32_t i = 0; i < node->children->size; i++){
        __k_d_fs_vfs_print_node((tree_node_t*) node->children->data[i], depth + 1);
    }
}

void  k_d_fs_vfs_print(){
    __k_d_fs_vfs_print_node(vfs_tree->root, 0);
}
