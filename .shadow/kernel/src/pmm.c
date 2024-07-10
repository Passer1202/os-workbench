#include <common.h>
#include <buddy.h>
#include <plock.h>

//修改jyy的速通版本，为每个cpu分配领地可有概率ac
//继续修改，将一把大锁分配给每个cpu,会导致分配出错
//问题所在：每个cpu的内存空间太小了

//TODO：重构代码

//初步设想：
//利用slab和buddy系统，将内存分配给每个cpu
//fastpath：slab
//slowpath：大内存分配，buddy

#define _4KB  4096
#define _64KB 65536

#define _16MB 16777216

#define MIN_SIZE 16//最小分配16字节

#define DATA_SIZE (_64KB-_4KB)//每个slab_page的data大小
#define SLAB_MAX (DATA_SIZE/MIN_SIZE)//slab_page最多有多少个slab

#define MAGIC_NUM 0x12345678

enum slab_kinds{
    _16B=0,
    _32B,
    _64B,
    _128B,
    _256B,
    _512B,
    _1024B,//1KB
    _2048B,//2KB
    _4096B,//4KB
    SLAB_KINDS
};

/*slab_page的结构*/
//参考了学长的设计
typedef union{
    struct{
        int magic;
        int cnt;//计数
        int val;//容量
        int sz;//每个slab的大小
        void* next;
        int slab_lock;
        uint8_t used[SLAB_MAX];
    };
    struct{
        uint8_t header[_4KB];
        uint8_t data[_64KB-_4KB];
    };
    uint8_t slabs[_64KB];//每个slab_page大小64KB，前4KB为header，后60KB为data
}slab_page;


//堆区大锁
static lock_t heap_lock;

//cpu内存空间
//一个slab一个锁
typedef struct{
    void* slab_ptr[SLAB_KINDS];
    lock_t page_lock[SLAB_KINDS];
}cpu_local_t;

static cpu_local_t cpu_local[CPU_MAX];

//static char* h_ptr;
/*
static void* heap_alloc(size_t size){
    size_t sz=1;
    
    //acquire_lock(&heap_lock);

    while(sz<size){
        sz*=2;
    }

    if(sz>(1<<24)){
        //release_lock(&heap_lock);
        return NULL;
    }

    

    char* p=h_ptr;

    
    while((intptr_t)p%sz!=0){
        p++;
    }


    char* ret=p;
    p+=sz;

    if((uintptr_t)p>(uintptr_t)heap.end){
        //release_lock(&heap_lock);

        return NULL;

    }


    h_ptr=p;
    
    //release_lock(&heap_lock);
    return ret;
}
*/

static void *kalloc(size_t size) {
    //先将size对齐到2的幂次
    int sz=MIN_SIZE;
    int slab_index=0;
    while(sz<size&&sz<_16MB){
        sz<<=1;
        slab_index++;
    }

    if(sz>_16MB){
         //拒绝分配16MB以上的内存
        return NULL;
    }

    if(sz>_4KB){
        //slowpath
        acquire_lock(&heap_lock);
        //TODO:buddy分配
        uintptr_t ret=(uintptr_t)buddy_alloc(sz);
        release_lock(&heap_lock);
        return (void*)ret;
    }
    else{
        //fastpath
        int cpu_now=cpu_current();
        acquire_lock(&cpu_local[cpu_now].page_lock[slab_index]);
        slab_page* page=cpu_local[cpu_now].slab_ptr[slab_index];
        if(!page){
            //分配新的slab_page
            acquire_lock(&heap_lock);
            page=(slab_page*)buddy_alloc(_64KB);
            release_lock(&heap_lock);
            if(page==NULL){
                release_lock(&cpu_local[cpu_now].page_lock[slab_index]);
                return NULL;
            }
            page->magic=MAGIC_NUM;
            page->val=DATA_SIZE/sz;
            page->cnt=0;
            page->sz=size;
            page->next=NULL;

            init_lock(&page->slab_lock);
            memset(page->used,0,SLAB_MAX);
            cpu_local[cpu_now].slab_ptr[slab_index]=page;

        }
        else{
            //遍历slab_pages
            while(page!=NULL){
                if(page->cnt<page->val){
                    break;
                }
                page=page->next;
            }
            if(page==NULL){
                //分配新的slab_page
                acquire_lock(&heap_lock);
                page=(slab_page*)buddy_alloc(_64KB);
                release_lock(&heap_lock);
                if(page==NULL){
                    
                    release_lock(&cpu_local[cpu_now].page_lock[slab_index]);
                    return NULL;
                }
                page->magic=MAGIC_NUM;
                page->cnt=0;
                
                page->val=DATA_SIZE/sz;
                page->sz=size;
                page->next=cpu_local[cpu_now].slab_ptr[slab_index];//头插法
                
                init_lock(&page->slab_lock);
                memset(page->used,0,SLAB_MAX);
                cpu_local[cpu_now].slab_ptr[slab_index]=page;
            }
        }
        for(int i=0;i<page->val;i++){
            if(page->used[i]==0){
                page->used[i]=1;
                page->cnt++;
                release_lock(&cpu_local[cpu_now].page_lock[slab_index]);

                return (void*)(page->data+i*sz);
            }
        }

        assert(0);
        return NULL;
    }

}
   

static void kfree(void *ptr) {

    //对齐到64KB
    slab_page* temp_page=(slab_page*)((uintptr_t)ptr&(~0xFFFF));

    if(temp_page->magic!=MAGIC_NUM){
        //slowpath
        acquire_lock(&heap_lock);
        buddy_free(ptr);
        release_lock(&heap_lock);
        return;
    }
    else{
        //fastpath
        acquire_lock(&temp_page->slab_lock);

        uintptr_t index=(uintptr_t)ptr-(uintptr_t)temp_page->data;
        index/=temp_page->sz;
        printf("index = %d\n",index);
        temp_page->used[index]=0;
        temp_page->cnt--;
        release_lock(&temp_page->slab_lock);
        /*
        uintptr_t p=(uintptr_t)ptr;
        slab_page* page=temp_page;
        p=p%_64KB;
        p=p-_4KB;
        assert(page->sz>0);
        p=p/page->sz;
        */
    }
   
}



// 框架代码中的 pmm_init (在 AbstractMachine 中运行)
static void pmm_init() {

    uintptr_t pmsize = ((uintptr_t)heap.end - (uintptr_t)heap.start);
    printf("Got %d MiB heap: [%p, %p)\n", pmsize >> 20, heap.start, heap.end);

    //初始化锁
    init_lock(&heap_lock);

    int cpu_cnt=cpu_count();
    
    //memset(cpu_local,0,sizeof(cpu_local));

    for(int i=0;i<cpu_cnt;i++){
        for(int j=0;j<SLAB_KINDS;j++){
            cpu_local[i].slab_ptr[j]=NULL;
            init_lock(&cpu_local[i].page_lock[j]);
        }
    }

    //初始化buddy系统
    //h_ptr=heap.start;
    buddy_init((uintptr_t)heap.start , (uintptr_t)heap.end);
    printf("PMM: init done\n");
    
}


void* alloc(int sz){
    
    uintptr_t a=(uintptr_t)kalloc(sz);


    uintptr_t align=a & -a ;

    //atomic{
    //acquire_lock(&heap_lock);
    //printf("CPU #%d : Alloc %d -> %p align = %d\n", cpu_current(),sz, a, align);
    //release_lock(&heap_lock);
//}

    assert(a&&align>=sz);
    return (void*)a;
}


void test_pmm() {
   
    //alloc(1);
    void* p[16];
    for(int i=0;i<10;i++){
        p[i]=alloc(4096);
    }
    for(int i=1;i<10;i++){
        kfree(p[i]);
    }
    //alloc(5);
    //alloc(10);
    //alloc(32);
    //alloc(16777216);
    //alloc(4096);
    ////alloc(4096);
    //alloc(4096);
    //alloc(4096);
   // alloc(5000);
    //atomic{
    //printf("PMM: test passed\n");
    //}
}

MODULE_DEF(pmm) = {
    .init  = pmm_init,
    .alloc = kalloc,
    .free  = kfree,
};