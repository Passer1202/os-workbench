#include <common.h>
//TODO：自定测试框架

//魔数
#define MAGIC_NUM 0X1234567

const int MIN_SIZE = 32;
const int _16KB = 16*1024;//16KB
const int _16MB = 16*1024*1024;//16MB

/*锁*/
enum LOCK_STATE {
    PMM_UNLOCKED=0, PMM_LOCKED
};
//一把大锁保平安（速通版）
#define atomic \
    for (int _cnt = (get_lock(&biglock),0); _cnt < 1; _cnt++ , release_lock(&biglock))

//int atomic_xchg(volatile int *addr, int newval);
//原子 (不会被其他处理器的原子操作打断) 地交换内存地址中的数值,返回原来的值
//初始化锁
static void init_lock(int * lock){
    atomic_xchg(lock, PMM_UNLOCKED);
}
//自旋直到获取锁
static void get_lock(int * lock){
    while(atomic_xchg(lock, PMM_LOCKED) == PMM_LOCKED);
}
//释放锁
static void release_lock(int * lock){
    assert(atomic_xchg(lock, PMM_UNLOCKED)==PMM_LOCKED);
}
//尝试获取锁
//static int try_lock(int * lock){
//    return atomic_xchg(lock, PMM_LOCKED);
//}

freenode head;

typedef int pmm_lock_t;
pmm_lock_t biglock;

static void *HUGE_SIZE_ALLOC(size_t size){
    //一把大锁保平安
    get_lock(&biglock);

    release_lock(&biglock);
    return NULL;


}

static void *kalloc(size_t size) {
    if(size<MIN_SIZE)
        size=MIN_SIZE;
    else if(size>_16KB){//非常罕见的大内存分配
        //一把大锁保平安

        //拒绝16MB以上的内存分配
        if(size>_16MB)
            return NULL;
        
        size_t sz=MIN_SIZE;
        while(sz<size)
            sz<<=1;
        void* ret=HUGE_SIZE_ALLOC(sz);
        return ret;    
    }
    return NULL;
}

static void kfree(void *ptr) {
    // TODO
    // You can add more .c files to the repo.
}


#ifndef TEST
// 框架代码中的 pmm_init (在 AbstractMachine 中运行)
static void pmm_init() {
  uintptr_t pmsize = ((uintptr_t)heap.end - (uintptr_t)heap.start);
  printf("Got %d MiB heap: [%p, %p)\n", pmsize >> 20, heap.start, heap.end);
  
  init_lock(&biglock);

}
#else
// 测试代码的 pmm_init ()
static void pmm_init() {
  char *ptr  = malloc(HEAP_SIZE);
  heap.start = ptr;
  heap.end   = ptr + HEAP_SIZE;
  printf("Got %d MiB heap: [%p, %p)\n", HEAP_SIZE >> 20, heap.start, heap.end);
}
#endif

void alloc(int sz){
    
    uintptr_t a=(uintptr_t)kalloc(sz);

    uintptr_t align=a & -a ;

    atomic{
    printf("CPU #%d : Alloc %d -> %p align = %d\n", cpu_current(),sz, a, align);
    }

    assert(a&&align>=sz);
}

void test_pmm() {
   
    alloc(1);
    alloc(5);
    alloc(10);
    alloc(32);
    alloc(4096);
    alloc(4096);
    while(1)
    alloc(4096);

}

MODULE_DEF(pmm) = {
    .init  = pmm_init,
    .alloc = kalloc,
    .free  = kfree,
};
