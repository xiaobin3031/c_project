#include "junit.h"
#include "expr.h"
#include "stmt.h"
#include "../project/project.h"
#include "../classfile/class_reader.h"
#include "../../../core/list/arraylist.h"
#include "../runtime/class.h"
#include "../runtime/frame.h"
#include "../runtime/local_vars.h"
#include "../runtime/operand_stack.h"
#include "../interpreter/interpreter.h"
#include "../utils/slots.h"
#include "../vm/classload.h"
#include "stmt.h"
#include "expr.h"
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

static void add_method_parameters(test_method_t *test_method, const char *descriptor) {
    const char *ptr = descriptor;
    ptr++;
    char arg_buffer[200];
    while(*ptr && *ptr != ')') {
        sprintf(arg_buffer, "arg%d", test_method->local_var_index++);
        var_decl_stmt_t *arg_decl = var_decl_stmt_new(NULL, arg_buffer, NULL);
        expr_t *init;
        switch(*ptr) {
            case 'B': {
                arg_decl->type = "byte";
                init = expr_new(EXPR_LITERAL);
                literal_expr_t *literal_expr = literal_expr_new(LIT_BYTE);
                literal_expr->i = 1;
                init->literal = literal_expr;
                break;
            }
            case 'C': {
                arg_decl->type = "char";
                init = expr_new(EXPR_LITERAL);
                literal_expr_t *literal_expr = literal_expr_new(LIT_CHAR);
                literal_expr->c = 'a';
                init->literal = literal_expr;
                break;
            }
            case 'D': {
                arg_decl->type = "double";
                init = expr_new(EXPR_LITERAL);
                literal_expr_t *literal_expr = literal_expr_new(LIT_DOUBLE);
                literal_expr->d = 1.0;
                init->literal = literal_expr;
                break;
            }
            case 'F': {
                arg_decl->type = "float";
                init = expr_new(EXPR_LITERAL);
                literal_expr_t *literal_expr = literal_expr_new(LIT_FLOAT);
                literal_expr->f = 1.0f;
                init->literal = literal_expr;
                break;
            }
            case 'I': {
                arg_decl->type = "int";
                init = expr_new(EXPR_LITERAL);
                literal_expr_t *literal_expr = literal_expr_new(LIT_INT);
                literal_expr->i = 1L;
                init->literal = literal_expr;
                break;
            }
            case 'J': {
                arg_decl->type = "long";
                init = expr_new(EXPR_LITERAL);
                literal_expr_t *literal_expr = literal_expr_new(LIT_LONG);
                literal_expr->l = 1L;
                init->literal = literal_expr;
                break;
            }
            case 'S': {
                arg_decl->type = "short";
                init = expr_new(EXPR_LITERAL);
                literal_expr_t *literal_expr = literal_expr_new(LIT_SHORT);
                literal_expr->i = 1;
                init->literal = literal_expr;
                break;
            }
            case 'Z': {
                arg_decl->type = "boolean";
                init = expr_new(EXPR_LITERAL);
                literal_expr_t *literal_expr = literal_expr_new(LIT_BOOL);
                literal_expr->b = 1;
                init->literal = literal_expr;
                break;
            }
            case 'L': {
                const char *end = strchr(ptr, ';');
                const char *full_type = strndup(ptr + 1, end - ptr - 1);
                char *simple_type = descriptor_to_simple_type(full_type);
                arg_decl->type = strdup(simple_type);
                if(strcmp("java/lang/String", full_type) == 0) {
                    init = expr_new(EXPR_LITERAL);
                    literal_expr_t *literal_expr = literal_expr_new(LIT_STRING);
                    literal_expr->s = strdup("a");
                    init->literal = literal_expr;
                }else{
                    init = expr_new(EXPR_METHOD_CALL);
                    method_call_expr_t *method_call_expr = method_call_expr_new(NULL, simple_type);
                    method_call_expr->method = "Util.newAndInit";
                    sprintf(arg_buffer, "%s.class", simple_type);
                    arraylist_add(method_call_expr->args, strdup(arg_buffer));
                    init->method_call = method_call_expr;
                }
                ptr = end;
                break;
            }
        }
        arg_decl->init = init;
        stmt_t *stmt = stmt_new(STMT_VAR_DECL);
        stmt->var_decl = arg_decl;
        arraylist_add(test_method->body, stmt);

        ptr++;
    }
}

void create_junit_test_class(
    const char *src_class_dir,
    const char *dest_class_dir,
    const char *new_package_name
) {
    project_t *project = load_project(src_class_dir, NULL);
    char act_call_buffer[1024];
    for(int i=0;i<project->class_file_source->size;i++) {
        class_file_source_t *source = arraylist_get(project->class_file_source, i);
        if(source->source != CLASS_FILE_SOURCE_FILE) {
            continue;
        }

        class_file_t *cf = read_class_file(source->path);
        class_t *klass = define_class(cf);
        add_class(klass);
        if(strcmp(klass->class_name, "com/shanshan/order/controller/EasyPayController") != 0) continue;
        if(cf) {
            test_class_t *test_class = test_class_new(new_package_name, klass->class_simple_name);
            add_common_imports(test_class);
            arraylist_add(test_class->annos, "@ExtendWith(MockitoExtension.class)");

            char *inject_field_name = strdup(klass->class_simple_name);
            *inject_field_name += 32;
            test_field_t *inject_field = test_field_new(inject_field_name, descriptor_to_simple_type(klass->class_name));
            arraylist_add(inject_field->annos, "@InjectMocks");
            arraylist_add(test_class->fields, inject_field);
            add_import(test_class, descriptor_to_type(klass->class_name));

            // fields
            for(size_t i = 0; i < klass->fields_count; i++) {
                field_t *field = &klass->fields[i];
                if(field->access_flags & FIELD_ACC_STATIC)
                    continue;
                char *descriptor = field->descriptor;
                add_import(test_class, descriptor_to_type(descriptor));
                test_field_t *test_field = test_field_new(field->name, descriptor_to_simple_type(descriptor));
                arraylist_add(test_field->annos, "@Mock(lenient = true)");
                arraylist_add(test_class->fields, test_field);
            }

            // methods
            jvm_thread_t *thread = jvm_thread_new();
            for(size_t i = 0; i < klass->methods_count; i++) { 
                method_t *method = &klass->methods[i];
                if(method->access_flags & METHOD_ACC_PUBLIC && *method->name != '<') {
                    sprintf(act_call_buffer, "this.%s.%s", inject_field_name, method->name);
                    method_call_expr_t *act_call_expr = method_call_expr_new(NULL, act_call_buffer);
                    test_method_t *test_method = test_method_new(method->name, "void");
                    add_method_parameters(test_method, method->descriptor);
                    frame_t *frame = frame_new(method, NULL);
                    // act call
                    arraylist *body = test_method->body;
                    for(size_t i = 0;i<body->size;i++) {
                        stmt_t *stmt = arraylist_get(body, i);
                        arraylist_add(act_call_expr->args, strdup(stmt->var_decl->name));
                        slot_t *slot = get_local(frame, i + 1);
                        test_field_t *test_field = test_field_new(stmt->var_decl->name, stmt->var_decl->type);
                        slot->test_field = test_field;
                    }
                    stmt_t *act_call_stmt = stmt_new(STMT_EXPR);
                    expr_t *act_expr = expr_new(EXPR_METHOD_CALL);
                    act_expr->method_call = act_call_expr;
                    act_call_stmt->expr = expr_stmt_new(act_expr);
                    test_method->act_call = act_call_stmt;
                    arraylist_add(test_method->annos, "@Test");
                    arraylist_add(test_class->methods, test_method);

                    // 增加方法体
                    frame->test_class = test_class;
                    frame->test_method = test_method;
                    push_frame(thread, frame);
                    interpret(thread);

                    slot_t *slot = &frame->operand_stack[0];
                    printf("\n\nprint value trace\n");
                    print_value_trace(slot->vt);

                    // 搜集结果
                    printf("interpret end\n");
                    break;
                }
            }

            free(inject_field_name);

            // print_test_class(test_class);

            // 暂时只解析第一个文件
            break;
        }
    }

}

static void print_expr(expr_t *expr) {
    switch (expr->kind) {
        case EXPR_LITERAL: {
            literal_expr_t *literal_expr = (literal_expr_t *) expr;
            switch(literal_expr->kind) {
                case LIT_STRING: {
                    printf("\"%s\"", literal_expr->s);
                    break;
                }
                case LIT_NULL: {
                    printf("null");
                    break;
                }
                case LIT_BOOL: {
                    printf("%s", literal_expr->b == 1 ? "true" : "false");
                    break;
                }
                case LIT_CHAR: {
                    printf("\'%c\'", literal_expr->c);
                    break;
                }
                case LIT_SHORT:
                case LIT_BYTE:
                case LIT_INT: {
                    printf("%d", literal_expr->i);
                    break;
                }
                case LIT_LONG: {
                    printf("%ld", literal_expr->l);
                    break;
                }
                case LIT_FLOAT: {
                    printf("%f", literal_expr->f);
                    break;
                }
                case LIT_DOUBLE: {
                    printf("%f", literal_expr->d);
                    break;
                }
            }
            break;
        }
        case EXPR_METHOD_CALL: {
            method_call_expr_t *method_call_expr = expr->method_call;
            printf("%s(", method_call_expr->method);
            arraylist *args = method_call_expr->args;
            for(size_t i = 0; i < args->size; i++) {
                char *arg = arraylist_get(args, i);
                printf("%s", arg);
                if(i < args->size - 1) {
                    printf(", ");
                }
            }
            printf(")");
            break;
        }
    }
}

static void print_stmt(stmt_t *stmt) {
    switch(stmt->kind) {
        case STMT_VAR_DECL: {
            var_decl_stmt_t *var_decl = stmt->var_decl;
            printf("%s %s", var_decl->type, var_decl->name);
            if(var_decl != NULL) {
                printf(" = ");
                print_expr(var_decl->init);
            }
            printf(";");
            break;
        }
        case STMT_EXPR: {
            expr_stmt_t *expr_stmt = stmt->expr;
            print_expr(expr_stmt->expr);
            printf(";");
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

    int tabs = 1;
    // fields
    for(size_t i = 0;i<fields->size;i++) {
        test_field_t *field = (test_field_t*) arraylist_get(fields, i);
        arraylist *annos = field->annos;
        for(size_t j = 0;j<annos->size;j++) {
            char *anno = (char*) arraylist_get(annos, j);
            printf("%*s%s\n", tabs * 4, "", anno);
        }
        printf("%*s%s %s %s;\n", tabs * 4, "", "private", field->type, field->name);
    }
    printf("\n");

    // methods
    arraylist *methods = test_class->methods;
    for(size_t i = 0;i<methods->size;i++) {
        tabs = 1;
        test_method_t *method = (test_method_t*) arraylist_get(methods, i);
        arraylist *annos = method->annos;
        for(size_t j = 0;j<annos->size;j++) {
            char *anno = (char*) arraylist_get(annos, j);
            printf("%*s%s\n", tabs * 4, "", anno);
        }
        printf("%*s%s %s %s() {\n\n", tabs * 4, "", "public", method->return_type, method->name);
        
        // print body...
        arraylist *body = method->body;
        tabs++;
        for(size_t j = 0;j<body->size;j++){ 
            stmt_t *stmt = (stmt_t*) arraylist_get(body, j);
            printf("%*s", tabs * 4, "");
            print_stmt(stmt);
            printf("\n");
        }

        printf("\n");
        printf("%*s", tabs * 4, "");
        print_stmt(method->act_call);

        printf("\n");

        tabs--;
        printf("%*s%s\n\n", tabs * 4, "", "}");
    }
    printf("}\n\n");
}