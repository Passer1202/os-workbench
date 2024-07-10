#include<buddy.h>

//将信息头和数据区分开
//信息头用于存储信息，数据区用于存储数据
//数据区一页最小64KB

//如果buddy_header占用空间太大，试着见效下面两个参数


#define BUDDY_HEADER_SIZE (1<<21)
#define BUDDY_PAGE_SIZE (1<<16)


//__16MB才是真的16MB!!!!!!!!
#define __16MB 16777216

enum BUDDY_STATE{
    FREE=0,
    USED
};


enum BUDDY_KIND{
    _64KB=0,
    _128KB,
    _256KB,
    _512KB,
    _1MB,
    _2MB,
    _4MB,
    _8MB,
    _16MB,
    BUDDY_KINDS
};

uintptr_t buddy_start=0;

typedef struct buddy_page_t{
    int used;
    int size;
    void* next;
}bpage;

typedef union buddy_header_t{
    struct{
        void* free_nodes[BUDDY_KINDS];//每个size的空闲链表
        bpage pages[BUDDY_PAGE_SIZE];//每个页的信息
    };
    char buddy_header[BUDDY_HEADER_SIZE];//给整个头分配2MB的空间//maybe too large？
}buddy_header;

buddy_header* bhdr=NULL;

void buddy_init(uintptr_t heap_start,uintptr_t heap_end){
    
    //初始化buddy_header
    bhdr=(buddy_header*)heap_start;
    memset(bhdr,0,sizeof(buddy_header));

    //使heap_start指向数据区、
    heap_start+=sizeof(buddy_header);
    //使heap_start对齐到16MB
    heap_start=(heap_start+__16MB-1)&~(__16MB-1);
    //使heap_end对齐到16MB//貌似没啥用？
    heap_end&=~(__16MB-1);

    //初始化buddy_start，指向数据区开始
    buddy_start=heap_start;

    //将16MB的头给更新好
    uintptr_t heap_size = heap_end-heap_start;
    uintptr_t cnt=heap_size>>24;

    for(int i=0;i<cnt;i++){
        int index=i*256;//256:=16MB/64KB
        bhdr->pages[index].used=FREE;
        bhdr->pages[index].size=_16MB;
        //bhdr->pages[i].next=NULL;
    }

    cnt--;
    for(int i=0;i<cnt;i++){
        int index=i*256;
        bhdr->pages[index].next=&bhdr->pages[index+256];
    }

    bhdr->pages[cnt*256].next=NULL;
    bhdr->free_nodes[_16MB]=&bhdr->pages[0];
}

void* buddy_alloc(size_t size){
    return NULL;
}

void buddy_free(void* ptr){

}