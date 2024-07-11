#include<buddy.h>
//#include<assert.h>
#include<stdio.h>

//AC:4/6 Too slow 

//此部分实现有问题

//将信息头和数据区分开
//信息头用于存储信息，数据区用于存储数据
//数据区一页最小64KB

//如果buddy_header占用空间太大，试着见效下面两个参数


#define BUDDY_HEADER_SIZE (1<<21)
#define BUDDY_PAGE_SIZE (1<<16)

//__16MB才是真的16MB!!!!!!!!
#define __16MB 16777216
#define __64KB 65536

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


/*
void print_mem_tree(){
    for(int i=0;i<BUDDY_KINDS;i++){
    bpage* tmp=bhdr->free_nodes[i];
    printf("%d:",i);
    while(tmp!=NULL){
        
        uintptr_t offset=(uintptr_t)tmp-(uintptr_t)bhdr->pages;
        offset/=sizeof(bpage);

        uintptr_t num=buddy_start+offset*__64KB;

        //uintptr_t num=map2addr((uintptr_t)tmp);
        printf("->%x(%d)",num,tmp->size);
        tmp=tmp->next;
    }
    printf("\n");
    }
}
*/


void buddy_init(uintptr_t heap_start,uintptr_t heap_end){
    
    //初始化buddy_header
    bhdr=(buddy_header*)heap_start;
    memset(bhdr,0,sizeof(buddy_header));

    

    //使heap_start指向数据区、
    //printf("heap_start:%p\n",heap_start);
    heap_start+=sizeof(buddy_header);
    //printf("heap_start:%p\n",heap_start);
    //使heap_start对齐到16MB
    heap_start=(heap_start+__16MB-1)&~(__16MB-1);
    //使heap_end对齐到16MB//貌似没啥用？
    heap_end&=~(__16MB-1);

    //初始化buddy_start，指向数据区开始
    buddy_start=heap_start;

    //将16MB的头给更新好
    
    uintptr_t heap_size=(heap_end-heap_start);

    printf("heap_start:%p\n",heap_start);
    printf("heap_end:%p\n",heap_end);
    int cnt=heap_size/__16MB;
    //printf("cnt:%d\n",cnt);

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
    //assert(0);

    printf("Buddy ready\n");
    //print_mem_tree();
    
}

void* buddy_alloc(size_t size){

    if(size>__16MB){
        return NULL;
    }
    //必然有size<16MB
    //assert(size<=__16MB);
    //assert(size>=__64KB);

    size_t sz=__64KB;

    int index=0;
    while(sz<size){
        sz<<=1;
        index++;
    }

    //两种情况
    //空闲列表里刚好有
    if(bhdr->free_nodes[index]!=NULL){
        uintptr_t offset=(uintptr_t)bhdr->free_nodes[index]-(uintptr_t)bhdr->pages;
        offset/=sizeof(bpage);

        uintptr_t ret=buddy_start+offset*__64KB;

        //更新空闲链表
        bpage* node=bhdr->free_nodes[index];
        bhdr->free_nodes[index]=node->next;

        node->next=NULL;
        node->used=USED;

        //print_mem_tree();
        return (void*)ret;
    }
    else{//空闲列表里无
        uintptr_t ret=(uintptr_t)buddy_alloc(size<<1);
        //printf("size: %p , ret: %p\n",size,ret);
        if((void*)ret!=NULL){
            //分配了一块大的空间
            uintptr_t offset=ret-buddy_start;
            offset/=__64KB;

            bpage* node=(bpage*)((uintptr_t)bhdr->pages+offset*sizeof(bpage));
            node->size=index;
            node->next=NULL;
            node->used=USED;

            offset=ret+size-buddy_start;
            offset/=__64KB;

            node=(bpage*)((uintptr_t)bhdr->pages+offset*sizeof(bpage));
            node->size=index;
            node->next=NULL;
            node->used=FREE;
            
            bhdr->free_nodes[index]=node;
         
        }
        //print_mem_tree();
        return (void*)ret;
    }
    
    
}

void buddy_free(void* ptr){
    //TODO
    uintptr_t offset=(uintptr_t)ptr-buddy_start;
    offset/=__64KB;

    bpage* node1=(bpage*)((uintptr_t)bhdr->pages+offset*sizeof(bpage));
    node1->used=FREE;

    for(int i=node1->size;i<_16MB;i++){
        //注意到：i是相对于64KB的幂次差

        bpage* node2=NULL;
        printf("i:%d\n",i);
        if(i!=node1->size){
            printf("i:%d\n",i);
            while(1);
        }
        //assert(i==node1->size);


        if(offset%(1<<(i+1))==0){
            //说明来的是左边的
            node2=node1+(1<<i);
            if(node2->used==FREE && node2->size==i){
                //从空闲列表中删除node2
                bpage* freenode=bhdr->free_nodes[i];
                if(freenode==node2){
                    bhdr->free_nodes[i]=node2->next;
                }
                else{
                    while(freenode!=NULL){
                        if(freenode->next==node2){
                            freenode->next=node2->next;
                            break;
                        }
                    }
                }
                //合并
                //清空node2的头
                node2->next=NULL;
                node2->size=0;

                node1->size++;

                node1->next=bhdr->free_nodes[node1->size];

                bhdr->free_nodes[node1->size]=node1;

            }
            else{
                break;
            }
           
        }
        else{
            //说明来的是右边的
            bpage* node2=node1-(1<<i);
            if(node2->used==FREE && node2->size==i){
                //从空闲列表中删除node2
                bpage* freenode=bhdr->free_nodes[i];
                if(freenode==node2){
                    bhdr->free_nodes[i]=node2->next;
                }
                else{
                    while(freenode!=NULL){
                        if(freenode->next==node2){
                            freenode->next=node2->next;
                            break;
                        }
                    }
                }
                //合并
                //清空node1的头
                node1->next=NULL;
                node1->size=0;

                node2->size++;

                node2->next=bhdr->free_nodes[node2->size];

                bhdr->free_nodes[node2->size]=node2;
                node1=node2;

            }
            else{
                break;
            }
        }
    }

}