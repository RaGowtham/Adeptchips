#include <linux/module.h>
#include <linux/init.h>
#include <linux/blkdev.h>
#include <linux/fs.h>

int myopen(struct block_device *blk_dev, fmode_t type)
{
    return 0;
}

void myclose(struct gendisk *blk_dev, fmode_t type)
{
}

int mymedia_changed(struct gendisk *blk_dev)
{
    return 0;
}

int myrevalidate_disk(struct gendisk *blk_dev)
{
    return 0;
}

struct block_device_operations myops = {
    .open = myopen,
    .release = myclose,
    .media_changed = mymedia_changed,
    .revalidate_disk = myrevalidate_disk,
};


__init int block_init(void)
{
    int majorno = register_blkdev(0,"Block_driver");
    printk(KERN_ALERT "IN %s\n",__FUNCTION__);
    printk(KERN_ALERT "MAJOR NO %d\n",majorno);
    return 0;
}

__exit void block_exit(void)
{
    printk(KERN_ALERT "IN %s\n",__FUNCTION__);
}

module_init(block_init);
module_exit(block_exit);
