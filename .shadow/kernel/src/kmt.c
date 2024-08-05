#include<os.h>
#include<common.h>


//实验要求：实现 Kernel Multi-threading
//本次实验的主要任务是实现 kmt 模块中的函数，
//需要完成 struct task, struct spinlock, struct semaphore 的定义，并实现 kmt 的全部 API。

spinlock_t task_lock;

kcpu cpu_info[CPU_MAX];//CPU信息

task_t *current[CPU_MAX];//CPU当前任务指针

task_t cpu_idle[CPU_MAX]={};//CPU空闲任务

task_t *task_head;//任务链表头


static void spin_init(spinlock_t *lk, const char *name){

    strcpy(lk->name, name);

    lk->locked=0;

    lk->cpu_no=-1;

}

static void spin_lock(spinlock_t *lk){

    int cpu_now=cpu_current();

    int intr=ienabled();//记录中断是否开启
    iset(false);//关闭中断

    if(cpu_info[cpu_now].ncli==0){//记录最外层的中断状态
        cpu_info[cpu_now].intr=intr;
    }
    cpu_info[cpu_now].ncli++;//嵌套层数+1

    //防御性编程：避免死锁
    int check= (lk->locked==1 && lk->cpu_no==cpu_now); 
    assert(check==0);

    while (atomic_xchg(&lk->locked, 1))
        ;
    __sync_synchronize();
    lk->cpu_no = cpu_now;
    
}


static void spin_unlock(spinlock_t *lk){

}




static Context *kmt_context_save(Event ev, Context *ctx){
    
    spin_lock(&task_lock);

    assert(ienabled()==0);//中断关闭

    int cpu_now=cpu_current();

    assert(current[cpu_now]!=NULL);
    
    //只要当前任务不被BLOCKED且非IDLE，将其改为RUNNABLE
    if (current[cpu_now]->status != BLOCKED && current[cpu_now]->status != IDLE) 
        current[cpu_now]->status = RUNNABLE;
    
    current[cpu_now]->context = ctx;//保存当前上下文

    spin_unlock(&task_lock);

    return NULL;
}

static Context *kmt_schedule(Event ev,Context *ctx){
    spin_lock(&task_lock);//一把大锁保平安

    assert(ienabled()==0);//中断关闭

    int cpu_now=cpu_current();

    //遍历任务链表，找到下一个RUNNABLE任务
    //若当前是IDLE，其next为NULL
    task_t *next=current[cpu_now]->next;

    if(next==NULL){
        next=task_head;
    }
    //找到下一个RUNNABLE任务
    while(next!=NULL){
        if(next->status==RUNNABLE){
            break;//找到了
        }
        next=next->next;
    }
    //没有找到RUNNABLE任务，则返回IDLE任务
    if(next==NULL){
        next=&cpu_idle[cpu_now];
    }

    //此时next一定是RUNNABLE或IDLE
    assert(next!=NULL);

    //切换任务，修改任务的状态
    current[cpu_now]=next;
    if(current[cpu_now]->status!=IDLE){
        current[cpu_now]->status=RUNNING;
    }

    spin_unlock(&task_lock);
    return NULL;
}

static void current_init(){

    int cpu_cnt=cpu_count();
    //最开始每个cpu上都是空闲任务
    for(int i=0;i<cpu_cnt;i++){
        
        current[i]=&cpu_idle[i];
        current[i]->status=IDLE;//空闲
        current[i]->name="idle";
        current[i]->entry=NULL;
        current[i]->next=NULL;
        current[i]->context=kcontext(
            (Area){current[i]->end, current[i]+1}, //from thread-os
            NULL, NULL
        );
    }

}

static void kmt_init(){

    //注册中断处理函数
    os->on_irq(INT_MIN, EVENT_NULL, kmt_context_save);
    os->on_irq(INT_MAX, EVENT_NULL, kmt_schedule);

    //初始化每个cpu上的current
    current_init();

    //初始化锁
    spin_init(&task_lock, "task_lock");


}

static int  kmt_create(task_t *task, const char *name, void (*entry)(void *arg), void *arg){

    spin_lock(&task_lock);
    //初始化任务
    task->status=RUNNABLE;
    task->name=name;
    task->entry=entry;
    task->context=kcontext(
        (Area){task->end, task+1}, //from thread-os
        entry, arg
    );

    //将任务插入任务链表(头插法)
    if(task_head==NULL){
        task_head=task;
        task->next=NULL;
    }
    else{
        task->next=task_head;
        task_head=task;
    }

    spin_unlock(&task_lock);
    return 0;
}

static void kmt_teardown(task_t *task){
    //按理说走不到这
    panic_on(1, "not implemented");
}


static void sem_init(sem_t *sem, const char *name, int value){

}


static void sem_wait(sem_t *sem){

}


static void sem_signal(sem_t *sem){

}


MODULE_DEF (kmt) = {
    .init = kmt_init,
    .create = kmt_create,
    .teardown = kmt_teardown,
    .spin_init = spin_init,
    .spin_lock = spin_lock,
    .spin_unlock = spin_unlock,
    .sem_init = sem_init,
    .sem_wait = sem_wait,
    .sem_signal = sem_signal,
};


