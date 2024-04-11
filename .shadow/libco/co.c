#include "co.h"
#include <stdlib.h>
#include <string.h>
#include <setjmp.h>
#include <stdint.h>
#include <assert.h>

#define STACK_SIZE 64 * 1024
#define CO_SIZE 128

enum co_status {
  CO_NEW = 1, // 新创建，还未执行过
  CO_RUNNING, // 已经执行过
  CO_WAITING, // 在 co_wait 上等待
  CO_DEAD,    // 已经结束，但还未释放资源
};

struct co {
  char name[64];
  void (*func)(void *); // co_start 指定的入口地址和参数
  void *arg;

  enum co_status status;  // 协程的状态
  struct co *    waiter;  // 是否有其他协程在等待当前协程
  jmp_buf        context; // 寄存器现场 (setjmp.h)
  uint8_t        stack[STACK_SIZE]__attribute__((aligned(16))); // 协程的堆栈
};

struct co *co_now; // 当前运行的协程
struct co *co_pointers[CO_SIZE]; // 所有协程的数组
int total; // 当前协程数

// 随机挑选出一个协程
struct co *get_next_co() {
  int count = 0;
  for (int i = 0; i < total; ++i) {
    assert(co_pointers[i]);
    if (co_pointers[i]->status == CO_NEW || co_pointers[i]->status == CO_RUNNING) {
      ++count;
    }
  }

  int id = rand() % count, i = 0;
  for (i = 0; i < total; ++i) {
    if (co_pointers[i]->status == CO_NEW || co_pointers[i]->status == CO_RUNNING) {
      if (id == 0) {
        break;
      }
      --id;
    }
  }
  return co_pointers[i];
}

struct co *co_start(const char *name, void (*func)(void *), void *arg) {
  struct co* res = (struct co*)malloc(sizeof(struct co));
  strcpy(res->name, name);
  res->func = func;
  res->arg = arg;
  res->status = CO_NEW;
  res->waiter = NULL;
  assert(total < CO_SIZE);
  co_pointers[total++] = res;

  return res;
}

void co_wait(struct co *co) {
  assert(co != NULL);
  co->waiter = co_now;
  co_now->status = CO_WAITING;
  while (co->status != CO_DEAD) {
    co_yield();
  }
  free(co);
  int id = 0;
  for (id = 0; id < total; ++id) {
    if (co_pointers[id] == co) {
      break;
    }
  }
  while (id < total - 1) {
    co_pointers[id] = co_pointers[id+1];
    ++id;
  }
  --total;
  co_pointers[total] = NULL;
}

void co_yield() {
  int val = setjmp(co_now->context);
  if (val == 0) {
    struct co *next = get_next_co();
    co_now = next;
    if (next->status == CO_NEW) {
      next->status = CO_RUNNING;
      asm volatile(
      #if __x86_64__
                "movq %%rdi, (%0); movq %0, %%rsp; movq %2, %%rdi; call *%1"
                :
                : "b"((uintptr_t)(next->stack + sizeof(next->stack))), "d"(next->func), "a"((uintptr_t)(next->arg))
                : "memory"
      #else
                "movl %%esp, 0x8(%0); movl %%ecx, 0x4(%0); movl %0, %%esp; movl %2, (%0); call *%1"
                :
                : "b"((uintptr_t)(next->stack + sizeof(next->stack) - 8)), "d"(next->func), "a"((uintptr_t)(next->arg))
                : "memory" 
      #endif
      );

      asm volatile(
      #if __x86_64__
                "movq (%0), %%rdi"
                :
                : "b"((uintptr_t)(next->stack + sizeof(next->stack)))
                : "memory"
      #else
                "movl 0x8(%0), %%esp; movl 0x4(%0), %%ecx"
                :
                : "b"((uintptr_t)(next->stack + sizeof(next->stack) - 8))
                : "memory"
      #endif
      );

      next->status = CO_DEAD;

      if (co_now->waiter) {
        co_now = co_now->waiter;
        longjmp(co_now->context, 1);
      }
      co_yield();
    } else if (next->status == CO_RUNNING) {
      longjmp(next->context, 1);
    } else {
      assert(0);
    }
  } else {
    // longjmp返回，不处理
  }
}

__attribute__((constructor)) void init() {
  struct co* main = (struct co*)malloc(sizeof(struct co));
  strcpy(main->name, "main");
  main->status = CO_RUNNING;
  main->waiter = NULL;
  co_now = main;
  total= 1;
  memset(co_pointers, 0, sizeof(co_pointers));
  co_pointers[0] = main;
}