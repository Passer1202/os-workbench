#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <dlfcn.h>
#include <stdbool.h>

//32存在问题

const char *lib_name = "/tmp/mylib.so";

char source_code[500000];

void run_cmd(const char *cmd){

     // 创建子进程
    pid_t pid = fork();
    if (pid < 0) {
        perror("fork");
        return ;
    } else if (pid == 0) {  // 子进程
        // 在子进程中执行编译命令
        execl("/bin/sh", "/bin/sh", "-c", cmd, NULL);
        // 如果exec失败则打印错误并退出
        perror("execl");
        exit(1);
    } else {  // 父进程
        // 等待子进程结束
        int status;
        waitpid(pid, &status, 0);
        // 检查子进程退出状态
        if (!WIFEXITED(status) || WEXITSTATUS(status) != 0) {
            fprintf(stderr, "Compilation failed\n");
            return ;
        }
    }


}

void init(){

    //初始化，生成一个空的共享库

    // 1. 创建临时源代码文件
    const char *code = "void _empty(){return ;}\n";
    strcat(source_code,code);

    const char *source_filename= "/tmp/source_code.c";

    FILE *source_file = fopen(source_filename, "w");

    
    if (source_file == NULL) {
        perror("fopen");
        return ;
    }
    fprintf(source_file, "%s", source_code);
    fclose(source_file);


    // 2. 编译源代码文件

    char cmd[256];

    snprintf(cmd, sizeof(cmd), "gcc -shared -o %s %s",lib_name, source_filename);
    //gcc -shared -o my_library.so my_library.c
    run_cmd(cmd);

    // 3. 清理临时文件
    remove(source_filename);

}

bool is_function(const char *s) {
    //目前的实现是必须‘int ’开头
    char *result = strstr(s, "int");
    if(result != NULL){
        if(s[0] == 'i' && s[1] == 'n' && s[2] == 't' && s[3] == ' ')
        return true;
    }

    return false;
}




int main(int argc, char *argv[]) {

    init();

    int no = 0;

    while (1) {
        printf("crepl> ");
        fflush(stdout);

        if (!fgets(line, sizeof(line), stdin)) {
            break;
        }
        
        if(is_function(line)){
            //输入是函数;

            // 1. 创建临时源代码文件
            const char *code = line;
            strcat(source_code,code);
            const char *source_filename = "/tmp/temp_code.c";
            FILE *source_file = fopen(source_filename, "w");
            if (source_file == NULL) {
                perror("fopen");
                return 1;
            }
            fprintf(source_file, "%s", source_code);
            fclose(source_file);

            // 2. 编译源代码文件

            char cmd[256];

            remove(lib_name);
            snprintf(cmd, sizeof(cmd), "gcc -shared -o %s %s",lib_name, source_filename);
    
            run_cmd(cmd);
  
            remove(source_filename);
        }
        else{
            //输入的应该是表达式
            //使用wrapper

            // 1. 创建临时源代码文件

            line[strlen(line)-1] = '\0';
            /*
            char code[5000];

            char wrapper[64];

            snprintf(code, sizeof(code), "int __expr_wrapper_%d() { return %s;}", no, line);
            snprintf(wrapper, sizeof(wrapper), "_empty");

            const char *source_code = code;
            printf("%s\n",source_code);

            const char *source_filename = "/tmp/temp_code.c";
            FILE *source_file = fopen(source_filename, "w");
            if (source_file == NULL) {
                perror("fopen");
                return 1;
            }
            fprintf(source_file, "%s", source_code);

            const char *library_filename = "/tmp/temp_code.o";
            char cmd[256];
            //gcc -c -fPIC new_function.c -o new_function.o
            snprintf(cmd, sizeof(cmd), "gcc -c -fPIC %s -o %s", source_filename,library_filename);

            run_cmd(cmd);

            //gcc -shared -o liboriginal.so -Wl,--whole-archive liboriginal.so -Wl,--no-whole-archive new_function.o

            const char *newlib_name = "/tmp/mylib_new.so";

            snprintf(cmd, sizeof(cmd), "gcc -shared -o %s -Wl,--whole-archive %s -Wl,--no-whole-archive %s", newlib_name, lib_name, library_filename);

            run_cmd(cmd);

            snprintf(cmd, sizeof(cmd), "mv %s %s", newlib_name, lib_name);

            run_cmd(cmd);

            // 3. 清理临时文件
            remove(source_filename);
            remove(library_filename);
            remove(newlib_name);

            */
            void *handle;
            int (*foo)(void);  // 假设foo是一个无参数且返回void的函数
            char *error;

            // 打开共享库
            handle = dlopen(lib_name, RTLD_LAZY);
            if (!handle) {
                fprintf(stderr, "%s\n", dlerror());
                return 1;
            }

            // 清除现有的错误
            dlerror();

            //我先将换行符删掉

            // 获取foo函数的地址
            *(void **) (&foo) = dlsym(handle, line);
            if ((error = dlerror()) != NULL)  {
                fprintf(stderr, "%s\n", error);
                dlclose(handle);
                return 1;
            }

            // 调用函数
            //foo();
            printf("%d\n", foo());

            // 关闭共享库
            dlclose(handle); 

        }
        
        
        // To be implemented.
        //printf("Got %zu chars.\n", strlen(line));
    }
}


/*

void *handle;
        int (*foo)(void);  // 假设foo是一个无参数且返回void的函数
        char *error;

        // 打开共享库
        handle = dlopen(lib_name, RTLD_LAZY);
        if (!handle) {
            fprintf(stderr, "%s\n", dlerror());
            return 1;
        }

        // 清除现有的错误
        dlerror();

        // 获取foo函数的地址
        *(void **) (&foo) = dlsym(handle, "_empty");
        if ((error = dlerror()) != NULL)  {
            fprintf(stderr, "%s\n", error);
            dlclose(handle);
            return 1;
        }

        // 调用函数
        printf("%d\n", foo());

        // 关闭共享库
        dlclose(handle);  

*/


/*
int main() {
    // 1. 创建临时源代码文件
    const char *source_code = "#include <stdio.h>\nint main() { printf(\"Hello, World!\\n\"); return 0; }";
    const char *source_filename = "/tmp/temp_code.c";
    FILE *source_file = fopen(source_filename, "w");
    if (source_file == NULL) {
        perror("fopen");
        return 1;
    }
    fprintf(source_file, "%s", source_code);
    fclose(source_file);

     // 2. 编译源代码文件
    const char *library_filename = "/tmp/temp_code.so";
    char compile_command[256];
    snprintf(compile_command, sizeof(compile_command), "gcc -shared -o %s %s", library_filename, source_filename);

    // 创建子进程
    pid_t pid = fork();
    if (pid < 0) {
        perror("fork");
        return 1;
    } else if (pid == 0) {  // 子进程
        // 在子进程中执行编译命令
        execl("/bin/sh", "/bin/sh", "-c", compile_command, NULL);
        // 如果exec失败则打印错误并退出
        perror("execl");
        exit(1);
    } else {  // 父进程
        // 等待子进程结束
        int status;
        waitpid(pid, &status, 0);
        // 检查子进程退出状态
        if (!WIFEXITED(status) || WEXITSTATUS(status) != 0) {
            fprintf(stderr, "Compilation failed\n");
            return 1;
        }
    }

    // 3. 清理临时文件
    remove(source_filename);

    return 0;

    return 0;
}
*/