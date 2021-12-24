#ifndef USBLCD_H
#define USBLCD_H

#include <linux/delay.h>
#include <linux/cdev.h>
#include <linux/fb.h>
#include <linux/gpio/consumer.h>
#include <linux/fs.h>
#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/property.h>
#include <linux/pwm.h>
#include <linux/uaccess.h>
#include <linux/types.h>
#include <linux/usb.h>
#include <asm/delay.h>

struct usblcd
{
    struct cdev cdev;
    struct class *cls;
    struct device *device;
    struct usb_device *usbdev;
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
    // lock
    struct mutex lock;
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

int usblcd_backlight(struct usblcd *lcd, bool onoff);
int usblcd_dat(struct usblcd *lcd, __u8 val);
int usblcd_display(struct usblcd *lcd, bool onoff);
int usblcd_cursor(struct usblcd *lcd, bool onoff);
int usblcd_blink(struct usblcd *lcd, bool onoff);
int usblcd_scroll_display(struct usblcd *lcd, bool right_left);
int usblcd_entry(struct usblcd *lcd, bool left_to_right);
int usblcd_auto_scroll(struct usblcd *lcd, bool onoff);
int usblcd_set_cursor(struct usblcd *lcd, __u8 row, __u8 col);
int usblcd_clear(struct usblcd *lcd);

// commands
#define LCD_CLEARDISPLAY 0x01
#define LCD_RETURNHOME 0x02
#define LCD_ENTRYMODESET 0x04
#define LCD_DISPLAYCONTROL 0x08
#define LCD_CURSORSHIFT 0x10
#define LCD_FUNCTIONSET 0x20
#define LCD_SETCGRAMADDR 0x40
#define LCD_SETDDRAMADDR 0x80

// flags for display entry mode
#define LCD_ENTRYRIGHT 0x00
#define LCD_ENTRYLEFT 0x02
#define LCD_ENTRYSHIFTINCREMENT 0x01
#define LCD_ENTRYSHIFTDECREMENT 0x00

// flags for display on/off control
#define LCD_DISPLAYON 0x04
#define LCD_DISPLAYOFF 0x00
#define LCD_CURSORON 0x02
#define LCD_CURSOROFF 0x00
#define LCD_BLINKON 0x01
#define LCD_BLINKOFF 0x00

// flags for display/cursor shift
#define LCD_DISPLAYMOVE 0x08
#define LCD_CURSORMOVE 0x00
#define LCD_MOVERIGHT 0x04
#define LCD_MOVELEFT 0x00

// flags for function set
#define LCD_8BITMODE 0x10
#define LCD_4BITMODE 0x00
#define LCD_2LINE 0x08
#define LCD_1LINE 0x00
#define LCD_5x10DOTS 0x04
#define LCD_5x8DOTS 0x00

// flags for backlight control
#define LCD_BACKLIGHT 0x08
#define LCD_NOBACKLIGHT 0x00

#define En 0B00000100 // Enable bit
#define Rw 0B00000010 // Read/Write bit
#define Rs 0B00000001 // Register select bit

#define OP_NOP 0x0      // 单长度扩展操作
#define OP_SET_IO 0x1   // 设置IO
#define OP_DELAY_US 0x2 // 延时微妙
#define OP_DELAY_MS 0x3 // 延时毫秒
#define OP_CMD_4 0x4    // 写4bit命令
#define OP_CMD_8 0x5    // 写8bit命令
#define OP_DAT_8 0x6    // 写8bit数据

#define GET_DATA(ins) (((ins)&0xf0) >> 4)
#define GET_OP(ins) ((ins)&0x0f)
#define MAKE_INS(op, dat) (((op)&0x0f) | ((dat) << 4))

#endif