#include "../test_runner.h"
#include "../../src/modules/jvm/classfile/class_reader.h"
#include "../../src/modules/jvm/classfile/constant_pool.h"
#include "../../src/modules/jvm/vm/vm.h"
#include "../../src/modules/jvm/project/project.h"
#include <string.h>

int test_parse_class() {
    int result = FAILURE;

    project_t *project = load_project("../data/jvm");

    run("Main", project);
    return result;
}

test_case_t test_parse_class_cases[] = {
    {"test parse class magic", test_parse_class},
    {NULL, NULL}
};