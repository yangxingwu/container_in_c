#include <sys/syscall.h>
#include <unistd.h>
#include <stdlib.h>
#include <sys/mount.h>
#include <string.h>
#include <libgen.h>
#include <limits.h>

#include "log/log.h"
#include "mount.h"

// Changes the root filesystem of the current process and its children.
// The directory at new_root becomes the new root (/), and the original
// root filesystem is moved to put_old.
//
// glibc does not provide a wrapper for it.
static long pivot_root(const char *new_root, const char *put_old) {
    log_debug("calling pivot_root syscall...");
    return syscall(SYS_pivot_root, new_root, put_old);
}

// Restricts access to resources the process has in its own mount namespace:
// - Create a temporary directory and one inside of it
// - Bind mount of the user argument onto the temporary directory
// - pivot_root makes the bind mount the new root and mounts the old root onto
// the inner temporary directory
// - umount the old root and remove the inner temporary directory.
int mount_set(char *mnt) {
    log_debug("setting mount...");

    // MS_PRIVATE makes the bind mount invisible outside of the namespace
    // MS_REC makes the mount recursive
    //
    // Ensures that all subsequent mount operations are contained within the
    // current mount namespace.
    //
    // It remounts the existing root filesystem (/) with the MS_PRIVATE flag.
    // The MS_PRIVATE flag prevents any mount or unmount events in this namespace
    // from propagating to the parent namespace (the host system), ensuring
    // complete isolation.
    //
    // The MS_REC flag applies this change recursively to all existing
    // sub-mounts under /.
    log_debug("remounting with MS_PRIVATE...");
    if (mount(NULL, "/", NULL, MS_REC | MS_PRIVATE, NULL)) {
        log_error("failed to mount /: %m");
        return -1;
    }
    log_debug("remounted");

    log_debug("creating temporary directory and...");
    char mount_dir[] = "/tmp/barco.XXXXXX";
    // The mkdtemp() function generates a uniquely named temporary directory
    // from template. The last six characters of template must be XXXXXX and
    // these are replaced with a string that makes the directory name unique.
    // The directory is then created with permissions 0700.  Since it will be
    // modified, template must not be a string constant, but should be declared
    // as a character array.
    if (!mkdtemp(mount_dir)) {
        log_error("failed to create directory %s: %m", mount_dir);
        return -1;
    }

    // Bind mounts the directory specified by the mnt variable onto the newly
    // created temporary directory (mount_dir). The MS_BIND flag creates a bind
    // mount, which is essentially a mirror of the original directory, and the
    // MS_PRIVATE flag ensures this specific bind mount also remains
    // isolated within the current namespace.
    log_debug("bind mount...");
    if (mount(mnt, mount_dir, NULL, MS_BIND | MS_PRIVATE, NULL)) {
        log_error("failed to bind mount on %s: %m", mnt);
        return -1;
    }

    // A second temporary directory, inner_mount_dir, is created inside the
    // first one. This directory will temporarily hold the old root filesystem
    // after the pivot_root call.
    log_debug("creating inner directory...");
    char inner_mount_dir[] = "/tmp/barco.XXXXXX/oldroot.XXXXXX";
    memcpy(inner_mount_dir, mount_dir, sizeof(mount_dir) - 1);
    if (!mkdtemp(inner_mount_dir)) {
        log_error("failed to create inner directory %s: %m", inner_mount_dir);
        return -1;
    }

    // The heart of the container's file system isolation. It atomically
    // changes the root filesystem of the current process and its children.
    // The directory at mount_dir becomes the new root (/), and the original
    // root filesystem is moved to inner_mount_dir.
    log_debug("pivot root with %s, %s...", mount_dir, inner_mount_dir);
    if (pivot_root(mount_dir, inner_mount_dir)) {
        log_error("failed to pivot root with %s, %s: %m", mount_dir,
                  inner_mount_dir);
        return -1;
    }

    // Cleanup
    //
    // After pivot_root moves the old root filesystem (the host system's root)
    // into a temporary directory, that directory still exists as a mount point.
    // If left untouched, a malicious or buggy process inside the container
    // could potentially navigate back out of its new root and access files on
    // the host system, completely compromising the container's isolation.
    // Unmounting this temporary directory is the only way to sever that connection.
    //
    // The umount() command first unmounts the old root directory from its
    // temporary location. This disassociates it from the container's file system.
    // The rmdir() command then removes the temporary directory itself, which is now
    // just an empty folder. It does not touch the files that were inside it, as
    // they were part of the original mount point

    log_debug("unmounting old root...");
    char *old_root_dir = basename(inner_mount_dir);
    char old_root[PATH_MAX];
    snprintf(old_root, sizeof(old_root), "/%s", old_root_dir);

    // The process's current working directory is changed to the new root (/).
    // This is necessary because the old working directory might no longer
    // exist or be accessible after the pivot_root call.
    log_debug("changing directory to /...");
    if (chdir("/")) {
        log_error("failed to chdir to /: %m");
        return -1;
    }

    // The old root filesystem, which is now located at the path stored in
    // old_root, is unmounted. The MNT_DETACH flag performs a lazy unmount,
    // which allows the unmount to happen as soon as the filesystem is no longer busy.
    log_debug("unmounting...");
    if (umount2(old_root, MNT_DETACH)) {
        log_error("failed to umount %s: %m", old_root);
        return -1;
    }

    // Finally, the temporary directory that held the old root filesystem is
    // removed. This completes the cleanup, leaving a fully isolated, clean
    // root filesystem for the container.
    log_debug("removing temporary directories...");
    if (rmdir(old_root)) {
        log_error("failed to rmdir %s: %m", old_root);
        return -1;
    }

    log_debug("mount set");
    return 0;
}
