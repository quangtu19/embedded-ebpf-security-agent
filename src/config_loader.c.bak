#include "config.h"

#include <ctype.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

static char *ltrim(char *s)
{
    while (*s && isspace((unsigned char)*s))
        s++;
    return s;
}

static void rtrim(char *s)
{
    size_t len = strlen(s);

    while (len > 0 && isspace((unsigned char)s[len - 1])) {
        s[len - 1] = '\0';
        len--;
    }
}

static int parse_bool(const char *s)
{
    if (s == NULL)
        return 0;

    if (strcmp(s, "true") == 0)
        return 1;

    if (strcmp(s, "yes") == 0)
        return 1;

    if (strcmp(s, "1") == 0)
        return 1;

    return 0;
}

static void set_default_config(struct agent_config *cfg)
{
    memset(cfg, 0, sizeof(*cfg));

    strncpy(cfg->log_path, "events.log", sizeof(cfg->log_path) - 1);
    cfg->heartbeat_interval_sec = 5;

    cfg->xdp_enable = 0;
    strncpy(cfg->xdp_interface, "enp0s8", sizeof(cfg->xdp_interface) - 1);
    strncpy(cfg->xdp_mode, "skb", sizeof(cfg->xdp_mode) - 1);
    strncpy(cfg->xdp_object_path, "bpf/xdp_filter.bpf.o",
            sizeof(cfg->xdp_object_path) - 1);
    cfg->xdp_stats_interval_sec = 5;
}

int config_load(const char *path, struct agent_config *cfg)
{
    FILE *fp;
    char line[512];

    if (path == NULL || cfg == NULL)
        return -1;

    set_default_config(cfg);

    fp = fopen(path, "r");
    if (fp == NULL)
        return -1;

    while (fgets(line, sizeof(line), fp) != NULL) {
        char *p;
        char *key;
        char *value;

        p = strchr(line, '#');
        if (p)
            *p = '\0';

        rtrim(line);
        key = ltrim(line);

        if (*key == '\0')
            continue;

        p = strchr(key, ':');
        if (!p)
            continue;

        *p = '\0';
        value = ltrim(p + 1);
        rtrim(key);
        rtrim(value);

        if (*value == '\0')
            continue;

        if (strcmp(key, "log_path") == 0) {
            strncpy(cfg->log_path, value, sizeof(cfg->log_path) - 1);
            cfg->log_path[sizeof(cfg->log_path) - 1] = '\0';
        } else if (strcmp(key, "heartbeat_interval_sec") == 0) {
            cfg->heartbeat_interval_sec = atoi(value);
            if (cfg->heartbeat_interval_sec <= 0)
                cfg->heartbeat_interval_sec = 5;
        } else if (strcmp(key, "xdp_enable") == 0) {
            cfg->xdp_enable = parse_bool(value);
        } else if (strcmp(key, "xdp_interface") == 0) {
            strncpy(cfg->xdp_interface, value, sizeof(cfg->xdp_interface) - 1);
            cfg->xdp_interface[sizeof(cfg->xdp_interface) - 1] = '\0';
        } else if (strcmp(key, "xdp_mode") == 0) {
            strncpy(cfg->xdp_mode, value, sizeof(cfg->xdp_mode) - 1);
            cfg->xdp_mode[sizeof(cfg->xdp_mode) - 1] = '\0';
        } else if (strcmp(key, "xdp_object_path") == 0) {
            strncpy(cfg->xdp_object_path, value,
                    sizeof(cfg->xdp_object_path) - 1);
            cfg->xdp_object_path[sizeof(cfg->xdp_object_path) - 1] = '\0';
        } else if (strcmp(key, "xdp_stats_interval_sec") == 0) {
            cfg->xdp_stats_interval_sec = atoi(value);
            if (cfg->xdp_stats_interval_sec <= 0)
                cfg->xdp_stats_interval_sec = 5;
        }
    }

    fclose(fp);
    return 0;
}
