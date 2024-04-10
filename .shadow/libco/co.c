#include "co.h"
#include <stdlib.h>
#include <stdint.h>
#include <setjmp.h>
#include <string.h>
#include <assert.h>

//每个协程的堆栈使用不超过 64 KiB
#define STACK_SIZE 64*1024
//任意时刻系统中的协程数量不会超过 128 个
#define CO_SIZE 128

#define NAME_SIZE 64


enum co_status {
    CO_NEW = 1,                         // 新创建，还未执行过
    CO_RUNNING,                         // 已经执行过
    CO_WAITING,                         // 在 co_wait 上等待
    CO_DEAD,                            // 已经结束，但还未释放资源
};

struct co {
    char name[NAME_SIZE];               // 名字
    void (*func)(void *);               // co_start 指定的入口地址和参数
    void *arg;

    enum co_status status;              // 协程的状态
    struct co *    waiter;              // 是否有其他协程在等待当前协程
    jmp_buf        context;             // 寄存器现场
    uint8_t        stack[STACK_SIZE]__attribute__((aligned(16)));   
                                        // 协程的堆栈,16字节对齐
};

struct co *co_pointers[CO_SIZE];         //存放所有协程的指针
struct co *co_now;                      //当前携程的指针

int total;                              //当前携程总数

struct co *co_start(const char *name, void (*func)(void *), void *arg) {
    
    assert(total<CO_SIZE);
    //开辟空间
    struct co* co_new=(struct co*)malloc(sizeof(struct co));
    
    //初始化
    co_new->func=func;
    co_new->arg=arg;
    strcpy(co_new->name,name);
    co_new->status=CO_NEW;
    co_new->waiter=NULL;

    //记录co_new指针
    co_pointers[total++]=co_new;

    return co_new;
}

void co_wait(struct co *co) {
}

void co_yield() {
}
