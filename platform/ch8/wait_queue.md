### 等待队列

* 表示一组睡眠的进程，当某一条件为真时，由内核唤醒他们

* 在 Linux 中, 一个等待队列由一个"等待队列头"来管理, 一个 wait_queue_head_t 类型的结构,

* 当一个任务需要在某个wait_queue_head上睡眠时，将自己的进程控制块信息封装到wait_queue中，然后挂载到wait_queue的链表中，执行调度睡眠。当某些事件发生后，另一个任务（进程）会唤醒wait_queue_head上的某个或者所有任务，唤醒工作也就是将等待队列中的任务设置为可调度的状态，并且从队列中删除。


### 等待队列头初始化
> DECLARE_WAIT_QUEUE_HEAD(name);

> wait_queue_head_t my_queue;  
 init_waitqueue_head(&my_queue);
  
### 等待队列
  > DECLARE_WAITQUEUE(wait, task);


### 添加移除等待队列
> add_wait_queue(wait_queue_head_t *q, wait_queue_t *wait);   
remove_wait_queue(wait_queue_head_t *q, wait_queue_t *wait);



### 简单睡眠
```
wait_event(queue, condition)
wait_event_interruptible(queue, condition)
wait_event_timeout(queue,condition,timeout)
wait_event_interruptible_timeout(queue, condition, timeout)
```
> queue 是等待队列头，condition 是条件，如果调用 wait_event 前 condition == 0 ，则调用 wait_event 之后，当前进程就会休眠


### 唤醒队列
> void wake_up(wait_queue_head_t *queue);void wake_up_interruptible(wait_queue_head_t *queue);
比如你调用 wake_up 去唤醒一个使用wait_event等待队列，进入休眠的进程，唤醒之后，它会判断 condition 是否为真，如果还是假的继续睡眠。

### 手动睡眠
* DECLARE_WAITQUEUE(name, tsk)  创建一个等待队列：
tsk一般为当前进行current. 这个宏定义并初始化一个名为name的等待队列.  
* 将等待队列头 加入/移除 等待队列：add_wait_queue(wait_queue_head_t *q, wait_queue_t *wait)
* 设置进程状态：set_current_state(TASK_INTERRUPTIBLE) 等 
* 进程调度：schedule() 或者 schedule_timeout()

