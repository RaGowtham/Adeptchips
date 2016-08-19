#include <linux/init.h>
#include <linux/module.h>

MODULE_LICENSE("Dual BSD/GPL\n");

int count = 1;

module_param(count,int,0644);

static int param_init(void)
{
    int i;
    for(i = 0; i < count; i++)
    {
	printk(KERN_ALERT "hello world index : %d\n",i);
    }
    return 0;
}

static void param_exit(void)
{
    int i;
    for(i = 0; i < count; i++)
    {
	printk(KERN_ALERT "bye world index : %d\n",i);
    }
}

module_init(param_init);
module_exit(param_exit);
