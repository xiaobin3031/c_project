#include "junit.h"
#include "expr.h"
#include "stmt.h"
#include "../project/project.h"
#include "../classfile/class_reader.h"
#include "../../../core/list/arraylist.h"
#include "../runtime/class.h"
#include <string.h>

static void add_common_imports(test_class_t *test_class) {
    arraylist_add(test_class->imports, "import com.shanshan.order.util.Util;");
    arraylist_add(test_class->imports, "import org.junit.jupiter.api.extension.ExtendWith;");
    arraylist_add(test_class->imports, "import org.mockito.InjectMocks;");
    arraylist_add(test_class->imports, "import org.mockito.Mock;");
    arraylist_add(test_class->imports, "import org.springframework.test.context.junit.jupiter.SpringExtension;");
    arraylist_add(test_class->imports, "import org.junit.jupiter.api.Test;");
}

void create_junit_test_class(
    const char *src_class_dir,
    const char *dest_class_dir,
    const char *new_package_name
) {
    project_t *project = load_project(src_class_dir, NULL);
    for(int i=0;i<project->class_file_source->size;i++) {
        class_file_source_t *source = arraylist_get(project->class_file_source, i);
        if(source->source != CLASS_FILE_SOURCE_FILE) {
            continue;
        }

        class_file_t *cf = read_class_file(source->path);
        class_t *klass = define_class(cf);
        if(cf) {
            test_class_t *test_class = test_class_new(new_package_name, klass->class_name);
            add_common_imports(test_class);

            // fields
            for(size_t i = 0; i < klass->fields_count; i++) {
                field_t *field = &klass->fields[i];
                char *descriptor = field->descriptor;
                test_field_t *test_field = test_field_new(field->name, field->descriptor);
                arraylist_add(test_class->fields, test_field);
            }
        }
    }
}