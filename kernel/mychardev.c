#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/fs.h>
#include <linux/cdev.h>
#include <linux/device.h>
#include <linux/uaccess.h>

#define DEVICE_NAME "mychardev"
#define CLASS_NAME  "mycharclass"
#define BUFFER_SIZE 1024

static dev_t dev_number;
static struct cdev my_cdev;
static struct class *my_class;
static struct device *my_device;

static char kernel_buffer[BUFFER_SIZE];
static size_t data_len = 0;

static int my_open(struct inode *inode, struct file *file)
{
    pr_info("mychardev: device opened\n");
    return 0;
}

static int my_release(struct inode *inode, struct file *file)
{
    pr_info("mychardev: device closed\n");
    return 0;
}

static ssize_t my_read(struct file *file, char __user *user_buf,
                       size_t count, loff_t *ppos)
{
    ssize_t bytes_to_read;

    if (*ppos >= data_len)
        return 0;

    bytes_to_read = data_len - *ppos;

    if (count < bytes_to_read)
        bytes_to_read = count;

    if (copy_to_user(user_buf, kernel_buffer + *ppos, bytes_to_read)) {
        pr_err("mychardev: failed to copy data to user\n");
        return -EFAULT;
    }

    *ppos += bytes_to_read;

    pr_info("mychardev: sent %zd bytes to user\n", bytes_to_read);

    return bytes_to_read;
}

static ssize_t my_write(struct file *file, const char __user *user_buf,
                        size_t count, loff_t *ppos)
{
    size_t bytes_to_write;

    bytes_to_write = count;

    if (bytes_to_write >= BUFFER_SIZE)
        bytes_to_write = BUFFER_SIZE - 1;

    memset(kernel_buffer, 0, BUFFER_SIZE);

    if (copy_from_user(kernel_buffer, user_buf, bytes_to_write)) {
        pr_err("mychardev: failed to copy data from user\n");
        return -EFAULT;
    }

    kernel_buffer[bytes_to_write] = '\0';
    data_len = bytes_to_write;

    pr_info("mychardev: received %zu bytes from user\n", bytes_to_write);

    return bytes_to_write;
}

static struct file_operations fops = {
    .owner = THIS_MODULE,
    .open = my_open,
    .release = my_release,
    .read = my_read,
    .write = my_write,
};

static int __init mychardev_init(void)
{
    int ret;

    ret = alloc_chrdev_region(&dev_number, 0, 1, DEVICE_NAME);
    if (ret < 0) {
        pr_err("mychardev: failed to allocate device number\n");
        return ret;
    }

    cdev_init(&my_cdev, &fops);
    my_cdev.owner = THIS_MODULE;

    ret = cdev_add(&my_cdev, dev_number, 1);
    if (ret < 0) {
        pr_err("mychardev: failed to add cdev\n");
        unregister_chrdev_region(dev_number, 1);
        return ret;
    }

    my_class = class_create(CLASS_NAME);
    if (IS_ERR(my_class)) {
        pr_err("mychardev: failed to create class\n");
        cdev_del(&my_cdev);
        unregister_chrdev_region(dev_number, 1);
        return PTR_ERR(my_class);
    }

    my_device = device_create(my_class, NULL, dev_number, NULL, DEVICE_NAME);
    if (IS_ERR(my_device)) {
        pr_err("mychardev: failed to create device\n");
        class_destroy(my_class);
        cdev_del(&my_cdev);
        unregister_chrdev_region(dev_number, 1);
        return PTR_ERR(my_device);
    }

    memset(kernel_buffer, 0, BUFFER_SIZE);
    data_len = 0;

    pr_info("mychardev: module loaded. major=%d minor=%d\n",
            MAJOR(dev_number), MINOR(dev_number));

    return 0;
}

static void __exit mychardev_exit(void)
{
    device_destroy(my_class, dev_number);
    class_destroy(my_class);
    cdev_del(&my_cdev);
    unregister_chrdev_region(dev_number, 1);

    pr_info("mychardev: module unloaded\n");
}

module_init(mychardev_init);
module_exit(mychardev_exit);

MODULE_LICENSE("GPL");
MODULE_AUTHOR("OSG Team");
MODULE_DESCRIPTION("Simple Linux character device driver with read and write");
MODULE_VERSION("1.0");
