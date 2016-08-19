#include <linux/init.h>
#include <linux/module.h>


static int test_init(void)
{
    printk(KERN_ALERT "hello world\n");
    return 0;
}

module_init(test_init);
