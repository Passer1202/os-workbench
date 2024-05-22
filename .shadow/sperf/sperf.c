
#include <time.h>
#include <stdio.h>
#include <regex.h>
#include <assert.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <sys/wait.h>

#define TIME_INTERVAL_SEC (1)
#define MAX_STRACE_OUTPUT_SIZE (4095)
#define BUFFER_SIZE (MAX_STRACE_OUTPUT_SIZE + 1)
#define PRINT_NUM (5)


// 链表
typedef struct list_node {
  char *name;
  double time;
  struct list_node *next;
} list_node;

// 链表插入节点，同时保证时间始终从大到小排序
list_node *insert_list_node(list_node *head, list_node *node) {
  if (node == NULL) {
    return head;
  }

  if (head == NULL || head->time < node->time) {
    if (head != NULL) {
      node->next = head;
    }
    return node;
  }

  list_node *prev = head;
  for (list_node *curr = head->next; curr != NULL; curr = curr->next) {
    if (curr->time < node->time) {
      break;
    }
    prev = curr;
  }

  node->next = prev->next;
  prev->next = node;
  return head;
}

// 释放链表内存
void free_list(list_node *head) {
  list_node *curr = head;
  while (curr != NULL) {
    list_node *next = curr->next;
    if (curr->name != NULL) {
      free(curr->name);
    }
    curr->name = NULL;
    free(curr);
    curr = next;
  }
  return;
}



int main(int argc, char *argv[])  {

/*创建匿名管道，子进程写，父进程读*/
    int fd[2]={0};
    
    //创建失败返回-1
    assert(pipe(fd) != -1);
    //创建子进程
    pid_t pid = fork();
    //创建失败返回-1
    assert(pid != -1);
  if (pid == -1) {
    // 子进程创建失败
    debug_printf("fork failed\n");
    exit(EXIT_FAILURE);
  } else if (pid == 0) {
    // 关闭管道读取端，并将管道写入端重定向到标准输出
    close(fd[0]);
    if (dup2(fd[1], STDERR_FILENO) == -1) {
      // 重定向失败
      debug_printf("dup2 failed!\n");
      close(fd[1]);
      exit(EXIT_FAILURE);
    }
    // 执行strace
    char *exec_argv[argc + 2];
    exec_argv[0] = "strace";
    exec_argv[1] = "-T";
    for (int i = 1; i < argc; i++) {
      exec_argv[i + 1] = argv[i];
    }
    exec_argv[argc + 1] = NULL;

    char *exec_envp[] = {
        "PATH=/bin",
        NULL,
    };
    execve("strace", exec_argv, exec_envp);
    execve("/bin/strace", exec_argv, exec_envp);
    execve("/usr/bin/strace", exec_argv, exec_envp);
    perror(argv[0]);
    close(fd[1]);
    exit(EXIT_FAILURE);
  }else {
        // 定义正则表达式
    regex_t reg1, reg2;
    if (regcomp(&reg1, "[^\\(\n\r\b\t]*\\(", REG_EXTENDED) ||
        regcomp(&reg2, "<.*>", REG_EXTENDED)) {
      debug_printf("regcomp failed!\n");
      exit(EXIT_FAILURE);
    }
    // 链表记录各项调用时间信息
    list_node *head = NULL;
    // 调用总时间
    double total_time = 0;
    // 管道读取缓存
    char buffer[BUFFER_SIZE];
    // 关闭管道写入端，并从管道中读取数据
    close(fd[1]);
    FILE *fp = fdopen(fd[0], "r");
    // 当前时间
    int curr_time = 1;
    // 运行标志
    int run_flag = 1;
    while (run_flag) {
      while (fgets(buffer, BUFFER_SIZE, fp) > 0) {
        // 读取到 +++ exited with 1/0 +++ 退出
        if (strstr(buffer, "+++ exited with 0 +++") != NULL ||
            strstr(buffer, "+++ exited with 1 +++") != NULL) {
          run_flag = 0;
          break;
        }
        // 使用正则表达式获取信息
        regmatch_t regmat1, regmat2;
        if (regexec(&reg1, buffer, 1, &regmat1, 0) ||
            regexec(&reg2, buffer, 1, &regmat2, 0)) {
          continue;
        }
        // 读取调用名称信息
        int len = 0;
        len = regmat1.rm_eo - regmat1.rm_so;
        char *name = (char *)malloc(sizeof(char) * len);
        strncpy(name, buffer + regmat1.rm_so, len);
        name[len - 1] = '\0';
        // 读取时间信息
        len = regmat2.rm_eo - regmat2.rm_so - 2;
        char *value = (char *)malloc(sizeof(char) * len);
        strncpy(value, buffer + regmat2.rm_so + 1, len);
        double spent_time = atof(value);
        // 及时释放value内存
        free(value);
        value = NULL;
        // 更新总时间
        total_time += spent_time;
        // 将信息保存到链表中
        list_node *node = NULL;
        int find_flag = 0;
        list_node *prev = NULL;
        for (list_node *curr = head; curr != NULL; curr = curr->next) {
          // 若信息已存在，则将其更新后，重新插入
          if (strcmp(curr->name, name) == 0) {
            find_flag = 1;
            if (prev == NULL) {
              head = curr->next;
            } else {
              prev->next = curr->next;
            }
            node = curr;
            // 信息存在，及时释放name内存
            free(name);
            name = NULL;
            break;
          }
          prev = curr;
        }

        if (find_flag) {
          // 更新信息
          node->time += spent_time;
        } else {
          // 新建节点，初始化信息
          node = (list_node *)malloc(sizeof(list_node));
          node->name = name;
          node->time = spent_time;
        }
        // 插入链表
        head = insert_list_node(head, node);
      }

      // 格式化打印前五占比调用信息
      printf("Time: %ds\n", curr_time);
      int k = 0;
      for (list_node *node = head; node != NULL; node = node->next) {
        if (k++ >= PRINT_NUM) {
          break;
        }
        printf("%s (%0.1f%%)\n", node->name, node->time / total_time * 100);
      }
      printf("=============\n");
      curr_time += TIME_INTERVAL_SEC;
      sleep(TIME_INTERVAL_SEC);
    }
    // 关闭管道读取端，释放相关资源
    regfree(&reg1);
    regfree(&reg2);
    fclose(fp);
    close(fd[0]);
    free_list(head);
    exit(EXIT_SUCCESS);
  }
}
