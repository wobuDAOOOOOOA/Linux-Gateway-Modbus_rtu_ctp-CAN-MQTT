#include <linux/module.h>
#include <linux/init.h>
#include <linux/kernel.h>

static int __init hello_init(void)
{
    printk(KERN_EMERG "[KERN_EMERG] HELLOWORLD MODULE INIT\n");
    return 0;
}

static void __exit hello_exit(void)
{
printk(KERN_EMERG "[KERN_EMERG] goodbye\n");
}

module_init(hello_init);
module_exit(hello_exit);

MODULE_LICENSE("GPL");
MODULE_AUTHOR("DZ");
MODULE_DESCRIPTION("Hello World driver for LubanCat");