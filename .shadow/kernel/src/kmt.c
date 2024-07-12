#include<os.h>
#include<common.h>

static void kmt_init(){

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


