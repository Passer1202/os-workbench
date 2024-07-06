#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>
#include <fcntl.h>
#include <unistd.h>
#include <sys/mman.h>
#include <sys/stat.h>
#include "fat32.h"


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

#define REST_SIZE(hdr) ((CLUS_SIZE(hdr)-sizeof(struct bmp_file_header)-sizeof(struct bmp_info_header)))//簇除去两个头文后剩余空间

void *mmap_disk(const char *fname);

int main(int argc, char *argv[]) {

    if(argc < 2){
        fprintf(stderr, "Usage: %s <disk image>\n", argv[0]);
        exit(1);
    }

    setbuf(stdout, NULL);

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


    //1. find the (short) directory entry
    for(int i=0;i<data_clu_cnt;i++){
        //遍历data区的每个簇
        uintptr_t pc = data_start + i * CLUS_SIZE(hdr);//当前簇的起始地址指针
        for(int j=0;j<(CLUS_SIZE(hdr)/sizeof(struct fat32dent));j++){
            //遍历当前簇的每个目录项
            struct fat32dent *pd=(struct fat32dent *)(pc+j*sizeof(struct fat32dent));//当前目录项的指针
            //判断是否是短目录项（.BMP)
            if(pd->DIR_Name[8]=='B' && pd->DIR_Name[9]=='M' && pd->DIR_Name[10]=='P'){
                if(pd->DIR_Name[0]!=0xe5 || pd->DIR_FileSize!=0) {//不是被删除的文件
                    //恢复文件名到name,得到.bmp文件的起始簇号bmp_clu1st
                    
                    int index_name =0;
                    char name[256];
                    memset(name,0, 256);

                    //2.find the first cluster of .bmp
                    u32 bmp_clu1st = ((u32)pd->DIR_FstClusLO | ((u32)(pd->DIR_FstClusHI) << 16))-2;//起始簇号，-2由于簇号从2开始
                    struct bmp_file_header *bmp_hdr = (struct bmp_file_header *)(data_start + (bmp_clu1st * CLUS_SIZE(hdr)));
                    if(bmp_hdr->bfType == 0x4d42){//确定是bmp文件
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
                    char tmp_path[256]="/tmp/DICM/";
                    strcat(tmp_path, name);
                    remove(tmp_path);//删除文件若已有，避免出现同名文件
                    FILE *bmp_tmp_file = fopen(tmp_path, "a");
                    assert(bmp_tmp_file != NULL);
                    //写入bmp文件头
                    fwrite(bmp_hdr, sizeof(struct bmp_file_header), 1, bmp_tmp_file);
                    //写入bmp信息头
                    struct bmp_info_header *bmp_ihdr = (struct bmp_info_header *)(bmp_hdr + 1);
                    fwrite(bmp_ihdr, sizeof(struct bmp_info_header), 1, bmp_tmp_file);
                    //写入bmp图像数据
                    uintptr_t img_start = (uintptr_t)bmp_hdr + IMG_OFFSET(bmp_hdr);

                    if(IMG_SIZE(bmp_ihdr)<=REST_SIZE(hdr)){
                        assert(0);
                       fwrite((void *)img_start, IMG_SIZE(bmp_ihdr), 1, bmp_tmp_file);
                    }else{
                        //该文件占了多个簇
                        fwrite((void *)img_start, REST_SIZE(hdr), 1, bmp_tmp_file);

                        u32 img_sz = IMG_SIZE(bmp_ihdr) - REST_SIZE(hdr);
                        uintptr_t img_current = img_start + REST_SIZE(hdr);
                        while(img_sz >= CLUS_SIZE(hdr)){
                            fwrite((void *)img_current, CLUS_SIZE(hdr), 1, bmp_tmp_file);
                            img_current += CLUS_SIZE(hdr);
                            img_sz -= CLUS_SIZE(hdr);
                        }
                        if(img_sz > 0){
                            fwrite((void *)img_current, img_sz, 1, bmp_tmp_file);
                        }
                    }
                    fclose(bmp_tmp_file);


                    //计算文件的sha1值
                    char cmd[256];
                    memset(cmd, 0, 256);
                    strcpy(cmd, "sha1sum ");
                    strcat(cmd, tmp_path);

                    char buf[40];
                    memset(buf, 0, 40);

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
