#include "notes.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <strings.h>
#include <ctype.h>
#include <errno.h>
#include <time.h>
#include <dirent.h>
#include <sys/stat.h>
#include <sys/wait.h>
#include <unistd.h>

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

/* Seed the default categories, but only the first time the store is
   created. Once base exists, the category set is whatever the user has
   made it, so re-seeding would resurrect categories they removed. */
static void init_notes_dir(const char *base) {
    if (is_dir(base)) return;
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

/* Numeric NN_ priority prefix of a category dir, or -1 if it has none. */
static int category_prefix(const char *name) {
    if (strlen(name) > 3 &&
        isdigit((unsigned char)name[0]) &&
        isdigit((unsigned char)name[1]) &&
        name[2] == '_') {
        return (name[0] - '0') * 10 + (name[1] - '0');
    }
    return -1;
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
        fprintf(stderr,
                "obl: category '%s' does not exist; "
                "create it with 'obl cat add %s'\n",
                category, category);
        return 1;
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
/* cmd_open                                                             */
/* ------------------------------------------------------------------ */

int cmd_open(int argc, char **argv) {
    if (argc < 1) {
        fprintf(stderr, "usage: obl raise <id|title>\n");
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

    char filepath[NOTES_MAX_PATH];
    snprintf(filepath, sizeof(filepath), "%s", found->filepath);
    free(notes);

    open_in_editor(filepath);
    return 0;
}

/* ------------------------------------------------------------------ */
/* cmd_view                                                             */
/* ------------------------------------------------------------------ */

int cmd_view(int argc, char **argv) {
    if (argc < 1) {
        fprintf(stderr, "usage: obl view <id|title>\n");
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

    char filepath[NOTES_MAX_PATH];
    snprintf(filepath, sizeof(filepath), "%s", found->filepath);
    free(notes);

    /* Stream the note verbatim (frontmatter and body) to stdout so it
       stays composable: `obl view <id> | glow`, `> backup.md`, etc. */
    FILE *f = fopen(filepath, "r");
    if (!f) {
        fprintf(stderr, "obl: could not open file: %s\n", filepath);
        return 1;
    }

    char buf[4096];
    size_t n;
    while ((n = fread(buf, 1, sizeof(buf), f)) > 0) {
        if (fwrite(buf, 1, n, stdout) != n) {
            fprintf(stderr, "obl: write error\n");
            fclose(f);
            return 1;
        }
    }

    fclose(f);
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

/* ------------------------------------------------------------------ */
/* cmd_export                                                           */
/* ------------------------------------------------------------------ */

static int run_archive(const char *const args[]) {
    pid_t pid = fork();
    if (pid < 0) return -1;
    if (pid == 0) {
        execvp(args[0], (char *const *)args);
        _exit(127);
    }
    int status;
    waitpid(pid, &status, 0);
    return WIFEXITED(status) ? WEXITSTATUS(status) : -1;
}

int cmd_export(int argc, char **argv) {
    char notes_dir[2048];
    if (get_notes_dir(notes_dir, sizeof(notes_dir)) != 0)
        return 1;

    /* Strip trailing slashes */
    size_t nd_len = strlen(notes_dir);
    while (nd_len > 1 && notes_dir[nd_len - 1] == '/') notes_dir[--nd_len] = '\0';

    /* Split into parent directory and basename for -C archiving.
       Use 2048-byte buffers: notes_dir is at most 2047 chars. */
    char notes_parent[2048];
    char notes_base[2048];
    const char *sl = strrchr(notes_dir, '/');
    if (sl && sl != notes_dir) {
        snprintf(notes_parent, sizeof(notes_parent), "%.*s",
                 (int)(sl - notes_dir), notes_dir);
        snprintf(notes_base, sizeof(notes_base), "%s", sl + 1);
    } else {
        snprintf(notes_parent, sizeof(notes_parent), ".");
        snprintf(notes_base, sizeof(notes_base), "%s", notes_dir);
    }

    /* Resolve output base to an absolute path.
       Keep cwd to 2047 chars so cwd + suffix fits in NOTES_MAX_PATH. */
    char outbase[NOTES_MAX_PATH];
    if (argc > 0) {
        if (argv[0][0] == '/') {
            snprintf(outbase, sizeof(outbase), "%s", argv[0]);
        } else {
            char cwd[2048];
            if (!getcwd(cwd, sizeof(cwd))) {
                fprintf(stderr, "obl: could not get current directory\n");
                return 1;
            }
            snprintf(outbase, sizeof(outbase), "%s/%s", cwd, argv[0]);
        }
    } else {
        char cwd[2048];
        if (!getcwd(cwd, sizeof(cwd))) {
            fprintf(stderr, "obl: could not get current directory\n");
            return 1;
        }
        time_t t = time(NULL);
        struct tm *tm_info = localtime(&t);
        char datestr[32];
        strftime(datestr, sizeof(datestr), "%Y-%m-%d", tm_info);
        snprintf(outbase, sizeof(outbase), "%s/oubliette-%s", cwd, datestr);
    }

    static const struct { const char *ext; const char *label; } formats[] = {
        { ".tar.gz", "tar.gz" },
        { ".zip",    "zip"    },
        { ".tar",    "tar"    },
    };
    int nformats = (int)(sizeof(formats) / sizeof(formats[0]));

    for (int i = 0; i < nformats; i++) {
        char outpath[NOTES_MAX_PATH];
        snprintf(outpath, sizeof(outpath), "%s%s", outbase, formats[i].ext);

        int rc;
        if (i == 1) {
            /* zip needs cwd=parent so the archive stores relative paths */
            pid_t pid = fork();
            if (pid < 0) {
                rc = -1;
            } else if (pid == 0) {
                if (chdir(notes_parent) != 0) _exit(127);
                execlp("zip", "zip", "-r", outpath, notes_base, (char *)NULL);
                _exit(127);
            } else {
                int status;
                waitpid(pid, &status, 0);
                rc = WIFEXITED(status) ? WEXITSTATUS(status) : -1;
            }
        } else {
            const char *flag = (i == 0) ? "-czf" : "-cf";
            const char *args[] = { "tar", flag, outpath, "-C", notes_parent, notes_base, NULL };
            rc = run_archive(args);
        }

        if (rc == 0) {
            printf("exported: %s\n", outpath);
            return 0;
        }

        unlink(outpath);
        if (i < nformats - 1)
            fprintf(stderr, "obl: %s failed, trying next format...\n", formats[i].label);
    }

    fprintf(stderr, "obl: export failed: no supported archive tool available\n");
    return 1;
}

/* ------------------------------------------------------------------ */
/* cmd_category                                                         */
/* ------------------------------------------------------------------ */

/* True if a category with this exact name (case-insensitive, ignoring
   the NN_ priority prefix) already exists under base. */
static int category_exists(const char *base, const char *name) {
    DIR *d = opendir(base);
    if (!d) return 0;
    int found = 0;
    struct dirent *e;
    while ((e = readdir(d)) != NULL) {
        if (e->d_name[0] == '.') continue;
        char path[3072];
        snprintf(path, sizeof(path), "%s/%s", base, e->d_name);
        if (!is_dir(path)) continue;
        if (strcasecmp(strip_prefix(e->d_name), name) == 0) { found = 1; break; }
    }
    closedir(d);
    return found;
}

/* After removing the category at `removed_prefix`, shift every
   lower-priority category up by one so the numbering stays contiguous.
   Renames only touch directory prefixes; note files ride along. */
static void compact_categories(const char *base, int removed_prefix) {
    for (int slot = removed_prefix; ; slot++) {
        DIR *d = opendir(base);
        if (!d) return;
        char oldname[256];
        oldname[0] = '\0';
        struct dirent *e;
        while ((e = readdir(d)) != NULL) {
            if (e->d_name[0] == '.') continue;
            if (category_prefix(e->d_name) == slot + 1) {
                snprintf(oldname, sizeof(oldname), "%s", e->d_name);
                break;
            }
        }
        closedir(d);
        if (oldname[0] == '\0') return; /* nothing left to shift */

        char oldpath[3072], newpath[3072];
        snprintf(oldpath, sizeof(oldpath), "%s/%s", base, oldname);
        snprintf(newpath, sizeof(newpath), "%s/%02d_%s",
                 base, slot, strip_prefix(oldname));
        rename(oldpath, newpath);
    }
}

static int cat_add(int argc, char **argv) {
    if (argc < 1) {
        fprintf(stderr, "usage: obl cat add <name>\n");
        return 1;
    }
    const char *name = argv[0];

    if (name[0] == '\0' || name[0] == '.' || strchr(name, '/')) {
        fprintf(stderr, "obl: invalid category name '%s'\n", name);
        return 1;
    }

    char base[2048];
    if (get_notes_dir(base, sizeof(base)) != 0) return 1;
    init_notes_dir(base);

    if (category_exists(base, name)) {
        fprintf(stderr, "obl: category '%s' already exists\n", name);
        return 1;
    }

    /* Append as the lowest priority (next prefix after the current max). */
    int prefix = next_category_prefix(base);
    char cat_dir[3072];
    snprintf(cat_dir, sizeof(cat_dir), "%s/%02d_%s", base, prefix, name);
    if (mkdir(cat_dir, 0755) != 0) {
        perror("mkdir");
        return 1;
    }

    printf("Created category %02d_%s\n", prefix, name);
    return 0;
}

static int cat_remove(int argc, char **argv) {
    if (argc < 1) {
        fprintf(stderr, "usage: obl cat rm <name>\n");
        return 1;
    }
    const char *name = argv[0];

    char base[2048];
    if (get_notes_dir(base, sizeof(base)) != 0) return 1;

    /* Locate the category by exact name (don't fuzzy-match a destructive op). */
    char target[3072];
    int found = 0, prefix = -1;
    DIR *d = opendir(base);
    if (d) {
        struct dirent *e;
        while ((e = readdir(d)) != NULL) {
            if (e->d_name[0] == '.') continue;
            char path[3072];
            snprintf(path, sizeof(path), "%s/%s", base, e->d_name);
            if (!is_dir(path)) continue;
            if (strcasecmp(strip_prefix(e->d_name), name) == 0) {
                snprintf(target, sizeof(target), "%s", path);
                prefix = category_prefix(e->d_name);
                found = 1;
                break;
            }
        }
        closedir(d);
    }

    if (!found) {
        fprintf(stderr, "obl: category '%s' does not exist\n", name);
        return 1;
    }

    /* rmdir refuses a non-empty directory, which is exactly the guard we
       want: never delete notes as a side effect of removing a category. */
    if (rmdir(target) != 0) {
        if (errno == ENOTEMPTY || errno == EEXIST)
            fprintf(stderr,
                    "obl: category '%s' is not empty; remove its notes first\n",
                    name);
        else
            perror("rmdir");
        return 1;
    }

    if (prefix >= 1)
        compact_categories(base, prefix);

    printf("Removed category %s\n", name);
    return 0;
}

int cmd_category(int argc, char **argv) {
    if (argc < 1) {
        fprintf(stderr, "usage: obl cat <add|rm> <name>\n");
        return 1;
    }
    const char *action = argv[0];
    if (strcmp(action, "add") == 0)
        return cat_add(argc - 1, argv + 1);
    if (strcmp(action, "rm") == 0 || strcmp(action, "remove") == 0)
        return cat_remove(argc - 1, argv + 1);

    fprintf(stderr, "obl: unknown category action '%s'\n", action);
    fprintf(stderr, "usage: obl cat <add|rm> <name>\n");
    return 1;
}
