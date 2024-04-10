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

struct co *co_pointers[CO_SIZE];        //存放所有协程的指针
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

    assert(co);
    //assert(0);

    co_now->status=CO_WAITING;
    co->waiter=co_now;                  
    assert(0);
    //等待co所指协程完成
    while(co->status!=CO_DEAD){
        co_yield();                     //必须yiele(),否则co永远不可能完成
    }
    
    //co所指协程完成后，我们需要删除掉它
    int index=0;
    while(index<total&&co_pointers[index]!=co){
        index++;
    }
    while(index+1<total){
        co_pointers[index]=co_pointers[index+1];
        index++;
    }
    co_pointers[index+1]=NULL;
    total--;
    free(co);

}

void co_yield() {
    
    int val=setjmp(co_now->context);
    if(val!=0) return;                  //maybe wrong?

    //现在需要获取一个线程来执行
    int index=rand()%total;
    struct co* choice=co_pointers[index];
    

    //有可能死循环？总有一个线程还活着
    while(!(choice->status==CO_NEW||choice->status==CO_RUNNING)){
        index=rand()%total;
        choice=co_pointers[index];
    }

    assert(choice->status==CO_NEW||choice->status==CO_RUNNING);

    if(choice->status==CO_NEW){
        //较为复杂的情况
        co_now=choice;
        choice->status=CO_RUNNING;

        asm volatile (
        #if __x86_64__
        "movq %0, %%rsp; movq %2, %%rdi; call *%1"
          :
          : "b"((uintptr_t)(choice->stack+sizeof(choice->stack))),
            "d"(choice->func),
            "a"((uintptr_t)choice->arg)    //(uintptr_t)
          : "memory"
        #else
        "movl %0, %%esp; movl %2, 4(%0); call *%1"
          :
          : "b"((uintptr_t)(choice->stack+sizeof(choice->stack)- 8)),
            "d"(choice->func),
            "a"((uintptr_t)(choice->arg))
          : "memory"
        #endif
        );


        choice->status=CO_DEAD;
        if(choice->waiter!=NULL){
            co_now=choice->waiter;
            longjmp(co_now->context,1);
        }
        co_yield();
    }
    else{
        co_now=choice;
        longjmp(co_now->context,1);
    }



}

__attribute__((constructor)) void init(){
    
    total=0;

    struct co* main=(struct co*)malloc(sizeof(struct co));
    
    strcpy(main->name,"main");
    main->status=CO_RUNNING;
    main->waiter=NULL;

    memset(co_pointers,0,sizeof(co_pointers));

    co_pointers[total++]=main;

}
