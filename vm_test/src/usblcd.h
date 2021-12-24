#ifndef USB_H
#define USB_H
#include <stdint.h>
#include <stdbool.h>

typedef uint8_t __u8;

struct usblcd
{
    // ep pipe
    int pipe_ep2_in;
    int pipe_ep2_out;
    // ep max packet
    int maxp_ep2_in;
    int maxp_ep2_out;
    // buffer
    int buf_idx;
    int buf_last_ins_idx;
    unsigned char buf_ep2[64];
    // property
    __u8 cols;
    __u8 rows;
    bool oled;
    // flags
    __u8 mode;
    __u8 control;
    __u8 function;
    // status
    bool backlight;
};

#endif