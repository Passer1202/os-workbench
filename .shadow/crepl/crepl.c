#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>



int main(int argc, char *argv[]) {
    static char line[4096];

    while (1) {
        printf("crepl> ");
        fflush(stdout);

        if (!fgets(line, sizeof(line), stdin)) {
            break;
        }

         // 1. 创建临时源代码文件
        const char *source_code = line;
        const char *source_filename = "/tmp/temp_code.c";
        FILE *source_file = fopen(source_filename, "w");
        if (source_file == NULL) {
            perror("fopen");
            return 1;
        }
        fprintf(source_file, "%s", source_code);
        fclose(source_file);

        // 2. 编译源代码文件
        const char *binary_filename = "/tmp/temp_code.out";
        char compile_command[256];
        snprintf(compile_command, sizeof(compile_command), "gcc %s -o %s", source_filename, binary_filename);

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


        

        // To be implemented.
        printf("Got %zu chars.\n", strlen(line));
    }
}


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
    const char *binary_filename = "/tmp/temp_code.out";
    char compile_command[256];
    snprintf(compile_command, sizeof(compile_command), "gcc %s -o %s", source_filename, binary_filename);

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