#include <stdio.h>
#include <assert.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <sys/wait.h>

//#define DEBUG 0

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
        assert(close(pipefd[0])!=-1);
        //将标准输出重定向到管道的写端
        assert(dup2(pipefd[1], STDOUT_FILENO)!=-1);
       
        //参考jyy,手工构建argv
        char exec_argv[argc+2];
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

        dup2(pipefd[0], STDIN_FILENO);
        
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
