#ifndef UTILS_H
#define UTILS_H

#include "notes.h"

int  get_notes_dir(char *path, size_t len);
void generate_id(char *id, size_t len);
void title_to_slug(const char *title, char *slug, size_t len);
int  parse_frontmatter(const char *filepath, Note *note);
void open_in_editor(const char *filepath);
int  contains_icase(const char *haystack, const char *needle);

#endif /* UTILS_H */
