#ifndef _FILESYSTEMS_HPP
#define _FILESYSTEMS_HPP

#include <btos/fs_interface.h>

void fs_init();
bool fs_mount(const char *name, const char *device, const char *fs);
bool fs_unmount(const char *name);
file_handle fs_open(const char *path, fs_mode_flags mode);
bool fs_close(file_handle &file);
size_t fs_read(file_handle &file, size_t bytes, char *buf);
size_t fs_write(file_handle &file, size_t bytes, char *buf);
bt_filesize_t fs_seek(file_handle &file, bt_filesize_t pos, uint32_t flags);
bool fs_setsize(file_handle &file, bt_filesize_t size);
int fs_ioctl(file_handle &file, int fn, size_t bytes, char *buf);
dir_handle fs_open_dir(const char *path, fs_mode_flags mode);
bool fs_close_dir(dir_handle &dir);
directory_entry fs_read_dir(dir_handle &dir);
bool fs_write_dir(dir_handle &dir, directory_entry entry);
size_t fs_seek_dir(dir_handle &dir, size_t pos, uint32_t flags);
directory_entry fs_stat(const char *path);
void fs_registerfs(const fs_driver &driver);
void fs_flush(file_handle &file);
bool fs_format(const char *name, const char *device, void *options);

#endif
