#include <linux/init.h>
#include <linux/module.h>

MODULE_LICENSE("Dual BSD/GPL\n");

__initdata static int index = 10;
static int index_common = 10;

__init int init_qualifier_exampl_init(void)  //init qualifier for after init got over module clean
{
    printk(KERN_ALERT "%s module inserted successfully\n",__FUNCTION__);
    printk(KERN_ALERT "index = %d\n",index);
    printk(KERN_ALERT "index = %d\n",index_common);
    return 0;
}

void init_qualifier_exampl_exit(void)
{
    printk(KERN_ALERT "%s module removed successfully\n",__FUNCTION__);
    printk(KERN_ALERT "index = %d\n",index_common);
}

module_init(init_qualifier_exampl_init);
module_exit(init_qualifier_exampl_exit);
