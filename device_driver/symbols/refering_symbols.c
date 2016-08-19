#include <linux/init.h>
#include <linux/module.h>

MODULE_LICENSE("GPL");

int function_for_symbol_example(void);

__init int refering_symbol_init(void)
{
    printk(KERN_ALERT "%s inserted successfully\n",__FUNCTION__);
    function_for_symbol_example();
    return 0;
}

void refering_symbol_exit(void)
{
    printk(KERN_ALERT "%s removed succesfully\n",__FUNCTION__);
}

module_init(refering_symbol_init);
module_exit(refering_symbol_exit);
