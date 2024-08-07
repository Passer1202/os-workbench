#include <os.h>




static void os_init() {
    pmm->init();
    kmt->init();

    //for(int i=0;i<3;i++)
    //  printf("i:%p\n",task_alloc());

    // 测试一: 简单测试，中断"c","d"交替出现


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
    //yield();
    while (1) ;
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
    for(int j=0;j<item_cnt;j++){
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
