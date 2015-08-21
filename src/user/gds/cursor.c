#include <stdint.h>
#include <stddef.h>
#include <terminal.h>

bt_terminal_pointer_bitmap pointer_bmp_4bpp={
    .w=11,
    .h=11,
    .bpp=4,
    .transparent=0xE,
    .spot_x=5,
    .spot_y=5,
    .datasize=((11*11)/2) + 1,
    .data = {
        0xEE, 0xEE, 0x00, 0x0E, 0xEE, 0xEE,
        0xEE, 0xE0, 0xF0, 0xEE, 0xEE,
        0xEE, 0xEE, 0x0F, 0x0E, 0xEE, 0xEE,
        0xEE, 0xE0, 0xF0, 0xEE, 0xEE,
        0x00, 0x00, 0x0F, 0x00, 0x00, 0x00,
        0xFF, 0xFF, 0xFF, 0xFF, 0xF0,
        0x00, 0x00, 0x0F, 0x00, 0x00, 0x0E,
        0xEE, 0xE0, 0xF0, 0xEE, 0xEE,
        0xEE, 0xEE, 0x0F, 0x0E, 0xEE, 0xEE,
        0xEE, 0xE0, 0xF0, 0xEE, 0xEE,
        0xEE, 0xEE, 0x00, 0x0E, 0xEE, 0xEE
    }
};