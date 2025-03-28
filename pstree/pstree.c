#include <string.h>
#include <dirent.h>
#include <stdio.h>
#include <assert.h>
#include <unistd.h>
#include <getopt.h>
#include <sys/types.h>
#include <stdlib.h>
//#include<stdbool.h>

typedef struct proc {

  pid_t pid;
  char name[1024];
  pid_t* child;
  int child_cnt;
  pid_t ppid;
  
} proc;

int rule(const void* a,const void* b){
  return strcmp(((proc*)a)->name,((proc*)b)->name);
}

const struct option opt_table[]={
  {"numeric-sort",no_argument,NULL,'n'},
  {"show-pids",no_argument,NULL,'p'},
  {"version",no_argument,NULL,'V'},
  {0,0,NULL,0},
};

int nf;//nflag
int pf;//pflag
int vf;//vflag

int opt;


int pid_max;

const char* base_path="/proc";
DIR* dir;

void MY_OUT_PUT(proc* p,proc* procs,int d,int pf){
  if(d>0){
    int len=(d-1)*4;
    printf("\n%*s |\n",len,"");
    //printf("\n");
    printf("%*s",len,"");
    printf(" +--");
  }
  printf("%s",p->name);
  if(pf){
    printf("(%d)",p->pid);
  }

  for(int i=0;i<p->child_cnt;i++){
    proc* maybe_child=procs;
    while(maybe_child!=NULL&&(maybe_child->pid!=p->child[i])){
      maybe_child++;
    }

    MY_OUT_PUT(maybe_child,procs,d+1,pf);
  }

}


int main(int argc, char *argv[]) {

 
  while((opt=getopt_long(argc,argv,"-npV",opt_table,NULL))!=-1){
    switch (opt)
    {
    case 'n':nf=1;break;
    case 'p':pf=1;break;
    case 'V':vf=1;break;
    case 0:break;
    default:
      printf("pstree: invalid option -- '%s'\n", argv[optind-1]);
      return 1;
    }
  }

  if(vf){
    fprintf(stderr,"pstree from lhj-221240073\n");
    return 0;
  }

  FILE* f=fopen("/proc/sys/kernel/pid_max","r");
  if(f==NULL){
    printf("pstree: cannot open file /proc/sys/kernel/pid_max: No such file or directory\n");
    return -1;
  }
  fscanf(f,"%d",&pid_max);
  fclose(f);

  //打开目录
  
  if((dir=opendir(base_path))==NULL){
    printf("pstree: cannot open directory /proc: No such file or directory\n");
    return -1;
  }


  //保存
  proc* procs=malloc(sizeof(proc)*(pid_max+1));//进程数组
  //pid_t* ppids=malloc(sizeof(pid_t)*pid_max);//父进程的pid

  struct dirent* ent;
  proc* p=procs;//当前进程指针

  while((ent=readdir(dir))!=NULL){
      //筛选出为数字的目录
      if(ent->d_name[0]>='0'&&ent->d_name[0]<='9'){
        p->pid=atoi(ent->d_name);
        //p->name=ent->d_name;
        
        //todo:

        char* path;
        int len=strlen(base_path)+strlen(ent->d_name)+7+2;
        path=malloc(len);
        memset(path,0,len);

        strcat(path,base_path);
        strcat(path,"/");
        strcat(path,ent->d_name);
        strcat(path,"/status");

        //printf("%s\n",path);

        FILE* fp=fopen(path,"r");

        if(fp==NULL){
          printf("pstree: cannot open file %s: No such file or directory\n",path);
          return -1;
        }

        char buf[1024];

        while(fscanf(fp,"%s",buf)!=EOF){
          if(strcmp(buf,"Name:")==0){
            fscanf(fp,"%s",p->name);
            //printf("%s\n",p->name);
          }
          if(strcmp(buf,"PPid:")==0){
            fscanf(fp,"%d",&procs[p-procs].ppid);
            //printf("%d\n",ppids[p-procs]);
          }
        }
        fclose(fp);
        //free(path);
        p++;
    }
    
  }
  

  int cnt=p-procs;
  //printf("%d\n",cnt);

  if(!nf){
    qsort(procs,cnt,sizeof(proc),rule);
  }

  
  for(int i=0;i<cnt;i++){
    procs[i].child=malloc(sizeof(pid_t)*cnt);
    procs[i].child_cnt=0;
  }


  //建树
  for(int i=0;i<cnt;i++){
    for(int j=0;j<cnt;j++){
      if(procs[i].pid==procs[j].ppid){
        int num=procs[i].child_cnt;
        procs[i].child[num]=procs[j].pid;
        procs[i].child_cnt++;
        //printf("ljy\n");
      }
    }
  }

  
  for(int i=0;i<cnt;i++){
    if(procs[i].ppid==0&&procs[i].pid==1){MY_OUT_PUT(&procs[i],procs,0,pf);break;}
  }
  
  //printf("nf=%d, pf=%d, vf=%d\n", nf, pf, vf);
  printf("\n");
  return 0;
}




  //for (int i = 0; i < argc; i++) {
  //  assert(argv[i]);
  //  printf("argv[%d] = %s\n", i, argv[i]);
  //}
  //assert(!argv[argc]);