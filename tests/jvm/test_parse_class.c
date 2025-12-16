#include "../test_runner.h"
#include "../../src/modules/jvm/classfile/class_reader.h"

int test_parse_class_magic() {
    class_t *class = read_class_file("../../data/jvm/Main.class");
    ASSERT_TRUE(class != NULL, "test class file not null");
    ASSERT_EQ(0xCAFEBABE, class->magic, "test class magic CAFEBABE");
    class_free(class);
    return SUCCESS;
}

test_case_t test_parse_class_cases[] = {
    {"test parse class magic", test_parse_class_magic},
    {NULL, NULL}
};