#include "../test_runner.h"
#include "../../modules/shentou/cve.h"
#include <string.h>
#include <stdlib.h>

int test_cve_download() {
    // 这里可以添加对 CVE 下载功能的测试代码
    long f_size = download_cve23("nvdcve-2.0-recent.json.gz", "https://nvd.nist.gov/feeds/json/cve/2.0/nvdcve-2.0-recent.json.gz");
    return f_size > 0 ? SUCCESS : FAILURE;
}

int test_cve_unzip() {
    // 这里可以添加对 CVE 解压功能的测试代码
    save_cve23("nvdcve-2.0-recent.json.gz");
    return SUCCESS;

}

test_case_t test_cve_cases[] = {
    {"CVE Download Test", test_cve_download},
    {"CVE Unzip Test", test_cve_unzip},
    {NULL, NULL} // 哨兵
};