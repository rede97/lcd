#include "usblcd.h"

#define DEV_NAME "usblcd"
const char *DEV_NAME_STR = DEV_NAME;

extern struct file_operations usblcd_ops;

static int usblcd_probe(struct usb_interface *intf, const struct usb_device_id *id)
{
    int ret = 0;
    struct usb_device *usbdev;
    struct usb_host_interface *interface;
    struct usb_endpoint_descriptor *endpoint;
    struct device *dev = &intf->dev;

    usbdev = interface_to_usbdev(intf);
    interface = intf->cur_altsetting;

    return 0;
}

static void usblcd_disconnect(struct usb_interface *intf)
{
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