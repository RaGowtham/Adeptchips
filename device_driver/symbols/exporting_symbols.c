#include <linux/init.h>
#include <linux/module.h>

MODULE_LICENSE("GPL");

int function_for_symbol_example(void)
{
    printk(KERN_ALERT "%s inside\n",__FUNCTION__);
    return 0;
}


//exporting symbols to out the module

EXPORT_SYMBOL(function_for_symbol_example);



__init int exporting_symbol_init(void)
{
    printk(KERN_ALERT "%s insmod successfully inserted\n",__FUNCTION__);
    function_for_symbol_example();
    return 0;
}


void exporting_symbol_exit(void)
{
    printk(KERN_ALERT "%s rmsmod successfully removed\n",__FUNCTION__);
}

module_init(exporting_symbol_init);
module_exit(exporting_symbol_exit);
