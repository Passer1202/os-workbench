#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>
#include <fcntl.h>
#include <unistd.h>
#include <sys/mman.h>
#include <sys/stat.h>
#include <pthread.h>
#include "fat32.h"






/*
// 假设这些全局变量和数据结构已被定义和初始化
u8 *clus_type;
u8 *data_start;
u8 *bmp_current;
u32 min_rgb;
int pk;
void *next_clu;
int no;



void *thread_func(void *arg) {
    int *range = (int *)arg;
    process_range(range[0], range[1]);
    return NULL;
}

int main() {
    pthread_t thread1, thread2;
    int range1[2] = {2, CLUS_INDEX / 2};
    int range2[2] = {CLUS_INDEX / 2, CLUS_INDEX};

    

    pthread_create(&thread1, NULL, thread_func, range1);
    pthread_create(&thread2, NULL, thread_func, range2);

    pthread_join(thread1, NULL);
    pthread_join(thread2, NULL);

    pthread_mutex_destroy(&mutex);

    // 打印或使用结果
    printf("Min RGB: %u, Next Cluster: %p, No: %d\n", min_rgb, next_clu, no);

    return 0;
}
*/

//#define EASY 0




enum CLUS_CLASS{
    CLUS_DENT=0,
    CLUS_UNUSE,
    CLUS_BMP_DATA,
    CLUS_BMP_HEAD,
    CLUS_OTHER,
};

#define CLUS_CNT 100000
#define CLUS_SZ 8192

int clus_type[CLUS_CNT];

int cluses[CLUS_SZ];


struct fat32hdr *hdr;

#define SEC_SIZE(hdr) ((hdr)->BPB_BytsPerSec) //扇区字节数
#define SEC_CNT(hdr) ((hdr)->BPB_TotSec32)    //扇区总数

#define RSVD_SEC_CNT(hdr) ((hdr)->BPB_RsvdSecCnt) //保留扇区数

#define FAT_CNT(hdr) ((hdr)->BPB_NumFATs)     //FAT表数量
#define FAT_SEC_CNT(hdr) ((hdr)->BPB_FATSz32)    //FAT表所占扇区数

#define CLUS_SEC_CNT(hdr) ((hdr)->BPB_SecPerClus) //簇所占扇区数
#define CLUS_SIZE(hdr) ((hdr)->BPB_BytsPerSec * (hdr)->BPB_SecPerClus)//簇字节数

#define IMG_OFFSET(hdr) ((hdr)->bfOffBits)//图像数据偏移量
#define IMG_SIZE(hdr) ((hdr)->biSizeImage)//图像数据大小

#define BMP_SIZE(hdr) ((hdr)->bfSize)//bmp文件大小

#define REST_SIZE(hdr) ((CLUS_SIZE(hdr)-sizeof(struct bmp_file_header)-sizeof(struct bmp_info_header)))//簇除去两个头文后剩余空间


struct poss{
    int start;
    int end;
    u32 min_rgb;
    uintptr_t data_start;
    uintptr_t bmp_current;
    struct fat32hdr* hdr;
    u32 pk;
    u32 bmp_wid;
    u32 bmp_row;
    u8 *next_clu;
    uintptr_t no;
};

void *mmap_disk(const char *fname);

void process_range(struct poss* x);

int main(int argc, char *argv[]) {

    if(argc < 2){
        fprintf(stderr, "Usage: %s <disk image>\n", argv[0]);
        exit(1);
    }
    setbuf(stdout, NULL);

    //pthread_mutex_init(&mutex, NULL);

    assert(sizeof(struct fat32hdr) == 512);
    assert(sizeof(struct fat32dent) == 32);
    assert(sizeof(struct fat32ldent) == 32);

    hdr=mmap_disk(argv[1]);

    const char dirpath[]="/tmp/DICM/";

    if(access(dirpath,0)==-1)
        assert(mkdir(dirpath,0755)!=-1);

    //TODO: fsrecov

    //0. get the data area

    //data区开始的扇区号
    u32 data_start_sec = RSVD_SEC_CNT(hdr) + FAT_CNT(hdr) * FAT_SEC_CNT(hdr);
    //data区的簇总数
    u32 data_clu_cnt = (SEC_CNT(hdr) - data_start_sec)/CLUS_SEC_CNT(hdr);
    //data区起始地址
    uintptr_t data_start = (uintptr_t)hdr + data_start_sec * SEC_SIZE(hdr);
    //data区结束地址
    uintptr_t data_end = (uintptr_t)hdr + SEC_CNT(hdr) * SEC_SIZE(hdr);


    



    //先分类每个簇
    int clus_index=2;

    for(int i=0;i<data_clu_cnt;i++){
        //遍历data区的每个簇
        uintptr_t pc = data_start + i * CLUS_SIZE(hdr);//当前簇的起始地址指针
        memcpy(cluses, (void *)pc, CLUS_SIZE(hdr));
        struct bmp_file_header *bmp_hdr = (struct bmp_file_header *)(pc);

        if(bmp_hdr->bfType == 0x4d42){
            //clus_type[clus_index]=CLUS_BMP_HEAD;
             clus_type[clus_index]=CLUS_BMP_HEAD;
            //assert(0);
        }
        else{
            
            int unuse_flag=0;
            for(int i=0;i<CLUS_SIZE(hdr)-2;i++){
                if(cluses[i]==0x00){
                    unuse_flag++;
                }
                else{
                    break;
                
                }
            }
            if(unuse_flag==CLUS_SIZE(hdr)-2){
            clus_type[clus_index]=CLUS_UNUSE;
            }
            else{
                clus_type[clus_index]=CLUS_BMP_DATA;

                for(int j=0;j<(CLUS_SIZE(hdr)/sizeof(struct fat32dent));j++){
                //遍历当前簇的每个目录项
                    struct fat32dent *pd=(struct fat32dent *)(pc+j*sizeof(struct fat32dent));//当前目录项的指针
                    //判断是否是短目录项（.BMP)
                    if(pd->DIR_Name[8]=='B' && pd->DIR_Name[9]=='M' && pd->DIR_Name[10]=='P'){
                        if(pd->DIR_NTRes==0){
                            clus_type[clus_index]=CLUS_DENT;
                            break;
                        }
                    }
                }
            }

        }
        clus_index++;
    }

    //assert(0);
    //1. find the (short) directory entry
    for(int i=0;i<data_clu_cnt;i++){
        //遍历data区的每个簇
        //if(clus_type[i]!=CLUS_DENT) continue;
        uintptr_t pc = data_start + i * CLUS_SIZE(hdr);//当前簇的起始地址指针
        for(int j=0;j<(CLUS_SIZE(hdr)/sizeof(struct fat32dent));j++){
            //遍历当前簇的每个目录项
            struct fat32dent *pd=(struct fat32dent *)(pc+j*sizeof(struct fat32dent));//当前目录项的指针
            //判断是否是短目录项（.BMP)
            if(pd->DIR_Name[8]=='B' && pd->DIR_Name[9]=='M' && pd->DIR_Name[10]=='P'){
                if(pd->DIR_Name[0]!=0xe5 && pd->DIR_FileSize!=0) {//不是被删除的文件
                    //恢复文件名到name,得到.bmp文件的起始簇号bmp_clu1st
                    
                    int index_name =0;
                    char name[256];
                    memset(name,0, 256);

                    //2.find the first cluster of .bmp
                    u32 bmp_clu1st = ((u32)pd->DIR_FstClusLO | ((u32)(pd->DIR_FstClusHI) << 16))-2;//起始簇号，-2由于簇号从2开始
                    //printf("bmp_clu1st: %d\n", bmp_clu1st);
                    //assert(0);
                    struct bmp_file_header *bmp_hdr = (struct bmp_file_header *)(data_start + (bmp_clu1st * CLUS_SIZE(hdr)));
                    //if(bmp_hdr->bfType != 0x4d42) continue;
                    {//确定是bmp文件
                            //3.find long directory entry
                            //手册：长目录项倒着紧放在短目录项前面
                            
                        uintptr_t pl = (uintptr_t)pd;
                        while(pl>data_start && pl<data_end){
                            pl -= sizeof(struct fat32ldent);
                            struct fat32ldent *pld = (struct fat32ldent *)pl;
                            if(pld->LDIR_Attr == ATTR_LONG_NAME && pld->LDIR_Type == 0 && pld->LDIR_FstClusLO == 0){//长目录项
                                for(int r=0;r<5;r++){
                                    if(pld->LDIR_Name1[r] != 0xffff){
                                        name[index_name++] = pld->LDIR_Name1[r];
                                    }
                                }
                                for(int r=0;r<6;r++){
                                    if(pld->LDIR_Name2[r] != 0xffff){
                                        name[index_name++] = pld->LDIR_Name2[r];
                                    }
                                }
                                for(int r=0;r<2;r++){
                                    if(pld->LDIR_Name3[r] != 0xffff){
                                        name[index_name++] = pld->LDIR_Name3[r];
                                    }
                                }

                            }else{
                                break;
                            }
                        }
                        if(index_name==0){
                            //short name
                            for(int r=0;r<8;r++){
                                if(pd->DIR_Name[r] != ' '){
                                    name[index_name++] = pd->DIR_Name[r];
                                }
                            }
                            name[index_name++] = '.';
                            for(int r=0;r<3;r++){
                                if(pd->DIR_Name[8+r] != ' '){
                                    name[index_name++] = pd->DIR_Name[8+r];
                                }
                            }
                        }

                            //printf("recovering %s\n", name);
                           //fflush(stdout);

                            
                    }

                    
                    //恢复文件

                    //assert(0);
                    char tmp_path[256]="/tmp/DICM/";
                    strcat(tmp_path, name);
                    remove(tmp_path);//删除文件若已有，避免出现同名文件
                    FILE *bmp_tmp_file = fopen(tmp_path, "a");
                    assert(bmp_tmp_file != NULL);
                    //写入bmp文件头
                    uintptr_t bmp_current = (uintptr_t)bmp_hdr;
                    

                    if(bmp_current>=data_end){
                        printf("d60e7d3d2b47d19418af5b0ba52406b86ec6ef83 %s\n",name);
                        continue;
                    }
                    assert(bmp_hdr->bfType == 0x4d42);//确定是bmp文件
                    
                    //assert(bmp_ihdr->biSize == 40);//信息头大小为40

                    int bmp_sz=BMP_SIZE(bmp_hdr);
                    
                    if(bmp_sz<=CLUS_SIZE(hdr)){
                        //assert(0);
                       fwrite((void *)bmp_current, bmp_sz, 1, bmp_tmp_file);

                    }else{
                        //该文件占了多个簇
                        //continue;
                        //printf("rest size: %d\n", (int)REST_SIZE(hdr));
                        struct bmp_info_header *bmp_ihdr = (struct bmp_info_header *)(bmp_hdr + 1);

                        assert(bmp_ihdr->biSize == 40);
                        
                        u32 bmp_wid=3*(bmp_ihdr->biWidth);
                        u32 bmp_row=(8*bmp_wid+31)/32*4;

                        int pk_flag=0;

                        //assert(0);
                        u32 pk=0;
                        while(bmp_sz >= CLUS_SIZE(hdr)){
                            //printf("name: %s\n", name);
                            //printf("img_sz: %d\n", img_s

                            if(bmp_current>=data_end){//&&(void *)bmp_current!=NULL){
                                break;
                            }

                            //assert(0);
                            fwrite((void *)bmp_current, CLUS_SIZE(hdr), 1, bmp_tmp_file);
                            
                            if(pk_flag==0){
                                pk+=(CLUS_SIZE(hdr)-bmp_hdr->bfOffBits);
                                pk%=bmp_row;
                                
                            }
                            else{
                               pk+=CLUS_SIZE(hdr);
                               pk%=bmp_row;
                            }

                            pk_flag=1;


                            u8* next_clu= (u8*)bmp_current +  CLUS_SIZE(hdr);

                            int next_flag=0;
                            if((uintptr_t)next_clu>=data_end){
                                next_flag=1;
                            }

                            u32 min_rgb=0;
                            uintptr_t no=(uintptr_t)next_clu-data_start;
                            no/=CLUS_SIZE(hdr);
                            no+=2;

                            if(next_flag==1){
                                min_rgb=0xffffffff;
                            }
                            //(struct bmp_file_header *)(data_start + (bmp_clu1st * CLUS_SIZE(hdr)))
                            else{
                                for(int k=0;k<bmp_row;k++){
                                    u8* rgb1=next_clu+k;
                                    u8* rgb2=(u8*)bmp_current+CLUS_SIZE(hdr)-bmp_row+k;
                                    if(*rgb1>*rgb2){
                                        min_rgb=min_rgb+(*rgb1-*rgb2);
                                    }
                                    else{
                                        min_rgb=min_rgb+(*rgb2-*rgb1);
                                    }
                                }
                            }
                            
                            

                            // printf("index: %d\n",clus_index);
                            //创建两个线程，一个线程跑一半的循环
                          /*
                          
                        struct poss{
                            int start;
                            int end;
                            u32 min_rgb;
                            uintptr_t data_start;
                            uintptr_t bmp_current;
                            struct fat32hdr* hdr;
                            u32 pk;
                            u32 bmp_wid;
                            u32 bmp_row;
                            u8 *next_clu;
                            uintptr_t no;
                        };

                          */
                            struct poss x1;
                            x1.start=2;
                            x1.end=clus_index/2;
                            x1.min_rgb=min_rgb;
                            x1.data_start=data_start;
                            x1.bmp_current=(uintptr_t)bmp_current;
                            x1.hdr=hdr;
                            x1.pk=pk;
                            x1.bmp_wid=bmp_wid;
                            x1.bmp_row=bmp_row;
                            x1.next_clu=next_clu;
                            x1.no=no;


                            struct poss x2;
                            x2.start=clus_index/2;
                            x2.end=clus_index;
                            x2.min_rgb=min_rgb;
                            x2.data_start=data_start;
                            x2.bmp_current=(uintptr_t)bmp_current;
                            x2.hdr=hdr;
                            x2.pk=pk;
                            x2.bmp_wid=bmp_wid;
                            x2.bmp_row=bmp_row;
                            x2.next_clu=next_clu;
                            x2.no=no;

                            pthread_t thread1, thread2;

                            pthread_create(&thread1, NULL, (void*)process_range, &x1);

                            pthread_create(&thread2, NULL, (void*)process_range, &x2);

                            pthread_join(thread1, NULL);

                            pthread_join(thread2, NULL);

                            if(x1.min_rgb<x2.min_rgb){
                                min_rgb=x1.min_rgb;
                                next_clu=x1.next_clu;
                                no=x1.no;
                            }
                            else{
                                min_rgb=x2.min_rgb;
                                next_clu=x2.next_clu;
                                no=x2.no;
                            }

                            clus_type[no]=CLUS_OTHER;

                            bmp_current = (uintptr_t)next_clu;
                            bmp_sz -= CLUS_SIZE(hdr);
                             
                        }
                        
                        if((void *)bmp_current!=NULL &&bmp_current<data_end&&bmp_sz > 0){
                            fwrite((void *)bmp_current, bmp_sz, 1, bmp_tmp_file);
                            
                        }
                    }
                    fclose(bmp_tmp_file);

                    
                    
                    //计算文件的sha1值
                    char cmd[256];
                    memset(cmd, 0, 256);
                    strcpy(cmd, "sha1sum ");
                    strcat(cmd, tmp_path);

                    char buf[64];
                    memset(buf, 0, 64);

                    //from jyy
                    FILE* fp = popen(cmd , "r");

                    assert(fp>0);

                    fscanf(fp, "%s", buf); // Get it!
                    pclose(fp);
                    
                    if(buf[0]=='\0')
                    printf("d60e7d3d2b47d19418af5b0ba52406b86ec6ef83 %s\n",name);
                    else printf("%s %s\n",buf,name);

                    fflush(stdout);

                }
            }
        }
    }


    //unmap disk
    munmap(hdr, hdr->BPB_TotSec32 * hdr->BPB_BytsPerSec);

}

void process_range(struct poss* x) {

    for (int z = x->start; z < x->end; z++) {
        if (clus_type[z] == CLUS_BMP_DATA) {
            u32 tmp_min_rgb = 0;
            u8 *tmp_clu = (u8 *)(x->data_start + (z - 2) * CLUS_SIZE(hdr));

            if ((x->pk < x->bmp_wid) && (*(tmp_clu + x->bmp_wid - x->pk) != 0)) {
                continue;
            }
            if ((x->pk >= x->bmp_wid) && (*(tmp_clu) != 0)) {
                continue;
            }

            for (int k = 0; k < x->bmp_row; k++) {
                u8 *rgb1 = (u8 *)tmp_clu + k;
                u8 *rgb2 = (u8 *)x->bmp_current + CLUS_SIZE(hdr) - x->bmp_row + k;
                if (*rgb1 > *rgb2) {
                    tmp_min_rgb += (*rgb1 - *rgb2);
                } else {
                    tmp_min_rgb += (*rgb2 - *rgb1);
                }
                if (tmp_min_rgb > x->min_rgb) {
                    break;
                }
            }

            //pthread_mutex_lock(&mutex);
            if (tmp_min_rgb < x->min_rgb) {
                x->min_rgb = tmp_min_rgb;
                x->next_clu = (void *)tmp_clu;
                x->no = z;
            }
            //pthread_mutex_unlock(&mutex);
        }
    }
}

//referenced from jyy
void *mmap_disk(const char *fname) {
    int fd = open(fname, O_RDWR);

    if (fd < 0) {
        perror("open disk");
        goto release;
    }

    off_t size = lseek(fd, 0, SEEK_END);
    if (size < 0) {
        perror("lseek disk");
        goto release;
    }

    struct fat32hdr *hdr = mmap(NULL, size, PROT_READ | PROT_WRITE, MAP_PRIVATE, fd, 0);
    if (hdr == MAP_FAILED) {
        goto release;
    }

    close(fd);

    assert(hdr->Signature_word == 0xaa55); // this is an MBR
    assert(hdr->BPB_TotSec32 * hdr->BPB_BytsPerSec == size);

    return hdr;

release:
    perror("map disk");
    if (fd > 0) {
        close(fd);
    }
    exit(1);
}
