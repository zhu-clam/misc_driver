

/*
* brief: DSP-P6A driver for reset P6A & hold P6A
*/


#include <linux/module.h>
#include <linux/moduleparam.h>
#include <linux/types.h>//add physical register read/write header file
#include <linux/string.h>
#include <linux/miscdevice.h>
#include <linux/fs.h>
#include <linux/init.h>
#include <asm/uaccess.h>
#include <linux/interrupt.h>
#include <linux/platform_device.h>
#include <linux/of.h>
#include <linux/io.h>
#include <linux/err.h>
#include <linux/mutex.h>


#define DRIVER_NAME 			"p6a"
struct irq_dev {
	char *name;
	void *reg_reset;
	void *reg_intl;
};
static struct irq_dev p6a_dev = {"p6airq"};

#define P6A_IOC_MAGIC 			'A'
//for iotcl command 
#define P6A_IOCRESET			_IO(P6A_IOC_MAGIC, 0)
#define P6A_IOCBLOCK			_IO(P6A_IOC_MAGIC, 1)
#define P6A_IOCENABLE			_IO(P6A_IOC_MAGIC, 2)
#define P6A_IOCDISABLE			_IO(P6A_IOC_MAGIC, 3)
#define P6A_IOC_MAXNR 4


#define DSP_RESET_ADDR 		0xf9700818 //reset reg addr
#define DSP_INTL_ADDR		0xf5809400 //intc reg addr
#define IOREMAP_SIZE 		1024

//for print debug info
//#define DEBUG

#ifdef DEBUG
#define print_debug printk
#else

static inline void print_debug(const char *fmt, ...)
{
	(void)fmt;
}
#endif

/*P6A software Reset bit*/
#define P6A_SW_RST_BIT (0x1 << 2)
/* #define P6A_SW_RST_BIT 0x0004 */
/*P6A interrupt clear bit*/
#define P6A_INTC_MASK (0x1 << 0)


#define P6A_INTC_OFFSET (0x50)

#define DSP_RELEASE_BIT (0xffffffff)


/* SPIN_LOCK FOR dsp-p6a driver*/
//static DEFINE_SPINLOCK(p6a_Lock);
static DEFINE_MUTEX(p6a_Lock);

/*declear wait queue for async event notify*/
static DECLARE_WAIT_QUEUE_HEAD(p6a_waitq);

static volatile int ev_press = 0;
static int p6a_status = 0;

static int p6a_open(struct inode *inode, struct file *file)
{
	printk("%s Do nothing\n",__func__);
	return 0;
}


static int p6a_release(struct inode *inode, struct file *file)
{
	printk("%s Do nothing\n",__func__);
	
	return 0;
}

static ssize_t p6a_write(struct file *file, const char __user *data, size_t len, loff_t *ppos)
{
	printk("%s Do nothing\n",__func__);
	return 0;
}

void p6a_ioreset(void)
{
	/*0xf9700818 bit2 set to 0 */
    u32 reg;
	reg = readl(p6a_dev.reg_reset);
	print_debug("before P6A IOCRESET:%#x\n", reg);
	
	if (reg & P6A_SW_RST_BIT ) //fff 
	{
		reg &= ~P6A_SW_RST_BIT;	
		writel(reg,(p6a_dev.reg_reset));//ffb
		print_debug("middle P6A IOCRESET:%#x\n", readl(p6a_dev.reg_reset));
		reg |= P6A_SW_RST_BIT; 
		writel(reg,(p6a_dev.reg_reset));//fff
	}
	else 
	{ 
		reg |= P6A_SW_RST_BIT;//FFB ->FFF		
		writel(reg,(p6a_dev.reg_reset ));
	}
	print_debug("after P6A IOCRESET:%#x\n", readl(p6a_dev.reg_reset));

}


void p6a_ioenable(void)
{
	u32 reg;

	reg = readl(p6a_dev.reg_reset);
	print_debug("before P6A_IOCENABLE :%#x\n", reg);
	if(p6a_status == 0)//meaning p6a is not start or disable;need to operate
	{
		if((reg & P6A_SW_RST_BIT) != 0)//p6a is not start;———— fff->ffb->fff
		{	
			reg &= ~P6A_SW_RST_BIT;
			writel(reg,(p6a_dev.reg_reset ));//ffb
			print_debug("middle PA6 IOCENABLE:%#x\n", readl(p6a_dev.reg_reset));
		
			reg |= P6A_SW_RST_BIT; 
			writel(reg,(p6a_dev.reg_reset ));//fff
			print_debug("after P6A IOCENABLE:%#x\n", readl(p6a_dev.reg_reset));
		}
		else //p6a is disable;———— ffb->fff
		{
			reg |= P6A_SW_RST_BIT;//FFB ->FFF		
			writel(reg,(p6a_dev.reg_reset ));//bit2 set to 1
			print_debug("LINE:%d P6A IOCENABLE:%#x\n",__LINE__, readl(p6a_dev.reg_reset));
		}
		
	}

}

void p6a_iodisable(void)
{
	u32 reg;
	reg = readl(p6a_dev.reg_reset);	
	print_debug("before P6A_IOCDISABLE :%#x\n", reg);

	reg &= ~P6A_SW_RST_BIT; //bit2 set to 0
	writel(reg,(p6a_dev.reg_reset));//ffb
	print_debug("after P6A_IOCDISABLE :%#x\n", readl(p6a_dev.reg_reset));
}

static long p6a_unlocked_ioctl(struct file *file, unsigned int cmd, unsigned long arg)
{
	mutex_lock(&p6a_Lock);
	switch(cmd)
	{
		case P6A_IOCRESET:
			p6a_ioreset();
			p6a_status = 1;
		break;
			
		case P6A_IOCBLOCK:
			/*put process into wait queue*/
			print_debug("[waitq]%s entry wait queue,ev_press = %d\n",__func__,ev_press);
			wait_event_interruptible(p6a_waitq, ev_press);
			ev_press = 0;
		break;
			
		case P6A_IOCENABLE:			
			p6a_ioenable();
			p6a_status = 1;			
		break;
	
		case P6A_IOCDISABLE:			
			p6a_iodisable();
			p6a_status = 0;
		break;
	
		default:
			print_debug("%s,Don't support cmd [%d]\n",__func__,cmd);
		break;
	}
	mutex_unlock(&p6a_Lock);
	return 0;
}

static irqreturn_t p6a_interrupt(int irq, void *dev)
{
	u32 intc_val;
	intc_val = readl(p6a_dev.reg_intl + P6A_INTC_OFFSET);
	
//	intc_val &= ~(0x1<<0);
	intc_val &= ~P6A_INTC_MASK;

	writel(intc_val, p6a_dev.reg_intl + P6A_INTC_OFFSET);

	print_debug("begin p6a interrupt!\n");
	/*wake up process whose in wait queue*/
	
	ev_press = 1;
	wake_up_interruptible(&p6a_waitq);
	print_debug("%s, ev_press = %d \n",__func__,ev_press);

	return IRQ_HANDLED;
}

static struct file_operations p6a_fops = {
	.owner        = THIS_MODULE,
	.llseek        = no_llseek,
	.write        = p6a_write,
	.unlocked_ioctl = p6a_unlocked_ioctl,
	.open        = p6a_open,
	.release    = p6a_release,
};


static struct miscdevice p6a_miscdev = {
	.minor        = MISC_DYNAMIC_MINOR,
	.name        = DRIVER_NAME,
	.fops        = &p6a_fops,
};

static int p6a_probe(struct platform_device *pdev)
{
	int ret = 0;
	
	int irq;
	/*use platform_set_drvdata & platform_get_drvdata to change data*/
	
	printk(KERN_INFO "%s : %s : %d - entry.\n", __FILE__, __func__, __LINE__);

	/*1. parse devicetree resource irq */
	p6a_dev.reg_reset = ioremap(DSP_RESET_ADDR, IOREMAP_SIZE);
	if(p6a_dev.reg_reset == NULL){
		printk(KERN_ERR "p6a reset ioremap:%#x failed\n", DSP_RESET_ADDR);
		return -1;
	}
	
	p6a_dev.reg_intl = ioremap(DSP_INTL_ADDR,IOREMAP_SIZE);
	if(p6a_dev.reg_intl == NULL){	
		printk(KERN_ERR "p6a intl ioremap:%#x failed\n", DSP_INTL_ADDR);
		goto err_iomapintl;
	}

	/*Software reset DSP module:DSP_RESET_ADDR */
	writel(DSP_RELEASE_BIT, p6a_dev.reg_reset);

	/*get irq num*/
	irq = platform_get_irq(pdev,0);
	printk("p6a get irq:%d .\n",irq);
	if (irq < 0)
        goto error;
	/*use devm_* API no need to self clean*/
	ret = devm_request_irq(&pdev->dev, irq, p6a_interrupt,IRQF_SHARED, p6a_dev.name, (void *)&p6a_dev);
    if (ret) {
        dev_err(&pdev->dev, "failure requesting irq %i\n", irq);
        goto error;
    }
	
	//ret = request_irq(irq, p6a_interrupt, IRQF_SHARED, p6a_dev.name, (void *)&p6a_dev);
	/*2.register misc device*/
	ret = misc_register(&p6a_miscdev);
	if(ret) {
		printk(KERN_ERR "cannot register miscdev (err=%d)\n", ret);
		goto error;
	}
	
	printk("Register p6a successful! \n");
	return 0;
error:
	iounmap(p6a_dev.reg_intl);
err_iomapintl:	
	iounmap(p6a_dev.reg_reset);
    return -1;
}

static int p6a_remove(struct platform_device *pdev)
{
	iounmap(p6a_dev.reg_reset);
	iounmap(p6a_dev.reg_intl);
	misc_deregister(&p6a_miscdev);
	return 0;
}


static const struct of_device_id p6a_match[] = {
	{ .compatible = "byavs,byavs-p6a",},
	{},
};
MODULE_DEVICE_TABLE(of, p6a_match);

static struct platform_driver p6a_drv = {
	.probe = p6a_probe,
	.remove = p6a_remove,
	
	.driver = {
		.name = "byavs,byavs-p6a",
		.of_match_table = p6a_match,
	},
	
};

static int __init p6a_init(void)
{
	printk(KERN_INFO "(%s:pid=%d), %s : %s : %d - entry.\n",current->comm, current->pid, __FILE__, __func__, __LINE__);
	return platform_driver_register(&p6a_drv);
}

static void __exit p6a_exit(void)
{
	printk(KERN_INFO "(%s:pid=%d), %s : %s : %d - leave.\n",current->comm, current->pid, __FILE__, __func__, __LINE__);
	platform_driver_unregister(&p6a_drv);
}

module_init(p6a_init);
module_exit(p6a_exit);

MODULE_AUTHOR("Byavs");
MODULE_DESCRIPTION("Byavs p6a irq event Device Driver");
MODULE_LICENSE("GPL");


