#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include "config.h"

static void trim_newline(char *s)
{
    size_t len = strlen(s);
    if (len > 0 && s[len - 1] == '\n') {
        s[len - 1] = '\0';
    }
}

int config_load(const char *path, struct agent_config *cfg)
{
    FILE *fp;
    char line[512];

    if (!path || !cfg) {
        return -1;
    }

    /* Default config */
    strncpy(cfg->log_path, "events.log", sizeof(cfg->log_path) - 1);
    cfg->log_path[sizeof(cfg->log_path) - 1] = '\0';
    cfg->heartbeat_interval_sec = 5;

    fp = fopen(path, "r");
    if (!fp) {
        return -1;
    }

    while (fgets(line, sizeof(line), fp)) {
        trim_newline(line);

        if (strstr(line, "log_path:") != NULL) {
            char *value = strchr(line, ':');
            if (value) {
                value++;
                while (*value == ' ') {
                    value++;
                }

                strncpy(cfg->log_path, value, sizeof(cfg->log_path) - 1);
                cfg->log_path[sizeof(cfg->log_path) - 1] = '\0';
            }
        }

        if (strstr(line, "heartbeat_interval_sec:") != NULL) {
            char *value = strchr(line, ':');
            if (value) {
                value++;
                while (*value == ' ') {
                    value++;
                }

                cfg->heartbeat_interval_sec = atoi(value);
                if (cfg->heartbeat_interval_sec <= 0) {
                    cfg->heartbeat_interval_sec = 5;
                }
            }
        }
    }

    fclose(fp);
    return 0;
}
