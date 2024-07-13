#include <common.h>

static void os_init() {
    pmm->init();
    kmt->init();
    //dev->init();
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
    while (1) ;
  }
#endif


MODULE_DEF(os) = {
    .init = os_init,
    .run  = os_run,
};
