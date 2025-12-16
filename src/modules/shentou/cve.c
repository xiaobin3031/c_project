#include "cve.h"
#include "../../core/list/arraylist.h"
#include "../../core/utils.h"
#include "../../core/json/json.h"
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <curl/curl.h>
#include <zlib.h>
#include <sqlite3.h>

#define CHUNK_SIZE 16384

void save_cve23_to_db (FILE *fp) {

}

size_t write_cb(void *ptr, size_t size, size_t nmemb, void *userdata) {
    FILE *fp = (FILE *)userdata;
    return fwrite(ptr, size, nmemb, fp);
}

arraylist *cpe23_db_load(const char *path) {
    FILE *fp = fopen(path, "r");
    if(fp == NULL) return NULL;

    arraylist *db = arraylist_new(1000);
    char *line;
    while((line = fgetline(fp))) {
        if(line[0] == '#') continue;

        char *cve_id = strtok(line, ":");
        if(!cve_id) {
            free(line);
            continue;
        }
        char *vulnerable = strtok(NULL, ":");
        char *part = strtok(NULL, ":");
        char *vendor = strtok(NULL, ":");
        char *product = strtok(NULL, ":");
        if(!part || !vendor || !product) {
            free(line);
            continue;
        }
        char *min_version = strtok(NULL, ":");
        char *max_version = strtok(NULL, ":");
        char *update = strtok(NULL, ":");
        char *edition = strtok(NULL, ":");
        char *language = strtok(NULL, ":");
        char *sw_edition = strtok(NULL, ":");
        char *target_sw = strtok(NULL, ":");
        char *target_hw = strtok(NULL, ":");
        char *other = strtok(NULL, ":");
        cpe23_t *cpe23 = malloc(sizeof(cpe23_t));
        cpe23->cve_id = strdup(cve_id);
        if(vulnerable) {
            cpe23->vulnerable = atoi(vulnerable);
        } else {
            cpe23->vulnerable = 0;
        }
        cpe23->part = strdup(part);
        cpe23->vendor = strdup(vendor);
        cpe23->product = strdup(product);
        if(min_version) {
            cpe23->min_version = strdup(min_version);
        }
        if(max_version) {
            cpe23->max_version = strdup(max_version);
        }
        if(update) {
            cpe23->update = strdup(update);
        }
        if(edition) {
            cpe23->edition = strdup(edition);
        }
        if(language) {
            cpe23->language = strdup(language);
        }
        if(sw_edition) {
            cpe23->sw_edition = strdup(sw_edition);
        }
        if(target_sw) {
            cpe23->target_sw = strdup(target_sw);
        }
        if(target_hw) {
            cpe23->target_hw = strdup(target_hw);
        }
        if(other) {
            cpe23->other = strdup(other);
        }
        arraylist_add(db, cpe23);
        free(line);
    }

    fclose(fp);
    return db;
}

long download_cve23(const char *store_file, const char *url) {
    CURL *curl;
    CURLcode res;
    FILE *fp;
    long f_size = 0;

    curl_global_init(CURL_GLOBAL_DEFAULT);
    curl = curl_easy_init();
    if(!curl) return 0;

    fp = fopen(store_file, "w");
    if(fp == NULL) return 0;

    fseek(fp, 0, SEEK_END);
    f_size = ftell(fp);
    // 文件存在，就直接返回
    if(f_size > 0) {
        printf("CPE23 data already exists, skipping download.\n");
        fclose(fp);
        curl_easy_cleanup(curl);
        curl_global_cleanup();
        return f_size;
    }

    curl_easy_setopt(curl, CURLOPT_URL, url);

    // 写文件
    curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, write_cb);
    curl_easy_setopt(curl, CURLOPT_WRITEDATA, fp);

    curl_easy_setopt(curl, CURLOPT_FOLLOWLOCATION, 1L);
    curl_easy_setopt(curl, CURLOPT_USERAGENT, "libcurl-nvd-fetch/1.0");

    // 超时
    curl_easy_setopt(curl, CURLOPT_CONNECTTIMEOUT, 10L);

    res = curl_easy_perform(curl);
    if(res != CURLE_OK) {
        fprintf(stderr, "curl_easy_perform() failed: %s\n", curl_easy_strerror(res));
    }else {
        printf("Downloaded CPE23 data to %s\n", store_file);
        fseek(fp, 0, SEEK_END);
        f_size = ftell(fp);
    }

    fclose(fp);
    curl_easy_cleanup(curl);
    curl_global_cleanup();

    // 如果目标文件存在，则删除
    // 删掉 .gz 后缀
    char output_file[256];
    snprintf(output_file, sizeof(output_file), "%s", store_file);
    char *dot = strrchr(output_file, '.');
    if(dot && strcmp(dot, ".gz") == 0) {
        *dot = '\0';
    }
    remove(output_file);
    return f_size;
}

int save_cve23(const char *store_file) {
    // 解压gz文件
    gzFile gzfp;
    FILE *output_fp;
    unsigned char buffer[CHUNK_SIZE];
    int bytes_read;
    int result = 0;

    // 删掉 .gz 后缀
    char output_file[256];
    snprintf(output_file, sizeof(output_file), "%s", store_file);
    char *dot = strrchr(output_file, '.');
    if(dot && strcmp(dot, ".gz") == 0) {
        *dot = '\0';
    }
    output_fp = fopen(output_file, "wb");
    if(!output_fp) {
        fprintf(stderr, "错误：无法创建输出文件 %s\n", output_file);
        gzclose(gzfp);
        return -1;
    }
    fseek(output_fp, 0, SEEK_END);
    if(ftell(output_fp) > 0) {
        printf("文件已存在，跳过\n");
        return result;
    }

    gzfp = gzopen(store_file, "rb");
    if(!gzfp) {
        perror("gzopen error");
        return -1;
    }

    // 解压文件
    while((bytes_read = gzread(gzfp, buffer, CHUNK_SIZE)) > 0) {
        if(fwrite(buffer, 1, bytes_read, output_fp) != bytes_read) {
            fprintf(stderr, "错误：无法写入输出文件 %s\n", output_file);
            result = -1;
            break;
        }
    }

    if(bytes_read < 0) {
        fprintf(stderr, "错误：解压失败 %s\n", store_file);
        result = -1;
    }

    fclose(output_fp);
    gzclose(gzfp);

    if(result == 0) {
        printf("解压完成: %s -> %s\n", store_file, output_file);
    }

    return result;
}

json_value *parse_cve23(const char *json_str) {
    json_value *root = json_parse(json_str);
}

void cpe23_free(void *cpe23_value) {

}

void cpe23_print(void *cpe23_value) {

}