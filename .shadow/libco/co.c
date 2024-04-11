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
    strcpy(co_new->name,name);
    co_new->func=func;
    co_new->arg=arg;

    co_new->status=CO_NEW;
    co_new->waiter=NULL;

    //记录co_new指针
    co_pointers[total++]=co_new;

    return co_new;
}

void co_wait(struct co *co) {

    assert(co!=NULL);
    //assert(0);
    //assert(0);
    co_now->status=CO_WAITING;
    co->waiter=co_now;                  

    //等待co所指协程完成
    while(co->status!=CO_DEAD){
        co_yield();                     //必须yiele(),否则co永远不可能完成
    }
    //co所指协程完成后，我们需要删除掉它
    int index=0;
    while(index<total&&co_pointers[index]!=co){
        index++;
    }
    assert(index>=total||co_pointers[index]==co);
    while(index+1<total){
        co_pointers[index]=co_pointers[index+1];
        index++;
    }
    co_pointers[index]=NULL;
    total--;
    assert(total>=0);
    free(co);

}

void co_yield() {
  int val = setjmp(co_now->context);
  if (val != 0) return;
    //现在需要获取一个线程来执行
    int index=rand()%total;
    struct co* choice=co_pointers[index];
    

    //有可能死循环？总有一个线程还活着
    while(!(choice->status==CO_NEW||choice->status==CO_RUNNING)){
        index=rand()%total;
        choice=co_pointers[index];
    }
    //assert(0);
    assert(choice->status==CO_NEW||choice->status==CO_RUNNING);
    co_now = choice;
    if (choice->status == CO_NEW) {
      choice->status = CO_RUNNING;

      asm volatile(
        #if __x86_64__
                    "movq %%rdi, (%0); movq %0, %%rsp; movq %2, %%rdi; call *%1"
                    :
                    : "b"((uintptr_t)(choice->stack + sizeof(choice->stack))), "d"(choice->func), "a"((uintptr_t)(choice->arg))
                    : "memory"
        #else
                    "movl %%esp, 0x8(%0); movl %%ecx, 0x4(%0); movl %0, %%esp; movl %2, (%0); call *%1"
                    :
                    : "b"((uintptr_t)(choice->stack + sizeof(choice->stack) - 8)), "d"(choice->func), "a"((uintptr_t)(choice->arg))
                    : "memory" 
        #endif
        );


      asm volatile(
      #if __x86_64__
                "movq (%0), %%rdi"
                :
                : "b"((uintptr_t)(choice->stack + sizeof(choice->stack)))
                : "memory"
      #else
                "movl 0x8(%0), %%esp; movl 0x4(%0), %%ecx"
                :
                : "b"((uintptr_t)(choice->stack + sizeof(choice->stack) - 8))
                : "memory"
      #endif
      );

      choice->status = CO_DEAD;

      if (co_now->waiter) {
        co_now = co_now->waiter;
        longjmp(co_now->context, 1);
      }
      co_yield();
    } else if (choice->status == CO_RUNNING) {
      longjmp(choice->context, 1);
    } else {
      assert(0);
    }

}

__attribute__((constructor)) void init(){
    
    total=0;

    struct co* main=(struct co*)malloc(sizeof(struct co));
    
    strcpy(main->name,"main");
    co_now=main;
    main->status=CO_RUNNING;
    main->waiter=NULL;

    memset(co_pointers,0,sizeof(co_pointers));

    co_pointers[total++]=main;

}
