#include "event_logger.h"

#include <stdio.h>

static FILE *log_file = NULL;

int logger_init(const char *path)
{
    log_file = fopen(path, "a");
    if (!log_file) {
        perror("fopen");
        return -1;
    }

    return 0;
}

int logger_write(const char *json_line)
{
    if (!log_file)
        return -1;

    fprintf(log_file, "%s\n", json_line);
    fflush(log_file);

    return 0;
}

void logger_close(void)
{
    if (log_file) {
        fclose(log_file);
        log_file = NULL;
    }
}
