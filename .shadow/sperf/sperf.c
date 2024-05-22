#include <stdio.h>
#include <regex.h>
#include <assert.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <sys/wait.h>

//#define DEBUG 0

void extract_syscall_times(const char *strace_output) {
    


    
}


#ifndef DEBUG
int main(int argc, char *argv[]) {
    /*创建匿名管道，子进程写，父进程读*/
    int pipefd[2];
    
    //创建失败返回-1
    assert(pipe(pipefd) != -1);
    //创建子进程
    pid_t pid = fork();
    //创建失败返回-1
    assert(pid != -1);

    if (pid == 0) {
        //子进程   //执行strace命令
        //关闭读端
        assert(close(1)!=-1);
        assert(close(pipefd[0])!=-1);
        //将标准输出重定向到管道的写端
        assert(dup2(pipefd[1], STDERR_FILENO)!=-1);
       
        //参考jyy,手工构建argv
        char* exec_argv[argc+2];
        exec_argv[0] = "strace";
        exec_argv[1] = "-T";
        for (int i = 1; i < argc; i++) {
            exec_argv[i+1] = argv[i];
        }
        //argv和envp中间隔一个“NULL”
        exec_argv[argc+1] = NULL;
        char *exec_envp[] = { "PATH=/bin", NULL, };

        //绝对路径处理
        execve("/usr/bin/strace", exec_argv, exec_envp);
        perror("execvp");
         //关闭写端
        assert(close(pipefd[1])!=-1);
        exit(EXIT_FAILURE);
    } else {
        //父进程    //读strace的输出并处理
        //关闭写端
        assert(close(pipefd[1])!=-1);
        char buf[4096];
        FILE *fp = fdopen(pipefd[0], "r");
        assert(fp);

        while(1){
            fflush(stdout);

            if (!fgets(buf, sizeof(buf), fp)) {
                break;
            }
            //目前看父进程会结束，先这样写
            
            //正则表达式
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
                char syscall[64];
                char time[64];
                snprintf(syscall, match_name.rm_eo - match_name.rm_so , "%s", buf + match_name.rm_so);
                snprintf(time, match_time.rm_eo - match_time.rm_so + 1, "%s", buf + match_time.rm_so);
                printf("Syscall: %s, Time: %s\n", syscall, time);
            }

            //输出是一行行来的
        }
    }
    return 0;
}
#else
int main(int argc, char *argv[]) {

    
    for (int i = 0; i < argc; i++) {
        assert(argv[i]);
        printf("argv[%d] = %s\n", i, argv[i]);
    }
    assert(!argv[argc]);
    return 0;
}
#endif
