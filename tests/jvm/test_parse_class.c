#include "../test_runner.h"
#include "../../src/modules/jvm/classfile/class_reader.h"
#include "../../src/modules/jvm/classfile/constant_pool.h"

int test_parse_class() {
    class_t *class = read_class_file("../src/data/jvm/Main.class");
    ASSERT_TRUE(class != NULL, "test class file not null");
    ASSERT_EQ(0xCAFEBABE, class->magic, "test class magic CAFEBABE");
    printf("\n");
    
    
    print_class_name(class->cp_pools, class->constant_pool_count);
    print_method_name(class->cp_pools, class->constant_pool_count);


    class_free(class);
    return SUCCESS;
}

test_case_t test_parse_class_cases[] = {
    {"test parse class magic", test_parse_class},
    {NULL, NULL}
};