#include <linux/init.h>
#include <linux/module.h>
#include <linux/fs.h>
#include <linux/uaccess.h>

MODULE_LICENSE("GPL");
char buffer[100];
int i;

int file_open(struct inode *pinode, struct file *pfile)
{
    printk(KERN_ALERT "%s of char driver\n",__FUNCTION__);
    return 0;
}
ssize_t file_read(struct file *pfile, char __user *buff, size_t length, loff_t *offset)
{
    printk(KERN_ALERT "%s of char driver\n",__FUNCTION__);
    for(i = 0; buffer[i] != '\0' && length > i;i++)
    {
    	buff[i] = buffer[i];
    }
    return i;
}
ssize_t file_write(struct file *pfille,const char __user *buff, size_t length, loff_t *offset)
{
    printk(KERN_ALERT "%s of char driver\n",__FUNCTION__);
    for(i = 0; buff[i] != '\0' && buff[i] != -1; i++)
    {
    	if(buff[i] > 96 && buff[i] < 123) // convert the string small to capital
	{
	    buffer[i] = buff[i] - 32;
	}
	else
	{
	    buffer[i] = buff[i];
	}
    }
    buffer[i] = 0;
    printk(KERN_ALERT "%s copeid\n",buffer);
    return i;
}
int file_close(struct inode *pinode, struct file *pfile)
{
    printk(KERN_ALERT "%s of char driver\n",__FUNCTION__);
    return 0;
}

struct file_operations entry_for_char_driver_file = {     /* register our function inseaded normal file operation function */
    .owner   = THIS_MODULE,
    .open    = file_open,
    .read    = file_read,
    .write   = file_write,
    .release = file_close,
};
static int __init init_function(void)                          /*  init function when insmod will run */
{
    printk(KERN_ALERT "%s of char driver\n",__FUNCTION__);

    register_chrdev(250/*Major NO*/,
    		    "simple_char_driver" /*Name of the driver*/,
    		    &entry_for_char_driver_file /*structrefor defining file operation*/);
    return 0;
}

__exit void exit_function(void)					/* exit fuction when rmmod command excute */
{
    printk(KERN_ALERT "%s of char driver\n",__FUNCTION__);
    unregister_chrdev(250,"simple_char_driver");
}


module_init(init_function);
module_exit(exit_function);
