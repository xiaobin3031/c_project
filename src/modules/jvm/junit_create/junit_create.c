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
#include "../project/project.h"
#include <string.h>
#include <unistd.h>
#include <stdarg.h>

static char buffer[1024];
static int log_console = 1;

static void add_common_imports(test_class_t *test_class) {
    arraylist_add(test_class->imports, "import com.shanshan.order.juninew.util.Util;");
    arraylist_add(test_class->imports, "import org.junit.jupiter.api.extension.ExtendWith;");
    arraylist_add(test_class->imports, "import org.mockito.InjectMocks;");
    arraylist_add(test_class->imports, "import org.mockito.Mock;");
    arraylist_add(test_class->imports, "import org.springframework.test.context.junit.jupiter.SpringExtension;");
    arraylist_add(test_class->imports, "import org.junit.jupiter.api.Test;");
    arraylist_add(test_class->imports, "import org.junit.jupiter.api.extension.ExtendWith;");
    arraylist_add(test_class->imports, "import static org.mockito.ArgumentMatchers.any;");
    arraylist_add(test_class->imports, "import static org.mockito.Mockito.when;");
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

static arraylist *parameters_init_stmts(const char *descriptor) {
    arraylist *stmts = arraylist_new(10);


    const char *ptr = descriptor;
    ptr++;
    char arg_buffer[200];
    while(*ptr && *ptr != ')') {
        sprintf(arg_buffer, "arg%d", stmts->size + 1);
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
                arg_decl->full_type = strdup(full_type);
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
        arraylist_add(stmts, stmt);

        ptr++;
    }

    return stmts;
}

/**
 * 本次frame，有没有运行到最后一行
 */
int is_frame_reach_end(frame_t *frame) {
    rt_code_t *code = frame->method->code;
    if(frame->pc == code->code_length - 1) {
        // 走完最后一步了
        return 1;
    }
    return 0;
}

/**
 * 方法的所有节点是否都已经结束
 */
int is_test_method_finish(test_method_t *test_method, frame_t *frame) {
    arraylist *branchs = test_method->branchs;
    for(size_t i=0;i<branchs->size;i++) {
        body_branch_t *branch = arraylist_get(branchs, i);
        if(branch->kind == BODY_BRANCH_IF && branch->if_branch->taken == 0) {
            // 只要有一个节点没跑到，方法就还未覆盖
            return 0;
        }
    }

    return 1;
}

void fill_act_call_response(char *descriptor, char *test_method_name, method_call_expr_t *mc_expr) {
    char *ptr = descriptor;
    while(*ptr && *ptr != ')') ptr++;
    if(*ptr == ')') ptr++;

    if(*ptr) {
        
    }
}

void create_junit_test_class(
    project_t *project,
    const char *dest_class_dir,
    const char *new_package_name
) {
    bootstrap(project);
    char dest_file_buffer[1024];
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
            sprintf(dest_file_buffer, "%s/Test%s.java", dest_class_dir, klass->class_simple_name);
            if(access(dest_file_buffer, F_OK) == 0) {
                continue;
            }
            char *dest_file_path = strdup(dest_file_buffer);
            sprintf(dest_file_buffer, "Test%s", klass->class_simple_name);
            test_class_t *test_class = test_class_new(new_package_name, dest_file_buffer);
            add_common_imports(test_class);
            arraylist_add(test_class->annos, "@ExtendWith(MockitoExtension.class)");

            char *inject_field_name = strdup(klass->class_simple_name);
            *inject_field_name += 32;
            test_field_t *inject_field = test_field_new(inject_field_name, descriptor_to_simple_type(klass->class_name), klass->class_name);
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
                test_field_t *test_field = test_field_new(field->name, descriptor_to_simple_type(descriptor), descriptor);
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
                    // 计算返回值
                    fill_act_call_response(method->descriptor, method->name, act_call_expr);
                    arraylist *arg_init_stmts = parameters_init_stmts(method->descriptor);
                    frame_t *frame = frame_new(method, NULL);
                    // act call
                    for(size_t i = 0;i<arg_init_stmts->size;i++) {
                        stmt_t *stmt = arraylist_get(arg_init_stmts, i);
                        arraylist_add(act_call_expr->args, strdup(stmt->var_decl->name));
                        slot_t *slot = get_local(frame, i + 1);
                        test_field_t *test_field = test_field_new(stmt->var_decl->name, stmt->var_decl->type, stmt->var_decl->full_type);
                        slot->test_field = test_field;
                    }
                    stmt_t *act_call_stmt = stmt_new(STMT_EXPR);
                    expr_t *act_expr = expr_new(EXPR_METHOD_CALL);
                    act_expr->method_call = act_call_expr;
                    act_call_stmt->expr = expr_stmt_new(act_expr);

                    // 增加方法体
                    frame->test_class = test_class;

                    int name_index = 1;
                    char name_buffer[256];
                    arraylist *all_ifs = arraylist_new(10);
                    int pc_reached_end = 0;
                    while(1) {
                        sprintf(name_buffer, "%s_%d", method->name, name_index);
                        test_method_t *test_method = test_method_new(name_buffer, "void");
                        test_method->act_call = act_call_stmt;
                        test_method->local_var_index = arg_init_stmts->size + 1;
                        test_method->all_ifs = all_ifs;
                        arraylist_add(test_method->annos, "@Test");
                        arraylist_add(test_class->methods, test_method);
                        frame->test_method = test_method;

                        // 复制初始化参数
                        for(size_t i = 0;i<arg_init_stmts->size;i++) {
                            stmt_t *stmt = arraylist_get(arg_init_stmts, i);
                            arraylist_add(test_method->body, stmt);
                        }

                        push_frame(thread, frame);
                        interpret(thread);
                        // 搜集结果
                        printf("interpret end\n\n");

                        if(test_method->short_circuit == 0) {
                            arraylist *branchs = test_method->branchs;
                            for(size_t i = 0;i<branchs->size;i++) {
                                body_branch_t *branch = (body_branch_t*)arraylist_get(branchs, i);
                                if(branch->kind == BODY_BRANCH_IF) {
                                    printf("print value trace: %d %s taken: %d\n", i, branch->if_branch->if_name, branch->if_branch->taken);
                                    print_value_trace(branch->if_branch->vt, 0);
                                }
                            }
                            printf("\n\n\n");


                            // 将if转成代码
                            for(size_t i = 0;i<branchs->size;i++) {
                                body_branch_t *branch = (body_branch_t*)arraylist_get(branchs, i);
                                if(branch->kind == BODY_BRANCH_IF) {
                                    value_trace_back_code(branch->if_branch->vt, test_method);
                                }else if(branch->kind == BODY_BRANCH_STMT) {
                                    arraylist_add(test_method->body, branch->stmt);
                                }
                            }

                            // 设置结束条件
                            if(is_frame_reach_end(frame) == 1) {
                                pc_reached_end = 1;
                            }
                            if(pc_reached_end == 1 && is_test_method_finish(test_method, frame) == 1) {
                                // 方法都覆盖了，退出，遍历下一个方法
                                break;
                            }

                            // 设置if条件的覆盖情况，从最后一个if节点开始累加，因为初始值是0，累加到1，就走到else分支，累加到2就遍历前一个if节点
                            int index = branchs->size - 1;
                            while(index >= 0) {
                                body_branch_t *branch = arraylist_get(branchs, index);
                                if(branch == NULL) {
                                    break;
                                }
                                if(branch->kind != BODY_BRANCH_IF) {
                                    index--;
                                    continue;
                                }
                                branch->if_branch->taken++;
                                if(branch->if_branch->taken == 2) {
                                    index--;
                                }else{
                                    break;
                                }
                            }
                        }else{
                            printf("short circuit, test_method_name: %s\n", name_buffer);
                        }

                        frame->pc = 0;
                        frame->sp = 0;

                        // todo  测试
                        if(name_index == 6) {
                            break;
                        }

                        name_index++;
                    }

                    // todo  测试
                    // break;
                }
            }

            free(inject_field_name);

            print_test_class(test_class, dest_file_path);

            free(dest_file_path);
            // 暂时只解析第一个文件
            break;
        }
    }

}

/**
 * 打印内容，同时打印文件和控制台
 */
static void print_content(FILE *file, const char *fmt, ...) {
    va_list args;

    if(log_console) {
        va_start(args, fmt);
        vprintf(fmt, args);
        va_end(args);
    }

    if(file) {
        va_start(args, fmt);
        vfprintf(file, fmt, args);
        va_end(args);
    }
}

static void print_expr(expr_t *expr, FILE *dest_file) {
    switch (expr->kind) {
        case EXPR_LITERAL: {
            literal_expr_t *literal_expr = expr->literal;
            switch(literal_expr->kind) {
                case LIT_STRING: {
                    print_content(dest_file, "\"%s\"", literal_expr->s);
                    break;
                }
                case LIT_NULL: {
                    print_content(dest_file, "null");
                    break;
                }
                case LIT_BOOL: {
                    print_content(dest_file, "%s", literal_expr->b == 1 ? "true" : "false");
                    break;
                }
                case LIT_CHAR: {
                    print_content(dest_file, "\'%c\'", literal_expr->c);
                    break;
                }
                case LIT_SHORT:
                case LIT_BYTE:
                case LIT_INT: {
                    print_content(dest_file, "%d", literal_expr->i);
                    break;
                }
                case LIT_LONG: {
                    print_content(dest_file, "%ld", literal_expr->l);
                    break;
                }
                case LIT_FLOAT: {
                    print_content(dest_file, "%f", literal_expr->f);
                    break;
                }
                case LIT_DOUBLE: {
                    print_content(dest_file, "%f", literal_expr->d);
                    break;
                }
            }
            break;
        }
        case EXPR_METHOD_CALL: {
            method_call_expr_t *method_call_expr = expr->method_call;
            if(method_call_expr->field != NULL) {
                print_content(dest_file, "%s.", method_call_expr->field);
            }
            print_content(dest_file, "%s(", method_call_expr->method);
            arraylist *args = method_call_expr->args;
            for(size_t i = 0; i < args->size; i++) {
                char *arg = arraylist_get(args, i);
                print_content(dest_file, "%s", arg);
                if(i < args->size - 1) {
                    print_content(dest_file, ", ");
                }
            }
            print_content(dest_file, ")");
            break;
        }
        case EXPR_VAR: {
            var_expr_t *var_expr = expr->var;
            print_content(dest_file, "%s", var_expr->type);
            if(var_expr->params != NULL) {
                print_content(dest_file, "<");
                for(int i = 0; i < var_expr->params->size; i++) {
                    char *param = (char*)arraylist_get(var_expr->params, i);
                    print_content(dest_file, "%s", param);
                    if(i != var_expr->params->size - 1) {
                        print_content(dest_file, ",");
                    }
                }
                print_content(dest_file, ">");
            }
            print_content(dest_file, " %s", var_expr->name);
            if(var_expr->init != NULL) {
                print_content(dest_file, " = ");
                print_expr(var_expr->init, dest_file);
            }
            break;
        }
        case EXPR_MOCK_METHOD_CALL: {
            mock_method_call_expr_t *expr_mock = expr->mock_method_call;
            if(expr_mock->type = MOCK_CALL_RETURN) {
                print_content(dest_file, "when(%s.%s(", expr_mock->field, expr_mock->method);
                for(int i = 0; i < expr_mock->args->size; i++) {
                    char *arg = arraylist_get(expr_mock->args, i);
                    print_content(dest_file, "%s", arg);
                    if(i != expr_mock->args->size - 1) {
                        print_content(dest_file, ", ");
                    }
                }
                print_content(dest_file, ")).thenReturn(%s)", expr_mock->mock_return);
            }else if(expr_mock->type = MOCK_CALL_THROW) {
                print_content(dest_file, "when(%s.%s(", expr_mock->field, expr_mock->method);
                for(int i = 0; i < expr_mock->args->size; i++) {
                    char *arg = arraylist_get(expr_mock->args, i);
                    print_content(dest_file, "%s", arg);
                    if(i != expr_mock->args->size - 1) {
                        print_content(dest_file, ", ");
                    }
                }
                print_content(dest_file, ")).thenThrow(%s)", expr_mock->mock_throw);
            }else{
                printf("unknown mock call type: %d\n", expr_mock->type);
                abort();
            }
            break;
        }
        case EXPR_NEW: {
            expr_new_t *exprnew = expr->new;
            print_content(dest_file, "new %s", exprnew->type);
            if(exprnew->has_params) {
                print_content(dest_file, "<>");
            }
            print_content(dest_file, "(");
            if(exprnew->args != NULL) {
                for(int i = 0; i < exprnew->args->size; i++) {
                    char *arg = arraylist_get(exprnew->args, i);
                    print_content(dest_file, "%s", arg);
                    if(i != exprnew->args->size - 1) {
                        print_content(dest_file, ", ");
                    }
                }
            }
            print_content(dest_file, ")");
            break;
        }
        default: {
            printf("unknown expr kind: %d\n", expr->kind);
            abort();
        }
    }
}

static void print_stmt(stmt_t *stmt, FILE *dest_file) {
    switch(stmt->kind) {
        case STMT_VAR_DECL: {
            var_decl_stmt_t *var_decl = stmt->var_decl;
            print_content(dest_file, "%s %s", var_decl->type, var_decl->name);
            if(var_decl != NULL) {
                print_content(dest_file, " = ");
                print_expr(var_decl->init, dest_file);
            }
            print_content(dest_file, ";");
            break;
        }
        case STMT_EXPR: {
            expr_stmt_t *expr_stmt = stmt->expr;
            print_expr(expr_stmt->expr, dest_file);
            print_content(dest_file, ";");
            break;
        }
        default: {
            printf("UNKNOWN STMT: %d\n", stmt->kind);
            abort();
        }
    }
}

void print_test_class(test_class_t *test_class, const char *dest_file_path) {
    FILE *dest_file = fopen(dest_file_path, "w");
    print_content(dest_file, "package %s;\n\n", test_class->package);
    arraylist *imports = test_class->imports;
    for(size_t i = 0;i<imports->size;i++) {
        char *import = (char*) arraylist_get(imports, i);
        print_content(dest_file, "%s\n", import);
    }

    print_content(dest_file, "\n");

    print_content(dest_file, "/**\n * Create by xuweibin\n */\n");
    arraylist *class_annos = test_class->annos;
    for(size_t i=0;i<class_annos->size;i++) {
        char *anno = arraylist_get(class_annos, i);
        print_content(dest_file, "%s\n", anno);
    }
    print_content(dest_file, "public class %s {\n\n", test_class->class_name);
    arraylist *fields = test_class->fields;

    int tabs = 1;
    // fields
    for(size_t i = 0;i<fields->size;i++) {
        test_field_t *field = (test_field_t*) arraylist_get(fields, i);
        arraylist *annos = field->annos;
        for(size_t j = 0;j<annos->size;j++) {
            char *anno = (char*) arraylist_get(annos, j);
            print_content(dest_file, "%*s%s\n", tabs * 4, "", anno);
        }
        print_content(dest_file, "%*s%s %s %s;\n", tabs * 4, "", "private", field->type, field->name);
    }
    print_content(dest_file, "\n");

    // methods
    arraylist *methods = test_class->methods;
    for(size_t i = 0;i<methods->size;i++) {
        tabs = 1;
        test_method_t *method = (test_method_t*) arraylist_get(methods, i);
        arraylist *annos = method->annos;
        for(size_t j = 0;j<annos->size;j++) {
            char *anno = (char*) arraylist_get(annos, j);
            print_content(dest_file, "%*s%s\n", tabs * 4, "", anno);
        }
        print_content(dest_file, "%*s%s %s %s() {\n\n", tabs * 4, "", "public", method->return_type, method->name);
        
        // print body...
        arraylist *body = method->body;
        tabs++;
        for(size_t j = 0;j<body->size;j++){ 
            stmt_t *stmt = (stmt_t*) arraylist_get(body, j);
            print_content(dest_file, "%*s", tabs * 4, "");
            print_stmt(stmt, dest_file);
            print_content(dest_file, "\n");
        }

        print_content(dest_file, "\n");
        print_content(dest_file, "%*s", tabs * 4, "");
        print_stmt(method->act_call, dest_file);

        print_content(dest_file, "\n");

        tabs--;
        print_content(dest_file, "%*s%s\n\n", tabs * 4, "", "}");
    }
    print_content(dest_file, "}\n\n");

    if(dest_file != NULL) {
        fclose(dest_file);
    }
}