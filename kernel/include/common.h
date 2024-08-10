#include <kernel.h>
#include <klib.h>
#include <klib-macros.h>


/********** pmm **********/
#define CPU_MAX 8//最大CPU数量


/********** kmt **********/

#define STACK_SIZE 4096//栈大小



#define INT_MAX 0x7fffffff
#define INT_MIN (-INT_MAX-1)

typedef struct kmt_cpu_t{
  //记录锁的嵌套层数和最外层的锁前中断与否
  int intr;//记录中断是否开启
  int ncli;//记录锁的嵌套层数
}kcpu;

//from xv6
struct spinlock {
  int locked;       // Is the lock held?

  // For debugging:
  char name[64];        // Name of lock.
  int cpu_no;   // The cpu holding the lock.
};

//BLOCKED (在等待某个锁，此时不能被调度执行)；RUNNABLE (可被调度执行)
enum TASK_STATUS
{
    BLOCKED=0,
    RUNNABLE,
    RUNNING,
    IDLE,
    ZOMBIE,//冷却
};

//from thread-os
struct task
{   
    union{
        struct {
            int status;
            const char    *name;
            void          (*entry)(void *);
            struct task    *next;
            Context*       context;
            char          end[0];
        };

        uint8_t stack[STACK_SIZE];
    };
};


struct semaphore
{
  int val;//信号量的值
  spinlock_t lock;

  //开始是用链表来实现的
  //后来意识到NEXT的改变会和task的链表产生冲突
  int qh;//队列头
  int qt;//队列尾：下一个可用位置
  int cnt_max;//队列中的任务最大数量
  task_t *wait_queue[128];//等待队列
  
  const char* name;

  //void * thread[NPROC];
  //unsigned int next;
  //unsigned int end;
};

typedef struct item {
    int seq;
    int event;
    handler_t handler;
}item_t;

