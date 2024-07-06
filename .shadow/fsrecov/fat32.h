#include <stdint.h>

typedef uint8_t u8;
typedef uint16_t u16;
typedef uint32_t u32;

struct fat32hdr {
    u8  BS_jmpBoot[3];
    //jmpBoot[0] = 0xEB, jmpBoot[1] = 0x??, jmpBoot[2] = 0x90  
    //jmpBoot[0] = 0xE9, jmpBoot[1] = 0x??, jmpBoot[2] = 0x??
    u8  BS_OEMName[8];
    //什么系统格式化的该卷.
    u16 BPB_BytsPerSec;
    //每个扇区的字节数： 512, 1024, 2048 or 4096.
    u8  BPB_SecPerClus;
    //每个分配单元（簇）的扇区数：1, 2, 4, 8, 16, 32, 64, and 128. 
    u16 BPB_RsvdSecCnt;
    //保留区域的扇区数
    u8  BPB_NumFATs;
    //FAT表的数量
    u16 BPB_RootEntCnt;
    //根目录文件数的最大值，FAT32中为0
    u16 BPB_TotSec16;
    //16位扇区总数，FAT32中为0，需要用BPB_TotSec32
    u8  BPB_Media;
    //介质描述符
    u16 BPB_FATSz16;
    //每个FAT表的扇区数，FAT32中为0，需要用BPB_FATSz32
    u16 BPB_SecPerTrk;
    //每个磁道的扇区数，在调用中断0x13时，每磁道扇区数通常作为参数的一部分传递给中断服务例程
    u16 BPB_NumHeads;
    //磁头数，在调用中断0x13时，磁头数通常作为参数的一部分传递给中断服务例程
    u32 BPB_HiddSec;
    //隐藏扇区数，引导扇区之前的扇区数
    u32 BPB_TotSec32;
    //32位扇区总数
    u32 BPB_FATSz32;
    //FAT表的扇区数
    u16 BPB_ExtFlags;
    //活动FAT与镜像FAT的标志
    u16 BPB_FSVer;
    //文件系统版本，必须为0x0
    u32 BPB_RootClus;
    //根目录簇号，FAT32中为2
    u16 BPB_FSInfo;
    //文件系统信息扇区的扇区号，通常为1
    u16 BPB_BkBootSec;
    //备份引导扇区的扇区号，通常为6或0
    u8  BPB_Reserved[12];
    //保留，必须为0
    u8  BS_DrvNum;
    //中断13的驱动器号，0x80 或 0x00
    u8  BS_Reserved1;
    //保留，必须为0
    u8  BS_BootSig;
    //扩展引导标志，0x29
    u32 BS_VolID;
    //卷序列号
    u8  BS_VolLab[11];
    //卷标
    u8  BS_FilSysType[8];
    //文件系统类型，必须为FAT32
    u8  __padding_1[420];
    //填充，使引导扇区大小为512字节
    u16 Signature_word;
    //引导扇区结束标志，0x55，0xAA
} __attribute__((packed));

struct fat32dent {
    u8  DIR_Name[11];
    //目录名
    u8  DIR_Attr;
    //文件属性
    u8  DIR_NTRes;
    //保留，必须为0
    u8  DIR_CrtTimeTenth;
    //文件创建时间的组成部分。十分之一秒的计数。有效范围为：0 <= DIR_CrtTimeTenth <= 199
    u16 DIR_CrtTime;
    //创建时间，每个单位代表2秒
    u16 DIR_CrtDate;
    //创建日期
    u16 DIR_LastAccDate;
    //最后访问日期
    u16 DIR_FstClusHI;
    //高16位的簇号
    u16 DIR_WrtTime;
    //最后修改时间
    u16 DIR_WrtDate;
    //最后修改日期
    u16 DIR_FstClusLO;
    //低16位的簇号
    u32 DIR_FileSize;
    //文件大小
} __attribute__((packed));

struct fat32ldent {
    u8  LDIR_Ord;
    //长文件名目录项序号
    u16 LDIR_Name1[5];
    //长文件名第1部分
    u8  LDIR_Attr;
    //文件属性，必须为ATTR_LONG_NAME
    u8  LDIR_Type;
    //目录项类型，必须为0
    u8  LDIR_Chksum;
    //校验和
    u16 LDIR_Name2[6];
    //长文件名第2部分
    u16 LDIR_FstClusLO;
    //簇号的低16位
    u16 LDIR_Name3[2];
    //长文件名第3部分
} __attribute__((packed));

struct bmp_file_header{
    u16  bfType;//文件类型，必须为BM
    u32 bfSize;//文件大小
    u16 bfReserved1;//保留，必须设置为0
    u16 bfReserved2;//保留，必须设置为0
    u32 bfOffBits;//从头到位图数据的偏移
} __attribute__((packed));

struct bmp_info_header{
    u32 biSize;//信息头大小,40
    u32 biWidth;//图像宽度
    u32 biHeight;//图像高度
    u16 biPlanes;//颜色平面数，必须为1
    u16 biBitCount;//每个像素的位数
    u32 biCompression;//压缩类型
    u32 biSizeImage;//位图大小
    u32 biXPelsPerMeter;//水平分辨率
    u32 biYPelsPerMeter;//垂直分辨率
    u32 biClrUsed;//位图实际使用的颜色表中的颜色数
    u32 biClrImportant;//对图像显示有重要影响的颜色索引的数目
} __attribute__((packed));

#define CLUS_INVALID   0xffffff7
#define ATTR_READ_ONLY 0x01
#define ATTR_HIDDEN    0x02
#define ATTR_SYSTEM    0x04
#define ATTR_VOLUME_ID 0x08
#define ATTR_DIRECTORY 0x10
#define ATTR_ARCHIVE   0x20