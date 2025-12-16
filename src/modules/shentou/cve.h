#pragma once
#include "../../core/list/arraylist.h"
#include "../../core/json/json.h"

typedef struct {
    char *vendor;
    char *product;
    char *v_op;
    char *version;

    char **cve_ids;
    int cve_count;
} cve_t;

typedef struct {
    char *cve_id;
    // 是否受影响 1 是 0 否
    int vulnerable;
    char *part;
    char *vendor;
    char *product;
    char *min_version;
    char *max_version;
    char *update;
    char *edition;
    char *language;
    // 软件类型（商业版、免费版等）
    char *sw_edition;
    // 运行环境（android, windows）
    char *target_sw;
    // CPU/架构
    char *target_hw;
    char *other;
} cpe23_t;

arraylist *cpe23_db_load(const char *path);

// 加载cpe数据
long download_cve23(const char *store_file, const char *url);
int save_cve23(const char *store_file);

void cpe23_free(void *cpe23_value);

void cpe23_print(void *cpe23_value);