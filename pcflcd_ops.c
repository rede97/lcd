#include "pcflcd.h"
#include "pcflcd_ioctl.h"

static int pcflcd_open(struct inode *inode, struct file *filp)
{
    struct pcflcd *lcd = container_of(inode->i_cdev, struct pcflcd, cdev);
    if (MUTEX_TRYLOCK_SUCCESS != mutex_trylock(&lcd->lock))
    {
        return -EBUSY;
    }
    filp->private_data = lcd;
    return 0;
}

static int pcflcd_release(struct inode *inode, struct file *filp)
{
    struct pcflcd *lcd = container_of(inode->i_cdev, struct pcflcd, cdev);
    mutex_unlock(&lcd->lock);
    return 0;
}

static ssize_t pcflcd_ops_write(struct file *filp, const char __user *buf, size_t count, loff_t *pos)
{
    int i;
    int ret;
    struct pcflcd *lcd = filp->private_data;
    char *buffer = kmalloc(count, GFP_KERNEL);
    ret = copy_from_user(buffer, buf, count);
    if (ret)
    {
        return -EFAULT;
    }
    for (i = 0; i < count; i++)
    {
        int err;
        err = pcflcd_dat(lcd, buffer[i]);
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

static long pcflcd_ops_ioctl(struct file *filp, unsigned int cmd, unsigned long arg)
{
    int ret;
    unsigned int val;
    struct pcflcd *lcd = filp->private_data;
    dev_info(lcd->device, "cmd: 0x%08x\n", cmd);
    if (_IOC_TYPE(cmd) != PCFLCD_MAGIC)
    {
        return -ENOTTY;
    }

    switch (cmd)
    {
    case PCFLCD_GET_SIZE:
        val = MK_POS(lcd->rows, lcd->cols);
        put_user(val, (unsigned int __user *)arg);
        return 0;
    case PCFLCD_GET_OLED:
        val = lcd->oled;
        put_user(val, (unsigned int __user *)arg);
        return 0;
    case PCFLCD_SET_BACKLIGHT:
        ret = pcflcd_backlight(lcd, arg);
        break;
    case PCFLCD_SET_DISPLAY:
        ret = pcflcd_display(lcd, arg);
        break;
    case PCFLCD_SET_CURSOR:
        ret = pcflcd_cursor(lcd, arg);
        break;
    case PCFLCD_SET_BLINK:
        ret = pcflcd_blink(lcd, arg);
        break;
    case PCFLCD_SET_SCROLL:
        ret = pcflcd_scroll_display(lcd, arg);
        break;
    case PCFLCD_SET_ENTRY:
        ret = pcflcd_entry(lcd, arg);
        break;
    case PCFLCD_SET_AUTO_SCROLL:
        ret = pcflcd_auto_scroll(lcd, arg);
        break;
    case PCFLCD_SET_CURSOR_POS:
        ret = pcflcd_set_cursor(lcd, POS_ROW(arg), POS_COL(arg));
        break;
    case PCFLCD_CLEAR:
        ret = pcflcd_clear(lcd);
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

struct file_operations pcflcd_ops = {
    .owner = THIS_MODULE,
    .open = pcflcd_open,
    .release = pcflcd_release,
    .write = pcflcd_ops_write,
    .unlocked_ioctl = pcflcd_ops_ioctl,
};