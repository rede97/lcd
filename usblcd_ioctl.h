#ifndef USBLCD_IOCTL_H
#define USBLCD_IOCTL_H

#include "linux/ioctl.h"
#include "linux/types.h"

#define MK_POS(row, col) (row << 8 | col)
#define POS_COL(pos) (pos & 0xff)
#define POS_ROW(pos) ((pos >> 8) & 0xff)

#define LCD_SCROLL_LEFT 0
#define LCD_SCROLL_RIGHT 1

#define LCD_ENTRY_RIGHT_TO_LEFT 0
#define LCD_ENTRY_LEFT_TO_RIGRHT 1

#define LCD_MAGIC 'P'
#define LCD_GET_SIZE _IOR(LCD_MAGIC, 0, unsigned int)
#define LCD_GET_OLED _IOR(LCD_MAGIC, 1, unsigned int)
#define LCD_SET_BACKLIGHT _IOW(LCD_MAGIC, 2, unsigned int)
#define LCD_SET_DISPLAY _IOW(LCD_MAGIC, 3, unsigned int)
#define LCD_SET_CURSOR _IOW(LCD_MAGIC, 4, unsigned int)
#define LCD_SET_BLINK _IOW(LCD_MAGIC, 5, unsigned int)
#define LCD_SET_SCROLL _IOW(LCD_MAGIC, 6, unsigned int)
#define LCD_SET_ENTRY _IOW(LCD_MAGIC, 7, unsigned int)
#define LCD_SET_AUTO_SCROLL _IOW(LCD_MAGIC, 8, unsigned int)
#define LCD_SET_CURSOR_POS _IOW(LCD_MAGIC, 9, unsigned int)
#define LCD_CLEAR _IOW(LCD_MAGIC, 10, unsigned int)

#endif