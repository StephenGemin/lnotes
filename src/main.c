#include <stdio.h>
#include <string.h>
#include "notes.h"

static void usage(void) {
    fprintf(stderr,
        "usage: notes <command> [options]\n"
        "\n"
        "Commands:\n"
        "  add <title> [-c <category>]               Create a new note\n"
        "  rm  <id|title>                             Remove a note\n"
        "  ls  [-v] [-c <category>]                  List notes\n"
        "  search <pattern> [-c <cat>] [-t] [-b]     Search notes (regex)\n"
        "\n"
        "Search flags:\n"
        "  -t    Title only\n"
        "  -b    Body only\n"
        "  (default: search both title and body)\n"
        "\n"
        "Environment:\n"
        "  LNOTES_DIR    Override default notes directory (~/<Documents>/lnotes)\n"
        "  EDITOR        Editor used when creating notes\n"
    );
}

int main(int argc, char **argv) {
    if (argc < 2) {
        usage();
        return 1;
    }

    const char *cmd = argv[1];
    int sub_argc = argc - 2;
    char **sub_argv = argv + 2;

    if (strcmp(cmd, "add") == 0)                      
        return cmd_add(sub_argc, sub_argv);
    if (strcmp(cmd, "rm") == 0 || strcmp(cmd, "remove") == 0)
        return cmd_remove(sub_argc, sub_argv);
    if (strcmp(cmd, "ls") == 0 || strcmp(cmd, "list") == 0)                     
        return cmd_list(sub_argc, sub_argv);
    if (strcmp(cmd, "search") == 0)                   
        return cmd_search(sub_argc, sub_argv);

    fprintf(stderr, "notes: unknown command '%s'\n\n", cmd);
    usage();
    return 1;
}
