#include<os.h>



//实验要求：实现 Kernel Multi-threading
//本次实验的主要任务是实现 kmt 模块中的函数，
//需要完成 struct task, struct spinlock, struct semaphore 的定义，并实现 kmt 的全部 API。

static spinlock_t task_lock;

kcpu cpu_info[CPU_MAX];//CPU信息

task_t *current[CPU_MAX];//CPU当前任务指针

task_t cpu_idle[CPU_MAX]={};//CPU空闲任务

task_t *task_head;//任务链表头

task_t *last[CPU_MAX];//CPU上次运行的任务



static void spin_init(spinlock_t *lk, const char *name){

    strcpy(lk->name, name);

    lk->locked=0;

    lk->cpu_no=-1;

}

static void spin_lock(spinlock_t *lk){

    int intr=ienabled();//记录中断是否开启
    iset(false);//关闭中断
    int cpu_now=cpu_current();

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
    
    //检查持有锁
    int cpu_now=cpu_current();

    int check= (lk->locked==1 && lk->cpu_no==cpu_now); 
    assert(check==1);

    assert(ienabled()==0);//中断关闭

    lk->cpu_no = -1;

    __sync_synchronize();
    atomic_xchg(&lk->locked, 0);

    assert(cpu_info[cpu_now].ncli>0);//嵌套层数大于0

    cpu_info[cpu_now].ncli--;//嵌套层数-1
    if(cpu_info[cpu_now].ncli==0){
        //恢复中断状态
        if(cpu_info[cpu_now].intr)   
            iset(true);
    }

}

//static int xxxx=0;


static Context *kmt_context_save(Event ev, Context *ctx){
    
    spin_lock(&task_lock);
    //printf("xxxx: %d\n",xxxx);
    //xxxx++;
    //assert(xxxx%2==1);

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
    //spin_lock(&task_lock);//一把大锁保平安
    //static int x=0;
    //printf("%d\n",x);
    //x++;
    //xxxx++;
    //printf("xxxx: %d\n",xxxx);
    //assert(xxxx%2==0);
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
    //printf("task: %s\n",next->name);
    current[cpu_now]=next;
    if(current[cpu_now]->status!=IDLE){
        current[cpu_now]->status=RUNNING;
    }

    last[cpu_now]=current[cpu_now];
    spin_unlock(&task_lock);
    return current[cpu_now]->context;
}

static void current_init(){

    int cpu_cnt=cpu_count();
    //最开始每个cpu上都是空闲任务
    for(int i=0;i<cpu_cnt;i++){
        last[i]=NULL;
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
    //printf("%d\n",INT_MAX);
    //assert(0);
    //assert(INT_MIN<INT_MAX);
    os->on_irq(INT_MIN, EVENT_NULL, kmt_context_save);
    os->on_irq(INT_MAX, EVENT_NULL, kmt_schedule);

    //初始化每个cpu上的current
    current_init();

    //初始化锁
    spin_init(&task_lock, "task_lock");


}

static int  kmt_create(task_t *task, const char *name, void (*entry)(void *arg), void *arg){

    spin_lock(&task_lock);
    //spin_lock(&task_lock);
    //初始化任务
    
    task->status=RUNNABLE;
    task->name=name;
    //printf("taskname: %s\n",task->name);
    task->entry=entry;
    task->context=kcontext(
        (Area){task->end, task+1}, //from thread-os
        entry, arg
    );
    

    //将任务插入任务链表(头插法)
    if(task_head==NULL){
        //assert(0);
        task_head=task;
        task_head->next=NULL;
    }
    else{
        
        task->next=task_head;

        task_head=task;

    }
    //printf("task_head name: %s\n",task_head->name);
    //if(task_head->next!=NULL)printf("task_head next name: %s\n",task_head->next->name);
    //else{printf("null\n");}

    assert(task_head!=NULL);

    spin_unlock(&task_lock);
    return 0;
}

static void kmt_teardown(task_t *task){
    //按理说走不到这
    //assert(0);
    //panic_on(1, "not implemented");
}


static void sem_init(sem_t *sem, const char *name, int value){
    //spin_lock(&task_lock);
    sem->val=value;
    spin_init(&sem->lock, name);
    sem->name=name;
    sem->qh=0;
    sem->qt=0;
    sem->cnt_max=128;//若改动此值，务必修改queue数组的大小
    //spin_unlock(&task_lock);
}

static void sem_wait(sem_t *sem){

    spin_lock(&task_lock);
    spin_lock(&sem->lock);
    int cpu_now=cpu_current();
    int flag=0;
    sem->val--;
    //int check=0;
    if(sem->val<0){
        flag=1;
        //check= current[cpu_now]->status;
        current[cpu_now]->status=BLOCKED;//等待状态
        
        //入队
        sem->wait_queue[sem->qt]=current[cpu_now];
        sem->qt=((sem->qt+1)>(sem->cnt_max))?(sem->qt+1-sem->cnt_max):(sem->qt+1);
        
    }
    spin_unlock(&sem->lock);
    spin_unlock(&task_lock);
    if(flag){
        //assert(0);
        //assert(ienabled()==true);

        
        //printf("wait name:%s\n",current[cpu_now]->name);
        yield();
        
        //printf("wait name:%s\n",current[cpu_now]->name);
        //assert(check== current[cpu_now]->status);
        //assert(0);
    }
    //assert(0);
}


static void sem_signal(sem_t *sem){
    spin_lock(&task_lock);
    spin_lock(&sem->lock);
    
    sem->val++;
    
    if(sem->val<=0){//有等待的任务
        
        assert(sem->qh!=sem->qt);
        task_t *task=sem->wait_queue[sem->qh];
        sem->qh=((sem->qh+1)>(sem->cnt_max))?(sem->qh+1-sem->cnt_max):(sem->qh+1);
        //printf("signal name:%s\n",task->name);
        task->status=RUNNABLE;
    }
    spin_unlock(&sem->lock);
    spin_unlock(&task_lock);
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


