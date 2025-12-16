#pragma once

#define SUCCESS 0
#define FAILURE 1

// --- 简单的断言宏 (仅用于测试) ---
#define ASSERT_TRUE(condition, test_name) \
    do { \
        if (!(condition)) { \
            fprintf(stderr, "[FAILURE] %s: Condition '\"%s\"' failed at line %d.\n", \
                    test_name, #condition, __LINE__); \
            return FAILURE; \
        } \
    } while (0)

#define ASSERT_STREQ(actual, expected, test_name) \
    do { \
        if (strcmp(actual, expected) != 0) { \
            fprintf(stderr, "[FAILURE] %s: Expected \"%s\", Got \"%s\" at line %d.\n", \
                    test_name, expected, actual, __LINE__); \
            return FAILURE; \
        } \
    } while (0)

#define ASSERT_EQ(actual, expected, test_name) \
    do { \
        if (actual != expected) { \
            fprintf(stderr, "[FAILURE] %s: Expected %d, Got %d at line %d.\n", \
                    test_name, expected, actual, __LINE__); \
            return FAILURE; \
        } \
    } while (0)

// --- 通用结构体定义 ---

// 定义一个测试案例结构体
typedef struct {
    char *name;
    int (*function)();
} test_case_t;

// 定义一个测试模块结构体，包含测试名称和测试案例数组
typedef struct {
    const char *module_name;
    test_case_t *tests;
} test_module_t;

// 外部接口函数：运行一个测试模块
int run_test_module(int argc, char *argv[], test_module_t *all_test_modules);