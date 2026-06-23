#include "notes.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <strings.h>
#include <ctype.h>
#include <time.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/wait.h>

int get_notes_dir(char *path, size_t len) {
    const char *env = getenv("LNOTES_DIR");
    if (env) {
        snprintf(path, len, "%s", env);
        return 0;
    }

    /* Respect XDG on Linux */
    const char *xdg = getenv("XDG_DOCUMENTS_DIR");
    if (xdg) {
        snprintf(path, len, "%s/lnotes", xdg);
        return 0;
    }

    const char *home = getenv("HOME");
    if (!home) {
        fprintf(stderr, "notes: HOME not set\n");
        return -1;
    }
    snprintf(path, len, "%s/Documents/lnotes", home);
    return 0;
}

void generate_id(char *id, size_t len) {
    struct timespec ts;
    clock_gettime(CLOCK_REALTIME, &ts);
    unsigned int seed = (unsigned int)((unsigned long)ts.tv_nsec ^ (unsigned long)ts.tv_sec ^ (unsigned long)getpid());
    srand(seed);
    unsigned int h = seed ^ (unsigned int)rand();
    /* xorshift finalizer */
    h ^= h << 13;
    h ^= h >> 17;
    h ^= h << 5;
    snprintf(id, len, "%08x", h);
}

void title_to_slug(const char *title, char *slug, size_t len) {
    size_t j = 0;
    for (size_t i = 0; title[i] && j + 1 < len; i++) {
        unsigned char c = (unsigned char)title[i];
        if (isalnum(c)) {
            slug[j++] = (char)tolower(c);
        } else if ((c == ' ' || c == '-' || c == '_') &&
                   j > 0 && slug[j - 1] != '-') {
            slug[j++] = '-';
        }
    }
    while (j > 0 && slug[j - 1] == '-') j--;
    slug[j] = '\0';
}

int contains_icase(const char *haystack, const char *needle) {
    if (!needle || !*needle) return 1;
    size_t nlen = strlen(needle);
    for (; *haystack; haystack++) {
        if (strncasecmp(haystack, needle, nlen) == 0) return 1;
    }
    return 0;
}

int parse_frontmatter(const char *filepath, Note *note) {
    FILE *f = fopen(filepath, "r");
    if (!f) return -1;

    memset(note, 0, sizeof(*note));
    snprintf(note->filepath, sizeof(note->filepath), "%s", filepath);

    char line[2048];
    int in_fm = 0, fm_done = 0;

    while (fgets(line, sizeof(line), f)) {
        size_t n = strlen(line);
        while (n > 0 && (line[n - 1] == '\n' || line[n - 1] == '\r')) line[--n] = '\0';

        if (!in_fm) {
            if (strcmp(line, "---") == 0) { in_fm = 1; continue; }
            break; /* no frontmatter */
        }
        if (strcmp(line, "---") == 0) { fm_done = 1; break; }

        char *colon = strchr(line, ':');
        if (!colon) continue;
        *colon = '\0';
        char *val = colon + 1;
        while (*val == ' ') val++;

        if (strcmp(line, "id") == 0)
            snprintf(note->id, sizeof(note->id), "%s", val);
        else if (strcmp(line, "title") == 0)
            snprintf(note->title, sizeof(note->title), "%s", val);
        else if (strcmp(line, "category") == 0)
            snprintf(note->category, sizeof(note->category), "%s", val);
        else if (strcmp(line, "date") == 0)
            snprintf(note->date, sizeof(note->date), "%s", val);
    }

    /* Capture first non-blank, non-heading body line as description */
    if (fm_done) {
        while (fgets(line, sizeof(line), f)) {
            size_t n = strlen(line);
            while (n > 0 && (line[n - 1] == '\n' || line[n - 1] == '\r')) line[--n] = '\0';
            if (n == 0 || line[0] == '#') continue;
            snprintf(note->description, sizeof(note->description), "%.*s",
                     (int)(sizeof(note->description) - 1), line);
            break;
        }
    }

    fclose(f);
    return fm_done ? 0 : -1;
}

void open_in_editor(const char *filepath) {
    const char *editor = getenv("EDITOR");
    if (!editor) editor = getenv("VISUAL");
    if (!editor) editor = "vi";

    pid_t pid = fork();
    if (pid == 0) {
        execlp(editor, editor, filepath, (char *)NULL);
        perror("exec");
        _exit(127);
    }
    if (pid > 0) {
        int status;
        waitpid(pid, &status, 0);
    }
}
