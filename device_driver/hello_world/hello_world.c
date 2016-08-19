/* To understand how module get inserted*/ 

#include <linux/init.h>
#include <linux/module.h>

MODULE_LICENSE("Dual BSD/GPL\n");

static int hello_init(void)
{
    printk(KERN_ALERT "hello world\n"); // in init time print hello world
    return 0;
}

static void hello_exit(void)
{
    printk(KERN_ALERT "Good Bye\n"); // in exit time print good bye
}

module_init(hello_init);
module_exit(hello_exit);
