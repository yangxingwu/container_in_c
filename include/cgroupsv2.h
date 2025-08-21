#ifndef __CGROUPSV2_H__
#define __CGROUPSV2_H__

#include <unistd.h>

// Used for cgroups limits initialization
#define CGROUPS_MEMORY_MAX      "1G"
#define CGROUPS_CPU_WEIGHT      "256"
#define CGROUPS_PIDS_MAX        "64"
#define CGROUPS_CGROUP_PROCS    "cgroup.procs"

enum {
    CGROUPS_CONTROL_FIELD_SIZE = 256
};

// Initializes cgroups for the hostname
int cgroupsv2_init(char *hostname, pid_t pid);

// Cleans up cgroups for the hostname
int cgroupsv2_free(char *hostname);

#endif
