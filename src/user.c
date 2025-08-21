#define _GNU_SOURCE
#include <sched.h>
#include <unistd.h>
#include <grp.h>
#include <fcntl.h>
#include <limits.h>
#include <errno.h>
#include <string.h>

#include "log.h"
#include "user.h"

// Lets the parent process know that the user namespace is started.
// The parent calls user_namespace_set_user to update the uid_map / gid_map.
// If successful, setgroups, setresgid, and setresuid are called in this
// function by the child. setgroups and setresgid are necessary because of two
// separate group mechanisms on Linux. The function assumes that every uid has a
// corresponding gid, which is often the case.
int user_namespace_init(uid_t uid, int fd) {
    int unshared = unshare(CLONE_NEWUSER);
    int result = 0;

    log_debug("setting user namespace...");

    log_debug("writing to socket...");
    if (write(fd, &unshared, sizeof(unshared)) != sizeof(unshared)) {
        log_error("failed to write socket %d: %m", fd);
        return -1;
    }

    log_debug("reading from socket...");
    if (read(fd, &result, sizeof(result)) != sizeof(result)) {
        log_error("failed to read from socket %d: %m", fd);
        return -1;
    }

    if (result) {
        return -1;
    }

    log_debug("switching to uid %d / gid %d...", uid, uid);

    log_debug("setting uid and gid mappings...");
    // setgroups() sets the supplementary group IDs for the calling process.
    // Appropriate privileges are required (see the description of the EPERM error, below).
    // The size argument specifies the number of supplementary group IDs in the
    // buffer pointed to by list
    if (setgroups(1, &uid) < 0) {
        log_error("failed to set supplementary group id to %d: %s", uid,
                  strerror(errno));
        return -1;
    }
    // setresuid() sets the real user ID, the effective user ID, and the saved
    // set-user-ID of the calling process.
    //
    // setresgid() sets the real, effective, and saved group IDs of the process
    if (setresgid(uid, uid, uid) || setresuid(uid, uid, uid)) {
        log_error("failed to set uid %d / gid %d mappings: %m", uid, uid);
        return -1;
    }

    log_debug("user namespace set");

    return 0;
}

// Listens for the child process to request setting uid / gid, then updates the
// uid_map / gid_map for the child process to use. uid_map and gid_map are a
// Linux kernel mechanism for mapping uids and gids between the parent and child
// parent. The parent process must be privileged to set the uid_map / gid_map.
int user_namespace_prepare_mappings(pid_t pid, int fd) {
    int map_fd = 0;
    int unshared = -1;

    log_debug("updating uid_map / gid_map...");
    log_debug("retrieving user namespaces status...");
    if (read(fd, &unshared, sizeof(unshared)) != sizeof(unshared)) {
        log_error("failed to retrieve status from socket %d: %m", fd);
        return -1;
    }

    if (!unshared) {
        char dir[PATH_MAX] = {0};

        log_debug("user namespaces enabled");

        log_debug("writing uid_map / gid_map...");
        for (char **file = (char *[]){"uid_map", "gid_map", 0}; *file; file++) {
            if (snprintf(dir, sizeof(dir), "/proc/%d/%s", pid, *file) >
                (int)sizeof(dir)) {
                log_error("failed to setup dir %s: %m", dir);
                return -1;
            }

            log_debug("writing %s...", dir);
            if ((map_fd = open(dir, O_WRONLY)) == -1) {
                log_error("failed to open %s: %m", dir);
                return -1;
            }

            log_debug("writing settings...");

            // The first number is the starting uid / gid of the parent
            // namespace, the second number is the starting uid / gid of the child
            // namespace, and the third number is the number of uids / gids to map.
            // This configuration tells the kernel that the parent uid / gid 0 is
            // mapped to USER_NAMESPACE_UID_CHILD_RANGE_START uid of the child
            //
            // For the child process with PID <pid>, establish the following ID
            // mapping: The child's UID range, starting at USER_NAMESPACE_UID_CHILD_RANGE_START
            // and spanning for USER_NAMESPACE_UID_CHILD_RANGE_SIZE IDs, should be
            // treated as the parent's UID range, starting at USER_NAMESPACE_UID_PARENT_RANGE_START.
            //
            // This is the critical step that provides security isolation. It allows the
            // child process to run as root (UID 0) inside its own isolated user namespace,
            // while the kernel sees its operations as being performed by a non-privileged
            // user (e.g., UID 100000) on the host system. This ensures that the process
            // can't perform privileged operations on the host, even if it has full
            // administrative control within its own namespace.
            if (dprintf(map_fd, "%d %d %d\n", USER_NAMESPACE_UID_PARENT_RANGE_START,
                        USER_NAMESPACE_UID_CHILD_RANGE_START,
                        USER_NAMESPACE_UID_CHILD_RANGE_SIZE) == -1) {
                log_error("failed to write uid_map '%d': %m", map_fd);
                close(map_fd);
                return -1;
            }

            close(map_fd);
        }

        log_debug("uid_map and gid_map updated");
    }

    log_debug("updating socket...");
    if (write(fd, &(int){0}, sizeof(int)) != sizeof(int)) {
        log_error("failed to update socket %d: %m", fd);
        return -1;
    }

    return 0;
}
