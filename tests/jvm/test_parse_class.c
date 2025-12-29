#include "../test_runner.h"
#include "../../src/modules/jvm/classfile/class_reader.h"
#include "../../src/modules/jvm/classfile/constant_pool.h"
#include "../../src/modules/jvm/vm/vm.h"
#include "../../src/modules/jvm/project/project.h"
#include "../../src/modules/jvm/utils/slots.h"
#include "../../src/modules/jvm/local_tools/junit_test.h"
#include <string.h>

int test_parse_class() {

    project_t *project = load_project("../data/jvm");

    run("Main", project);
    return SUCCESS;
}

int test_slot_count() {
    // 1. 无参情况
    ASSERT_EQ(slot_count_from_desciptor("()V"), 0, "No arguments");

    // 2. 基本单槽类型 (B, C, F, I, S, Z)
    ASSERT_EQ(slot_count_from_desciptor("(I)V"), 1, "Single int");
    ASSERT_EQ(slot_count_from_desciptor("(BCFISZ)V"), 6, "All single-slot types");

    // 3. 双槽类型 (J=Long, D=Double)
    ASSERT_EQ(slot_count_from_desciptor("(J)V"), 2, "Single long");
    ASSERT_EQ(slot_count_from_desciptor("(D)V"), 2, "Single double");
    ASSERT_EQ(slot_count_from_desciptor("(JD)V"), 4, "Long and Double mixed");
    ASSERT_EQ(slot_count_from_desciptor("(IJD)V"), 5, "Int, Long, Double mixed");

    // 4. 对象引用 (L...;) - 无论路径多长只占 1 Slot
    ASSERT_EQ(slot_count_from_desciptor("(Ljava/lang/String;)V"), 1, "String object");
    ASSERT_EQ(slot_count_from_desciptor("(Ljava/lang/Object;I)V"), 2, "Object and Int");

    // 5. 数组类型 ([) - JVM 规定数组引用只占 1 Slot，无论维度和元素类型
    ASSERT_EQ(slot_count_from_desciptor("([I)V"), 1, "Int array");
    ASSERT_EQ(slot_count_from_desciptor("([[I)V"), 1, "2D Int array");
    ASSERT_EQ(slot_count_from_desciptor("([Ljava/lang/Object;)V"), 1, "Object array");
    ASSERT_EQ(slot_count_from_desciptor("([[J)V"), 1, "2D Long array (Should be 1 slot, not 2)");

    // 6. 综合复杂场景
    // (Ljava/lang/String;ID[JLjava/lang/Object;)V
    // String(1) + int(1) + double(2) + long[](1) + Object(1) = 6
    ASSERT_EQ(slot_count_from_desciptor("(Ljava/lang/String;ID[JLjava/lang/Object;)V"), 6, "Complex mixed");

    printf("[SUCCESS] All descriptor tests passed!\n");
    return SUCCESS;
}

int junit_test_create() {
    // char *src_class_dir = "/mnt/e/code/shanshan-biz-order/shanshan-biz-order-admin/target/classes/com/shanshan/order/controller";
    // char *dest_class_dir = "/mnt/e/code/shanshan-biz-order/shanshan-biz-order-coverage/src/test/java/com/shanshan/mock_test/order_admin";
    // char *package_name = "com.shanshan.mock_test.order_admin";
    // create_junit_test_class(src_class_dir, dest_class_dir, package_name);
    return SUCCESS;
}

int junit_test_class_init() {

    project_t *project = load_project("../data/jvm");

    run("MainInit", project);
    return SUCCESS;
}

test_case_t test_parse_class_cases[] = {
    {"test parse class magic", test_parse_class},
    {"test slot count", test_slot_count},
    {"junit test create", junit_test_create},
    {"junit test class init", junit_test_class_init},
    {NULL, NULL}
};