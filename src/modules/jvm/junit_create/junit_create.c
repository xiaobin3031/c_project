#include "junit.h"
#include "expr.h"
#include "stmt.h"
#include "../project/project.h"
#include "../classfile/class_reader.h"
#include "../../../core/list/arraylist.h"
#include "../runtime/class.h"
#include "../runtime/frame.h"
#include "../interpreter/interpreter.h"
#include <string.h>

static char buffer[1024];

static void add_common_imports(test_class_t *test_class) {
    arraylist_add(test_class->imports, "import com.shanshan.order.util.Util;");
    arraylist_add(test_class->imports, "import org.junit.jupiter.api.extension.ExtendWith;");
    arraylist_add(test_class->imports, "import org.mockito.InjectMocks;");
    arraylist_add(test_class->imports, "import org.mockito.Mock;");
    arraylist_add(test_class->imports, "import org.springframework.test.context.junit.jupiter.SpringExtension;");
    arraylist_add(test_class->imports, "import org.junit.jupiter.api.Test;");
}

static void add_import(test_class_t *test_class, const char *type) {
    sprintf(buffer, "import %s;", type);
    for(size_t i = 0; i < test_class->imports->size; i++) {
        char *import = (char*)arraylist_get(test_class->imports, i);
        if(strcmp(import, buffer) == 0) {
            return;
        }
    }
    arraylist_add(test_class->imports, strdup(buffer));
}

static char *descriptor_to_type(const char *descriptor) {
    const char *start = strrchr(descriptor, '/');
    const char *end = strrchr(descriptor, ';');
    return strndup(start + 1, end - start - 1);
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
            test_class_t *test_class = test_class_new(new_package_name, klass->class_simple_name);
            add_common_imports(test_class);

            // fields
            for(size_t i = 0; i < klass->fields_count; i++) {
                field_t *field = &klass->fields[i];
                if(field->access_flags & FIELD_ACC_STATIC)
                    continue;
                char *descriptor = field->descriptor;
                char *desc = strdup(descriptor);
                const char *end = strrchr(desc, ';');
                char *ptr = desc;
                while(*ptr) {
                    if(*ptr == '/') *ptr = '.';
                    ptr++;
                }
                add_import(test_class, strndup(desc + 1, end - desc - 1));
                free(desc);
                test_field_t *test_field = test_field_new(field->name, descriptor_to_type(descriptor));
                arraylist_add(test_field->annos, "@Mock(lenient = true)");
                arraylist_add(test_class->fields, test_field);
            }

            // methods
            jvm_thread_t *thread = jvm_thread_new();
            for(size_t i = 0; i < klass->methods_count; i++) { 
                method_t *method = &klass->methods[i];
                if(method->access_flags & METHOD_ACC_PUBLIC && *method->name != '<') {
                    test_method_t *test_method = test_method_new(method->name, "void");
                    arraylist_add(test_method->annos, "@Test");
                    arraylist_add(test_class->methods, test_method);

                    // 增加方法体
                    frame_t *frame = frame_new(method, NULL);
                    push_frame(thread, frame);
                    // interpret(thread);

                    // 搜集结果
                }
            }
            print_test_class(test_class);

            // 暂时只解析第一个文件
            break;
        }
    }

}

void print_test_class(test_class_t *test_class) {
    printf("package %s;\n\n", test_class->package);
    arraylist *imports = test_class->imports;
    for(size_t i = 0;i<imports->size;i++) {
        char *import = (char*) arraylist_get(imports, i);
        printf("%s\n", import);
    }

    printf("\n");

    printf("/**\n * Create by xuweibin\n */\n");
    printf("public class Test%s {\n\n", test_class->class_name);
    arraylist *fields = test_class->fields;

    // fields
    for(size_t i = 0;i<fields->size;i++) {
        test_field_t *field = (test_field_t*) arraylist_get(fields, i);
        arraylist *annos = field->annos;
        for(size_t j = 0;j<annos->size;j++) {
            char *anno = (char*) arraylist_get(annos, j);
            printf("    %s\n", anno);
        }
        printf("    private %s %s;\n", field->type, field->name);
    }
    printf("\n");

    // methods
    arraylist *methods = test_class->methods;
    for(size_t i = 0;i<methods->size;i++) {
        test_method_t *method = (test_method_t*) arraylist_get(methods, i);
        arraylist *annos = method->annos;
        for(size_t j = 0;j<annos->size;j++) {
            char *anno = (char*) arraylist_get(annos, j);
            printf("    %s\n", anno);
        }
        printf("    public %s %s() {\n\n", method->return_type, method->name);

        printf("    }\n\n");
    }
    printf("}\n\n");
}