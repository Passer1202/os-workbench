#include <kernel.h>
#include <klib.h>
#include <klib-macros.h>

void test_pmm();
typedef int pmm_lock_t;


//实现Slab分配器需要的数据结构

//页的头节点信息
typedef struct pageheader_t{
    struct pageheader_t *next;
    int size;                   //slab 大小
    unsigned free_1st;          //第一个空闲块的位置
}pheader;

//管理分配个cpu的页的空闲块的节点信息
typedef struct pagefreenode_t{
    struct pagefreenode_t *next;
    int size;
    int magic;
}pfreenode;

//已分配节点的头
typedef struct header_t{
    int size;
    int magic;
}header;

//管理堆中的空闲节点
typedef struct freenode_t{
    struct freenode_t *next;
    int size;
    int magic;
}freenode;

//管理cpu所拥有的页
typedef struct page_t{
   pheader *header;
   pmm_lock_t lock; 
}page_t;



