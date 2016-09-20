#include <asm/uaccess.h>
#include <linux/cdev.h>
#include <linux/device.h>
#include <linux/fs.h>
#include <linux/hashtable.h>
#include <linux/init.h>
#include <linux/ioctl.h>
#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/slab.h>

#include "ht530_drv.h"

#define DEVICE_NAME "ht530"
#define TABLE_NAME ht530_tbl
DEFINE_HASHTABLE(TABLE_NAME, 7);

static int majorNumber;
static struct class *ht530_drv_class;
static struct device *ht530_dev_device;

typedef struct ht_element {
    ht_object_t object;
    struct hlist_node node;
} ht_element_t;

int ht530_drv_open(struct inode *inode, struct file *file) {
    printk("Open called\n");
    return 0;
}

ssize_t ht530_drv_write(struct file *file, const char *buf, size_t count, loff_t *ppos) {
    ht_object_t *object = (ht_object_t *)buf;
    ht_element_t *element = NULL;
    int key, data;
    get_user(key, &object->key);
    get_user(data, &object->data);
    printk("write called, key=%d, data=%d\n", key, data);
    hash_for_each_possible(TABLE_NAME, element, node, key) {
        break;
    }
    if (data) {
        if (!element) {
            element = kmalloc(sizeof(ht_element_t), GFP_KERNEL);
            element->object.key = key;
            hash_add(TABLE_NAME, &element->node, key);
        }
        element->object.data = data;
    } else {
        if (element) {
            hash_del(&element->node);
            kfree(element);
        }
    }
    return 0;
}

ssize_t ht530_drv_read(struct file *file, char *buf, size_t count, loff_t *ppos) {
    ht_object_t *object = (ht_object_t *)buf;
    ht_element_t *element;
    int key;
    get_user(key, &object->key);
    printk("read called, key=%d\n", key);
    hash_for_each_possible(TABLE_NAME, element, node, key) {
        put_user(element->object.data, &object->data);
        return 0;
    }
    return -EINVAL;
}

long ht530_drv_ioctl (struct file *file, unsigned int cmd, unsigned long arg) {
    dump_arg_t *ptr = (dump_arg_t *)arg;
    int n, count = 0;
    ht_element_t *element;
    get_user(n, &ptr->n);
    if (n < 0 || n > 127)
        return -EINVAL;
    switch (cmd) {
        case DUMP:
            hlist_for_each_entry(element, &TABLE_NAME[n], node) {
                if (count < 8)
                    copy_to_user(&ptr->object_array[count++], &element->object, sizeof(ht_object_t));
                else
                    break;
            }
            break;
        default:
            return -EINVAL;
    }
    return count;
}

int ht530_drv_release(struct inode *inode, struct file *file) {
    printk("release called\n");
    return 0;
}

static struct file_operations fops = {
    .open = ht530_drv_open,                 /* Open method */
    .write = ht530_drv_write,               /* Write method */
    .read = ht530_drv_read,                 /* Read method */
    .unlocked_ioctl = ht530_drv_ioctl,      /* Ioctl method */
    .release = ht530_drv_release,           /* Release method */
};


int __init ht530_drv_init(void) {
    majorNumber = register_chrdev(0, DEVICE_NAME, &fops);
    if (majorNumber < 0) {
        printk("Failed to register a major number\n");
        return majorNumber;
    }
    ht530_drv_class = class_create(THIS_MODULE, DEVICE_NAME);
    if (IS_ERR(ht530_drv_class)) {
            unregister_chrdev(majorNumber, DEVICE_NAME);
            printk("Failed to create the device\n");
    }
    ht530_dev_device = device_create(ht530_drv_class, NULL, MKDEV(majorNumber, 0), NULL, DEVICE_NAME);
    hash_init(TABLE_NAME);
    printk("ht530_drv initialized.\n");
    return 0;
}

void __exit ht530_drv_exit(void) {
    int bkt;
    ht_element_t *element;
    device_destroy (ht530_drv_class, MKDEV(majorNumber, 0));
    class_unregister(ht530_drv_class);
    class_destroy(ht530_drv_class);
    unregister_chrdev(majorNumber, DEVICE_NAME);
    hash_for_each(TABLE_NAME, bkt, element, node) {
        hash_del(&element->node);
        kfree(element);
    }
    printk("Goodbye!\n");
}

module_init(ht530_drv_init);
module_exit(ht530_drv_exit);
MODULE_LICENSE("GPL v2");
