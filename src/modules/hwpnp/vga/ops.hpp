#ifndef _OPS_HPP
#define _OPS_HPP

#include "vga.hpp"

template<typename T> static T max(T a, T b){ return (a>b)?a:b; }

struct vga_instance;

size_t text_read(vga_instance *inst, size_t bytes, char *buf);
size_t text_write(vga_instance *inst, size_t bytes, char *buf);
size_t text_seek(vga_instance *inst, size_t pos, uint32_t flags);
int text_ioctl(vga_instance *inst, int fn, size_t bytes, char *buf);
void init_text();

size_t graphics_read(vga_instance *inst, size_t bytes, char *buf);
size_t graphics_write(vga_instance *inst, size_t bytes, char *buf);
size_t graphics_seek(vga_instance *inst, size_t pos, uint32_t flags);
void graphics_end();

#endif
