#include <common.h>

//TODO：提速
//
//bug:虚拟机会神秘重启
//目前来看是kfree存在问题




/*定义锁lock_t*/
typedef int lock_t;

void *cpu_ptr[8]; 
void *cpu_ptr_end[8];


/*锁的状态*/
enum LOCK_STATE {
    UNLOCKED=0, LOCKED
};

/*锁们*/
//堆区的大锁，slowpath
lock_t cpu_lock[8];

lock_t heap_lock;

/*锁的API*/
//int atomic_xchg(volatile int *addr, int newval);
//原子 (不会被其他处理器的原子操作打断) 地交换内存地址中的数值,返回原来的值

//初始化锁
static void init_lock(int * lock){
    atomic_xchg(lock, UNLOCKED);
}
//自旋直到获取锁
static void acquire_lock(int * lock){
    while(atomic_xchg(lock, LOCKED) == LOCKED);
}
//释放锁
static void release_lock(int * lock){
    assert(atomic_xchg(lock, UNLOCKED)==LOCKED);
}
//尝试获取锁
//static int try_lock(int * lock){
//    return atomic_xchg(lock, PMM_LOCKED);
//}

//一把大锁保平安（速通版）
#define atomic \
    for (int _cnt = (acquire_lock(&heap_lock),0); _cnt < 1; _cnt++ , release_lock(&heap_lock))



static void *kalloc(size_t size) {
    int cpu_now=cpu_current();

    size_t sz=1;
    
    acquire_lock(&cpu_lock[cpu_now]);

    while(sz<size){
        sz*=2;
    }

    if(sz>(1<<24)){
        release_lock(&cpu_lock[cpu_now]);
        return NULL;
    }

    

    static char* p;
    
    if(!p){
        p=cpu_ptr[cpu_now];
    }

    
    while((intptr_t)p%sz!=0){
        p++;
    }
    
   

    if((uintptr_t)(p+sz)>(uintptr_t)cpu_ptr_end[cpu_now]){
        release_lock(&cpu_lock[cpu_now]);
        return NULL;
    }


    char* ret=p;
    p+=sz;
    
    release_lock(&cpu_lock[cpu_now]);
    return ret;
    

}

static void kfree(void *ptr) {
   
}



// 框架代码中的 pmm_init (在 AbstractMachine 中运行)
static void pmm_init() {
    uintptr_t pmsize = ((uintptr_t)heap.end - (uintptr_t)heap.start);
    int cpu_cnt=cpu_count();
    intptr_t cpu_sz=pmsize/cpu_cnt;

    for(int i=0;i<cpu_cnt;i++){
        cpu_ptr[i]=heap.start+i*cpu_sz;
        cpu_ptr_end[i]=heap.start+(i+1)*cpu_sz;
    }

    printf("Got %d MiB heap: [%p, %p)\n", pmsize >> 20, heap.start, heap.end);

    init_lock(&heap_lock);
    for(int i=0;i<cpu_cnt;i++){
        init_lock(&cpu_lock[i]);
        //printf("CPU #%d : [%p, %p)\n", i, cpu_ptr[i], cpu_ptr_end[i]);
    }
    
    //printf("PMM: init done\n");
    //锁不分配给CPU，分配给slab_page.


}


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
    alloc(5000);
    alloc(5000);
    atomic{
    printf("PMM: test passed\n");
    }
}

MODULE_DEF(pmm) = {
    .init  = pmm_init,
    .alloc = kalloc,
    .free  = kfree,
};