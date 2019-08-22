#include <linux/init.h>
#include <linux/module.h>
#include <linux/cdev.h>
#include <linux/fs.h>
#include <linux/slab.h>
#include <linux/uaccess.h>
#include <linux/device.h>
#include <linux/err.h>

#define GLOBAL_SIZE 256
#define GLOBALMEM_MAJOR 0
#define GLOBAL_CLEAR 1
static int globalmem_major = GLOBALMEM_MAJOR;
struct globalmem_dev {
	struct cdev cdev;
	char mem[GLOBAL_SIZE];
};

struct globalmem_dev *globalmem_devp = NULL;
dev_t devnum;

static ssize_t globalmem_write(struct file *filp, const char __user * buf,
		                                size_t size, loff_t * ppos)
{
	// *ppos 是要读的位置相对于文件开头的便宜
	// size 要读取的个数
	unsigned long p = *ppos;
	unsigned int count = size;
	struct globalmem_dev *dev = filp->private_data;
	int ret = 0;

	if (p >= GLOBAL_SIZE) {
		return 0;
	}

	if (count > GLOBAL_SIZE - p) {
		count = GLOBAL_SIZE -p;
		printk(KERN_INFO "cout = %d\n",count);
	}

	if ((ret = copy_from_user(dev->mem +p, buf,count))){
		printk(KERN_INFO "write erro %d\n",ret);
		ret = -EFAULT;
	} else {
		*ppos += count;
		ret = count;

	}


		printk(KERN_INFO "read %u bytes from %lu, string is %s \n",count, p, dev->mem);
	return ret;
}

static int globalmem_open(struct inode *inode, struct file *filp)
{
	//将文件的私有数据指向设备结构体
	filp->private_data = globalmem_devp;
	return 0;

}
static int globalmem_release(struct inode *inode, struct file *filp)
{
	return 0;
}
static long globalmem_ioctl(struct file *filp, unsigned int cmd,
		unsigned long arg)
{
	struct globalmem_dev  *dev = filp->private_data;

	switch(cmd){
		case GLOBAL_CLEAR:
			memset(dev->mem,0,GLOBAL_SIZE);
			printk(KERN_INFO"mem clear");
			break;
		default:
			return -EINVAL;

	}
	return 0;
}
static ssize_t globalmem_read(struct file *filp, char __user * buf, size_t size,loff_t * ppos)
{
	// *ppos 是要读的位置相对于文件开头的便宜
	// size 要读取的个数
	unsigned long p = *ppos;
	unsigned int count = size;
	struct globalmem_dev *dev = filp->private_data;
	int ret = 0;

	if (p > GLOBAL_SIZE) {
		return 0;
	}

	if (count >= (GLOBAL_SIZE - p)) {
		count = GLOBAL_SIZE -p;
	}

	if (copy_to_user(buf,dev->mem + p, count)){
		ret = -EFAULT;
	} else {
		*ppos += count;
		ret = count;

		printk(KERN_INFO "read %u bytes from %lu\n",count, p);
	}

	return ret;
}

static loff_t globalmem_llseek(struct file *filp, loff_t offset, int orig)
{
	return 0;
}
static const struct file_operations globalmem_fops = {
	.owner = THIS_MODULE,
	.llseek = globalmem_llseek,
	.read = globalmem_read,
	.write = globalmem_write,
	.unlocked_ioctl = globalmem_ioctl,
	.open = globalmem_open,
	.release = globalmem_release,
};
#if 0
static int register_chardev_old(struct globalmem_dev *dev) 
{
	int ret;
	//1、定义设备号
	//dev_t devnum= MKDEV(globalmem_major,0);
	//2、申请设备号
	ret = alloc_chrdev_region(&devnum, 0, 1, "globalmem");
	//3.初始化
	cdev_init(&dev->cdev,&globalmem_fops);
	dev->cdev.owner = THIS_MODULE;

	//注册
	cdev_add(&dev->cdev,devnum,1);
	
	return ret;
}
#endif
static struct class *global_class;
static struct device *global_class_dev;
static int register_chardev_new(struct globalmem_dev *dev)
{
	//第一个参数：0为随机分配，其他为申请那个号
	//第二个参数：随机分配最小数
	
	globalmem_major = register_chrdev(globalmem_major, "globalmem", &globalmem_fops);
	globalmem_major = globalmem_major;
	if (globalmem_major < 0) {
		printk(KERN_INFO " register dev erro \n");
		return globalmem_major;
	}

	global_class = class_create(THIS_MODULE,"globalmem");
	if (IS_ERR(global_class)){
		unregister_chrdev(globalmem_major,"globalmem");
		return PTR_ERR(global_class);
	}

	printk(KERN_INFO "globalmem_major is %d\n",globalmem_major);
	devnum = MKDEV(globalmem_major,0);
	global_class_dev = device_create(global_class,NULL,devnum,dev,"globalmem");
	return 0;
}

static int __init globalmem_init(void)
{
	int ret;
	printk(KERN_INFO "globalmem_init\n");
	globalmem_devp = kzalloc(sizeof(struct globalmem_dev),GFP_KERNEL);
	if (!globalmem_devp) {
		ret = -ENOMEM;
	}
#if 0
	//使用传统版本
	//register_chardev_old(globalmem_devp);
#endif	
	//使用__register_chrdev
	register_chardev_new(globalmem_devp);

	return 0;
}
module_init(globalmem_init);


static void __exit globalmem_exit(void)
{
	kfree(globalmem_devp);
#if 0
	unregister_chrdev_region(devnum,1);
	cdev_del(globalmem_devp);
#endif
	device_destroy(global_class,devnum);
	class_destroy(global_class);
	unregister_chrdev(globalmem_major,"globalmem");
	printk(KERN_INFO "globalmem_exit\n");
}

module_exit(globalmem_exit);
MODULE_AUTHOR("chaolong <243843120@qq.com>");
MODULE_LICENSE("GPL v2");
MODULE_DESCRIPTION("A simple Hello World Module");
MODULE_ALIAS("a simplest module");
