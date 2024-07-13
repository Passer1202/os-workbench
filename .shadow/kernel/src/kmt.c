#include<os.h>
#include<common.h>


//实验要求：实现 Kernel Multi-threading
//本次实验的主要任务是实现 kmt 模块中的函数，
//需要完成 struct task, struct spinlock, struct semaphore 的定义，并实现 kmt 的全部 API。


static Context *kmt_context_save(Event ev, Context *ctx){
    
    return NULL;
}

static Context *kmt_schedule(Event ev,Context *ctx){
    
    return NULL;
}

static void kmt_init(){

    os->on_irq(INT_MIN, EVENT_NULL, kmt_context_save);
    os->on_irq(INT_MAX, EVENT_NULL, kmt_schedule);

}

static int  kmt_create(task_t *task, const char *name, void (*entry)(void *arg), void *arg){

    return 0;
}

static void kmt_teardown(task_t *task){

}
static void spin_init(spinlock_t *lk, const char *name){

}

static void spin_lock(spinlock_t *lk){

}


static void spin_unlock(spinlock_t *lk){

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


