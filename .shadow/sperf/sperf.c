#include <stdio.h>
#include <assert.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <sys/wait.h>

#define DEBUG 0

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
        assert(close(pipefd[0])!=-1);
        assert(dup2(pipefd[1], STDOUT_FILENO)!=-1);
        assert(close(pipefd[1])!=-1);
        execvp(argv[1], argv + 1);
        perror("execvp");
        exit(1);
    } else {
        //父进程    //读strace的输出并处理
        close(pipefd[1]);
        dup2(pipefd[0], STDIN_FILENO);
        close(pipefd[0]);
        execvp(argv[2], argv + 2);
        perror("execvp");
        exit(1);
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
