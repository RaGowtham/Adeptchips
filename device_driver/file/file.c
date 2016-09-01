#include <linux/init.h>
#include <linux/device.h>
#include <linux/cdev.h>
#include <linux/module.h>
#include <linux/uaccess.h>
#include <linux/errno.h>
#include <linux/fs.h>
#include <linux/types.h>

#define FIRST_MINOR 0
#define MINOR_COUNT 5


MODULE_LICENSE("GPL");

dev_t dev;
struct class *cl;
struct cdev cdev;



char buffer[100] = {0};
int i, j;

int myopen(struct inode *node, struct file *fp)
{
    j = 0;
    return 0;
}
int myclose(struct inode *node, struct file *fp)
{
    j = 0;
    return 0;
}
ssize_t myread(struct file *fp, char __user *usrbuff, size_t len, loff_t *offset)
{
    int count = 0;
    for(i = j; buffer[i] != '\0' && i < len; i++,j++)
    {
    	usrbuff[i] = buffer[j];
    	count++;
    }
    return count;
}
ssize_t mywrite(struct file *fp, const char __user *usrbuff, size_t len, loff_t *offset)
{
    for(i = 0; i < len;i++)
    {
    	buffer[i] = usrbuff[i];
    }
    return i;
}

struct file_operations myfile_operations = {
    .owner = THIS_MODULE,
    .open = myopen,
    .release = myclose,
    .read = myread,
    .write = mywrite,
};

__init int file_driver_init(void)
{
    printk(KERN_ALERT "INIT PROCESS\n");

    if(alloc_chrdev_region(&dev,FIRST_MINOR,MINOR_COUNT,"file_driver") < 0)
    {
	printk(KERN_ALERT "ERROR IN REGISTER\n");
	return -1;
    }
    printk(KERN_ALERT "MAJOR %d\n",MAJOR(dev));
    printk(KERN_ALERT "MINOR %d\n",MINOR(dev));
    if((cl = class_create(THIS_MODULE,"file_driver")) == NULL)
    {
	unregister_chrdev_region(dev,MINOR_COUNT);
	printk(KERN_ALERT "ERROR IN CREATING CLASS\n");
	return -1;
    }
    if(device_create(cl,NULL,dev,NULL,"file_driver%d",0) == NULL)
    {
	unregister_chrdev_region(dev,MINOR_COUNT);
	class_destroy(cl);
	printk(KERN_ALERT "ERROR IN CREATING DEVICE\n");
	return -1;
    }
    cdev_init(&cdev,&myfile_operations);
    if(cdev_add(&cdev,dev,MINOR_COUNT) < 0)
    {
	device_destroy(cl,dev);
	class_destroy(cl);
	unregister_chrdev_region(dev,MINOR_COUNT);
	printk(KERN_ALERT "ERROR IN CDEV OPERATION ADDING\n");
	return -1;
    }
    return 0;
}


__exit void file_driver_exit(void)
{
    device_destroy(cl,dev);
    class_destroy(cl);
    cdev_del(&cdev);
    unregister_chrdev_region(dev,MINOR_COUNT);
    printk(KERN_ALERT "EXIT PROCESS\n");
}

module_init(file_driver_init);
module_exit(file_driver_exit);
