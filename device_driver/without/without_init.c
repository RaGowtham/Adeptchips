#include <linux/init.h>
#include <linux/module.h>

MODULE_LICENSE("GBL");


static void test_exit(void)
{
    printk(KERN_ALERT "Good Bye\n");
}

module_exit(test_exit);
