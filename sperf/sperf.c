
#include <time.h>
#include <stdio.h>
#include <regex.h>
#include <assert.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <sys/wait.h>
#include <sys/time.h>


extern char **environ;
//#define DEBUG 0

//todo:处理数据

typedef struct syscall_{
    double time;
    char* name;
    struct syscall_* next;
}sys_;


sys_* renew_list(sys_* h,sys_* p){
    
    assert(p!=NULL);
    if (h == NULL) {
        return p;
    }

    if (h->time<p->time){
        p->next=h;
        return p;
    }
    
    sys_ *pre = h;
    sys_ *cur = h->next;
    while(cur){
        if(cur->time<p->time)
            break;
        pre=cur;
        cur=cur->next;
    }
    assert(pre->next==cur);
    p->next = cur;
    pre->next = p;
    
    return h;
    
}


int main(int argc, char *argv[]) {
    /*创建匿名管道，子进程写，父进程读*/
    int pipefd[2]={0};
    
    //创建失败返回-1
    assert(pipe(pipefd) != -1);
    //创建子进程
    pid_t pid = fork();
    //创建失败返回-1
    assert(pid != -1);
    
    if (pid == 0) {
        //子进程   //执行strace命令

        //assert(close(1)!=-1);
        //关闭读端
        assert(close(pipefd[0])!=-1);
        //将标准错误输出重定向到管道的写端
        assert(dup2(pipefd[1], STDERR_FILENO)!=-1);
       
        //参考jyy,手工构建argv
        char* exec_argv[argc+2];
        //3
        //0 1 2
        //strace -T 
        //./
        //
        exec_argv[0] = "strace";
        exec_argv[1] = "-T";
        for (int i = 1; i < argc; i++) {
            exec_argv[i+1] = argv[i];
        }
        //argv和envp中间隔一个“NULL”
        exec_argv[argc+1] = NULL;
       
        
        //绝对路径处理
        execve("/strace", exec_argv, environ);
        execve("/bin/strace", exec_argv, environ);
        execve("/usr/bin/strace", exec_argv, environ);
        perror("execvp");
         //关闭写端
        assert(close(pipefd[1])!=-1);
        exit(EXIT_FAILURE);
    } else {
        //父进程    //读strace的输出并处理
        //关闭写端
        //assert(close(0)!=-1);
        assert(close(pipefd[1])!=-1);

        //printf("s\n");

        char buf[4096];
        sys_ *head=NULL;

        FILE *fp = fdopen(pipefd[0], "r");
        assert(fp);

        //总时间
        double total_time = 0;

        struct timeval last_output_time;
        struct timeval current_time;

        gettimeofday(&last_output_time, NULL); // 获取当前时间
        //int run_flag=1;
        int print_flag=0;
        //while(run_flag==1){
            //printf("ss\n");
            while (fgets(buf, 4096, fp)!=0) 
            {
                //printf("s\n");
                //printf("aaaaa-----%s", buf);
                /* if (strstr(buf, "+++ exited with 0 +++") != NULL ||
                    strstr(buf, "+++ exited with 1 +++") != NULL) {
                run_flag = 0;
                break;
                }*/
                regex_t regex_name,regex_time;
                regmatch_t match_name,match_time;

                const char pattern_name[] = "[^\\(\n\r\b\t]*\\(";
                const char pattern_time[] = "<[0-9\\.]+>";

                assert(regcomp(&regex_name, pattern_name, REG_EXTENDED)==0);
                assert(regcomp(&regex_time, pattern_time, REG_EXTENDED)==0);
                
                //执行正则匹配
                int ret_name = regexec(&regex_name, buf, 1, &match_name, 0);
                int ret_time = regexec(&regex_time, buf, 1, &match_time, 0);
                if (ret_name == 0 && ret_time == 0) {
                    //提取匹配的系统调用名和时间
                    char* syscall=malloc(sizeof(char)*(match_name.rm_eo - match_name.rm_so));
                    char time[64];
                    snprintf(syscall, match_name.rm_eo - match_name.rm_so , "%s", buf + match_name.rm_so);
                    snprintf(time, match_time.rm_eo - match_time.rm_so -1 , "%s", buf + match_time.rm_so + 1);
                    double t = atof(time);
                    total_time=total_time+t;
                    //调试信息
                    #ifdef DEBUG
                    printf("Syscall: %s, Time: %s %f\n", syscall, time, t);
                    #endif
                    //遍历链表，能找到就更新时间，否则插入新的节点
                    //assert(0);
                    sys_* p=head;
                    sys_* pre=NULL;
                    while(p){
                        if(strcmp(p->name,syscall)==0)break;
                        pre=p;
                        p=p->next;
                    }
                    if(p){
                        //找到了
                        p->time=p->time+t;
                        if(pre){
                            pre->next=p->next;
                        }
                        else{
                            head=p->next;
                        }
                        free(syscall);
                        //更新链表
                    }
                    else{
                        sys_* np=(sys_*)malloc(sizeof(sys_));
                        np->name=syscall;
                        np->time=t;
                        p=np;
                    }
                    //assert(0);
                    head=renew_list(head, p);

                }
                gettimeofday(&current_time, NULL); // 获取当前时间

                // 如果距离上次输出已经过了0.1秒
                
                long long elapsed = (current_time.tv_sec - last_output_time.tv_sec) * 1000000LL + (current_time.tv_usec - last_output_time.tv_usec);
                if (elapsed >= 100000LL) {
                    sys_* p=head;
                    for(int i=0;i<5;i++){
                        //printf("s\n");
                        if(!p)break;
                        //assert(0);
                        printf("%s (%0.0f%%)\n", p->name, p->time/total_time*100);
                        p=p->next;
                        
                    }
                    fflush(stdout);
                    char null_char = '\0';
                    for(int i=0;i<80;i++){
                        fwrite(&null_char, sizeof(char), 1, stdout);
                    }
                    //assert(0);
                print_flag=1;
                last_output_time = current_time;
                
                }
            
            }
            
        //}
        if(!print_flag){
                sys_* p=head;
                    for(int i=0;i<5;i++){
                        //printf("s\n");
                        if(!p)break;
                        //assert(0);
                        printf("%s (%0.0f%%)\n", p->name, p->time/total_time*100);
                        p=p->next;
                        
                    }
                    fflush(stdout);
                    char null_char = '\0';
                    for(int i=0;i<80;i++){
                        fwrite(&null_char, sizeof(char), 1, stdout);
                    }
            }
    }
    return 0;
}

