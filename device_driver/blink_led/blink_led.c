#include <linux/module.h>
#include <linux/device.h>
#include <linux/init.h>
#include <linux/fs.h>
#include <linux/cdev.h>
#include <linux/errno.h>
#include <linux/device.h>
#include <linux/types.h>
#include <linux/delay.h>
#include <asm/uaccess.h>
#include <linux/serial_reg.h>
#include <asm/io.h>

int major_no;
unsigned int *addr;
unsigned int data;

MODULE_LICENSE("GPL");

int myopen(struct inode *inod, struct file *fptr)
{
    printk(KERN_ALERT "%s blink led\n",__FUNCTION__);
    return 0;
}

ssize_t myread(struct file *fptr, char __user *usrbuff, size_t size, loff_t *offset)
{
    printk(KERN_ALERT "%s blink led\n",__FUNCTION__);
    return size;
}

int atoi(const char *buff)
{
    int i = 0, result = 0;
    for(i = 0; buff[i] != 0;i++)
    {
	if(buff[i] <= '9' && buff[i] >= '0')
	{
	    result = (result * 10) + (buff[i] - 48);
	}
	else
	{
	    break;
	}
    }
    return result;
}

ssize_t mywrite(struct file *fptr, const char __user *usrbuff, size_t size, loff_t *offset)
{
    int pin = atoi(usrbuff),i,value = 0;
    printk(KERN_ALERT "PIN %d\n",pin);
    for(i = 0; usrbuff != '\0' && usrbuff[i] != ' '; i++);
    if(usrbuff[i] != '\0')
    {
	value = atoi(&usrbuff[i + 1]);
    }
    printk(KERN_ALERT "VALUE %d\n",value);
    printk(KERN_ALERT "%s blink led\n",__FUNCTION__);
    addr = (unsigned int *)ioremap(0x4804C13C,sizeof(int));
    data = ioread32(addr);
    data = ((data | (1 << pin)) & ((value << pin) | ~(1 << pin)));
    iowrite32(data,addr);

    return size;
}

int myclose(struct inode *inod, struct file *fptr)
{
    printk(KERN_ALERT "%s blink led\n",__FUNCTION__);
    return 0;
}

struct file_operations file_operation = {
    .owner = THIS_MODULE,
    .open = myopen,
    .release = myclose,
    .read = myread,
    .write = mywrite,
};

dev_t dev;
struct cdev cdev;
struct class *cl;

__init int blink_led_init(void)
{
    printk(KERN_ALERT "%s\n",__FUNCTION__);
    if(alloc_chrdev_region(&dev,0,5,"led_blink") < 0)
    {
	printk(KERN_ALERT "ERROR %s\n",__FUNCTION__);
	return -1;
    }
    printk(KERN_ALERT "MINOR %d\n",MINOR(dev));
    printk(KERN_ALERT "MAJOR %d\n",MAJOR(dev));
    if((cl = class_create(THIS_MODULE,"led_blink")) == NULL)
    {
	unregister_chrdev_region(dev,5);
	printk(KERN_ALERT "ERROR %s\n",__FUNCTION__);
	return -1;
    }
    if(device_create(cl,NULL,dev,NULL,"led%d",0) == NULL)
    {
	class_destroy(cl);
	unregister_chrdev_region(dev,5);
	printk(KERN_ALERT "ERROR %s\n",__FUNCTION__);
	return -1;
    }
    cdev_init(&cdev,&file_operation);
    if(cdev_add(&cdev,dev,5) < 0)
    {
	device_destroy(cl,0);
	class_destroy(cl);
	unregister_chrdev_region(dev,5);
	printk(KERN_ALERT "ERROR %s\n",__FUNCTION__);
	return -1;
    }

    addr = (unsigned int *)ioremap(0x4804C134,sizeof(int));
    data = (ioread32(addr) & ~(1 << 16));
    iowrite32(0,addr);
    addr = (unsigned int *)ioremap(0x4804C13C,sizeof(int));
    data = ioread32(addr);
    iowrite32(-1,addr);

    return 0;
}


__exit void  blink_led_exit(void)
{
    printk(KERN_ALERT "%s\n",__FUNCTION__);
    cdev_del(&cdev);
    class_destroy(cl);
    device_destroy(cl,dev);
    unregister_chrdev_region(dev,5);
}

module_init(blink_led_init);
module_exit(blink_led_exit);
