#ifndef __CONTAINER_H__
#define __CONTAINER_H__

#include <sys/types.h>

enum {
    // The stack size for the container
    CONTAINER_STACK_SIZE = (1024 * 1024),
};

enum {
    ARGV_CMD_INDEX          = 0,
    ARGV_ARG_INDEX          = 1,
    ARGV_TERMINATED_INDEX   = 2,
    ARGV_MAX                = 3,
};

// Represents the configuration for a container.
typedef struct {
    uid_t uid;
    int fd;
    const char *hostname;
    const char *cmd;
    const char *mnt;
    char *argv[ARGV_MAX];
} container_config;

// Initializes the container.
int container_init(container_config *config, char *stack);

// Waits for the container to exit.
int container_wait(int container_pid);

// Stops the container.
void container_stop(int container_pid);


#endif
