#include <kernel.h>
#include <klib.h>
#include <klib-macros.h>

void test_pmm();

//参考了WYY的定义
//未分配内存和已分配内存可以共用这个 header，
//当内存块处于未分配状态时，内存块被放在链表中，
//union 字段存放的是下一个链表节点的地址；
//当内存块处于已分配状态时，union 字段存放魔数。
//一个 kfree() 地址来临时，
//只要查看其前一个 uintptr_t 中是不是 ALLOC_MAGIC，
//就能确定这个地址合不合法，再往前看就能获得内存块的 size。

typedef struct __header_t {
    size_t sz;
    union {
        uintptr_t magic;
        struct __header_t *next;
    };
}header_t;

//魔数
#define MAGIC_NUM 0X1234567
//cpu数量不超过8个
#define CPU_MAX 8
//分配的最小内存块大小
#define MIN_SIZE  32
//16KB
#define _16KB  16*1024
//64KB
#define _64KB  64*1024
//16MB
#define _16MB  16*1024*1024

/*定义锁lock_t*/
typedef int lock_t;

/*锁的状态*/
enum LOCK_STATE {
    UNLOCKED=0, LOCKED
};

/*锁的API*/
//int atomic_xchg(volatile int *addr, int newval);
//原子 (不会被其他处理器的原子操作打断) 地交换内存地址中的数值,返回原来的值
