#include <asm/uaccess.h>
#include <linux/cdev.h>
#include <linux/delay.h>
#include <linux/device.h>
#include <linux/fs.h>
#include <linux/init.h>
#include <linux/i2c.h>
#include <linux/i2c-dev.h>
#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/slab.h>
#include <linux/workqueue.h>

#include "i2c_flash.h"

#define DEVICE_NAME "i2c_flash"
#define DEVICE_ADDRESS 0x51
#define READ_OP 0
#define WRITE_OP 1
#define ERASE_OP 2

typedef struct {
    struct cdev cdev;
    struct i2c_client *client;
    int pageNum;
    int isBusy;
    char *readPtr;
    int readCount;
} i2c_flash_dev;


typedef struct {
    struct work_struct work;
    i2c_flash_dev *dev_ptr;
    char *content;
    int count;
    int work_func;
} work_t;


static dev_t i2c_flash_number;
static struct class *i2c_flash_class;
static struct device *i2c_flash_device;
static i2c_flash_dev *devp;
static struct workqueue_struct *work_queue;


void setAddress(char *address, int pageNum) {
    address[0] = pageNum >> 2;
    address[1] = (pageNum & 0x03) << 6;
}


static void work_func(struct work_struct *_work) {
    static char address[2];
    static char buf[66];
    work_t *work = container_of(_work, work_t, work);
    i2c_flash_dev *dev_ptr = work->dev_ptr;
    char *content;
    int count = work->count;
    int result = -1, pageNum;
    dev_ptr->isBusy--;
    switch (work->work_func) {
        case READ_OP:
            dev_ptr->readPtr = (char *)kmalloc(count * 64, GFP_KERNEL);
            content = dev_ptr->readPtr;
            while (++result < count) {
                setAddress(address, dev_ptr->pageNum);
                if (i2c_master_send(dev_ptr->client, address, 2) < 0) {
                    printk("ERROR: Set position failed.\n");
                    kfree(dev_ptr->readPtr);
                    dev_ptr->readPtr = NULL;
                    return;
                }
                udelay(10000);
                if (i2c_master_recv(dev_ptr->client, content, 64) < 0) {
                    printk("ERROR: Read from EEPROM failed.\n");
                    kfree(dev_ptr->readPtr);
                    dev_ptr->readPtr = NULL;
                    return;
                }
                content += 64;
                dev_ptr->pageNum = (dev_ptr->pageNum + 1) % 512;
            }
            dev_ptr->readCount = result;
            break;
        case WRITE_OP:
            content = work->content;
            while (++result < count) {
                setAddress(buf, dev_ptr->pageNum);
                strncpy(buf + 2, content, 64);
                if (i2c_master_send(dev_ptr->client, buf, 66) < 0) {
                    printk("ERROR: Write to EEPROM failed.\n");
                    kfree(work->content);
                    return;
                }
                content += 64;
                dev_ptr->pageNum = (dev_ptr->pageNum + 1) % 512;
                udelay(10000);
            }
            // Release temporary buffer
            kfree(work->content);
            break;
        case ERASE_OP:
            pageNum = -1;
            memset(buf, 0xFF, sizeof(buf));
            while (++pageNum < 512) {
                setAddress(buf, pageNum);
                if (i2c_master_send(dev_ptr->client, buf, 66) < 0) {
                    printk("ERROR: Write to EEPROM failed.\n");
                    return;
                }
                udelay(10000);
            }
            dev_ptr->pageNum = 0;
            break;
        default:
            break;
    }
    kfree(work);
}


int i2c_flash_open(struct inode *inode, struct file *file) {
    i2c_flash_dev *dev_ptr = container_of(inode->i_cdev, i2c_flash_dev, cdev);
    file->private_data = dev_ptr;
    printk("Open called.\n");
    return 0;
}


ssize_t i2c_flash_write(struct file *file, const char *content, size_t count, loff_t *ppos) {
    i2c_flash_dev *dev_ptr = file->private_data;
    work_t *work;
    if (dev_ptr->isBusy) {
        printk("ERROR: Driver is busy.\n");
        return -EBUSY;
    }
    // Create work with content from user side
    work = (work_t *)kmalloc(sizeof(work_t), GFP_KERNEL);
    if (work == NULL) {
        printk("ERROR: Kmalloc in write failed.\n");
        return -EBUSY;
    }
    dev_ptr->isBusy++;
    INIT_WORK(&work->work, work_func);
    work->dev_ptr = dev_ptr;
    work->content = (char *)kmalloc(count * 64, GFP_KERNEL);
    if (work->content == NULL) {
        printk("ERROR: Kmalloc in write failed.\n");
        kfree(work);
        return -EBUSY;
    }
    if (copy_from_user(work->content, content, count * 64) != 0) {
        printk("ERROR: Copy from user failed.\n");
        return -EBUSY;
    }
    work->count = count;
    work->work_func = WRITE_OP;
    queue_work(work_queue, &work->work);
    return 0;
}


ssize_t i2c_flash_read(struct file *file, char *content, size_t count, loff_t *ppos) {
    i2c_flash_dev *dev_ptr = file->private_data;
    work_t *work;
    int result = 0;
    if (dev_ptr->readPtr) {
        if (dev_ptr->readCount < count)
            count = dev_ptr->readCount;
        if (copy_to_user(content, dev_ptr->readPtr, count * 64) != 0) {
            printk("ERROR: Copy to user failed.\n");
            result = -1;
        }
        // Release temporary buffer after transferred to user
        kfree(dev_ptr->readPtr);
        dev_ptr->readPtr = NULL;
        return result;
    }
    if (dev_ptr->isBusy) {
        return -EBUSY;
    }
    // Create work with desired count
    work = (work_t *)kmalloc(sizeof(work_t), GFP_KERNEL);
    if (work == NULL) {
        printk("ERROR: Kmalloc in read failed.\n");
        return -EAGAIN;
    }
    dev_ptr->isBusy++;
    INIT_WORK(&work->work, work_func);
    work->dev_ptr = dev_ptr;
    work->count = count;
    work->work_func = READ_OP;
    queue_work(work_queue, &work->work);
    return -EAGAIN;
}


long i2c_flash_ioctl(struct file *file, unsigned int cmd, unsigned long arg) {
    i2c_flash_dev *dev_ptr = file->private_data;
    int isBusy = dev_ptr->isBusy;
    work_t *work;
    switch(cmd) {
        case FLASHGETS:
            if (copy_to_user((char *)arg, (char *)&isBusy, sizeof(isBusy)) != 0) {
                printk("ERROR: Copy to user failed.\n");
            }
            break;
        case FLASHGETP:
            printk("Current page is %d", dev_ptr->pageNum);
            if (copy_to_user((char *)arg, (char *)&dev_ptr->pageNum, sizeof(dev_ptr->pageNum))) {
                printk("ERROR: Copy to user failed.\n");
            }
            break;
        case FLASHSETP:
            if (arg < 0 || arg >= 512)
                return -EINVAL;
            else if (isBusy)
                return -EBUSY;
            dev_ptr->pageNum = (int)arg;
            break;
        case FLASHERASE:
            if (isBusy)
                return -EBUSY;
            // Create work with erase operation
            work = (work_t *)kmalloc(sizeof(work_t), GFP_KERNEL);
            if (work == NULL) {
                printk("ERROR: Kmalloc in read failed.\n");
                return -EBUSY;
            }
            dev_ptr->isBusy++;
            INIT_WORK(&work->work, work_func);
            work->dev_ptr = dev_ptr;
            work->work_func = ERASE_OP;
            queue_work(work_queue, &work->work);
            break;
        default:
            return -EINVAL;
    }
    return 0;
}


int i2c_flash_release(struct inode *inode, struct file *file) {
    printk("Release called.\n");
    return 0;
}

static struct file_operations fops = {
    .open = i2c_flash_open,
    .write = i2c_flash_write,
    .read = i2c_flash_read,
    .unlocked_ioctl = i2c_flash_ioctl,
    .release = i2c_flash_release, 
};

int __init i2c_flash_init(void) {
    if (alloc_chrdev_region(&i2c_flash_number, 0, 1, DEVICE_NAME) < 0) {
        printk("Can't register device.\n");
        return -1;
    }
    i2c_flash_class = class_create(THIS_MODULE, DEVICE_NAME);
    devp = kmalloc(sizeof(i2c_flash_dev), GFP_KERNEL);
    cdev_init(&devp->cdev, &fops);
    devp->cdev.owner = THIS_MODULE;
    cdev_add(&devp->cdev, i2c_flash_number, 1);
    i2c_flash_device = device_create(i2c_flash_class, NULL, MKDEV(MAJOR(i2c_flash_number), 0), NULL, DEVICE_NAME);
    devp->client = (struct i2c_client *)kmalloc(sizeof(struct i2c_client), GFP_KERNEL);
    devp->client->addr = DEVICE_ADDRESS;
    snprintf(devp->client->name, I2C_NAME_SIZE, "i2c_flash");
    devp->client->adapter = i2c_get_adapter(0);
    if (!devp->client->adapter) {
        printk("ERROR: Acquire I2C adapter failed.\n");
        return -1;
    }
    devp->pageNum = 0;
    devp->readPtr = NULL;
    devp->isBusy = devp->readCount = 0;
    // Create a work queue
    work_queue = create_workqueue("work_queue");
    printk("i2c_flash initialized.\n");
    return 0;
}


void __exit i2c_flash_exit(void) {
    destroy_workqueue(work_queue);
    i2c_put_adapter(devp->client->adapter);
    kfree(devp->client);
    unregister_chrdev_region(i2c_flash_number, 1);
    device_destroy(i2c_flash_class, MKDEV(MAJOR(i2c_flash_number), 0));
    cdev_del(&devp->cdev);
    kfree(devp);
    class_destroy(i2c_flash_class);
    printk("Goodbye!\n");
}

module_init(i2c_flash_init);
module_exit(i2c_flash_exit);
MODULE_LICENSE("GPL v2");
