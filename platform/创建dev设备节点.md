> 查看/proc/devices 文件可以获知系统中注册的设备，第 1 列为主设备号，第 2 列为设备名,cat /dev/ 可以查看到所有的设备节点
### 手动创建设备名
> mknod /dev/设备名 设备类型(字符：c,块：b) 主设备号 从设备号

### 自动创建
> 在驱动初始化代码里调用class_create为该设备创建一个class，再为每个设备调用device_create创建对应的设备。  
* 驱动加载：分配设备号----注册字符设备------------动态创建设备节点。
* 驱动卸载：删除设备节点----取消字符设备的注册-----------删除设备号。

1、定义一个class，和一个设备的结构指针  
* static struct class *class
* static struct device *class_dev

2、创建字符设备

3、调用 test_class = class_create(THIS_MODULE,“test_class”)      
   会在sys/class目录下生成test_class文件夹，test_class可以随意命名

4、调用test_class_dev = device_create(test_class, NULL, MKDEV(major, 0),    NULL, “xxx”)   
会在/dev目录下生成xxx的设备文件，major是主设备号



5、注销顺序
* device_destroy() 删除设备节点
* class_destroy()  删除class
* unregister_chrdev()