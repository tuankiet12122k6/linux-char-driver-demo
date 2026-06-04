#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/init.h>

static int __init hello_init(void)
{
pr_info("OSG202: Hello kernel module loaded!\n");
return 0;
}
static void __exit hello_exit(void)
{
pr_info("OSG202: Hello kernel module unloaded!\n");
}
module_init(hello_init);
module_exit(hello_exit);
MODULE_LICENSE("GPL");
MODULE_AUTHOR("OSG Team");
MODULE_DESCRIPTION("Linux Kernel Module for OSG202 project");
MODULE_VERSION("1.0");


