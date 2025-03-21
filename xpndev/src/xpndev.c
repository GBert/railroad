/*
 * "THE BEER-WARE LICENSE" (Revision 42):
 * <info@gerhard-bertelsmann.de> wrote this file. As long as you retain this
 * notice you can do whatever you want with this stuff. If we meet some day,
 * and you think this stuff is worth it, you can buy me a beer in return
 * 2021 Gerhard Bertelsmann
 * ----------------------------------------------------------------------------
 */

#include <linux/cdev.h>
#include <linux/fs.h>
#include <linux/gpio.h>
#include <linux/module.h>
#include <linux/uaccess.h>
#include <linux/version.h>
#include <linux/mutex.h>
#include "allwinner_tty.h"

static DEFINE_MUTEX(xpnchar_mutex);

/* BPi CAN PH2 -> GPIO 226 */
static uint32_t direction_pin = 226;
module_param(direction_pin, int, S_IRUSR);
MODULE_PARM_DESC(direction_pin, "direction pin");

static int majorNumber;
#define  DEVICE_NAME "xpn"
#define  CLASS_NAME  "xpn"

static struct class *xpncharClass = NULL;
static struct device *xpncharDevice = NULL;

struct xpn_device_data {
    struct cdev cdev;
    /* my data starts here */
    size_t size;
    uint8_t write_buffer[256];
    uint8_t read_buffer[256];
};

struct xpn_device_data xpn_data;

// static DEFINE_SPINLOCK(xpn_lock);

static long xpndev_ioctl(struct file *file, unsigned int cmd, unsigned long arg) {
    return 0;
}

static int xpn_open(struct inode *inode, struct file *file) {
    struct xpn_device_data *xpn_data;
    xpn_data = container_of(inode->i_cdev, struct xpn_device_data, cdev);

    printk(KERN_INFO "open\n");

    /* validate access to device */
    file->private_data = xpn_data;

    /* initialize device */

    return 0;
}

static int xpn_release(struct inode *inode, struct file *file) {
    mutex_unlock(&xpnchar_mutex);
    printk(KERN_INFO "release\n");

    return 0;
}

static int xpn_read(struct file *file, char __user * user_buffer, size_t size, loff_t * offset) {
    struct xpn_device_data *xpn_data = (struct xpn_device_data *)file->private_data;
    // ssize_t len = min(xpn_data->size - *offset, size);
    ssize_t len = 0;

    printk(KERN_INFO "read\n");
    //if (len <= 0)
    //  return 0;

    /* read data from my_data->buffer to user buffer */
    if (copy_to_user(user_buffer, xpn_data->read_buffer + *offset, len))
	return -EFAULT;

    *offset += len;
    return len;
}

static int xpn_write(struct file *file, const char __user * user_buffer, size_t size, loff_t * offset) {
    struct xpn_device_data *xpn_data = (struct xpn_device_data *)file->private_data;
    size_t len = 256;

    /* read data from user buffer to my_data->buffer */
    printk(KERN_INFO "write: %d\n", size);

    if (size < len) {
	len = size;
    }

    if (copy_from_user(xpn_data->write_buffer, user_buffer, len))
	return -EFAULT;

    // *offset += len;
    return len;
}

struct file_operations fops = {
    .owner = THIS_MODULE,
    .open = xpn_open,
    .read = xpn_read,
    .write = xpn_write,
    .release = xpn_release,
    .unlocked_ioctl = xpndev_ioctl,
};

static int __init xpn_device_init(void) {
    int ret;
    struct platform_device *pdev;

    ret = gpio_request(direction_pin, "direction pin");
    if (ret) {
	printk(KERN_ALERT "XPNChar: can't get direction PIN %d\n", direction_pin);
	goto EXIT;
    }

    ret = gpio_direction_output(direction_pin, 1);
    if (ret) {
	printk(KERN_ALERT "XPNChar: can't set direction PIN %d to output\n", direction_pin);
	goto GPIO_EXIT;
    }
    printk(KERN_INFO "XPNChar: use pin %d for RS485 D/RE\n", direction_pin);

    majorNumber = register_chrdev(0, DEVICE_NAME, &fops);
    if (majorNumber < 0) {
	printk(KERN_ALERT "XPNChar failed to register a major number\n");
	goto GPIO_EXIT;
    }
    printk(KERN_INFO "XPNChar: registered with major number %d\n", majorNumber);

    xpncharClass = class_create(THIS_MODULE, CLASS_NAME);
    if (IS_ERR(xpncharClass)) {
	printk(KERN_ALERT "Failed to register device class\n");
	goto CLASSDEV_EXIT;
    }

    xpncharDevice = device_create(xpncharClass, NULL, MKDEV(majorNumber, 0), NULL, DEVICE_NAME);
    if (IS_ERR(xpncharDevice)) {
	printk(KERN_ALERT "Failed to create the device\n");
	goto CHARDEV_EXIT;
    }
    printk(KERN_INFO "XPNChar: device class registered and created\n");

/*
    ret = allwinner_probe(pdev);
    if (ret) {
        printk(KERN_ALERT "resource mapping error\n");
        goto CHARDEV_EXIT;
    }
*/
    return 0;

CHARDEV_EXIT:
    class_destroy(xpncharClass);
CLASSDEV_EXIT:
    unregister_chrdev(majorNumber, DEVICE_NAME);
GPIO_EXIT:
    gpio_free(direction_pin);
EXIT:
    return ret;
}

static void xpn_device_exit(void) {
    gpio_free(direction_pin);
    device_destroy(xpncharClass, MKDEV(majorNumber, 0));
    class_destroy(xpncharClass);
    unregister_chrdev(majorNumber, DEVICE_NAME);
}

module_init(xpn_device_init);
module_exit(xpn_device_exit);
MODULE_DESCRIPTION("XpressNet Device Driver for Allwinner SoC");
MODULE_AUTHOR("Gerhard Bertelsmann <info@gerhard-bertelsmann.de>");
MODULE_LICENSE("GPL");
