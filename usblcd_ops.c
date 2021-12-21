#include "usblcd.h"
#include "usblcd_ioctl.h"

static int usblcd_open(struct inode *inode, struct file *filp)
{
    struct usblcd *lcd = container_of(inode->i_cdev, struct usblcd, cdev);
    if (MUTEX_TRYLOCK_SUCCESS != mutex_trylock(&lcd->lock))
    {
        return -EBUSY;
    }
    filp->private_data = lcd;
    return 0;
}

static int usblcd_release(struct inode *inode, struct file *filp)
{
    struct usblcd *lcd = container_of(inode->i_cdev, struct usblcd, cdev);
    mutex_unlock(&lcd->lock);
    return 0;
}

static ssize_t usblcd_ops_write(struct file *filp, const char __user *buf, size_t count, loff_t *pos)
{
    int i;
    int ret;
    struct usblcd *lcd = filp->private_data;
    char *buffer = kmalloc(count, GFP_KERNEL);
    ret = copy_from_user(buffer, buf, count);
    if (ret)
    {
        return -EFAULT;
    }
    for (i = 0; i < count; i++)
    {
        int err;
        err = usblcd_dat(lcd, buffer[i]);
        if (err)
        {
            ret = -EFAULT;
            goto write_err;
        }
    }

write_err:
    kfree(buffer);
    return ret == 0 ? i : ret;
}

static long usblcd_ops_ioctl(struct file *filp, unsigned int cmd, unsigned long arg)
{
    int ret;
    unsigned int val;
    struct usblcd *lcd = filp->private_data;
    dev_info(lcd->device, "cmd: 0x%08x\n", cmd);
    if (_IOC_TYPE(cmd) != usblcd_MAGIC)
    {
        return -ENOTTY;
    }

    switch (cmd)
    {
    case usblcd_GET_SIZE:
        val = MK_POS(lcd->rows, lcd->cols);
        put_user(val, (unsigned int __user *)arg);
        return 0;
    case usblcd_GET_OLED:
        val = lcd->oled;
        put_user(val, (unsigned int __user *)arg);
        return 0;
    case usblcd_SET_BACKLIGHT:
        ret = usblcd_backlight(lcd, arg);
        break;
    case usblcd_SET_DISPLAY:
        ret = usblcd_display(lcd, arg);
        break;
    case usblcd_SET_CURSOR:
        ret = usblcd_cursor(lcd, arg);
        break;
    case usblcd_SET_BLINK:
        ret = usblcd_blink(lcd, arg);
        break;
    case usblcd_SET_SCROLL:
        ret = usblcd_scroll_display(lcd, arg);
        break;
    case usblcd_SET_ENTRY:
        ret = usblcd_entry(lcd, arg);
        break;
    case usblcd_SET_AUTO_SCROLL:
        ret = usblcd_auto_scroll(lcd, arg);
        break;
    case usblcd_SET_CURSOR_POS:
        ret = usblcd_set_cursor(lcd, POS_ROW(arg), POS_COL(arg));
        break;
    case usblcd_CLEAR:
        ret = usblcd_clear(lcd);
        break;
    default:
        return -ENOTTY;
    }

    if (ret)
    {
        return -EFAULT;
    }
    return 0;
}

struct file_operations usblcd_ops = {
    .owner = THIS_MODULE,
    .open = usblcd_open,
    .release = usblcd_release,
    .write = usblcd_ops_write,
    .unlocked_ioctl = usblcd_ops_ioctl,
};