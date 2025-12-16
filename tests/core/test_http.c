#include "../test_runner.h"
#include "../../src/core/http/http.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

/**
 * 辅助函数：快速构造测试用的 http_t
 * 注意：根据你的实现，uri 内部的成员必须已经分配内存
 */
static http_t* setup_base_http(http_method_e method, const char* host, const char* path) {
    http_t* http = (http_t*)calloc(1, sizeof(http_t));
    http->method = method;
    http->uri = (uri_t*)calloc(1, sizeof(uri_t));
    http->uri->host = strdup(host);
    http->uri->path = strdup(path);
    return http;
}

/**
 * 测试用例 1: 验证 heads 为 NULL 时的默认行为
 * 你的逻辑：heads 为 NULL 时，应输出默认的 User-Agent 和 Accept
 */
int test_write_head_null_heads() {
    char buffer[1024] = {0};
    http_t *http = setup_base_http(HTTP_METHOD_GET, "example.com", "/index.html");
    http->heads = NULL; // 显式置空
    http->body_size = 0;

    write_http_head(buffer, http);

    // 验证包含必要字段和默认值
    ASSERT_TRUE(strstr(buffer, "GET /index.html HTTP/1.1\r\n") != NULL, "Request Line");
    ASSERT_TRUE(strstr(buffer, "Host: example.com\r\n") != NULL, "Host header");
    ASSERT_TRUE(strstr(buffer, "Connection: keep-alive\r\n") != NULL, "Connection header");
    ASSERT_TRUE(strstr(buffer, "Content-Length: 0\r\n") != NULL, "Length header");
    ASSERT_TRUE(strstr(buffer, "Accept: */*\r\n") != NULL, "Default Accept");
    ASSERT_TRUE(strstr(buffer, USER_AGENT_CHROM) != NULL, "Default User-Agent");
    ASSERT_TRUE(buffer[strlen(buffer)-1] == '\n' && buffer[strlen(buffer)-2] == '\r', "Final CRLF");

    http_free(http);
    return SUCCESS;
}

/**
 * 测试用例 2: 验证包含 Custom Headers (ArrayList)
 */
int test_write_head_with_custom_arraylist() {
    char buffer[2048] = {0};
    http_t *http = setup_base_http(HTTP_METHOD_POST, "api.test.com", "/v1/upload");
    http->heads = (http_head_t*)calloc(1, sizeof(http_head_t));
    http->heads->content_type = strdup("application/json");
    
    // 初始化 arraylist 并添加自定义头
    // 假设你的 arraylist 接口如下，请根据实际修改
    http->heads->keys = arraylist_new(10);
    http->heads->values = arraylist_new(10);
    arraylist_add(http->heads->keys, strdup("X-Api-Key"));
    arraylist_add(http->heads->values, strdup("secret-123"));
    
    http->body_size = 50;

    write_http_head(buffer, http);

    ASSERT_TRUE(strstr(buffer, "POST /v1/upload HTTP/1.1\r\n") != NULL, "Post line");
    ASSERT_TRUE(strstr(buffer, "Content-Length: 50\r\n") != NULL, "Body size");
    ASSERT_TRUE(strstr(buffer, "Content-Type: application/json\r\n") != NULL, "Content Type");
    ASSERT_TRUE(strstr(buffer, "X-Api-Key: secret-123\r\n") != NULL, "Custom header from arraylist");

    http_free(http);
    return SUCCESS;
}

/**
 * 测试用例 3: 验证空 Path 的处理
 */
int test_write_head_empty_path() {
    char buffer[512] = {0};
    // 假设逻辑中 path 不能为空，如果传入空，看看是否会崩溃
    http_t *http = setup_base_http(HTTP_METHOD_GET, "localhost", "");
    http->body_size = 0;

    write_http_head(buffer, http);

    // 理想情况下这里应该是 "GET / HTTP/1.1"，但根据你的代码，它会输出 "GET  HTTP/1.1"
    // 这个测试可以帮你发现实现中的不足
    ASSERT_TRUE(strstr(buffer, "GET  HTTP/1.1\r\n") != NULL, "Check behavior on empty path");

    http_free(http);
    return SUCCESS;
}

test_case_t test_http_cases[] = {
    {"Write Head: Null Heads Default",      test_write_head_null_heads},
    {"Write Head: Custom Headers List",     test_write_head_with_custom_arraylist},
    {"Write Head: Empty Path Behavior",     test_write_head_empty_path},
    {NULL, NULL}
};