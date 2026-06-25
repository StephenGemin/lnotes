#include <stdio.h>
#include <string.h>
#include "notes.h"

static void usage(void) {
    fprintf(stderr,
        "usage: obl <command> [options]\n"
        "\n"
        "Commands:\n"
        "  add    <title> [-c <category>]            Create a new note\n"
        "  raise  <id|title>                         Open a note in $EDITOR\n"
        "  view   <id|title>                         Print a note to stdout\n"
        "  rm     <id|title>                         Remove a note\n"
        "  ls     [-v] [-c <category>]               List notes\n"
        "  search <pattern> [-c <cat>] [-t] [-b]     Search notes (regex)\n"
        "  export [output-base]                       Archive all notes to a file\n"
        "  cat    <add|rm> <name>                    Create or remove a category\n"
        "\n"
        "Search flags:\n"
        "  -t    Title only\n"
        "  -b    Body only\n"
        "  (default: search both title and body)\n"
        "\n"
        "Environment:\n"
        "  OBL_DIR       Override default notes directory (~/<Documents>/oubliette)\n"
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
    if (strcmp(cmd, "raise") == 0)
        return cmd_open(sub_argc, sub_argv);
    if (strcmp(cmd, "view") == 0)
        return cmd_view(sub_argc, sub_argv);
    if (strcmp(cmd, "export") == 0)
        return cmd_export(sub_argc, sub_argv);
    if (strcmp(cmd, "cat") == 0 || strcmp(cmd, "category") == 0)
        return cmd_category(sub_argc, sub_argv);

    fprintf(stderr, "obl: unknown command '%s'\n\n", cmd);
    usage();
    return 1;
}
