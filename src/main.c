#include <stdio.h>
#include <unistd.h>
#include <signal.h>
#include <stdbool.h>

#include "event_logger.h"

static volatile bool running = true;

void handle_signal(int sig) {
    (void)sig;
    running = false;
}

int main(int argc, char **argv) {
    (void)argc;
    (void)argv;

    signal(SIGINT, handle_signal);
    signal(SIGTERM, handle_signal);

    if (logger_init("events.log") != 0) {
        fprintf(stderr, "Failed to initialize logger\n");
        return 1;
    }

    logger_write("{\"event_type\":\"AGENT_STARTED\",\"severity\":\"INFO\"}");

    while (running) {
        logger_write("{\"event_type\":\"HEARTBEAT\",\"severity\":\"INFO\"}");
        sleep(5);
    }

    logger_write("{\"event_type\":\"AGENT_STOPPED\",\"severity\":\"INFO\"}");
    logger_close();

    return 0;
}
