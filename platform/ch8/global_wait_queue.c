#include <linux/init.h>
#include <linux/module.h>
#include <linux/cdev.h>
#include <linux/fs.h>
#include <linux/slab.h>
#include <linux/uaccess.h>
#include <linux/device.h>
#include <linux/err.h>
#include <linux/sched.h>
#include <linux/poll.h>

#define GLOBAL_SIZE 256
#define GLOBALMEM_MAJOR 0
#define GLOBAL_CLEAR 1
static int globalmem_major = GLOBALMEM_MAJOR;
struct globalmem_dev {
	struct cdev cdev;
	unsigned int current_len;
	char mem[GLOBAL_SIZE];
	struct mutex mutex;
	wait_queue_head_t r_wait;
	wait_queue_head_t w_wait;
};

struct globalmem_dev *globalmem_devp = NULL;
dev_t devnum;

static ssize_t globalmem_write(struct file *filp, const char __user * buf,
		                                size_t size, loff_t * ppos)
{
	// *ppos 是要读的位置相对于文件开头的便宜
	// size 要读取的个数
	unsigned int count = size;
	struct globalmem_dev *dev = filp->private_data;
	int ret = 0;

	DECLARE_WAITQUEUE(wait, current);
	add_wait_queue(&dev->w_wait,&wait);
	mutex_lock(&dev->mutex);

	while(dev->current_len == GLOBAL_SIZE) {
		if (filp->f_flags) {
			ret = -EAGAIN;
			goto out;
		}
		__set_current_state(TASK_INTERRUPTIBLE);
		mutex_unlock(&dev->mutex);
		schedule();
		//检查当前进程是否有信号处理，返回不为0表示有信号需要处理。
		if (signal_pending(current)) {
			ret = -ERESTARTSYS;
			goto out2;
		}
		//和前面unlock成对
		 mutex_lock(&dev->mutex);

	}

	if (count > GLOBAL_SIZE - dev->current_len) {
		count = GLOBAL_SIZE - dev->current_len;
		printk(KERN_INFO "cout = %d\n",count);
	}

	if ((ret = copy_from_user(dev->mem + dev->current_len, buf,count))){
		printk(KERN_INFO "write erro %d\n",ret);
		ret = -EFAULT;
	} else {
		dev->current_len += count;
		printk(KERN_INFO "written %d bytes(s),current_len:%d\n", count,dev->current_len);
		wake_up_interruptible(&dev->r_wait);

		ret = count;

	}
out:
	mutex_unlock(&dev->mutex);
out2:
	remove_wait_queue(&dev->w_wait, &wait);
	set_current_state(TASK_RUNNING);
	printk(KERN_INFO "read %u bytes from %u, string is %s \n",count, dev->current_len, dev->mem);
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
			mutex_lock(&dev->mutex);
			printk(KERN_INFO "ioctl global_clear");
			dev->current_len = 0;
			memset(dev->mem,0,GLOBAL_SIZE);
			printk(KERN_INFO"mem clear");
			mutex_unlock(&dev->mutex);
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

	DECLARE_WAITQUEUE(wait,current);
	mutex_lock(&dev->mutex);
	add_wait_queue(&dev->r_wait,&wait);

	while (dev->current_len == 0) {
		if (filp->f_flags & O_NONBLOCK) {
			ret = -EAGAIN;
			goto out;
		}
		
		__set_current_state(TASK_INTERRUPTIBLE);
		mutex_unlock(&dev->mutex);
		schedule();
		//检查当前进程是否有信号处理，返回不为0表示有信号需要处理。
		if (signal_pending(current)) {
			ret = -ERESTARTSYS;
			goto out2;
		}
		//和前面unlock成对
		 mutex_lock(&dev->mutex);
	}
	
	if (count >= dev->current_len) {
		count = dev->current_len;
	}

	if (copy_to_user(buf,dev->mem + p, count)){
		ret = -EFAULT;
		goto out;
	} else {
		memcpy(dev->mem, dev->mem + count, dev->current_len - count);
		dev->current_len -= count;
		printk(KERN_INFO "read %d bytes(s),current_len:%d\n", count,dev->current_len);
		wake_up_interruptible(&dev->w_wait);
		ret = count;
	}

out:
	printk(KERN_INFO"out 0000");
	mutex_unlock(&dev->mutex);
out2: 
	printk(KERN_INFO"out 111");
	remove_wait_queue(&dev->w_wait, &wait);
	set_current_state(TASK_RUNNING);

	return ret;
}

static loff_t globalmem_llseek(struct file *filp, loff_t offset, int orig)
{
	return 0;
}
static unsigned int globalmem_poll(struct file *filp, poll_table * wait)
{
	int mask = 0;
	struct globalmem_dev *dev = filp->private_data;
	mutex_lock(&dev->mutex);

	poll_wait(filp, &dev->r_wait, wait);
	poll_wait(filp, &dev->w_wait, wait);
	if (dev->current_len != 0) {
		mask |= POLLIN | POLLRDNORM; 
	}
	if (dev->current_len != GLOBAL_SIZE) {
		mask |= POLLIN | POLLWRNORM;
	}
	mutex_unlock(&dev->mutex);
	return mask;
}
static const struct file_operations globalmem_fops = {
	.owner = THIS_MODULE,
	.llseek = globalmem_llseek,
	.read = globalmem_read,
	.write = globalmem_write,
	.unlocked_ioctl = globalmem_ioctl,
	.open = globalmem_open,
	.release = globalmem_release,
	.poll = globalmem_poll,
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

	mutex_init(&globalmem_devp->mutex);
	//调试忘记初始化等待对了，初始化的时候就挂了
	init_waitqueue_head(&globalmem_devp->r_wait);
	init_waitqueue_head(&globalmem_devp->w_wait);
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
