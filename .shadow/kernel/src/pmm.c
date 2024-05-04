#include <common.h>
//TODO：自定测试框架

//魔数
#define MAGIC_NUM 0X1234567
//cpu数量不超过8个
#define CPU_MAX 8

const int MIN_SIZE = 32;
const int _16KB = 16*1024;//16KB
const int _64KB = 64*1024;//64KB
const int _16MB = 16*1024*1024;//16MB


/*锁*/
enum LOCK_STATE {
    PMM_UNLOCKED=0, PMM_LOCKED
};
//一把大锁保平安（速通版）
#define atomic \
    for (int _cnt = (get_lock(&biglock),0); _cnt < 1; _cnt++ , release_lock(&biglock))

//int atomic_xchg(volatile int *addr, int newval);
//原子 (不会被其他处理器的原子操作打断) 地交换内存地址中的数值,返回原来的值
//初始化锁
static void init_lock(int * lock){
    atomic_xchg(lock, PMM_UNLOCKED);
}
//自旋直到获取锁
static void get_lock(int * lock){
    while(atomic_xchg(lock, PMM_LOCKED) == PMM_LOCKED);
}
//释放锁
static void release_lock(int * lock){
    assert(atomic_xchg(lock, PMM_UNLOCKED)==PMM_LOCKED);
}
//尝试获取锁
//static int try_lock(int * lock){
//    return atomic_xchg(lock, PMM_LOCKED);
//}

freenode* head;

pmm_lock_t biglock;

page_t localpage[CPU_MAX];//cpu本地的页们

static void *HUGE_SIZE_ALLOC(size_t size){
    //一把大锁保平安
    get_lock(&biglock);
    freenode *p=head;
    freenode *pre=NULL;
    while(p){
        if(p->size>=size){//能分配的下
            if(pre==NULL)
                pre=p;
            else
                pre=(pre<p)?p:pre;
        }
        p=p->next;
    }
    if(pre==NULL){
        release_lock(&biglock);
        return NULL;
    }
    assert(pre!=NULL);
    assert(pre->size>=size);
    void *ret=(void*)pre+pre->size-size;
    header *h=ret-sizeof(header);
    h->size=size;
    h->magic=MAGIC_NUM;
    release_lock(&biglock);
    return ret;
}
void get_page(pheader **header){
    //TODO
    //分配一页
    freenode *node=NULL;

    get_lock(&biglock);

    freenode *p=head;
    freenode *pre=NULL;
    while(p){
        if(p->size>=_64KB){
            node=p;
            break;
        }
        pre=p;
        p=p->next;
    }
    if(p){
        if(node->size==_64KB){
            if(pre){
                assert(pre->next==node);
                pre->next=node->next;
            }
        }
        else{
            assert(node->size>_64KB);
            freenode *freenode=(void*)node+_64KB;
            freenode->size=node->size-_64KB;
            freenode->next=node->next;
            if(pre){
                assert(pre->next==node);
                pre->next=freenode;
            }
            else{
                assert(head==node);
                head=freenode;
            }
        }
        release_lock(&biglock);
        *header=(void*)node;
        return;
    }
    else{
        release_lock(&biglock);
        *header=NULL;
        return;
    }
}

static void *kalloc(size_t size) {
    if(size<MIN_SIZE)
        size=MIN_SIZE;
    else if(size>_16KB){//非常罕见的大内存分配
        //一把大锁保平安

        //拒绝16MB以上的内存分配
        if(size>_16MB)
            return NULL;
        
        size_t sz=MIN_SIZE;
        while(sz<size)
            sz<<=1;
        void* ret=HUGE_SIZE_ALLOC(sz);
        return ret;    
    }
    size_t sz=MIN_SIZE;
    while(sz<size)
        sz<<=1;
    int cpunow=cpu_current();
    if(localpage[cpunow].header==NULL){
        get_lock(&localpage[cpunow].lock);
        //分配一页
        get_page(&localpage[cpunow].header);
        //TODO
        assert(localpage[cpunow].header!=NULL);
        localpage[cpunow].header->next=NULL;
        localpage[cpunow].header->size=sz;//slab大小
        localpage[cpunow].header->free_1st = 1;//第一个空闲块的位置
        void* firstnode=(void*)localpage[cpunow].header+sz;
        pfreenode *p=(pfreenode*)firstnode;
        p->size=_64KB-sz;
        p->magic=MAGIC_NUM;
        p->next=NULL;
        release_lock(&localpage[cpunow].lock);
    }
    get_lock(&localpage[cpunow].lock);
    pheader *ph=localpage[cpunow].header;
    while(ph){
        if(ph->size==sz&&ph->free_1st>0)
            break;
        ph=ph->next;
    } 
    if(ph){
        assert(ph->size==sz&&ph->free_1st>0);
        pfreenode *node=(void* )ph+ph->free_1st*sz;
        if(node->size>=sz){
            assert(node->magic==MAGIC_NUM);
            node->size-=sz;
            if(node->size==0){
                if(node->next==NULL){
                    ph->free_1st=0;
                }
                else{
                    ph->free_1st=((uintptr_t)node->next-(uintptr_t)ph)/sz;
                    assert((uintptr_t)ph+ph->free_1st*sz==(uintptr_t)node->next);

                }
            }
            else{
                pfreenode *newnode=(void*)node+sz;
                newnode->size=node->size;
                newnode->magic=MAGIC_NUM;
                newnode->next=node->next;
                ph->free_1st++;
                assert((uintptr_t)ph+ph->free_1st*sz==(uintptr_t)newnode);
                assert(ph->free_1st*sz<_64KB);
            }
            release_lock(&localpage[cpunow].lock);
            return (void*)node;
        }
        else{
            release_lock(&localpage[cpunow].lock);
            assert(0);
        }
    }
    else{
        //没这种slab
        pheader *pagehead=localpage[cpunow].header;
        while(pagehead){
            if(pagehead->size==0&&pagehead->free_1st>=0){
                pagehead->size=sz;
                break;
            }
            pagehead=pagehead->next;
        }
        if(pagehead==NULL){
            get_page(&pagehead);
            if(pagehead==NULL){//没有空闲页了
                release_lock(&localpage[cpunow].lock);
                return NULL;
            }
            pagehead->size=sz;
            pagehead->free_1st=1;
            pagehead->next=localpage[cpunow].header;
            localpage[cpunow].header=pagehead;
        }
        void* ret=(void*)pagehead+pagehead->free_1st*sz;
        pfreenode *newnode=ret+sz;
        newnode->next=NULL;
        newnode->magic=MAGIC_NUM;
        pagehead->free_1st++;
        newnode->size=_64KB-pagehead->free_1st*sz;
        release_lock(&localpage[cpunow].lock);
        return ret;
    }
}

static void kfree(void *ptr) {

    //这里面的锁是不是不对劲
    assert(ptr!=NULL);
    //遍历cpu本地的页
    get_lock(&localpage[cpu_current()].lock);
    pheader *ph=NULL;
    int isok=0;
    for(size_t i=0;i<cpu_count();i++){
        ph=localpage[i].header;
        while(ph){
            if((uintptr_t)ptr>=(uintptr_t)ph&&(uintptr_t)ptr<(uintptr_t)ph+_64KB){
                isok=1;
                break;
            }
        }
        if(isok)
            break;
    }
    if(ph){
        assert(isok);
        assert((uintptr_t)ptr>=(uintptr_t)ph&&(uintptr_t)ptr<(uintptr_t)ph+_64KB);
        pfreenode *nodea=NULL;//after
        pfreenode *nodeb=NULL;//before
        pfreenode *p=(void*)ph+ph->free_1st*ph->size;
        //遍历，看看是不是在cpu里的
        while(p){
            if((uintptr_t)p>(uintptr_t)ptr){
                if(nodea==NULL){
                    nodea=p;
                }
                else{
                    nodea=(nodea>p)?p:nodea;
                }
            }
            if((uintptr_t)p<(uintptr_t)ptr){
                if(nodeb==NULL){
                    nodeb=p;
                }
                else{
                    nodeb=(nodeb<p)?p:nodeb;
                }
            }
            p=p->next;
        }
        //前一个节点挨着ptr
        if(nodeb&&(uintptr_t)nodeb+nodeb->size==(uintptr_t)ptr){
            nodeb->size+=ph->size;
            if(ptr+ph->size==(void*)nodea){
                nodeb->size+=nodea->size;
                p=(void*)ph+ph->free_1st*ph->size;
                while(p){
                    if(p->next==nodea){
                        break;
                    }
                    p=p->next;
                }
                if(p){
                    p->next=nodea->next;
                }
                else{
                    ph->free_1st=((uintptr_t)nodeb-(uintptr_t)ph)/ph->size;
                }
            }
            if(nodeb->size==_64KB-ph->size){
                ph->size=0;
            }
        }
        //后一个节点挨着ptr
        else if(ptr+ph->size==(void*)nodea){
           pfreenode *pf=(pfreenode *)ptr;
           pf->magic=MAGIC_NUM;
           pf->size+=nodea->size;
           pf->next=(void*)ph+ph->free_1st*ph->size;
           ph->free_1st=((uintptr_t)pf-(uintptr_t)ph)/ph->size;
           assert(ph->free_1st>0);
           assert(ph->free_1st*ph->size<_64KB);
           p=(void*)ph+ph->free_1st*ph->size;
           while(p){
            if(p->next==nodea){
                break;
            }
            p=p->next;
           }
           p->next=nodea->next;
           if(pf->size==_64KB-ph->size){
               ph->size=0;
           }
        }
        //没有节点挨着ptr
        else{
            pfreenode *pf=(pfreenode *)ptr;
            pf->magic=MAGIC_NUM;
            pf->size=ph->size;
            pfreenode *pre=(void*)ph+ph->free_1st*ph->size;
            pf->next=pre;
            ph->free_1st=((uintptr_t)pf-(uintptr_t)ph)/ph->size;
            assert(ph->free_1st>0);
        }
        release_lock(&localpage[cpu_current()].lock);
        return;

    }
    else{
        release_lock(&localpage[cpu_current()].lock);
        get_lock(&biglock);
        header *h=ptr-sizeof(header);
        freenode *nodea=NULL;
        freenode *nodeb=NULL;
        freenode *p=head;
        assert(h->magic==MAGIC_NUM);
        size_t sz=h->size;
        freenode *fn=(void*)h;
        while(p){
            if((uintptr_t)p>(uintptr_t)h){
                if(nodea==NULL){
                    nodea=p;
                }
                else{
                    nodea=(nodea>p)?p:nodea;
                }
            }
            if((uintptr_t)p<(uintptr_t)h){
                if(nodeb==NULL){
                    nodeb=p;
                }
                else{
                    nodeb=(nodeb<p)?p:nodeb;
                }
            }
            p=p->next;
        }
        if(nodeb&&(uintptr_t)nodeb+nodeb->size==(uintptr_t)h){
            nodeb->size+=sz;
            nodeb->size+=sizeof(header);
            if(ptr+sz==(void*)nodea){
                nodeb->size+=nodea->size;
                p=head;
                while(p){
                    if(p->next==nodea){
                        break;
                    }
                    p=p->next;
                }
                if(p){
                    p->next=nodea->next;
                }
                else{
                    head=head->next;
                }
            }
        }
        else if(ptr+sz==(void*)nodea){
            fn->size=nodea->size+sz;
            fn->next=head;
            head=fn;
            p=fn;
            while(p){
                if(p->next==nodea){
                    break;
                }
                p=p->next;
            }
            p->next=nodea->next;
        }
        else{
            fn->size=sz;
            fn->next=head;
            head=fn;
        }
        release_lock(&biglock);
        return;
    }
}


#ifndef TEST
// 框架代码中的 pmm_init (在 AbstractMachine 中运行)
static void pmm_init() {
  uintptr_t pmsize = ((uintptr_t)heap.end - (uintptr_t)heap.start);
  printf("Got %d MiB heap: [%p, %p)\n", pmsize >> 20, heap.start, heap.end);
  
  head = heap.start;
  assert(head!=NULL);
  head->size = pmsize;
  assert(head->size==heap.end-heap.start);
  head->next=NULL;
  init_lock(&biglock);
  
  for (size_t i = 0; i<cpu_count(); i++){
    localpage[i].header = NULL;//还没给分配页
    init_lock(&localpage[i].lock);
  }

  
}
#else
// 测试代码的 pmm_init ()
static void pmm_init() {
  char *ptr  = malloc(HEAP_SIZE);
  heap.start = ptr;
  heap.end   = ptr + HEAP_SIZE;
  printf("Got %d MiB heap: [%p, %p)\n", HEAP_SIZE >> 20, heap.start, heap.end);
}
#endif

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
    void* a=kalloc(4096);
    kfree((void*)a);
    while(1)
    alloc(4096);
    
}

MODULE_DEF(pmm) = {
    .init  = pmm_init,
    .alloc = kalloc,
    .free  = kfree,
};
