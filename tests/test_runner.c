#include "test_runner.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

/**
 * 核心运行函数：运行单个测试模块中的所有案例
 */
static int run_test_module_inner(const char *module_name, test_case_t tests[]) {
    int total_tests = 0;
    int failed_tests = 0;
    test_case_t *current = tests;

    printf("\n--- Running Module: %s ---\n", module_name);

    while (current->function != NULL) {
        total_tests++;
        printf("  Running case: %s... \n", current->name);
        if (current->function() != SUCCESS) {
            failed_tests++;
            printf("FAILED.\n");
        } else {
            printf("PASSED.\n");
        }
        current++;
    }
    
    printf("  Module Summary: %d/%d tests passed.\n", total_tests - failed_tests, total_tests);
    return failed_tests;
}

/**
 * 主函数：解析命令行参数并运行指定的测试
 */
int run_test_module(int argc, char *argv[], test_module_t *all_test_modules) {
    int overall_failed = 0;
    int modules_to_run_count = argc - 1;

    if (modules_to_run_count == 0) {
        // 如果没有参数，则运行所有注册的模块
        printf("No module specified. Running ALL tests.\n");
        for (test_module_t *mod = all_test_modules; mod->module_name != NULL; mod++) {
            overall_failed += run_test_module_inner(mod->module_name, mod->tests);
        }
    } else {
        // 根据命令行参数运行指定的模块
        for (int i = 1; i < argc; i++) {
            const char *target_module_name = argv[i];
            int module_found = 0;
            
            for (test_module_t *mod = all_test_modules; mod->module_name != NULL; mod++) {
                if (strcmp(mod->module_name, target_module_name) == 0) {
                    overall_failed += run_test_module_inner(mod->module_name, mod->tests);
                    module_found = 1;
                    break;
                }
            }
            
            if (!module_found) {
                fprintf(stderr, "Error: Test module '%s' not found.\n", target_module_name);
                overall_failed++; // 找不到模块也算作失败
            }
        }
    }

    printf("\n--- OVERALL TEST SUMMARY ---\n");
    if (overall_failed == 0) {
        printf("Result: ALL MODULES PASSED! 🎉\n");
        return EXIT_SUCCESS;
    } else {
        printf("Result: %d test cases failed across all modules. 😭\n", overall_failed);
        return EXIT_FAILURE;
    }
}