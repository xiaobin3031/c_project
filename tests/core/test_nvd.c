#include "../test_runner.h"
#include "../../src/core/http/http.h"
#include <string.h>
#include <stdlib.h>

int test_nvd_download() {
    // 这里可以添加对 NVD 下载功能的测试代码
    uri_t *url = parse_uri("https://nvd.nist.gov/feeds/json/cve/2.0/nvdcve-2.0-recent.json.gz");
    // uri_t *url = parse_uri("https://nvd.nist.gov");
    http_t *http = malloc(sizeof(http_t));
    http->method = HTTP_METHOD_GET;
    http->uri = url;
    http->heads = malloc(sizeof(http_head_t));
    http->heads->accept = strdup("text/html,application/xhtml+xml,application/xml;q=0.9,image/avif,image/webp,image/apng,*/*;q=0.8,application/signed-exchange;v=b3;q=0.7");
    http->heads->keys = arraylist_new(10);
    http->heads->values = arraylist_new(10);
    arraylist_add(http->heads->keys, strdup("Accept-Encoding"));
    arraylist_add(http->heads->values, strdup("gzip, deflate, br"));
    http->body = NULL;
    http->body_size = 0;
    http_res_t *res = send_http(http);
    http_res_print(res);

    http_free(http);
    http_res_free(res);
    return SUCCESS;
}

test_case_t test_nvd_cases[] = {
    {"NVD Download Test", test_nvd_download},
    {NULL, NULL} // 哨兵
};