#include<os.h>
#include<common.h>


//实验要求：实现 Kernel Multi-threading
//本次实验的主要任务是实现 kmt 模块中的函数，
//需要完成 struct task, struct spinlock, struct semaphore 的定义，并实现 kmt 的全部 API。

spinlock_t task_lock;

kcpu cpu[CPU_MAX];//CPU信息

task_t *current[CPU_MAX];//CPU当前任务指针

task_t cpu_idle[CPU_MAX]={};//CPU空闲任务


static void spin_init(spinlock_t *lk, const char *name){
    
    lk->name=name;

    lk->locked=0;

    lk->cpu_no=-1;

}

static void spin_lock(spinlock_t *lk){

}


static void spin_unlock(spinlock_t *lk){

}




static Context *kmt_context_save(Event ev, Context *ctx){
    
    return NULL;
}

static Context *kmt_schedule(Event ev,Context *ctx){
    
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

    return 0;
}

static void kmt_teardown(task_t *task){

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


