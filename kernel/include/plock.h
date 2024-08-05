

//锁的状态
enum LOCK_STATE {
    UNLOCKED=0, LOCKED
};

/*定义锁lock_t*/
typedef int lock_t;

/*锁的API*/

//from jyy 
//int atomic_xchg(volatile int *addr, int newval);
//原子 (不会被其他处理器的原子操作打断) 地交换内存地址中的数值,返回原来的值

//初始化锁
void init_lock(int * lock){
    atomic_xchg(lock, UNLOCKED);
}
//自旋直到获取锁
void acquire_lock(int * lock){
    while(atomic_xchg(lock, LOCKED) == LOCKED);
}
//释放锁
void release_lock(int * lock){
    assert(atomic_xchg(lock, UNLOCKED)==LOCKED);
}
