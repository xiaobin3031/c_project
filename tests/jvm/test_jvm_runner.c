#include "../test_runner.h"
#include "../../src/modules/jvm/interpreter/interpreter.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <signal.h>

// 1. 声明所有外部测试模块的案例数组 (例如，来自 test_uri.c)
// 对于每个新的测试文件 (如 test_http.c)，您都需要在这里添加一个 extern 声明。
extern test_case_t test_parse_class_cases[];

// 2. 注册所有测试模块
// 将所有可运行的测试模块添加到这个数组中
test_module_t all_test_modules[] = {
    {"test_parse_class", test_parse_class_cases},
    {NULL, NULL} // 哨兵
};

void crash_handler(int sig) {
    printf("Caught signal %d\n", sig);
    // print_stacktrace();
    exit(1);
}

/**
 * 主函数：解析命令行参数并运行指定的测试
 */
int main(int argc, char *argv[]) {
    signal(SIGSEGV, crash_handler);
    signal(SIGABRT, crash_handler);
    return run_test_module(argc, argv, all_test_modules);
}