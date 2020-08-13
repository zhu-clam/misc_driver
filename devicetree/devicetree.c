/* 
*
* get devicetree information
* author: helb
* date: 2018-08-08
* 
*/


#include <linux/module.h>
#include <linux/moduleparam.h>
#include <linux/types.h>
#include <linux/string.h>
#include <linux/miscdevice.h>
#include <linux/fs.h>
#include <linux/init.h>
#include <asm/uaccess.h>
#include <linux/of.h>
#include <linux/of_fdt.h>
#include <linux/of_device.h>
#include <linux/libfdt.h>

#define DRIVER_NAME 		"dtb"
#define DTB_IOC_MAGIC 			'D'
#define DTB_IOCDTBSIZE 			_IO(DTB_IOC_MAGIC, 0)
#define DTB_IOCGETDTB 			_IO(DTB_IOC_MAGIC, 1)

//ADD SPIN_LOCK for protect share-data area
static DEFINE_SPINLOCK(dtbsize_Lock);
static DEFINE_SPINLOCK(getdtb_Lock);

static void *fdt = NULL;

struct dtb_info{
	void *params;
	unsigned int size;
	unsigned int reserve[2];
};

static int dtb_open(struct inode *inode, struct file *file)
{
	printk("Open: do nothing\n");
	return 0;
}

static int dtb_release(struct inode *inode, struct file *file)
{
	printk("Release: do nothing\n");
	return 0;
}

static ssize_t dtb_write(struct file *file, const char __user *data, size_t len, loff_t *ppos)
{
	printk("Write: do nothing\n");
	return 0;
}

static long dtb_unlocked_ioctl(struct file *file, unsigned int cmd, unsigned long arg)
{
	struct dtb_info st_dtb;
	int ret = -1;
	
	memset((void *)&st_dtb, 0, sizeof(struct dtb_info));

	switch(cmd)
	{
		case DTB_IOCDTBSIZE:
			spin_lock(&dtbsize_Lock);
			ret = copy_from_user(&st_dtb, (void __user *)arg, sizeof(struct dtb_info));
			if(ret){
				printk("Copy from user failed\n");
				goto error;
			}
			st_dtb.size = fdt_totalsize(fdt);
			ret = copy_to_user((void __user *)arg, &st_dtb, sizeof(struct dtb_info));
			if(ret){
				printk("Copy to user failed\n");
				goto error;
			}
			spin_unlock(&dtbsize_Lock);
		break;
		case DTB_IOCGETDTB:
			spin_lock(&getdtb_Lock);
			ret = copy_from_user(&st_dtb, (void __user *)arg, sizeof(struct dtb_info));
			if(ret){
				printk("Copy from user failed\n");
				goto error;
			}
			if(NULL == st_dtb.params){
				printk("Please malloc dtb params space\n");
				goto error;
			}
			memcpy(st_dtb.params, fdt, st_dtb.size);
			ret = copy_to_user((void __user *)arg, &st_dtb, sizeof(struct dtb_info));
			if(ret){
				printk("Copy to user failed\n");
				goto error;
			}
			spin_lock(&getdtb_Lock);
		break;
		default:
			printk("Don't support cmd [%d]\n", cmd);
			break;
	}
	return 0;
error:
	return -EFAULT;
}

/*
 *    Kernel Interfaces
 */

static struct file_operations dtb_fops = {
    .owner        = THIS_MODULE,
    .llseek        = no_llseek,
    .write        = dtb_write,
    .unlocked_ioctl = dtb_unlocked_ioctl,
    .open        = dtb_open,
    .release    = dtb_release,
};

static struct miscdevice dtb_miscdev = {
    .minor        = MISC_DYNAMIC_MINOR,
    .name        = DRIVER_NAME,
    .fops        = &dtb_fops,
};

static int __init dtb_init(void)
{
    int ret = 0;

    ret = misc_register(&dtb_miscdev);
    if(ret) {
        printk (KERN_ERR "cannot register miscdev (err=%d)\n", ret);
		return ret;
    }
	fdt = initial_boot_params;
	printk("Get dtb information, at:%p, initial_boot_params: %#x\n", fdt, (u32)initial_boot_params);

	return 0;
}

static void __exit dtb_exit(void)
{    
    misc_deregister(&dtb_miscdev);
}

module_init(dtb_init);
module_exit(dtb_exit);

MODULE_AUTHOR("Byavs");
MODULE_DESCRIPTION("Byavs devicetree Device Driver");
MODULE_LICENSE("GPL");

