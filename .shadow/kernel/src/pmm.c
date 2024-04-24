#include <common.h>

//TODO：自定测试框架

//锁的状态
enum LOCK_STATE {
    UNLOCKED=0, LOCKED
};

//int atomic_xchg(volatile int *addr, int newval);
//原子 (不会被其他处理器的原子操作打断) 地交换内存地址中的数值,返回原来的值

//初始化锁
static void init_lock(int * lock){
    atomic_xchg(lock, UNLOCKED);
}

//自旋直到获取锁
static void get_lock(int * lock){
    while(atomic_xchg(lock, LOCKED) == LOCKED);
}

//释放锁
static void release_lock(int * lock){
    assert(atomic_xchg(lock, UNLOCKED)==LOCKED);
}

//尝试获取锁
static int try_lock(int * lock){
    return atomic_xchg(lock, LOCKED);
}

static void *kalloc(size_t size) {
    // TODO
    // You can add more .c files to the repo.
    int lock;
    init_lock(&lock);
    get_lock(&lock);
    release_lock(&lock);
    try_lock(&lock);
    release_lock(&lock);
    release_lock(&lock);
    return NULL;
}

static void kfree(void *ptr) {
    // TODO
    // You can add more .c files to the repo.
}

static void pmm_init() {
    uintptr_t pmsize = (
        (uintptr_t)heap.end
        - (uintptr_t)heap.start
    );

    printf(
        "Got %d MiB heap: [%p, %p)\n",
        pmsize >> 20, heap.start, heap.end
    );
}

MODULE_DEF(pmm) = {
    .init  = pmm_init,
    .alloc = kalloc,
    .free  = kfree,
};
