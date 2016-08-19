#include <linux/init.h>
#include <linux/module.h>
#include <linux/fs.h>
#include <linux/uaccess.h>
#include <linux/ioctl.h>

#define GET _IOW('b', 1, char *)
#define SET _IOR('b', 2, char *)


char buffer[100] = { 0}, *buff;
int i;

int my_open(struct inode *pnode, struct file *pname)
{
    printk(KERN_ALERT "%s is calling\n",__FUNCTION__);
    return 0;
}
int my_close(struct inode *pnode, struct file *pname)
{
    printk(KERN_ALERT "%s is calling\n",__FUNCTION__);
    return 0;
}
long my_ioctl(struct file *pname, unsigned int cmd, unsigned long arg)
{
    printk(KERN_ALERT "%s is calling\n",__FUNCTION__);
    buff = (char *) arg;
    switch(cmd)
    {
	case SET:
	    printk(KERN_ALERT "SET control\n");
	    for(i = 0; buff[i] != '\0' && buff[i] != -1; i++)
	    {
		if(buff[i] > 96 && buff[i] < 123)
		{
		    buffer[i] = buff[i] - 32;
		}
		else
		{
		    buffer[i] = buff[i];
		}
	    }
	    break;
	case GET:
	    printk(KERN_ALERT "GET control\n");
	    for(i = 0; buffer[i] != '\0';i++)
	    {
	    	buff[i] = buffer[i];
	    }
	    buff[i] = 0;
	    break;
	default:
	    printk(KERN_ALERT "DEFAULT control\n");
	    break;
    }
    return 0;
}

struct file_operations file_operation = {
    .owner = THIS_MODULE,
    .open = my_open,
    .release = my_close,
    .unlocked_ioctl = my_ioctl,
};

__init int ioctl_init(void)
{
    printk(KERN_ALERT "%s inserrted\n",__FUNCTION__);
    register_chrdev(250,"ioctl_dev",&file_operation);
    return 0;
}
__exit void ioctl_exit(void)
{
    printk(KERN_ALERT "%s exit\n",__FUNCTION__);
    unregister_chrdev(250,"ioctl_dev");
}

module_init(ioctl_init);
module_exit(ioctl_exit);
