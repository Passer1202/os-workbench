#include <kernel.h>
#include <klib.h>
#include <klib-macros.h>


/********** pmm **********/
#define CPU_MAX 8//最大CPU数量


/********** kmt **********/

#define STACK_SIZE 8192//栈大小


#define INT_MIN 0x80000000
#define INT_MAX 0x7fffffff

typedef struct kmt_cpu_t{
  //记录锁的嵌套层数和最外层的锁前中断与否
  int intr;//记录中断是否开启
  int ncli;//记录锁的嵌套层数
}kcpu;

//from xv6
struct spinlock {
  int locked;       // Is the lock held?

  // For debugging:
  const char *name;        // Name of lock.
  int cpu_no;   // The cpu holding the lock.
};

//BLOCKED (在等待某个锁，此时不能被调度执行)；RUNNABLE (可被调度执行)
enum TASK_STATUS
{
    BLOCKED=0,
    RUNNABLE,
    RUNNING,
    IDLE,
};

//from thread-os
struct task
{   
    union{
        struct {
            int status;
            const char    *name;
            void          (*entry)(void *);
            Context*       context;
            struct task    *next;
            char          end[0];
        };

        uint8_t stack[STACK_SIZE];
    };
};


struct semaphore
{
  int val;//信号量的值
  spinlock_t lock;

  task_t *queue;//等待队列

  const char* name;

  //void * thread[NPROC];
  //unsigned int next;
  //unsigned int end;
};
