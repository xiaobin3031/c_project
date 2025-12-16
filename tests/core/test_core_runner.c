#include "../test_runner.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

// 1. 声明所有外部测试模块的案例数组 (例如，来自 test_uri.c)
// 对于每个新的测试文件 (如 test_http.c)，您都需要在这里添加一个 extern 声明。
extern test_case_t test_uri_cases[];
extern test_case_t test_http_cases[];
extern test_case_t test_nvd_cases[];

// 2. 注册所有测试模块
// 将所有可运行的测试模块添加到这个数组中
test_module_t all_test_modules[] = {
    {"test_uri", test_uri_cases},
    {"test_http", test_http_cases},
    {"test_nvd", test_nvd_cases},
    {NULL, NULL} // 哨兵
};

/**
 * 主函数：解析命令行参数并运行指定的测试
 */
int main(int argc, char *argv[]) {
    return run_test_module(argc, argv, all_test_modules);
}