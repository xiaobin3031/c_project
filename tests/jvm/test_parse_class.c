#include "../test_runner.h"
#include "../../src/modules/jvm/classfile/class_reader.h"
#include "../../src/modules/jvm/classfile/constant_pool.h"
#include "../../src/modules/jvm/vm/vm.h"
#include "../../src/modules/jvm/project/project.h"
#include "../../src/modules/jvm/utils/slots.h"
#include "../../src/modules/jvm/runtime/class.h"
#include "../../src/modules/jvm/junit_create/junit.h"
#include <string.h>

#define JDK_HOME "/mnt/c/Program Files/Java/jdk-17"
// #define JDK_HOME "/Users/lixiaolin/Documents/jdk-17.0.2.jdk/Contents/Home"


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

int junit_test_class_clinit() {

    class_file_t *cf = read_class_file("/mnt/e/code_new/shanshan-biz-order-app/shanshan-biz-order-app/target/classes/com/shanshan/order/controller/EasyPayController.class");
    class_t *class = define_class(cf);
    return SUCCESS;
}

int junit_test_class_init() {
    const char *file = "/mnt/e/code_new/shanshan-biz-order-app/shanshan-biz-order-app/target/classes/com/shanshan/order/controller";
    // const char *file = "/Users/lixiaolin/Documents/xiaobin/杉杉/shanshan-biz-order-app/shanshan-biz-order-app/target/classes/com/shanshan/order/controller";
    const char *dest_path = "/mnt/e/code_new/shanshan-biz-order-app/shanshan-biz-order-app/src/test/java/junit_mock";
    project_t *project = load_project(file, NULL);
    // load class from jdk or jar
    create_junit_test_class(project, dest_path);
    return SUCCESS;
}

test_case_t test_parse_class_cases[] = {
    // {"test slot count", test_slot_count},
    {"junit test class init", junit_test_class_init},
    // {"junit test class clinit", junit_test_class_clinit},
    {NULL, NULL}
};