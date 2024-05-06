#include <common.h>
//TODO：自定测试框架

//重构代码！！！！



static void *kalloc(size_t size) {
   return NULL;
}
//有死锁
static void kfree(void *ptr) {
    return;
}



// 框架代码中的 pmm_init (在 AbstractMachine 中运行)
static void pmm_init() {
  uintptr_t pmsize = ((uintptr_t)heap.end - (uintptr_t)heap.start);
  printf("Got %d MiB heap: [%p, %p)\n", pmsize >> 20, heap.start, heap.end);
  
}


void alloc(int sz){
    
    uintptr_t a=(uintptr_t)kalloc(sz);

    uintptr_t align=a & -a ;

    //atomic{
    printf("CPU #%d : Alloc %d -> %p align = %d\n", cpu_current(),sz, a, align);
    //}

    assert(a&&align>=sz);
}

void test_pmm() {
   
    alloc(1);
    alloc(5);
    alloc(10);
    alloc(32);
    while(1){
        alloc(4096);
    }
}

MODULE_DEF(pmm) = {
    .init  = pmm_init,
    .alloc = kalloc,
    .free  = kfree,
};
