#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <dlfcn.h>



int main(int argc, char *argv[]) {
    static char line[4096];

    while (1) {
        printf("crepl> ");
        fflush(stdout);

        if (!fgets(line, sizeof(line), stdin)) {
            break;
        }

        const char *source_code = "";
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
        snprintf(compile_command, sizeof(compile_command), "gcc -shared -o %s %s",library_filename, source_filename);

        //gcc -shared -o my_library.so my_library.c

        int compile_result = system(compile_command);
        if (compile_result != 0) {
            fprintf(stderr, "Compilation failed with error code %d\n", compile_result);
            return 1;
        }


        const char *new_source_code = "int add(int a, int b) { return a + b;}";
        const char *new_source_filename = "/tmp/new_temp_code.c";
        FILE *new_source_file = fopen(new_source_filename, "w");
        if (new_source_file == NULL) {
            perror("fopen");
            return 1;
        }
        fprintf(new_source_file, "%s", new_source_code);
        fclose(new_source_file);

        // 2. 编译源代码文件
        const char *new_library_filename = "/tmp/new_temp_code.o";
        char new_compile_command[256];
        snprintf(new_compile_command, sizeof(new_compile_command), "gcc -c -o %s %s",new_library_filename, new_source_filename);

        //gcc -shared -o my_library.so my_library.c

        compile_result = system(new_compile_command);

        char cmd[256];
        snprintf(cmd, sizeof(cmd), "gcc -shared -o /tmp/temp_code.so /tmp/new_temp_code.o /tmp/temp_code.so");

        if (compile_result != 0) {
            fprintf(stderr, "Compilation failed with error code %d\n", compile_result);
            return 1;
        }

        void *handle;
        int (*foo)();  // 假设foo是一个无参数且返回void的函数
        char *error;

        // 打开共享库
        handle = dlopen("/tmp/temp_code.so", RTLD_LAZY);
        if (!handle) {
            fprintf(stderr, "%s\n", dlerror());
            return 1;
        }

        // 清除现有的错误
        dlerror();

        // 获取foo函数的地址
        *(void **) (&foo) = dlsym(handle, "add");
        if ((error = dlerror()) != NULL)  {
            fprintf(stderr, "%s\n", error);
            dlclose(handle);
            return 1;
        }

        // 调用函数
        printf("%d\n",foo(1,2));

        // 关闭共享库
        dlclose(handle);

        remove(source_filename);
        remove(library_filename);

        return 0;
        
        // To be implemented.
        printf("Got %zu chars.\n", strlen(line));
    }
}


/*
int main() {
    // 1. 创建临时源代码文件
    const char *source_code = "#include <stdio.h>\nint main() { printf(\"Hello, World!\\n\"); return 0; }";
    const char *source_filename = "/temp_code.c";
    FILE *source_file = fopen(source_filename, "w");
    if (source_file == NULL) {
        perror("fopen");
        return 1;
    }
    fprintf(source_file, "%s", source_code);
    fclose(source_file);

    // 2. 编译源代码文件
    const char *library_filename = "/temp_code.so";
    char compile_command[256];
    snprintf(compile_command, sizeof(compile_command), "gcc -shared -o %s %s",library_filename, source_filename);

    //gcc -shared -o my_library.so my_library.c

    int compile_result = system(compile_command);
    if (compile_result != 0) {
        fprintf(stderr, "Compilation failed with error code %d\n", compile_result);
        return 1;
    }

    // 3. 检查生成的二进制文件并执行
    if (access(binary_filename, X_OK) == 0) {
        printf("Compilation succeeded, executing the binary...\n");
        int exec_result = system(binary_filename);
        if (exec_result != 0) {
            fprintf(stderr, "Execution failed with error code %d\n", exec_result);
            return 1;
        }
    } else {
        fprintf(stderr, "Binary file is not executable\n");
        return 1;
    }

    // 清理临时文件
    remove(source_filename);
    remove(binary_filename);

    return 0;
}
*/