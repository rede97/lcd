#include "usblcd.h"
#include "usblcd_ioctl.h"

const __u8 char_heart[] = {
    0b00000,
    0b01010,
    0b11111,
    0b11111,
    0b11111,
    0b01110,
    0b00100,
    0b00000,
};

static int usblcd_open(struct inode *inode, struct file *filp)
{
    struct usblcd *lcd = container_of(inode->i_cdev, struct usblcd, cdev);
    if (MUTEX_TRYLOCK_SUCCESS != mutex_trylock(&lcd->lock))
    {
        return -EBUSY;
    }
    filp->private_data = lcd;

    {
        int err;
        lcd->function = LCD_4BITMODE | LCD_1LINE | LCD_5x8DOTS;
        if (lcd->rows > 1)
        {
            lcd->function |= LCD_2LINE;
        }

        err = usblcd_backlight(lcd, true);
        if (err)
            goto lcd_init_err;
        // start init
        err = usblcd_cmd4(lcd, 0b11);
        if (err)
            goto lcd_init_err;
        err = usblcd_mdelay(lcd, 1, 4);
        if (err)
        {
            return err;
        }
        err = usblcd_cmd4(lcd, 0b11);
        if (err)
            goto lcd_init_err;
        err = usblcd_mdelay(lcd, 1, 4);
        if (err)
        {
            return err;
        }
        err = usblcd_cmd4(lcd, 0b11);
        if (err)
            goto lcd_init_err;
        err = usblcd_udelay(lcd, 1, 150);
        if (err)
        {
            return err;
        }
        err = usblcd_cmd4(lcd, 0b10);
        if (err)
            goto lcd_init_err;

        usblcd_cmd8(lcd, LCD_FUNCTIONSET | lcd->function);
        if (err)
            goto lcd_init_err;

        lcd->control = LCD_DISPLAYON | LCD_CURSOROFF | LCD_BLINKOFF;
        usblcd_display(lcd, true);
        if (err)
            goto lcd_init_err;

        err = usblcd_clear(lcd);
        if (err)
            goto lcd_init_err;

        lcd->mode = LCD_ENTRYLEFT | LCD_ENTRYSHIFTDECREMENT;
        err = usblcd_cmd8(lcd, LCD_ENTRYMODESET | lcd->mode);
        if (err)
            goto lcd_init_err;

        err = usblcd_home(lcd);
        if (err)
            goto lcd_init_err;

        err = usblcd_new_char(lcd, 0, char_heart);
        if (err)
            goto lcd_init_err;

        err = usblcd_emit(lcd);
        if (err)
            goto lcd_init_err;
    }

    return 0;
lcd_init_err:
    mutex_unlock(&lcd->lock);
    return -EIO;
}

static int usblcd_release(struct inode *inode, struct file *filp)
{
    struct usblcd *lcd = container_of(inode->i_cdev, struct usblcd, cdev);
    {
        usblcd_clear(lcd);
        usblcd_set_cursor(lcd, 0, 0);
        usblcd_display(lcd, false);
        usblcd_backlight(lcd, false);
        usblcd_emit(lcd);
    }
    mutex_unlock(&lcd->lock);
    return 0;
}

static ssize_t usblcd_ops_write(struct file *filp, const char __user *buf, size_t count, loff_t *pos)
{
    int ret;
    struct usblcd *lcd = filp->private_data;
    char *kbuf = kmalloc(count, GFP_KERNEL);
    ret = copy_from_user(kbuf, buf, count);
    if (ret)
    {
        goto write_err;
    }
    ret = usblcd_dat8(lcd, kbuf, count);
    if (ret)
    {
        goto write_err;
    }
    ret = usblcd_emit(lcd);
    if (ret)
    {
        goto write_err;
    }
write_err:
    kfree(kbuf);
    return ret == 0 ? count : ret;
}

static long usblcd_ops_ioctl(struct file *filp, unsigned int cmd, unsigned long arg)
{
    int ret;
    unsigned int val;
    struct usblcd *lcd = filp->private_data;
    dev_info(lcd->device, "cmd: 0x%08x\n", cmd);
    if (_IOC_TYPE(cmd) != LCD_MAGIC)
    {
        return -ENOTTY;
    }

    switch (cmd)
    {
    case LCD_GET_SIZE:
        val = MK_POS(lcd->rows, lcd->cols);
        put_user(val, (unsigned int __user *)arg);
        return 0;
    case LCD_GET_OLED:
        val = lcd->oled;
        put_user(val, (unsigned int __user *)arg);
        return 0;
    case LCD_SET_BACKLIGHT:
        ret = usblcd_backlight(lcd, arg);
        break;
    case LCD_SET_DISPLAY:
        ret = usblcd_display(lcd, arg);
        break;
    case LCD_SET_CURSOR:
        ret = usblcd_cursor(lcd, arg);
        break;
    case LCD_SET_BLINK:
        ret = usblcd_blink(lcd, arg);
        break;
    case LCD_SET_SCROLL:
        ret = usblcd_scroll_display(lcd, arg);
        break;
    case LCD_SET_ENTRY:
        ret = usblcd_entry(lcd, arg);
        break;
    case LCD_SET_AUTO_SCROLL:
        ret = usblcd_auto_scroll(lcd, arg);
        break;
    case LCD_SET_CURSOR_POS:
        ret = usblcd_set_cursor(lcd, POS_ROW(arg), POS_COL(arg));
        break;
    case LCD_CLEAR:
        ret = usblcd_clear(lcd);
        break;
    default:
        break;
    }

    if (ret)
    {
        return -EFAULT;
    }
    return usblcd_emit(lcd);
}

struct file_operations usblcd_ops = {
    .owner = THIS_MODULE,
    .open = usblcd_open,
    .release = usblcd_release,
    .write = usblcd_ops_write,
    .unlocked_ioctl = usblcd_ops_ioctl,
};