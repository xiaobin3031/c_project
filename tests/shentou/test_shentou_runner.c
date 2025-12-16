#include "../test_runner.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

extern test_case_t test_cve_cases[];

test_module_t all_test_modules[] = {
    {"test_cve", test_cve_cases},
    {NULL, NULL} // 哨兵
};

/**
 * 主函数：解析命令行参数并运行指定的测试
 */
int main(int argc, char *argv[]) {
    return run_test_module(argc, argv, all_test_modules);
}