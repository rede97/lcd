#ifndef USBLCD_IOCTL_H
#define USBLCD_IOCTL_H

#include "linux/ioctl.h"
#include "linux/types.h"

#define MK_POS(row, col) (row << 8 | col)
#define POS_COL(pos) (pos & 0xff)
#define POS_ROW(pos) ((pos >> 8) & 0xff)

#define USBLCD_SCROLL_LEFT 0
#define USBLCD_SCROLL_RIGHT 1

#define USBLCD_ENTRY_RIGHT_TO_LEFT 0
#define USBLCD_ENTRY_LEFT_TO_RIGRHT 1

#define USBLCD_MAGIC 'P'
#define USBLCD_GET_SIZE _IOR(USBLCD_MAGIC, 0, unsigned int)
#define USBLCD_GET_OLED _IOR(USBLCD_MAGIC, 1, unsigned int)
#define USBLCD_SET_BACKLIGHT _IOW(USBLCD_MAGIC, 2, unsigned int)
#define USBLCD_SET_DISPLAY _IOW(USBLCD_MAGIC, 3, unsigned int)
#define USBLCD_SET_CURSOR _IOW(USBLCD_MAGIC, 4, unsigned int)
#define USBLCD_SET_BLINK _IOW(USBLCD_MAGIC, 5, unsigned int)
#define USBLCD_SET_SCROLL _IOW(USBLCD_MAGIC, 6, unsigned int)
#define USBLCD_SET_ENTRY _IOW(USBLCD_MAGIC, 7, unsigned int)
#define USBLCD_SET_AUTO_SCROLL _IOW(USBLCD_MAGIC, 8, unsigned int)
#define USBLCD_SET_CURSOR_POS _IOW(USBLCD_MAGIC, 9, unsigned int)
#define USBLCD_CLEAR _IOW(USBLCD_MAGIC, 10, unsigned int)

#endif