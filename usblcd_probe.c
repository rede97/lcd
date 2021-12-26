#include "usblcd.h"

#define DEV_NAME "usblcd"
const char *DEV_NAME_STR = DEV_NAME;

extern struct file_operations usblcd_ops;

int usblcd_backlight(struct usblcd *lcd, bool onoff)
{
    lcd->backlight = onoff;
    return usblcd_submit(lcd, OP_SET_IO, lcd->backlight == 1, NULL, 0);
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
    return usblcd_cmd8(lcd, LCD_DISPLAYCONTROL | lcd->control);
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
    return usblcd_cmd8(lcd, LCD_DISPLAYCONTROL | lcd->control);
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
    return usblcd_cmd8(lcd, LCD_DISPLAYCONTROL | lcd->control);
}

int usblcd_scroll_display(struct usblcd *lcd, bool right_left)
{
    return usblcd_cmd8(lcd, LCD_CURSORSHIFT | LCD_DISPLAYMOVE | (right_left ? LCD_MOVERIGHT : LCD_MOVELEFT));
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
    return usblcd_cmd8(lcd, LCD_ENTRYMODESET | lcd->mode);
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
    return usblcd_cmd8(lcd, LCD_ENTRYMODESET | lcd->mode);
}

int usblcd_set_cursor(struct usblcd *lcd, __u8 row, __u8 col)
{
    __u8 row_offsets[] = {0x00, 0x40, 0x14, 0x54};
    if (row > lcd->rows)
    {
        row = lcd->rows - 1;
    }
    return usblcd_cmd8(lcd, LCD_SETDDRAMADDR | (col + row_offsets[row % 4]));
}

int usblcd_clear(struct usblcd *lcd)
{
    int err;
    err = usblcd_cmd8(lcd, LCD_CLEARDISPLAY);
    if (err)
    {
        return err;
    }
    err = usblcd_mdelay(lcd, 1, 2);
    if (err)
    {
        return err;
    }
    if (lcd->oled)
    {
        return usblcd_set_cursor(lcd, 0, 0);
    }
    return 0;
}

int usblcd_home(struct usblcd *lcd)
{
    int err;
    err = usblcd_cmd8(lcd, LCD_RETURNHOME);
    if (err)
    {
        return err;
    }
    return usblcd_mdelay(lcd, 1, 2);
}

int usblcd_new_char(struct usblcd *lcd, __u8 location, const char *charmap)
{
    int err;
    location &= 0x07;
    err = usblcd_cmd8(lcd, LCD_SETCGRAMADDR | (location << 3));
    if (err)
    {
        return err;
    }
    return usblcd_dat8(lcd, charmap, 8);
}

static void usblcd_info_init(struct device *dev, struct usblcd *lcd, __u8 *info)
{
    switch (info[0])
    {
    case 1:
    case 2:
        lcd->rows = (__u8)info[0];
        break;
    default:
        dev_alert(dev, "invaild lcd.rows: %d\n", info[0]);
        lcd->rows = 2;
        break;
    }

    switch (info[1])
    {
    case 8:
    case 16:
    case 20:
        lcd->cols = (__u8)info[1];
        break;
    default:
        dev_alert(dev, "invaild lcd.cols: %d\n", info[1]);
        lcd->cols = 16;
        break;
    }

    lcd->oled = info[2] == 1;
}

static inline int usblcd_ep_check(struct device *dev, struct usb_endpoint_descriptor *endpoint, __u8 idx, __u8 addr, __u8 attr)
{
    dev_dbg(dev, "endpoint %d, addr: 0x%x attr: 0x%x\n", idx, endpoint->bEndpointAddress, endpoint->bmAttributes);
    if (endpoint->bEndpointAddress != addr)
    {
        dev_alert(dev, "endpoint %d: addr: 0x%x, expect 0x81\n", idx, endpoint->bEndpointAddress);
        return -ENODEV;
    }
    if (endpoint->bmAttributes != attr)
    {
        dev_alert(dev, "endpoint %d: attribute: 0x%x, expect 0x03", idx, endpoint->bmAttributes & 0x7f);
        return -ENODEV;
    }
    return 0;
}

static int usblcd_probe(struct usb_interface *intf, const struct usb_device_id *id)
{
    int ret;
    int len;
    dev_t devno;
    struct usb_device *usbdev;
    struct usb_host_interface *interface;
    struct usb_endpoint_descriptor *endpoint;
    struct device *dev = &intf->dev;
    struct usblcd *lcd = devm_kzalloc(dev, sizeof(struct usblcd), GFP_KERNEL);
    if (NULL == lcd)
    {
        return -ENOMEM;
    }
    usb_set_intfdata(intf, lcd);

    usbdev = interface_to_usbdev(intf);
    lcd->usbdev = usbdev;

    interface = intf->cur_altsetting;
    if (interface->desc.bNumEndpoints != 2)
    {
        dev_alert(dev, "enpoint: %d, expect 3\n", interface->desc.bNumEndpoints);
        return -ENODEV;
    }

    endpoint = &interface->endpoint[0].desc;
    if (usblcd_ep_check(dev, endpoint, 2, 0x82, 0x02))
    {
        return -ENODEV;
    }
    lcd->pipe_ep2_in = usb_rcvbulkpipe(usbdev, endpoint->bEndpointAddress);
    lcd->maxp_ep2_in = usb_maxpacket(usbdev, lcd->pipe_ep2_in, usb_pipeout(lcd->pipe_ep2_in));

    endpoint = &interface->endpoint[1].desc;
    if (usblcd_ep_check(dev, endpoint, 2, 0x02, 0x02))
    {
        return -ENODEV;
    }
    lcd->pipe_ep2_out = usb_sndbulkpipe(usbdev, endpoint->bEndpointAddress);
    lcd->maxp_ep2_out = usb_maxpacket(usbdev, lcd->pipe_ep2_out, usb_pipeout(lcd->pipe_ep2_out));

    dev_dbg(dev, "maxpacket: ep2 in: %d, ep2 out: %d\n", lcd->maxp_ep2_in, lcd->maxp_ep2_out);

    ret = usb_bulk_msg(usbdev, lcd->pipe_ep2_in, lcd->buf_ep2, lcd->maxp_ep2_in, &len, 20 * HZ);
    if (ret)
    {
        dev_alert(dev, "read dev info failed: %d\n", ret);
        return ret;
    }

    if (len < 3)
    {
        dev_alert(dev, "invaild length of dev info!\n");
        return -ENODEV;
    }
    usblcd_info_init(dev, lcd, (__u8 *)lcd->buf_ep2);
    dev_info(dev, "lcd rows: %d cols: %d oled: %d", lcd->rows, lcd->cols, lcd->oled);

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
    return 0;
}

static void usblcd_disconnect(struct usb_interface *intf)
{
    struct usblcd *lcd = usb_get_intfdata(intf);

    {
        dev_t devno = lcd->cdev.dev;
        device_destroy(lcd->cls, devno);
        class_destroy(lcd->cls);
        cdev_del(&lcd->cdev);
        unregister_chrdev_region(devno, 1);
    }

    dev_info(&intf->dev, "disconnect device!");
    return;
}

static struct usb_device_id id_table[] = {
    {USB_DEVICE(0x1362, 0x5532)},
    {},
};

MODULE_DEVICE_TABLE(usb, id_table);

static struct usb_driver usblcd_driver = {
    .name = DEV_NAME,
    .id_table = id_table,
    .probe = usblcd_probe,
    .disconnect = usblcd_disconnect,
};

module_usb_driver(usblcd_driver);
MODULE_LICENSE("GPL");