#include <stdint.h>
#include <kernel.h>

#define FAILURE -2
#define SUCCESS 0

int devfs_list(char* path, vfs_metadata_t* metadata, uint32_t entry, char* dev) {
    if (entry >= device_ptr) return FAILURE;
    kstrcpy(metadata->filename, device_list[entry].name);
    metadata->filetype = FILE_TYPE;
    metadata->size = 0xffffffffffffffff;
    return SUCCESS;
}

int devfs_write(char* path, uint8_t val, uint64_t loc, char* dev) {
    if (*path == '/') path++;
    for (int i = 0; i < device_ptr; i++) {
        if (!kstrcmp(path, device_list[i].name))
            return (int)(*device_list[i].io_wrapper)(device_list[i].gp_value, loc, 1, val);
    }
    return FAILURE;
}

int devfs_read(char* path, uint64_t loc, char* dev) {
    if (*path == '/') path++;
    for (int i = 0; i < device_ptr; i++) {
        if (!kstrcmp(path, device_list[i].name))
            return (int)(*device_list[i].io_wrapper)(device_list[i].gp_value, loc, 0, 0);
    }
    return FAILURE;
}

int devfs_get_metadata(char* path, vfs_metadata_t* metadata, int type, char* dev) {
    if (type == DIRECTORY_TYPE) {
        if (!kstrcmp(path, "/") || !*path) {
            metadata->filetype = type;
            metadata->size = 0;
            kstrcpy(metadata->filename, "/");
            return SUCCESS;
        }
        else return FAILURE;
    }

    return SUCCESS;
}

int devfs_mount(char* device) { return 0; }

void install_devfs(void) {
    vfs_install_fs("devfs", &devfs_read, &devfs_write, &devfs_get_metadata, &devfs_list, &devfs_mount);
}
