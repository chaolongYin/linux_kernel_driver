> �鿴/proc/devices �ļ����Ի�֪ϵͳ��ע����豸���� 1 ��Ϊ���豸�ţ��� 2 ��Ϊ�豸��,cat /dev/ ���Բ鿴�����е��豸�ڵ�
### �ֶ������豸��
> mknod /dev/�豸�� �豸����(�ַ���c,�飺b) ���豸�� ���豸��

### �Զ�����
> ��������ʼ�����������class_createΪ���豸����һ��class����Ϊÿ���豸����device_create������Ӧ���豸��  
* �������أ������豸��----ע���ַ��豸------------��̬�����豸�ڵ㡣
* ����ж�أ�ɾ���豸�ڵ�----ȡ���ַ��豸��ע��-----------ɾ���豸�š�

1������һ��class����һ���豸�Ľṹָ��  
* static struct class *class
* static struct device *class_dev

2�������ַ��豸

3������ test_class = class_create(THIS_MODULE,��test_class��)      
   ����sys/classĿ¼������test_class�ļ��У�test_class������������

4������test_class_dev = device_create(test_class, NULL, MKDEV(major, 0),    NULL, ��xxx��)   
����/devĿ¼������xxx���豸�ļ���major�����豸��



5��ע��˳��
* device_destroy() ɾ���豸�ڵ�
* class_destroy()  ɾ��class
* unregister_chrdev()