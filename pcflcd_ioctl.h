#ifndef PCFLCD_IOCTL_H
#define PCFLCD_IOCTL_H

#include "linux/ioctl.h"
#include "linux/types.h"

#define MK_POS(row, col) (row << 8 | col)
#define POS_COL(pos) (pos & 0xff)
#define POS_ROW(pos) ((pos >> 8) & 0xff)

#define PCFLCD_SCROLL_LEFT 0
#define PCFLCD_SCROLL_RIGHT 1

#define PCFLCD_ENTRY_RIGHT_TO_LEFT 0
#define PCFLCD_ENTRY_LEFT_TO_RIGRHT 1

#define PCFLCD_MAGIC 'P'
#define PCFLCD_GET_SIZE _IOR(PCFLCD_MAGIC, 0, unsigned int)
#define PCFLCD_GET_OLED _IOR(PCFLCD_MAGIC, 1, unsigned int)
#define PCFLCD_SET_BACKLIGHT _IOW(PCFLCD_MAGIC, 2, unsigned int)
#define PCFLCD_SET_DISPLAY _IOW(PCFLCD_MAGIC, 3, unsigned int)
#define PCFLCD_SET_CURSOR _IOW(PCFLCD_MAGIC, 4, unsigned int)
#define PCFLCD_SET_BLINK _IOW(PCFLCD_MAGIC, 5, unsigned int)
#define PCFLCD_SET_SCROLL _IOW(PCFLCD_MAGIC, 6, unsigned int)
#define PCFLCD_SET_ENTRY _IOW(PCFLCD_MAGIC, 7, unsigned int)
#define PCFLCD_SET_AUTO_SCROLL _IOW(PCFLCD_MAGIC, 8, unsigned int)
#define PCFLCD_SET_CURSOR_POS _IOW(PCFLCD_MAGIC, 9, unsigned int)
#define PCFLCD_CLEAR _IOW(PCFLCD_MAGIC, 10, unsigned int)

#endif