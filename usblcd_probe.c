#include "usblcd.h"

#define DEV_NAME "usblcd"
const char *DEV_NAME_STR = DEV_NAME;

extern struct file_operations usblcd_ops;

static void usblcd_property_init(struct device *dev, struct usblcd *lcd)
{
    __u32 prop = 0;

    if (device_property_read_u32(dev, "lcd,rows", &prop))
    {
        prop = 2;
    }

    switch (prop)
    {
    case 1:
    case 2:
        lcd->rows = (__u8)prop;
        break;
    default:
        lcd->rows = 2;
        break;
    }

    if (device_property_read_u32(dev, "lcd,cols", &prop))
    {
        prop = 16;
    }

    switch (prop)
    {
    case 8:
    case 16:
    case 20:
        lcd->cols = (__u8)prop;
        break;
    default:
        lcd->cols = 16;
        break;
    }

    lcd->oled = device_property_read_bool(dev, "lcd,oled");
}

int usblcd_backlight(struct usblcd *lcd, bool onoff)
{
    if (onoff)
    {
        lcd->pin |= LCD_BACKLIGHT;
    }
    else
    {
        lcd->pin &= (~LCD_BACKLIGHT);
    }
    return i2c_smbus_write_byte(lcd->client, lcd->pin);
}

static inline int usblcd_write(struct usblcd *lcd, bool cmd, __u8 val)
{
    int err;
    val = (val & 0xf0) | (lcd->pin & LCD_BACKLIGHT) | En | (cmd ? 0 : Rs);
    err = i2c_smbus_write_byte(lcd->client, val);
    if (err)
    {
        return err;
    }
    val &= (~En);
    ndelay(1000);
    err = i2c_smbus_write_byte(lcd->client, val);
    if (err)
    {
        return err;
    }
    lcd->pin = val;
    ndelay(50000);
    return 0;
}

static inline int usblcd_send(struct usblcd *lcd, bool cmd, __u8 val)
{
    int err;
    err = usblcd_write(lcd, cmd, val & 0xf0); // high 4bit
    if (err)
    {
        return err;
    }
    return usblcd_write(lcd, cmd, val << 4); // low 4bit
}

int usblcd_cmd(struct usblcd *lcd, __u8 val)
{
    return usblcd_send(lcd, true, val);
}

int usblcd_dat(struct usblcd *lcd, __u8 val)
{
    return usblcd_send(lcd, false, val);
}

int usblcd_display(struct usblcd *lcd, bool onoff)
{
    if (onoff)
    {
        lcd->control |= LCD_DISPLAYON;
    }
    else
    {
        lcd->control &= (~LCD_DISPLAYON);
    }
    return usblcd_cmd(lcd, LCD_DISPLAYCONTROL | lcd->control);
}

int usblcd_cursor(struct usblcd *lcd, bool onoff)
{
    if (onoff)
    {
        lcd->control |= LCD_CURSORON;
    }
    else
    {
        lcd->control &= (~LCD_CURSORON);
    }
    return usblcd_cmd(lcd, LCD_DISPLAYCONTROL | lcd->control);
}

int usblcd_blink(struct usblcd *lcd, bool onoff)
{
    if (onoff)
    {
        lcd->control |= LCD_BLINKON;
    }
    else
    {
        lcd->control &= (~LCD_BLINKON);
    }
    return usblcd_cmd(lcd, LCD_DISPLAYCONTROL | lcd->control);
}

int usblcd_scroll_display(struct usblcd *lcd, bool right_left)
{
    return usblcd_cmd(lcd, LCD_CURSORSHIFT | LCD_DISPLAYMOVE | (right_left ? LCD_MOVERIGHT : LCD_MOVELEFT));
}

int usblcd_entry(struct usblcd *lcd, bool left_to_right)
{
    if (left_to_right)
    {
        lcd->mode |= LCD_ENTRYLEFT;
    }
    else
    {
        lcd->mode &= (~LCD_ENTRYLEFT);
    }
    return usblcd_cmd(lcd, LCD_ENTRYMODESET | lcd->mode);
}

int usblcd_auto_scroll(struct usblcd *lcd, bool onoff)
{
    if (onoff)
    {
        lcd->mode |= LCD_ENTRYSHIFTINCREMENT;
    }
    else
    {
        lcd->mode &= (~LCD_ENTRYSHIFTINCREMENT);
    }
    return usblcd_cmd(lcd, LCD_ENTRYMODESET | lcd->mode);
}

int usblcd_set_cursor(struct usblcd *lcd, __u8 row, __u8 col)
{
    __u8 row_offsets[] = {0x00, 0x40, 0x14, 0x54};
    if (row > lcd->rows)
    {
        row = lcd->rows - 1;
    }
    return usblcd_cmd(lcd, LCD_SETDDRAMADDR | (col + row_offsets[row % 4]));
}

int usblcd_clear(struct usblcd *lcd)
{
    int err;
    err = usblcd_cmd(lcd, LCD_CLEARDISPLAY);
    if (err)
    {
        return err;
    }
    msleep(2);
    if (lcd->oled)
    {
        return usblcd_set_cursor(lcd, 0, 0);
    }
    return 0;
}

static inline int usblcd_write4(struct usblcd *lcd, __u8 val)
{
    return usblcd_write(lcd, true, val);
}

static int usblcd_probe(struct i2c_client *client)
{
    int ret;
    int err;
    dev_t devno;
    struct device *dev = &client->dev;
    struct usblcd *lcd = devm_kzalloc(dev, sizeof(struct usblcd), GFP_KERNEL);
    i2c_set_clientdata(client, lcd);
    lcd->client = client;
    usblcd_property_init(dev, lcd);
    dev_info(dev, "usblcd init: rows: %d cols: %d oled: %d\n", lcd->rows, lcd->cols, lcd->oled);
    lcd->function = LCD_4BITMODE | LCD_1LINE | LCD_5x8DOTS;
    if (lcd->rows > 1)
    {
        lcd->function |= LCD_2LINE;
    }

    err = usblcd_backlight(lcd, true);
    if (err)
        return -EBADF;
    // start init
    err = usblcd_write4(lcd, 0b11 << 4);
    if (err)
        return -EBADF;
    msleep(4);
    err = usblcd_write4(lcd, 0b11 << 4);
    if (err)
        return -EBADF;
    msleep(4);
    err = usblcd_write4(lcd, 0b11 << 4);
    if (err)
        return -EBADF;
    msleep(1);
    err = usblcd_write4(lcd, 0b10 << 4);
    if (err)
        return -EBADF;

    usblcd_cmd(lcd, LCD_FUNCTIONSET | lcd->function);
    if (err)
        return -EBADF;

    lcd->control = LCD_DISPLAYON | LCD_CURSORON | LCD_BLINKOFF;
    usblcd_display(lcd, true);
    if (err)
        return -EBADF;

    err = usblcd_clear(lcd);
    if (err)
        return -EBADF;

    lcd->mode = LCD_ENTRYLEFT | LCD_ENTRYSHIFTDECREMENT;
    err = usblcd_cmd(lcd, LCD_ENTRYMODESET | lcd->mode);
    if (err)
        return -EBADF;

    {
        mutex_init(&lcd->lock);
        ret = alloc_chrdev_region(&devno, 0, 1, DEV_NAME_STR);
        if (ret < 0)
        {
            dev_alert(dev, "alloc chrdev error: %d\n", ret);
            goto alloc_chrdev_err;
        }

        cdev_init(&lcd->cdev, &usblcd_ops);
        lcd->cdev.owner = THIS_MODULE;
        ret = cdev_add(&lcd->cdev, devno, 1);
        if (ret)
        {
            dev_alert(dev, "cdev add error: %d\n", ret);
            goto cdev_add_err;
        }

        lcd->cls = class_create(THIS_MODULE, "lcd");
        lcd->device = device_create(lcd->cls, dev, devno, lcd, DEV_NAME_STR);
        if (IS_ERR(lcd->device))
        {
            dev_alert(dev, "create device err\n");
            goto dev_create_err;
        }
    }

    return 0;

dev_create_err:
    class_destroy(lcd->cls);
    cdev_del(&lcd->cdev);
cdev_add_err:
    unregister_chrdev_region(devno, 1);
alloc_chrdev_err:
    return ret;
}

static int usblcd_remove(struct i2c_client *client)
{
    // int err;
    struct usblcd *lcd = i2c_get_clientdata(client);

    {
        dev_t devno = lcd->cdev.dev;
        device_destroy(lcd->cls, devno);
        class_destroy(lcd->cls);
        cdev_del(&lcd->cdev);
        unregister_chrdev_region(devno, 1);
    }

    usblcd_cursor(lcd, false);
    usblcd_clear(lcd);
    usblcd_display(lcd, false);
    i2c_smbus_write_byte(client, 0x00);

    return 0;
}

static struct usb_device_id id_table[] = {
    {USB_DEVICE(0x1362, 0x5532)},
    {},
};

MODULES_DEVICE_TABLE(usb, is_table);

static struct usb_driver usblcd_driver = {}