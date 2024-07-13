#include <kernel.h>
#include <klib.h>
#include <klib-macros.h>


/********** pmm **********/
#define CPU_MAX 8//最大CPU数量


/********** kmt **********/

#define STACK_SIZE 8192//栈大小


#define INT_MIN 0x80000000
#define INT_MAX 0x7fffffff

//from xv6
struct spinlock {
  int locked;       // Is the lock held?

  // For debugging:
  const char *name;        // Name of lock.
  struct cpu *cpu;   // The cpu holding the lock.
};

//BLOCKED (在等待某个锁，此时不能被调度执行)；RUNNABLE (可被调度执行)
enum TASK_STATUS
{
    BLOCKED=0,
    RUNNABLE,
};

//from thread-os
struct task
{   
    union{
        struct {
            int status;
            const char    *name;
            void          (*entry)(void *);
            Context       context;
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
