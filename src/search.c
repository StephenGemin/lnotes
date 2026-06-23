#include "notes.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <regex.h>

/* Returns 1 if the note body (after frontmatter) matches re.
   Also copies the first matching line into match_buf if non-NULL. */
static int body_matches(const char *filepath, const regex_t *re,
                        char *match_buf, size_t match_len) {
    FILE *f = fopen(filepath, "r");
    if (!f) return 0;

    char line[4096];
    int in_fm = 0, fm_done = 0, found = 0;

    while (!found && fgets(line, sizeof(line), f)) {
        size_t n = strlen(line);
        while (n > 0 && (line[n - 1] == '\n' || line[n - 1] == '\r')) line[--n] = '\0';

        if (!fm_done) {
            if (!in_fm && strcmp(line, "---") == 0) { in_fm = 1; continue; }
            if (in_fm  && strcmp(line, "---") == 0) { fm_done = 1; continue; }
            if (in_fm) continue;
        }

        if (n == 0) continue;

        if (regexec(re, line, 0, NULL, 0) == 0) {
            found = 1;
            if (match_buf && match_len > 0)
                snprintf(match_buf, match_len, "%s", line);
        }
    }

    fclose(f);
    return found;
}

static void print_field(const char *s, int width) {
    int len = (int)strlen(s);
    if (len <= width) {
        printf("%-*s", width, s);
    } else if (width > 3) {
        printf("%-.*s...", width - 3, s);
    } else {
        printf("%-.*s", width, s);
    }
}

int cmd_search(int argc, char **argv) {
    if (argc < 1) {
        fprintf(stderr,
            "usage: notes search <pattern> [-c <category>] [-t] [-b]\n"
            "  -t  Search title only\n"
            "  -b  Search body only\n"
            "  (default: search both)\n");
        return 1;
    }

    const char *pattern = argv[0];
    const char *filter_cat = NULL;
    int title_only = 0, body_only = 0;

    for (int i = 1; i < argc; i++) {
        if (strcmp(argv[i], "-c") == 0 && i + 1 < argc)
            filter_cat = argv[++i];
        else if (strcmp(argv[i], "-t") == 0)
            title_only = 1;
        else if (strcmp(argv[i], "-b") == 0)
            body_only = 1;
    }

    int search_title = !body_only;
    int search_body  = !title_only;

    regex_t re;
    int rc = regcomp(&re, pattern, REG_EXTENDED | REG_ICASE | REG_NOSUB);
    if (rc != 0) {
        char errbuf[256];
        regerror(rc, &re, errbuf, sizeof(errbuf));
        fprintf(stderr, "notes: invalid pattern: %s\n", errbuf);
        return 1;
    }

    Note *notes = malloc(NOTES_MAX_COUNT * sizeof(Note));
    if (!notes) { perror("malloc"); regfree(&re); return 1; }

    int count = collect_all_notes(filter_cat, notes, NOTES_MAX_COUNT);
    if (count < 0) { free(notes); regfree(&re); return 1; }

    int hits = 0;
    printf("%-8s  %-32s  %-20s  %s\n", "ID", "Title", "Category", "Date");
    printf("%-8s  %-32s  %-20s  %s\n",
           "--------",
           "--------------------------------",
           "--------------------",
           "----------");

    for (int i = 0; i < count; i++) {
        int title_hit = search_title &&
                        regexec(&re, notes[i].title, 0, NULL, 0) == 0;

        char match_line[512] = {0};
        int body_hit = search_body &&
                       body_matches(notes[i].filepath, &re,
                                    match_line, sizeof(match_line));

        if (!title_hit && !body_hit) continue;

        print_field(notes[i].id, 8);
        printf("  ");
        print_field(notes[i].title, 32);
        printf("  ");
        print_field(notes[i].category, 20);
        printf("  %s\n", notes[i].date);

        /* Show matched body context only when body (not title) drove the hit */
        if (body_hit && !title_hit && match_line[0]) {
            printf("          > ");
            print_field(match_line, 68);
            printf("\n");
        }

        hits++;
    }

    if (!hits) printf("No matches found.\n");

    free(notes);
    regfree(&re);
    return 0;
}
