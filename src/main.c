#include <stdio.h>
#include <libgen.h>

#include <argtable3.h>

#include "version.h"
#include "log/log.h"

enum {
    // ARGTABLE_ARG_MAX is the maximum number of arguments
    ARGTABLE_ARG_MAX = 20
};

/* global arg_xxx structs */
struct arg_lit *help, *version;
struct arg_int *uid;
struct arg_str *mnt;
struct arg_str *cmd;
struct arg_str *arg;
struct arg_lit *vrb;
struct arg_end *end;

int main(int argc, char **argv) {
    int exitcode = 0;
    int nerrors = 0;
    const char *progname = basename(argv[0]);

    // the global arg_xxx structs are initialised within the argtable
    void *argtable[] = {
        help    = arg_litn(NULL, "help", 0, 1, "display this help and exit"),
        version = arg_litn(NULL, "version", 0, 1, "display version info and exit"),
        uid     = arg_intn("u", "uid", "<n>", 1, 1, "uid and gid of the user in the container"),
        mnt     = arg_strn("m", "mnt", "<s>", 1, 1, "directory to mount as root in the container"),
        cmd     = arg_strn("c", "cmd", "<s>", 1, 1, "command to run in the container"),
        arg     = arg_strn("a", "arg", "<s>", 0, 1, "argument to pass to the command"),
        vrb     = arg_litn("v", "verbosity", 0, 1, "verbose output"),
        end     = arg_end(ARGTABLE_ARG_MAX),
    };

    nerrors = arg_parse(argc, argv, argtable);

    // special case: '--help' takes precedence over error reporting
    if (help->count > 0) {
        printf("Usage: %s", progname);
        arg_print_syntax(stdout, argtable, "\n");
        arg_print_glossary(stdout, argtable, "  %-25s %s\n");
        goto exit;
    }

    if (version->count > 0) {
        printf("%s %s\n", progname, PROJECT_VERSION);
        goto exit;
    }

    // If the parser returned any errors then display them and exit
    if (nerrors > 0) {
        // Display the error details contained in the arg_end struct.
        arg_print_errors(stdout, end, progname);
        printf("Try '%s --help' for more information.\n", progname);
        exitcode = 1;
        goto exit;
    }

    // Set verbosity level
    if (vrb->count > 0)
        log_set_level(LOG_TRACE);
    else
        log_set_level(LOG_INFO);

exit:
    arg_freetable(argtable, sizeof(argtable) / sizeof(argtable[0]));
    return exitcode;
}
