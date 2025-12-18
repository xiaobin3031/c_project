#include "../test_runner.h"
#include "../../src/modules/jvm/classfile/class_reader.h"
#include "../../src/modules/jvm/classfile/constant_pool.h"
#include "../../src/modules/jvm/vm/vm.h"
#include <string.h>

int test_parse_class() {
    int result = FAILURE;
    class_t *class = read_class_file("../data/jvm/Main.class");
    ASSERT_TRUE(class != NULL, "test class file not null");
    ASSERT_EQ(0xCAFEBABE, class->magic, "test class magic CAFEBABE");
    printf("\n");
    
    
    print_class_name(class->cp_pools, class->constant_pool_count);
    print_method_name(class->cp_pools, class->constant_pool_count);

    ASSERT_TRUE(class->methods != NULL, "test class methods not null");
    method_t *main_method;
    for(int i=0;i<class->methods_count;i++){
        method_t *method = class->methods[i];
        char *method_name = get_utf8(class->cp_pools[method->name_index]);
        if(strcmp(method_name, "main") == 0) {
            printf("`main` method found\n");
            main_method = method;
            break;
        }
    }

    if(main_method) {
        // 执行方法
        printf("run main method\n");
        run(main_method, class);
    }

    class_free(class);
    return SUCCESS;
}

test_case_t test_parse_class_cases[] = {
    {"test parse class magic", test_parse_class},
    {NULL, NULL}
};