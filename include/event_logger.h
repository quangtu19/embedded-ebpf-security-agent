#ifndef EVENT_LOGGER_H
#define EVENT_LOGGER_H
int logger_init(const char *path);
int logger_write(const char *json_line);
void logger_close(void);
#endif

