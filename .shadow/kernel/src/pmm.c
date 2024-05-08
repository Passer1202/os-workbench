#include <common.h>
//TODO：自定测试框架
//重构代码！！！！

//魔数
#define MAGIC_NUM 0X1234567
//cpu数量不超过8个
#define CPU_MAX 8
//分配的最小内存块大小
#define MIN_SIZE  16
//4KB
#define _4KB  4*1024
//64KB
#define _64KB  64*1024
//16MB
#define _16MB  16*1024*1024

/*定义锁lock_t*/
typedef int lock_t;

/*锁的状态*/
enum LOCK_STATE {
    UNLOCKED=0, LOCKED
};

/*锁们*/
//堆区的大锁，slowpath
lock_t heap_lock;

/*锁的API*/
//int atomic_xchg(volatile int *addr, int newval);
//原子 (不会被其他处理器的原子操作打断) 地交换内存地址中的数值,返回原来的值

//初始化锁
static void init_lock(int * lock){
    atomic_xchg(lock, UNLOCKED);
}
//自旋直到获取锁
static void acquire_lock(int * lock){
    while(atomic_xchg(lock, LOCKED) == LOCKED);
}
//释放锁
static void release_lock(int * lock){
    assert(atomic_xchg(lock, UNLOCKED)==LOCKED);
}
//尝试获取锁
//static int try_lock(int * lock){
//    return atomic_xchg(lock, PMM_LOCKED);
//}

//一把大锁保平安（速通版）
#define atomic \
    for (int _cnt = (acquire_lock(&heap_lock),0); _cnt < 1; _cnt++ , release_lock(&heap_lock))


//hader_t参考了WYY的定义
typedef struct __header_t {
    size_t sz;
    union {
        uintptr_t magic;
        struct __header_t *next;
    };
}header_t;

//堆区的头节点
header_t *head=NULL;

//每个page的头节点，指示当前page的slab大小以及下一页page的地址
typedef struct __page_header_t {
    size_t sz;
    struct __page_header_t *next;
    void* first_slab;
}pheader_t;

lock_t cpu_page_lock[CPU_MAX];

typedef pheader_t cpheader_t;
cpheader_t* cpu_page[CPU_MAX];


static void *kalloc(size_t size) {
    
    if(size>_16MB){
        return NULL;
    }

    //assert(size<=_16MB);

    size_t sz=MIN_SIZE;
    while(sz<size){
            sz<<=1;
    }

    //assert(sz>=MIN_SIZE);
    //assert(sz>=size);

    if(sz>_4KB){
        //(4KB,16MB]
        //大内存分配,一把大锁保平安
        acquire_lock(&heap_lock);
        printf("CPU #%d : Alloc %d\n", cpu_current(),sz);

        header_t *p=head;
        header_t *pre=NULL;

        while(p){
            if(p->sz>=sz+sizeof(header_t)){
                char* pos=(char*)p+sizeof(header_t)+p->sz-sz;
                while((intptr_t)pos%sz!=0){
                    pos--;
                }
                if(pos>=(char*)p+sizeof(header_t)){
                    //此时pos有两种情况
                    //assert(pos>=((char*)p+sizeof(header_t)));

                    if(pos<((char*)p+2*sizeof(header_t))){
                        //原来的header_t要作废，形成空洞
                        if(pre){
                            assert(pre->next==p);
                            pre->next=p->next;
                            //跳了过去
                        }
                        else{
                            assert(p==head);
                            head=p->next;
                        }

                    }
                    else{
                        //原来的header_t不会被破坏，只需要略加维护
                        //此时原header_t对应的size可能是0
                        p->sz=p->sz-sz-sizeof(header_t);
                    }
                    header_t *new=(header_t*)pos-sizeof(header_t);
                    new->sz=sz;
                    new->magic=MAGIC_NUM;
                    //p<pos-sizeof(header_t)<p+sizeof(header_t)
                    //p-sizeof(header_t)>p+sizeof(header_t)

                    //返回pos
                    release_lock(&heap_lock);
                    return (void*)pos;
                }
            }
            pre=p;
            p=p->next;
        }

        release_lock(&heap_lock);
        return NULL;
    }
    //[16B,4KB]

    //没slab或slab满了就分配一个slab
    int cpu_now=cpu_current();
    acquire_lock(&cpu_page_lock[cpu_now]);
    
    if(cpu_page[cpu_now]==NULL){
        //分配一页64KB的内存
        //void* newslab=alloc_page();
        release_lock(&cpu_page_lock[cpu_now]);
        void *newslab=kalloc(_64KB);
        acquire_lock(&cpu_page_lock[cpu_now]);

        if(newslab==NULL){
            release_lock(&cpu_page_lock[cpu_now]);
            return NULL;
        }

        //初始化slab_page

        //先不考虑释放
        //不停往前走
        pheader_t* ph=(pheader_t*)newslab;
        cpu_page[cpu_now]=ph;

        ph->sz=sz;
        ph->next=NULL;
        ph->first_slab=(void*)ph+sizeof(pheader_t);
        while((intptr_t)ph->first_slab%sz!=0){
            ph->first_slab++;
        }

        //找到第一个slab

        void *ret=ph->first_slab;
        ph->first_slab+=sz;
        release_lock(&cpu_page_lock[cpu_now]);
        return ret;

        //分配第一个slab

    }
    else{
        //先找有没有slab页满足要求
        pheader_t* ph=cpu_page[cpu_now];
        while(ph){
            if(ph->sz==sz){
                if(ph->first_slab+sz<(void*)ph + _64KB){
                    break;
                }
            }
            ph=ph->next;
        }
        if(ph){
            //找到了
            void *ret=ph->first_slab+sz;
            ph->first_slab+=sz;
            release_lock(&cpu_page_lock[cpu_now]);
            return ret;
        }
        else{
            void *newslab=kalloc(_64KB);

            if(newslab==NULL){
                release_lock(&cpu_page_lock[cpu_now]);
                return NULL;
            }
            pheader_t* nph=(pheader_t*)newslab;
            nph->next=cpu_page[cpu_now];
            cpu_page[cpu_now]=nph;
            nph->sz=sz;
            nph->first_slab=(void*)nph+sizeof(pheader_t);
            while((intptr_t)nph->first_slab%sz!=0){
                nph->first_slab++;
            }
            void *ret=nph->first_slab;
            nph->first_slab+=sz;
            release_lock(&cpu_page_lock[cpu_now]);
            return ret;
        }

    }

}

static void kfree(void *ptr) {
    return;
}



// 框架代码中的 pmm_init (在 AbstractMachine 中运行)
static void pmm_init() {
    uintptr_t pmsize = ((uintptr_t)heap.end - (uintptr_t)heap.start);
    printf("Got %d MiB heap: [%p, %p)\n", pmsize >> 20, heap.start, heap.end);

    //维护空闲节点
    head = (header_t *)heap.start;
    head->sz = pmsize-sizeof(header_t);//减去头部大小
    head->next = NULL;

    
    //初始化每个CPU的page，都为空NULL
    for(size_t i=0;i<cpu_count();i++){
        cpu_page[i]=NULL;
        init_lock(&cpu_page_lock[i]);
    }

    init_lock(&heap_lock);
    printf("PMM: init done\n");
    //锁不分配给CPU，分配给slab_page.


}


void alloc(int sz){
    
    uintptr_t a=(uintptr_t)kalloc(sz);

    uintptr_t align=a & -a ;

    atomic{
    printf("CPU #%d : Alloc %d -> %p align = %d\n", cpu_current(),sz, a, align);
    }

    assert(a&&align>=sz);
}

void test_pmm() {
   
    alloc(1);
    alloc(5);
    alloc(10);
    alloc(32);
    alloc(4096);
    alloc(5000);
    //while(1){
        //alloc(4096);
    //}
}

MODULE_DEF(pmm) = {
    .init  = pmm_init,
    .alloc = kalloc,
    .free  = kfree,
};