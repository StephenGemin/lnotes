#ifndef NOTES_H
#define NOTES_H

#include <stddef.h>

#define NOTES_MAX_ID      9
#define NOTES_MAX_TITLE   256
#define NOTES_MAX_CAT     64
#define NOTES_MAX_DATE    32
#define NOTES_MAX_DESC    512
#define NOTES_MAX_PATH    4096
#define NOTES_MAX_SLUG    256
#define NOTES_MAX_COUNT   4096

typedef struct {
    char id[NOTES_MAX_ID];
    char title[NOTES_MAX_TITLE];
    char category[NOTES_MAX_CAT];
    char date[NOTES_MAX_DATE];
    char description[NOTES_MAX_DESC];
    char filepath[NOTES_MAX_PATH];
} Note;

/* utils.c */
int  get_notes_dir(char *path, size_t len);
void generate_id(char *id, size_t len);
void title_to_slug(const char *title, char *slug, size_t len);
int  parse_frontmatter(const char *filepath, Note *note);
void open_in_editor(const char *filepath);
int  contains_icase(const char *haystack, const char *needle);

/* notes.c */
int  collect_all_notes(const char *filter_cat, Note *notes, int max);

/* commands */
int cmd_add(int argc, char **argv);
int cmd_remove(int argc, char **argv);
int cmd_list(int argc, char **argv);
int cmd_search(int argc, char **argv);
int cmd_open(int argc, char **argv);
int cmd_view(int argc, char **argv);
int cmd_export(int argc, char **argv);
int cmd_category(int argc, char **argv);

#endif /* NOTES_H */
