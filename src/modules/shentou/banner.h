#pragma once

#define BANNER_MAX 4096

typedef enum {
    BANNER_DNS,
    BANNER_FTP,
    BANNER_HTTP,
    BANNER_HTTPS,
    BANNER_IMAP,
    BANNER_ICMP,
    BANNER_MYSQL,
    BANNER_NTP,
    BANNER_POP3,
    BANNER_REDIS,
    BANNER_RDP,
    BANNER_SSH,
    BANNER_SMTP,
    BANNER_SMB,
    BANNER_TELNET,

    BANNER_UNKNOWN
} banner_type ;

typedef struct {
    char service[64];
    char protocol[32];
    char version[128];
} service_info;

typedef struct {
    banner_type type;
    char *banner;
    int port;

    service_info *service;
} banner_t ;

typedef struct {
    char *service;
    char *protocol;
    char **match_strings;
    int match_count;
    char *version_regexp;
} finger_rule;

typedef struct {
    finger_rule **rules;
    int rule_count;
} finger_rule_db;

banner_t *grab_banner(const char *ip, int port, const finger_rule_db *db);

void banner_free(banner_t *banner);
void banner_print(banner_t *banner);

finger_rule_db *finger_rule_db_load();
void finger_rule_db_free(finger_rule_db *db);
void finger_rule_db_print(finger_rule_db *db);
void finger_rule_free(finger_rule *rule);
void finger_rule_print(finger_rule *rule);

void service_info_print(service_info *info);