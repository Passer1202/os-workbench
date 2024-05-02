#include <common.h>

//TODO：自定测试框架

//锁的状态
enum LOCK_STATE {
    PMM_UNLOCKED=0, PMM_LOCKED
};

#define MAGIC_NUM 0X1234567

//int atomic_xchg(volatile int *addr, int newval);
//原子 (不会被其他处理器的原子操作打断) 地交换内存地址中的数值,返回原来的值


typedef int pmm_lock_t;


//#define TEST
//4GiB
#define HEAP_SIZE (4LL << 30)

/*
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
static int try_lock(int * lock){
    return atomic_xchg(lock, PMM_LOCKED);
}
*/
static void *kalloc(size_t size) {

    static char* pos;

    char* ret;

    int sz=1;

    if(!pos)pos=heap.start;

    while(sz<size){
        sz*=2;
    }

    while((intptr_t)pos%sz!=0){
        pos++;
    }
    
    ret=pos;

    pos+=sz;

    assert(pos <= (char*)heap.end);

    return ret;
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

    uintptr_t align=a & -a;

    printf("ok\n");

    printf("alloc: %d bytes, align = %d, addr = %p\n", sz, align, a);

    assert(a&&align>=sz);
}

void test_pmm() {
   
    alloc(1);
    alloc(5);
    alloc(10);
    alloc(32);
    alloc(4096);
    alloc(4096);
    alloc(4096);
}

MODULE_DEF(pmm) = {
    .init  = pmm_init,
    .alloc = kalloc,
    .free  = kfree,
};
