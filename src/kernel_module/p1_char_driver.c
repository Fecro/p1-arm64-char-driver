#include <linux/init.h>
#include <linux/module.h>
#include <linux/fs.h>
#include <linux/cdev.h>
#include <linux/device.h>
#include <linux/uaccess.h>
#include <linux/mutex.h>
#include <linux/slab.h>

#define DEVICE_NAME "p1_char"
#define CLASS_NAME "p1_char_class"
#define BUFFER_SIZE 1024

MODULE_LICENSE("GPL");
MODULE_AUTHOR("Franklin Custodio <fecr88@protonmail.ch>");
MODULE_DESCRIPTION(
	"Character Device Driver for ARM64 with Synchronization and Dynamic Allocation");
MODULE_VERSION("1.0");

/* Internal representation structure of the device */
struct p1_char_dev {
	dev_t dev_num; /* Combined major and minor numbers */
	struct cdev cdev; /* VFS cdev structure */
	struct class *dev_class; /* sysfs class for udev/devtmpfs */
	struct device *dev_device; /* Generated device node */
	char *kernel_buffer; /* Memory buffer allocated with kmalloc */
	size_t data_size; /* Amount of data currently stored */
	struct mutex lock; /* Mutex for concurrency control */
};

static struct p1_char_dev g_dev;

/* VFS function prototypes */
static int dev_open(struct inode *inode, struct file *file);
static int dev_release(struct inode *inode, struct file *file);
static ssize_t dev_read(struct file *file, char __user *user_buf, size_t count,
			loff_t *ppos);
static ssize_t dev_write(struct file *file, const char __user *user_buf,
			 size_t count, loff_t *ppos);

/* File operations table (VFS interface) */
static const struct file_operations fops = {
	.owner = THIS_MODULE,
	.open = dev_open,
	.release = dev_release,
	.read = dev_read,
	.write = dev_write,
};

/**
 * dev_open - Invoked when a user-space application calls open()
 */
static int dev_open(struct inode *inode, struct file *file)
{
	pr_info("p1_char: Process '%s' (PID: %d) opened the device\n",
		current->comm, current->pid);
	return 0;
}

/**
 * dev_release - Called when the application calls close()
 */
static int dev_release(struct inode *inode, struct file *file)
{
	pr_info("p1_char: Device successfully closed by PID: %d\n",
		current->pid);
	return 0;
}

/**
 * dev_read - Invoked when the application calls read()
 * Data transfer from Kernel to User Space (EL1 -> EL0)
 */
static ssize_t dev_read(struct file *file, char __user *user_buf, size_t count,
			loff_t *ppos)
{
	ssize_t bytes_to_read;
	unsigned long uncopied;

	/* Acquisition of the lock to prevent concurrent modifications */
	if (mutex_lock_interruptible(&g_dev.lock)) {
		pr_err("p1_char: Interruption while waiting for the mutex during read\n");
		return -ERESTARTSYS;
	}

	if (*ppos >= g_dev.data_size) {
		/* EOF (End of File) reached */
		mutex_unlock(&g_dev.lock);
		return 0;
	}

	/* Calculation of available bytes starting from position ppos */
	bytes_to_read = min_t(size_t, count, g_dev.data_size - *ppos);

	/* Safe copy from kernel space to user space */
	uncopied = copy_to_user(user_buf, g_dev.kernel_buffer + *ppos,
				bytes_to_read);
	if (uncopied != 0) {
		pr_err("p1_char: Failed to copy %lu bytes to user space\n",
		       uncopied);
		mutex_unlock(&g_dev.lock);
		return -EFAULT;
	}

	*ppos += bytes_to_read;
	pr_info("p1_char: Sent %zd bytes to user space (pos: %lld)\n",
		bytes_to_read, *ppos);

	mutex_unlock(&g_dev.lock);
	return bytes_to_read;
}

/**
 * dev_write - Invoked when the application calls write()
 * Data transfer from User Space to the Kernel (EL0 -> EL1)
 */
static ssize_t dev_write(struct file *file, const char __user *user_buf,
			 size_t count, loff_t *ppos)
{
	ssize_t bytes_to_write;
	unsigned long uncopied;

	if (mutex_lock_interruptible(&g_dev.lock)) {
		pr_err("p1_char: Interruption while waiting for the mutex during write\n");
		return -ERESTARTSYS;
	}

	/* Limit writing to the maximum capacity of the kernel buffer */
	bytes_to_write = min_t(size_t, count, (size_t)BUFFER_SIZE);

	/* Clear buffer before new write */
	memset(g_dev.kernel_buffer, 0, BUFFER_SIZE);

	/* Safe copy from user space to kernel space */
	uncopied =
		copy_from_user(g_dev.kernel_buffer, user_buf, bytes_to_write);
	if (uncopied != 0) {
		pr_err("p1_char: Failed to receive %lu bytes from user space\n",
		       uncopied);
		mutex_unlock(&g_dev.lock);
		return -EFAULT;
	}

	g_dev.data_size = bytes_to_write;
	*ppos = bytes_to_write;

	pr_info("p1_char: Received %zd bytes from user space\n",
		bytes_to_write);

	mutex_unlock(&g_dev.lock);
	return bytes_to_write;
}

/**
 * p1_char_init - Module initialization function
 */
static int __init p1_char_init(void)
{
	int ret;

	pr_info("p1_char: Initializing ARM64 character driver...\n");

	/* 1. Dynamic assignment of Major and Minor numbers */
	ret = alloc_chrdev_region(&g_dev.dev_num, 0, 1, DEVICE_NAME);
	if (ret < 0) {
		pr_err("p1_char: Failed to assign major/minor number (error: %d)\n",
		       ret);
		return ret;
	}
	pr_info("p1_char: Major assigned number: %d, Minor: %d\n",
		MAJOR(g_dev.dev_num), MINOR(g_dev.dev_num));

	/* 2. CDEV initialization and linking with VFS operations */
	cdev_init(&g_dev.cdev, &fops);
	g_dev.cdev.owner = THIS_MODULE;

	ret = cdev_add(&g_dev.cdev, g_dev.dev_num, 1);
	if (ret < 0) {
		pr_err("p1_char: Failed to add cdev to system (error: %d)\n",
		       ret);
		goto fail_unregister_chrdev;
	}

	/* 3. Creation of the class in sysfs (/sys/class/p1_char_class) */
	g_dev.dev_class = class_create(CLASS_NAME);
	if (IS_ERR(g_dev.dev_class)) {
		ret = PTR_ERR(g_dev.dev_class);
		pr_err("p1_char: Failed to create sysfs class (error: %d)\n",
		       ret);
		goto fail_cdev_del;
	}

	/* 4. Creation of the device node (/dev/p1_char) */
	g_dev.dev_device = device_create(g_dev.dev_class, NULL, g_dev.dev_num,
					 NULL, DEVICE_NAME);
	if (IS_ERR(g_dev.dev_device)) {
		ret = PTR_ERR(g_dev.dev_device);
		pr_err("p1_char: Failed to create device node (error: %d)\n",
		       ret);
		goto fail_class_destroy;
	}

	/* 5. Memory allocation in Kernel Space */
	g_dev.kernel_buffer = kmalloc(BUFFER_SIZE, GFP_KERNEL);
	if (!g_dev.kernel_buffer) {
		pr_err("p1_char: No memory to allocate kernel buffer\n");
		ret = -ENOMEM;
		goto fail_device_destroy;
	}

	/* 6. Mutex initialization */
	mutex_init(&g_dev.lock);
	g_dev.data_size = 0;

	pr_info("p1_char: Driver loaded successfully. Node available in /dev/%s\n",
		DEVICE_NAME);
	return 0;

fail_device_destroy:
	device_destroy(g_dev.dev_class, g_dev.dev_num);
fail_class_destroy:
	class_destroy(g_dev.dev_class);
fail_cdev_del:
	cdev_del(&g_dev.cdev);
fail_unregister_chrdev:
	unregister_chrdev_region(g_dev.dev_num, 1);
	return ret;
}

/**
 * p1_char_exit - Module cleanup and unload function
 */
static void __exit p1_char_exit(void)
{
	pr_info("p1_char: Removing the module...\n");

	mutex_destroy(&g_dev.lock);
	kfree(g_dev.kernel_buffer);
	device_destroy(g_dev.dev_class, g_dev.dev_num);
	class_destroy(g_dev.dev_class);
	cdev_del(&g_dev.cdev);
	unregister_chrdev_region(g_dev.dev_num, 1);

	pr_info("p1_char: Driver unloaded successfully.\n");
}

module_init(p1_char_init);
module_exit(p1_char_exit);
