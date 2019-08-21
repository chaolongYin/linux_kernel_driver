#include <linux/init.h>
#include <linux/module.h>

static int __init globalmem_init(void)
{
	printk(KERN_INFO "globalmem_init\n");
	return 0;
}
module_init(globalmem_init);

static void __exit globalmem_exit(void)
{
	printk(KERN_INFO "globalmem_exit\n");
}

module_exit(globalmem_exit);
MODULE_AUTHOR("chaolong <243843120@qq.com>");
MODULE_LICENSE("GPL v2");
MODULE_DESCRIPTION("A simple Hello World Module");
MODULE_ALIAS("a simplest module");
