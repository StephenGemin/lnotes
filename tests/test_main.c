#include "notes.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <unistd.h>

/* ------------------------------------------------------------------ */
/* Minimal test runner                                                  */
/* ------------------------------------------------------------------ */

static int passes   = 0;
static int failures = 0;

#define ASSERT(expr) \
    do { \
        if (expr) { \
            passes++; \
        } else { \
            fprintf(stderr, "  FAIL [%s:%d] %s\n", __FILE__, __LINE__, #expr); \
            failures++; \
        } \
    } while (0)

#define ASSERT_STR_EQ(a, b) \
    do { \
        const char *_a = (a), *_b = (b); \
        if (strcmp(_a, _b) == 0) { \
            passes++; \
        } else { \
            fprintf(stderr, "  FAIL [%s:%d] \"%s\" != \"%s\"\n", \
                    __FILE__, __LINE__, _a, _b); \
            failures++; \
        } \
    } while (0)

#define RUN_TEST(fn) \
    do { \
        int _before = failures; \
        fn(); \
        printf("  %-44s %s\n", #fn, failures == _before ? "ok" : "FAIL"); \
    } while (0)

/* ------------------------------------------------------------------ */
/* title_to_slug                                                        */
/* ------------------------------------------------------------------ */

static void test_slug_basic(void) {
    char s[NOTES_MAX_SLUG];
    title_to_slug("Hello World", s, sizeof(s));
    ASSERT_STR_EQ(s, "hello-world");
}

static void test_slug_already_lowercase(void) {
    char s[NOTES_MAX_SLUG];
    title_to_slug("hello world", s, sizeof(s));
    ASSERT_STR_EQ(s, "hello-world");
}

static void test_slug_multiple_spaces(void) {
    char s[NOTES_MAX_SLUG];
    title_to_slug("Hello  World", s, sizeof(s));
    ASSERT_STR_EQ(s, "hello-world");
}

static void test_slug_special_chars_stripped(void) {
    char s[NOTES_MAX_SLUG];
    title_to_slug("C++: The Language!", s, sizeof(s));
    ASSERT_STR_EQ(s, "c-the-language");
}

static void test_slug_with_numbers(void) {
    char s[NOTES_MAX_SLUG];
    title_to_slug("Note 123", s, sizeof(s));
    ASSERT_STR_EQ(s, "note-123");
}

static void test_slug_leading_trailing_spaces(void) {
    char s[NOTES_MAX_SLUG];
    title_to_slug("  hello  ", s, sizeof(s));
    ASSERT_STR_EQ(s, "hello");
}

static void test_slug_no_trailing_dash(void) {
    char s[NOTES_MAX_SLUG];
    title_to_slug("Hello!", s, sizeof(s));
    ASSERT_STR_EQ(s, "hello");
}

static void test_slug_dashes_preserved(void) {
    char s[NOTES_MAX_SLUG];
    title_to_slug("hello-world", s, sizeof(s));
    ASSERT_STR_EQ(s, "hello-world");
}

static void test_slug_underscores_become_dashes(void) {
    char s[NOTES_MAX_SLUG];
    title_to_slug("hello_world", s, sizeof(s));
    ASSERT_STR_EQ(s, "hello-world");
}

static void test_slug_no_consecutive_dashes(void) {
    char s[NOTES_MAX_SLUG];
    title_to_slug("a - b", s, sizeof(s));
    ASSERT_STR_EQ(s, "a-b");
}

/* ------------------------------------------------------------------ */
/* contains_icase                                                       */
/* ------------------------------------------------------------------ */

static void test_contains_exact_match(void) {
    ASSERT(contains_icase("software", "software"));
}

static void test_contains_case_insensitive_lower(void) {
    ASSERT(contains_icase("02_software", "SOFTWARE"));
}

static void test_contains_case_insensitive_upper(void) {
    ASSERT(contains_icase("02_SOFTWARE", "software"));
}

static void test_contains_substring(void) {
    ASSERT(contains_icase("software", "soft"));
}

static void test_contains_no_match(void) {
    ASSERT(!contains_icase("02_software", "media"));
}

static void test_contains_empty_needle(void) {
    ASSERT(contains_icase("anything", ""));
}

static void test_contains_needle_longer_than_haystack(void) {
    ASSERT(!contains_icase("hi", "hello"));
}

static void test_contains_stripped_prefix_match(void) {
    /* Simulate matching after stripping "02_" prefix */
    ASSERT(contains_icase("personal_finance", "finance"));
}

/* ------------------------------------------------------------------ */
/* generate_id                                                          */
/* ------------------------------------------------------------------ */

static void test_id_is_eight_chars(void) {
    char id[NOTES_MAX_ID];
    generate_id(id, sizeof(id));
    ASSERT((int)strlen(id) == 8);
}

static void test_id_is_lowercase_hex(void) {
    char id[NOTES_MAX_ID];
    generate_id(id, sizeof(id));
    int all_hex = 1;
    for (int i = 0; id[i]; i++) {
        char c = id[i];
        if (!((c >= '0' && c <= '9') || (c >= 'a' && c <= 'f'))) {
            all_hex = 0;
            break;
        }
    }
    ASSERT(all_hex);
}

static void test_id_unique_across_calls(void) {
    char a[NOTES_MAX_ID], b[NOTES_MAX_ID], c[NOTES_MAX_ID];
    struct timespec ms = {0, 2000000}; /* 2 ms between calls */
    generate_id(a, sizeof(a));
    nanosleep(&ms, NULL);
    generate_id(b, sizeof(b));
    nanosleep(&ms, NULL);
    generate_id(c, sizeof(c));
    ASSERT(strcmp(a, b) != 0);
    ASSERT(strcmp(b, c) != 0);
    ASSERT(strcmp(a, c) != 0);
}

/* ------------------------------------------------------------------ */
/* parse_frontmatter helpers                                            */
/* ------------------------------------------------------------------ */

static char *write_tmp_note(const char *content) {
    char tmpl[] = "/tmp/obl_test_XXXXXX";
    int fd = mkstemp(tmpl);
    if (fd < 0) return NULL;
    size_t len = strlen(content);
    if (write(fd, content, len) != (ssize_t)len) {
        close(fd);
        unlink(tmpl);
        return NULL;
    }
    close(fd);
    return strdup(tmpl);
}

/* ------------------------------------------------------------------ */
/* parse_frontmatter                                                    */
/* ------------------------------------------------------------------ */

static void test_parse_all_fields(void) {
    const char *body =
        "---\n"
        "id: deadbeef\n"
        "title: My Test Note\n"
        "category: software\n"
        "date: 2026-06-23\n"
        "---\n\n"
        "# My Test Note\n\n"
        "Some body text here.\n";

    char *path = write_tmp_note(body);
    Note n;
    ASSERT(parse_frontmatter(path, &n) == 0);
    ASSERT_STR_EQ(n.id,       "deadbeef");
    ASSERT_STR_EQ(n.title,    "My Test Note");
    ASSERT_STR_EQ(n.category, "software");
    ASSERT_STR_EQ(n.date,     "2026-06-23");
    ASSERT_STR_EQ(n.description, "Some body text here.");
    unlink(path);
    free(path);
}

static void test_parse_filepath_stored(void) {
    const char *body =
        "---\nid: cafebabe\ntitle: T\ncategory: g\ndate: 2026-01-01\n---\n\n";
    char *path = write_tmp_note(body);
    Note n;
    parse_frontmatter(path, &n);
    ASSERT_STR_EQ(n.filepath, path);
    unlink(path);
    free(path);
}

static void test_parse_no_frontmatter_returns_error(void) {
    char *path = write_tmp_note("# Just a heading\n\nSome text.\n");
    Note n;
    ASSERT(parse_frontmatter(path, &n) == -1);
    unlink(path);
    free(path);
}

static void test_parse_unclosed_frontmatter_returns_error(void) {
    char *path = write_tmp_note("---\ntitle: Unclosed\n");
    Note n;
    ASSERT(parse_frontmatter(path, &n) == -1);
    unlink(path);
    free(path);
}

static void test_parse_title_with_colon_in_value(void) {
    const char *body =
        "---\n"
        "id: aabbccdd\n"
        "title: Notes on C: The Language\n"
        "category: software\n"
        "date: 2026-01-01\n"
        "---\n\nFirst line.\n";
    char *path = write_tmp_note(body);
    Note n;
    ASSERT(parse_frontmatter(path, &n) == 0);
    ASSERT_STR_EQ(n.title, "Notes on C: The Language");
    unlink(path);
    free(path);
}

static void test_parse_description_skips_headings(void) {
    const char *body =
        "---\nid: 11223344\ntitle: T\ncategory: g\ndate: 2026-01-01\n---\n\n"
        "# Main Heading\n\n"
        "## Sub Heading\n\n"
        "First real paragraph.\n";
    char *path = write_tmp_note(body);
    Note n;
    parse_frontmatter(path, &n);
    ASSERT_STR_EQ(n.description, "First real paragraph.");
    unlink(path);
    free(path);
}

static void test_parse_description_skips_blank_lines(void) {
    const char *body =
        "---\nid: aabbccdd\ntitle: T\ncategory: g\ndate: 2026-01-01\n---\n\n"
        "\n\n"
        "First content line.\n";
    char *path = write_tmp_note(body);
    Note n;
    parse_frontmatter(path, &n);
    ASSERT_STR_EQ(n.description, "First content line.");
    unlink(path);
    free(path);
}

static void test_parse_empty_body_leaves_description_empty(void) {
    const char *body =
        "---\nid: 00000000\ntitle: T\ncategory: g\ndate: 2026-01-01\n---\n";
    char *path = write_tmp_note(body);
    Note n;
    ASSERT(parse_frontmatter(path, &n) == 0);
    ASSERT(n.description[0] == '\0');
    unlink(path);
    free(path);
}

/* ------------------------------------------------------------------ */
/* main                                                                 */
/* ------------------------------------------------------------------ */

int main(void) {
    printf("title_to_slug\n");
    RUN_TEST(test_slug_basic);
    RUN_TEST(test_slug_already_lowercase);
    RUN_TEST(test_slug_multiple_spaces);
    RUN_TEST(test_slug_special_chars_stripped);
    RUN_TEST(test_slug_with_numbers);
    RUN_TEST(test_slug_leading_trailing_spaces);
    RUN_TEST(test_slug_no_trailing_dash);
    RUN_TEST(test_slug_dashes_preserved);
    RUN_TEST(test_slug_underscores_become_dashes);
    RUN_TEST(test_slug_no_consecutive_dashes);

    printf("\ncontains_icase\n");
    RUN_TEST(test_contains_exact_match);
    RUN_TEST(test_contains_case_insensitive_lower);
    RUN_TEST(test_contains_case_insensitive_upper);
    RUN_TEST(test_contains_substring);
    RUN_TEST(test_contains_no_match);
    RUN_TEST(test_contains_empty_needle);
    RUN_TEST(test_contains_needle_longer_than_haystack);
    RUN_TEST(test_contains_stripped_prefix_match);

    printf("\ngenerate_id\n");
    RUN_TEST(test_id_is_eight_chars);
    RUN_TEST(test_id_is_lowercase_hex);
    RUN_TEST(test_id_unique_across_calls);

    printf("\nparse_frontmatter\n");
    RUN_TEST(test_parse_all_fields);
    RUN_TEST(test_parse_filepath_stored);
    RUN_TEST(test_parse_no_frontmatter_returns_error);
    RUN_TEST(test_parse_unclosed_frontmatter_returns_error);
    RUN_TEST(test_parse_title_with_colon_in_value);
    RUN_TEST(test_parse_description_skips_headings);
    RUN_TEST(test_parse_description_skips_blank_lines);
    RUN_TEST(test_parse_empty_body_leaves_description_empty);

    printf("\n%d passed, %d failed\n", passes, failures);
    return failures > 0 ? 1 : 0;
}
