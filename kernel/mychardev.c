#include <linux/mutex.h>
#include <linux/ioctl.h>
#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/fs.h>
#include <linux/cdev.h>
#include <linux/device.h>
#include <linux/uaccess.h>

#define DEVICE_NAME "mychardev"
#define CLASS_NAME  "mycharclass"
#define BUFFER_SIZE 1024
#define MYCHAR_IOC_MAGIC 'k'
#define MYCHAR_IOC_CLEAR _IO(MYCHAR_IOC_MAGIC, 1)
#define MYCHAR_IOC_GET_LEN _IOR(MYCHAR_IOC_MAGIC, 2, int)

static dev_t dev_number;
static struct cdev my_cdev;
static struct class *my_class;
static struct device *my_device;

static char kernel_buffer[BUFFER_SIZE];
static size_t data_len = 0;
static DEFINE_MUTEX(buffer_mutex);
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

    if (mutex_lock_interruptible(&buffer_mutex))
        return -ERESTARTSYS;

    if (*ppos >= data_len) {
        mutex_unlock(&buffer_mutex);
        return 0;
    }

    bytes_to_read = data_len - *ppos;

    if (count < bytes_to_read)
        bytes_to_read = count;

    if (copy_to_user(user_buf, kernel_buffer + *ppos, bytes_to_read)) {
        mutex_unlock(&buffer_mutex);
        pr_err("mychardev: failed to copy data to user\n");
        return -EFAULT;
    }

    *ppos += bytes_to_read;

    pr_info("mychardev: sent %zd bytes to user\n", bytes_to_read);

    mutex_unlock(&buffer_mutex);
    return bytes_to_read;
}

static ssize_t my_write(struct file *file, const char __user *user_buf,
                        size_t count, loff_t *ppos)
{
    size_t bytes_to_write;

    bytes_to_write = count;

    if (bytes_to_write >= BUFFER_SIZE)
        bytes_to_write = BUFFER_SIZE - 1;

    if (mutex_lock_interruptible(&buffer_mutex))
        return -ERESTARTSYS;

    memset(kernel_buffer, 0, BUFFER_SIZE);

    if (copy_from_user(kernel_buffer, user_buf, bytes_to_write)) {
        mutex_unlock(&buffer_mutex);
        pr_err("mychardev: failed to copy data from user\n");
        return -EFAULT;
    }

    kernel_buffer[bytes_to_write] = '\0';
    data_len = bytes_to_write;

    pr_info("mychardev: received %zu bytes from user\n", bytes_to_write);

    mutex_unlock(&buffer_mutex);
    return bytes_to_write;
}

static long my_ioctl(struct file *file, unsigned int cmd, unsigned long arg)
{
    int len;

    switch (cmd) {
    case MYCHAR_IOC_CLEAR:
        if (mutex_lock_interruptible(&buffer_mutex))
            return -ERESTARTSYS;

        memset(kernel_buffer, 0, BUFFER_SIZE);
        data_len = 0;

        mutex_unlock(&buffer_mutex);

        pr_info("mychardev: buffer cleared by ioctl\n");
        return 0;

    case MYCHAR_IOC_GET_LEN:
        if (mutex_lock_interruptible(&buffer_mutex))
            return -ERESTARTSYS;

        len = data_len;

        mutex_unlock(&buffer_mutex);

        if (copy_to_user((int __user *)arg, &len, sizeof(len))) {
            pr_err("mychardev: failed to copy length to user\n");
            return -EFAULT;
        }

        pr_info("mychardev: buffer length sent to user\n");
        return 0;

    default:
        pr_err("mychardev: invalid ioctl command\n");
        return -EINVAL;
    }
}

static ssize_t buffer_size_show(struct device *dev,
                                struct device_attribute *attr,
                                char *buf)
{
    return sprintf(buf, "%d\n", BUFFER_SIZE);
}

static DEVICE_ATTR_RO(buffer_size);

static struct file_operations fops = {
    .owner = THIS_MODULE,
    .open = my_open,
    .release = my_release,
    .read = my_read,
    .write = my_write,
    .unlocked_ioctl = my_ioctl,
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
   
    ret = device_create_file(my_device, &dev_attr_buffer_size);
    if (ret < 0) {
        pr_err("mychardev: failed to create sysfs file\n");
        device_destroy(my_class, dev_number);
        class_destroy(my_class);
        cdev_del(&my_cdev);
        unregister_chrdev_region(dev_number, 1);
        return ret;
}

    memset(kernel_buffer, 0, BUFFER_SIZE);
    data_len = 0;

    pr_info("mychardev: module loaded. major=%d minor=%d\n",
            MAJOR(dev_number), MINOR(dev_number));

    return 0;
}

static void __exit mychardev_exit(void)
{
    device_remove_file(my_device, &dev_attr_buffer_size);
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
