#include <os.h>

static inline task_t *task_alloc() { return pmm->alloc(sizeof(task_t)); }
//目前来看alloc存在问题

// 测试一
//#define TEST_1
void print(void *arg) {
    char *c = (char *)arg;
    while (1) {
        putch(*c);
        for (int i = 0; i < 100000; i++)
            ;
    }
}
// 测试二
 //#define TEST_2
static spinlock_t lk1;
static spinlock_t lk2;
void lock_test(void *arg) {
    int *intr = (int *)arg;
    // intr = 0, 关中断, ienabled() = false
    // intr = 1， 开中断, ienabled() = false
    // ABAB 形的锁测试
    if (!*intr)
        iset(false);
    else
        iset(true);
    kmt->spin_lock(&lk1);
    for (int i = 0; i < 100000; i++)
        ;
    panic_on(ienabled() == true, "不应该开中断！");
    kmt->spin_lock(&lk2);
    for (int i = 0; i < 100000; i++)
        ;
    panic_on(ienabled() == true, "不应该开中断！");
    kmt->spin_unlock(&lk1);
    for (int i = 0; i < 100000; i++)
        ;
    panic_on(ienabled() == true, "不应该开中断！");
    kmt->spin_unlock(&lk2);
    for (int i = 0; i < 100000; i++)
        ;
    panic_on(ienabled() != *intr, "中断恢复错误！");
    printf("pass test for ABAB lock, 锁的初始状态是：[%d]\n", *intr);
    if (!*intr)
        iset(false);
    else
        iset(true);
    kmt->spin_lock(&lk1);
    for (int i = 0; i < 100000; i++)
        ;
    panic_on(ienabled() == true, "不应该开中断！");
    kmt->spin_lock(&lk2);
    for (int i = 0; i < 100000; i++)
        ;
    panic_on(ienabled() == true, "不应该开中断！");
    kmt->spin_unlock(&lk1);
    for (int i = 0; i < 100000; i++)
        ;
    panic_on(ienabled() == true, "不应该开中断！");
    kmt->spin_unlock(&lk2);
    for (int i = 0; i < 100000; i++)
        ;
    panic_on(ienabled() != *intr, "中断恢复错误！");
    printf("pass test for ABBA lock, 锁的初始状态是：[%d]\n", *intr);
    iset(true);
    while (1)
        ;
}

#define TEST_3
#define P kmt->sem_wait
#define V kmt->sem_signal
sem_t empty, fill;
void producer(void *arg) {
    while (1) {
        //printf("cpu_current: %d\n",cpu_current());

        P(&empty);
        putch('(');
        V(&fill);
    }
}
void consumer(void *arg) {
    
    while (1) {
        //printf("cpu_current: %d\n",cpu_current());
        P(&fill);
        //assert(0);
        putch(')');
        V(&empty);
    }
}





static void os_init() {
    pmm->init();
    kmt->init();

    //for(int i=0;i<3;i++)
    //  printf("i:%p\n",task_alloc());

    // 测试一: 简单测试，中断"c","d"交替出现
#ifdef TEST_1
    kmt->create(task_alloc(), "a", print, "c");
    kmt->create(task_alloc(), "b", print, "d");
#endif

// 测试二： 锁的测试
#ifdef TEST_2
    static int zero = 0, one = 1;
    kmt->spin_init(&lk1, NULL);
    kmt->spin_init(&lk2, NULL);
    int *arg0 = &zero;
    int *arg1 = &one;
    kmt->create(task_alloc(), "lock_test", lock_test, (void *)arg0);
    kmt->create(task_alloc(), "lock_test", lock_test, (void *)arg1);
#endif

#ifdef TEST_3
    kmt->sem_init(&empty, "empty",1);  // 缓冲区大小为 5
    kmt->sem_init(&fill, "fill", 0);

    for (int i = 0; i < 4; i++)  // 4 个生产者
        kmt->create(task_alloc(), "producer", producer, NULL);
    for (int i = 0; i < 5; i++)  // 5 个消费者
        kmt->create(task_alloc(), "consumer", consumer, NULL);
#endif

}




//#define TEST 

#if defined TEST
  static void os_run() {
    printf("==================\n");
    printf("Test_begin:\n");
    test_pmm();
  }
#else
  static void os_run() {

    // for (const char *s = "Hello World from CPU #*\n"; *s; s++) {
    //    putch(*s == '*' ? '0' + cpu_current() : *s);
    //}
    iset(true);
    yield();
    while (1) {
        yield();
    };
  }
#endif

#define ITEM_MAX 128
static item_t items[ITEM_MAX];
static int item_cnt = 0;

static Context *os_trap(Event ev, Context *ctx) {

    assert(ienabled()==false);

    Context *next = NULL;

    //printf("item_cnt: %d\n",item_cnt);
    //assert(0);
    for (int i=0;i<item_cnt;i++) {
        if (items[i].event == EVENT_NULL || items[i].event == ev.event) {
            Context *r = items[i].handler(ev, ctx);
            panic_on(r && next, "return to multiple contexts");
            if (r) next = r;
        }
    }
    panic_on(!next, "return to NULL context");
    return next;
}

static void os_irq(int seq,int event,handler_t handler){
  
  items[item_cnt].seq=seq;
  items[item_cnt].event=event;
  items[item_cnt].handler=handler;

  item_cnt++;

  //bubble_sort
  for(int i=0;i<item_cnt;i++){
    for(int j=i+1;j<item_cnt;j++){
      if (items[i].seq > items[j].seq) {
        item_t temp=items[i];
        items[i]=items[j];
        items[j]=temp;        
      }
    }
  }


}

MODULE_DEF(os) = {
    .init = os_init,
    .run  = os_run,
    .trap =os_trap,
    .on_irq=os_irq,
};
