#include "notes.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <strings.h>
#include <ctype.h>
#include <time.h>
#include <dirent.h>
#include <sys/stat.h>

static const char *DEFAULT_CATEGORIES[] = {
    "01_general",
    "02_software",
    "03_media",
    "04_personal_finance",
    "05_medical",
    NULL
};

static int is_dir(const char *path) {
    struct stat st;
    return stat(path, &st) == 0 && S_ISDIR(st.st_mode);
}

static void makedirs(const char *path) {
    char tmp[NOTES_MAX_PATH];
    snprintf(tmp, sizeof(tmp), "%s", path);
    for (char *p = tmp + 1; *p; p++) {
        if (*p == '/') {
            *p = '\0';
            mkdir(tmp, 0755);
            *p = '/';
        }
    }
    mkdir(tmp, 0755);
}

static void init_notes_dir(const char *base) {
    makedirs(base);
    char path[NOTES_MAX_PATH];
    for (int i = 0; DEFAULT_CATEGORIES[i]; i++) {
        snprintf(path, sizeof(path), "%s/%s", base, DEFAULT_CATEGORIES[i]);
        mkdir(path, 0755);
    }
}

/* Strip the NN_ numeric prefix from a category directory name. */
static const char *strip_prefix(const char *name) {
    if (strlen(name) > 3 &&
        isdigit((unsigned char)name[0]) &&
        isdigit((unsigned char)name[1]) &&
        name[2] == '_') {
        return name + 3;
    }
    return name;
}

/* Return the next available numeric prefix (max existing + 1). */
static int next_category_prefix(const char *base) {
    DIR *d = opendir(base);
    if (!d) return 1;
    int max = 0;
    struct dirent *e;
    while ((e = readdir(d)) != NULL) {
        if (e->d_name[0] == '.') continue;
        char path[NOTES_MAX_PATH];
        snprintf(path, sizeof(path), "%s/%s", base, e->d_name);
        if (!is_dir(path)) continue;
        int n = atoi(e->d_name);
        if (n > max) max = n;
    }
    closedir(d);
    return max + 1;
}

/* Find a category directory whose name (or stripped name) contains `cat`. */
static int find_category_dir(const char *base, const char *cat,
                              char *out, size_t out_len) {
    DIR *d = opendir(base);
    if (!d) return -1;
    int found = -1;
    struct dirent *e;
    while ((e = readdir(d)) != NULL) {
        if (e->d_name[0] == '.') continue;
        char path[NOTES_MAX_PATH];
        snprintf(path, sizeof(path), "%s/%s", base, e->d_name);
        if (!is_dir(path)) continue;
        if (contains_icase(strip_prefix(e->d_name), cat) ||
            contains_icase(e->d_name, cat)) {
            snprintf(out, out_len, "%s", path);
            found = 0;
            break;
        }
    }
    closedir(d);
    return found;
}

/* ------------------------------------------------------------------ */

int collect_all_notes(const char *filter_cat, Note *notes, int max) {
    /* Tiered buffer sizes let GCC verify concatenations statically:
       base(2047) + "/" + dname(255) = 2303 < 3072
       cat_path(3071) + "/" + dname(255) = 3327 < 4096 */
    char base[2048];
    if (get_notes_dir(base, sizeof(base)) != 0) return -1;

    DIR *d = opendir(base);
    if (!d) return 0; /* directory not yet created */

    int count = 0;
    struct dirent *e;
    while ((e = readdir(d)) != NULL && count < max) {
        if (e->d_name[0] == '.') continue;
        char cat_path[3072];
        snprintf(cat_path, sizeof(cat_path), "%s/%s", base, e->d_name);
        if (!is_dir(cat_path)) continue;

        if (filter_cat &&
            !contains_icase(strip_prefix(e->d_name), filter_cat) &&
            !contains_icase(e->d_name, filter_cat)) {
            continue;
        }

        DIR *cd = opendir(cat_path);
        if (!cd) continue;

        struct dirent *fe;
        while ((fe = readdir(cd)) != NULL && count < max) {
            if (fe->d_name[0] == '.') continue;
            size_t nlen = strlen(fe->d_name);
            if (nlen < 4 || strcmp(fe->d_name + nlen - 3, ".md") != 0) continue;

            char fpath[NOTES_MAX_PATH]; /* 3071+1+255=3327 < 4096 */
            snprintf(fpath, sizeof(fpath), "%s/%s", cat_path, fe->d_name);

            Note *n = &notes[count];
            if (parse_frontmatter(fpath, n) == 0) {
                if (n->category[0] == '\0')
                    snprintf(n->category, sizeof(n->category), "%s",
                             strip_prefix(e->d_name));
                count++;
            }
        }
        closedir(cd);
    }
    closedir(d);
    return count;
}

/* ------------------------------------------------------------------ */
/* cmd_add                                                              */
/* ------------------------------------------------------------------ */

int cmd_add(int argc, char **argv) {
    if (argc < 1) {
        fprintf(stderr, "usage: obl add <title> [-c <category>]\n");
        return 1;
    }

    const char *title = argv[0];
    const char *category = "general";

    for (int i = 1; i < argc; i++) {
        if (strcmp(argv[i], "-c") == 0 && i + 1 < argc)
            category = argv[++i];
    }

    char base[2048];
    if (get_notes_dir(base, sizeof(base)) != 0) return 1;
    init_notes_dir(base);

    char cat_dir[3072];
    if (find_category_dir(base, category, cat_dir, sizeof(cat_dir)) != 0) {
        int prefix = next_category_prefix(base);
        snprintf(cat_dir, sizeof(cat_dir), "%s/%02d_%s", base, prefix, category);
        if (mkdir(cat_dir, 0755) != 0) {
            perror("mkdir");
            return 1;
        }
    }

    char id[NOTES_MAX_ID];
    generate_id(id, sizeof(id));

    char slug[NOTES_MAX_SLUG];
    title_to_slug(title, slug, sizeof(slug));

    char filepath[NOTES_MAX_PATH];
    snprintf(filepath, sizeof(filepath), "%s/%s_%s.md", cat_dir, id, slug);

    time_t t = time(NULL);
    struct tm *tm_info = localtime(&t);
    char date[NOTES_MAX_DATE];
    strftime(date, sizeof(date), "%Y-%m-%d", tm_info);

    FILE *f = fopen(filepath, "w");
    if (!f) { perror("fopen"); return 1; }
    fprintf(f, "---\n");
    fprintf(f, "id: %s\n", id);
    fprintf(f, "title: %s\n", title);
    fprintf(f, "category: %s\n", category);
    fprintf(f, "date: %s\n", date);
    fprintf(f, "---\n\n");
    fprintf(f, "# %s\n\n", title);
    fclose(f);

    printf("Created note %s: %s\n", id, title);
    open_in_editor(filepath);
    return 0;
}

/* ------------------------------------------------------------------ */
/* cmd_remove                                                           */
/* ------------------------------------------------------------------ */

int cmd_remove(int argc, char **argv) {
    if (argc < 1) {
        fprintf(stderr, "usage: obl rm <id|title>\n");
        return 1;
    }

    const char *target = argv[0];

    Note *notes = malloc(NOTES_MAX_COUNT * sizeof(Note));
    if (!notes) { perror("malloc"); return 1; }

    int count = collect_all_notes(NULL, notes, NOTES_MAX_COUNT);
    if (count < 0) { free(notes); return 1; }

    Note *found = NULL;
    for (int i = 0; i < count; i++) {
        if (strcmp(notes[i].id, target) == 0 ||
            strcasecmp(notes[i].title, target) == 0) {
            found = &notes[i];
            break;
        }
    }

    if (!found) {
        fprintf(stderr, "obl: '%s' not found\n", target);
        free(notes);
        return 1;
    }

    printf("Remove '%s' (%s)? [y/N] ", found->title, found->id);
    fflush(stdout);

    char ans[8];
    if (!fgets(ans, sizeof(ans), stdin) || (ans[0] != 'y' && ans[0] != 'Y')) {
        printf("Aborted.\n");
        free(notes);
        return 0;
    }

    if (remove(found->filepath) != 0) {
        perror("remove");
        free(notes);
        return 1;
    }

    printf("Removed %s: %s\n", found->id, found->title);
    free(notes);
    return 0;
}

/* ------------------------------------------------------------------ */
/* cmd_list                                                             */
/* ------------------------------------------------------------------ */

static int cmp_by_date_desc(const void *a, const void *b) {
    /* Newest first */
    return strcmp(((const Note *)b)->date, ((const Note *)a)->date);
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

int cmd_list(int argc, char **argv) {
    int verbose = 0;
    const char *filter_cat = NULL;

    for (int i = 0; i < argc; i++) {
        if (strcmp(argv[i], "-v") == 0)
            verbose = 1;
        else if (strcmp(argv[i], "-c") == 0 && i + 1 < argc)
            filter_cat = argv[++i];
    }

    Note *notes = malloc(NOTES_MAX_COUNT * sizeof(Note));
    if (!notes) { perror("malloc"); return 1; }

    int count = collect_all_notes(filter_cat, notes, NOTES_MAX_COUNT);
    if (count < 0) { free(notes); return 1; }

    if (count == 0) {
        printf("No notes found.\n");
        free(notes);
        return 0;
    }

    qsort(notes, count, sizeof(Note), cmp_by_date_desc);

    printf("%-8s  %-32s  %-20s  %s\n", "ID", "Title", "Category", "Date");
    printf("%-8s  %-32s  %-20s  %s\n",
           "--------",
           "--------------------------------",
           "--------------------",
           "----------");

    for (int i = 0; i < count; i++) {
        print_field(notes[i].id, 8);
        printf("  ");
        print_field(notes[i].title, 32);
        printf("  ");
        print_field(notes[i].category, 20);
        printf("  %s\n", notes[i].date);
        if (verbose && notes[i].description[0]) {
            printf("          %s\n", notes[i].description);
        }
    }

    free(notes);
    return 0;
}
