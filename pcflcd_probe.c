#include "pcflcd.h"

#define DEV_NAME "pcflcd"
const char *DEV_NAME_STR = DEV_NAME;

extern struct file_operations pcflcd_ops;

static void pcflcd_property_init(struct device *dev, struct pcflcd *lcd)
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

int pcflcd_backlight(struct pcflcd *lcd, bool onoff)
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

static inline int pcflcd_write(struct pcflcd *lcd, bool cmd, __u8 val)
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

static inline int pcflcd_send(struct pcflcd *lcd, bool cmd, __u8 val)
{
    int err;
    err = pcflcd_write(lcd, cmd, val & 0xf0); // high 4bit
    if (err)
    {
        return err;
    }
    return pcflcd_write(lcd, cmd, val << 4); // low 4bit
}

int pcflcd_cmd(struct pcflcd *lcd, __u8 val)
{
    return pcflcd_send(lcd, true, val);
}

int pcflcd_dat(struct pcflcd *lcd, __u8 val)
{
    return pcflcd_send(lcd, false, val);
}

int pcflcd_display(struct pcflcd *lcd, bool onoff)
{
    if (onoff)
    {
        lcd->control |= LCD_DISPLAYON;
    }
    else
    {
        lcd->control &= (~LCD_DISPLAYON);
    }
    return pcflcd_cmd(lcd, LCD_DISPLAYCONTROL | lcd->control);
}

int pcflcd_cursor(struct pcflcd *lcd, bool onoff)
{
    if (onoff)
    {
        lcd->control |= LCD_CURSORON;
    }
    else
    {
        lcd->control &= (~LCD_CURSORON);
    }
    return pcflcd_cmd(lcd, LCD_DISPLAYCONTROL | lcd->control);
}

int pcflcd_blink(struct pcflcd *lcd, bool onoff)
{
    if (onoff)
    {
        lcd->control |= LCD_BLINKON;
    }
    else
    {
        lcd->control &= (~LCD_BLINKON);
    }
    return pcflcd_cmd(lcd, LCD_DISPLAYCONTROL | lcd->control);
}

int pcflcd_scroll_display(struct pcflcd *lcd, bool right_left)
{
    return pcflcd_cmd(lcd, LCD_CURSORSHIFT | LCD_DISPLAYMOVE | (right_left ? LCD_MOVERIGHT : LCD_MOVELEFT));
}

int pcflcd_entry(struct pcflcd *lcd, bool left_to_right)
{
    if (left_to_right)
    {
        lcd->mode |= LCD_ENTRYLEFT;
    }
    else
    {
        lcd->mode &= (~LCD_ENTRYLEFT);
    }
    return pcflcd_cmd(lcd, LCD_ENTRYMODESET | lcd->mode);
}

int pcflcd_auto_scroll(struct pcflcd *lcd, bool onoff)
{
    if (onoff)
    {
        lcd->mode |= LCD_ENTRYSHIFTINCREMENT;
    }
    else
    {
        lcd->mode &= (~LCD_ENTRYSHIFTINCREMENT);
    }
    return pcflcd_cmd(lcd, LCD_ENTRYMODESET | lcd->mode);
}

int pcflcd_set_cursor(struct pcflcd *lcd, __u8 row, __u8 col)
{
    __u8 row_offsets[] = {0x00, 0x40, 0x14, 0x54};
    if (row > lcd->rows)
    {
        row = lcd->rows - 1;
    }
    return pcflcd_cmd(lcd, LCD_SETDDRAMADDR | (col + row_offsets[row % 4]));
}

int pcflcd_clear(struct pcflcd *lcd)
{
    int err;
    err = pcflcd_cmd(lcd, LCD_CLEARDISPLAY);
    if (err)
    {
        return err;
    }
    msleep(2);
    if (lcd->oled)
    {
        return pcflcd_set_cursor(lcd, 0, 0);
    }
    return 0;
}

static inline int pcflcd_write4(struct pcflcd *lcd, __u8 val)
{
    return pcflcd_write(lcd, true, val);
}

static int pcflcd_probe(struct i2c_client *client)
{
    int ret;
    int err;
    dev_t devno;
    struct device *dev = &client->dev;
    struct pcflcd *lcd = devm_kzalloc(dev, sizeof(struct pcflcd), GFP_KERNEL);
    i2c_set_clientdata(client, lcd);
    lcd->client = client;
    pcflcd_property_init(dev, lcd);
    dev_info(dev, "pcflcd init: rows: %d cols: %d oled: %d\n", lcd->rows, lcd->cols, lcd->oled);
    lcd->function = LCD_4BITMODE | LCD_1LINE | LCD_5x8DOTS;
    if (lcd->rows > 1)
    {
        lcd->function |= LCD_2LINE;
    }

    err = pcflcd_backlight(lcd, true);
    if (err)
        return -EBADF;
    // start init
    err = pcflcd_write4(lcd, 0b11 << 4);
    if (err)
        return -EBADF;
    msleep(4);
    err = pcflcd_write4(lcd, 0b11 << 4);
    if (err)
        return -EBADF;
    msleep(4);
    err = pcflcd_write4(lcd, 0b11 << 4);
    if (err)
        return -EBADF;
    msleep(1);
    err = pcflcd_write4(lcd, 0b10 << 4);
    if (err)
        return -EBADF;

    pcflcd_cmd(lcd, LCD_FUNCTIONSET | lcd->function);
    if (err)
        return -EBADF;

    lcd->control = LCD_DISPLAYON | LCD_CURSORON | LCD_BLINKOFF;
    pcflcd_display(lcd, true);
    if (err)
        return -EBADF;

    err = pcflcd_clear(lcd);
    if (err)
        return -EBADF;

    lcd->mode = LCD_ENTRYLEFT | LCD_ENTRYSHIFTDECREMENT;
    err = pcflcd_cmd(lcd, LCD_ENTRYMODESET | lcd->mode);
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

        cdev_init(&lcd->cdev, &pcflcd_ops);
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

static int pcflcd_remove(struct i2c_client *client)
{
    // int err;
    struct pcflcd *lcd = i2c_get_clientdata(client);

    {
        dev_t devno = lcd->cdev.dev;
        device_destroy(lcd->cls, devno);
        class_destroy(lcd->cls);
        cdev_del(&lcd->cdev);
        unregister_chrdev_region(devno, 1);
    }

    pcflcd_cursor(lcd, false);
    pcflcd_clear(lcd);
    pcflcd_display(lcd, false);
    i2c_smbus_write_byte(client, 0x00);

    return 0;
}

static const struct i2c_device_id pcflcd_id[] = {
    {"pcf8574-lcd", 0},
    {},
};

MODULE_DEVICE_TABLE(i2c, pcflcd_id);

static const struct of_device_id pcflcd_of_match[] = {
    {.compatible = "pcflcd,pcf8574-lcd", .data = NULL},
    {},
};
MODULE_DEVICE_TABLE(of, pcflcd_of_match);

static struct i2c_driver pcflcd_driver = {
    .probe_new = pcflcd_probe,
    .remove = pcflcd_remove,
    .id_table = pcflcd_id,
    .driver = {
        .name = "pcdlcf",
        .of_match_table = pcflcd_of_match,
    },
};

module_i2c_driver(pcflcd_driver);

MODULE_LICENSE("GPL");
