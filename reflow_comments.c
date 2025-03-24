/*
 * reflow_comments.c
 *
 * A tool to reformat Python comments in source files.
 *
 * This program performs several tasks on Python files:
 *
 * 1. **Rule A: Process Commented-Out Print Statements**
 *    If a full-line comment starts with a print statement (e.g. "# print(...)"),
 *    the tool uncomments it, runs it through the Black formatter (with a 79-char limit),
 *    and then wraps the formatted code in a triple-quoted block (""" ... """).
 *
 * 2. **Rule B: Split Inline Comments**
 *    If a code line contains an inline comment (where '#' is not the first non-space)
 *    and the line exceeds 79 characters, it splits the line into two:
 *      - The first line is the comment (prefixed with "# " and preserving indentation).
 *      - The second line is the code.
 *
 * 3. **Rule C: Merge Consecutive Full-Line Comments**
 *    Consecutive full-line comments (lines starting with '#' after indentation)
 *    that exceed 79 characters are merged into a single block. The text is flattened,
 *    then rewrapped using a heuristic (via the wrap_text helper) and enclosed in triple quotes.
 *
 * 4. **Rule D: Reflow Existing Triple-Quoted Comment Blocks**
 *    If an existing triple-quoted block is found, its inner content is merged and rewrapped
 *    so that no resulting line (taking common indentation into account) exceeds 79 characters.
 *
 * The program also removes trailing whitespace from comment blocks.
 *
 * Usage:
 *   reformat_print <path>
 *
 * If <path> is a file, that single file is processed.
 * If <path> is a directory, the tool recursively finds all files with a ".py" extension
 * and processes each file.
 *
 * Dependencies:
 *   - Requires the Python code formatter "black" to be available in the system PATH.
 *
 * Compile with:
 *   gcc -O2 -g -o reformat_print reflow_comments.c
 *
 * Then, for example, install:
 *   sudo mv reformat_print /usr/local/bin/
 *
 * Always back up your files or use version control before running this tool.
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <unistd.h>
#include <stdbool.h>
#include <sys/stat.h>
#include <dirent.h>

#define BUFFER_SIZE 8192
#define MAX_LEN 79

// Forward declarations of processing functions.
char *process_commented_print_line(const char *original_line);
char *split_inline_comment(const char *original_line);
char *merge_comment_block(char **lines, size_t start, size_t count, size_t *end_index);
char *wrap_text(const char *text, int max_width);
char *process_triple_quote_block(char **lines, size_t start, size_t count, size_t *end_index);

// ----------------- Helper Functions -----------------

/* strip_crlf()
 * Removes trailing newline and carriage return characters from the string.
 */
void strip_crlf(char *s) {
    size_t len = strlen(s);
    while (len > 0 && (s[len-1] == '\n' || s[len-1] == '\r'))
        s[--len] = '\0';
}

/* rtrim()
 * Trims trailing whitespace from the string.
 */
void rtrim(char *s) {
    size_t len = strlen(s);
    while (len > 0 && isspace((unsigned char)s[len-1]))
        s[--len] = '\0';
}

/* ltrim()
 * Trims leading whitespace (including newlines) in-place.
 */
void ltrim(char *s) {
    size_t index = 0;
    while (s[index] && isspace((unsigned char)s[index]))
        index++;
    if (index > 0)
        memmove(s, s+index, strlen(s+index) + 1);
}

/* is_full_line_comment()
 * Returns true if, after optional indentation, the line begins with '#'.
 */
bool is_full_line_comment(const char *line) {
    const char *p = line;
    while (*p && isspace((unsigned char)*p))
        p++;
    return (*p == '#');
}

/* is_commented_print()
 * Heuristically checks if a full-line comment (after indentation) is a commented-out print statement.
 */
bool is_commented_print(const char *line) {
    const char *p = line;
    while (*p && isspace((unsigned char)*p))
        p++;
    if (*p != '#')
        return false;
    p++; // skip '#'
    while (*p && isspace((unsigned char)*p))
        p++;
    return (strncmp(p, "print(", 6) == 0);
}

/* check_black_available()
 * Returns true if the "black" command is found in the PATH.
 */
bool check_black_available() {
    int ret = system("command -v black > /dev/null 2>&1");
    return (ret == 0);
}

/* run_black()
 * Runs the "black" formatter on a temporary file with a maximum line length of MAX_LEN.
 */
bool run_black(const char *tmp_filename) {
    char cmd[BUFFER_SIZE];
    snprintf(cmd, BUFFER_SIZE, "black --line-length %d %s > /dev/null 2>&1", MAX_LEN, tmp_filename);
    int ret = system(cmd);
    return (ret == 0);
}

/* read_file_contents()
 * Reads the entire contents of a file pointer and returns it as a dynamically allocated string.
 * The caller is responsible for freeing the returned string.
 */
char *read_file_contents(FILE *fp) {
    size_t capacity = BUFFER_SIZE, total = 0;
    char *result = malloc(capacity);
    if (!result) return NULL;
    result[0] = '\0';
    char buf[BUFFER_SIZE];
    while (fgets(buf, sizeof(buf), fp)) {
        size_t len = strlen(buf);
        if (total + len + 1 > capacity) {
            capacity = (total + len + 1) * 2;
            char *tmp = realloc(result, capacity);
            if (!tmp) {
                free(result);
                return NULL;
            }
            result = tmp;
        }
        strcpy(result + total, buf);
        total += len;
    }
    return result;
}

/* wrap_text()
 * Wraps a single long string (with no newlines) into multiple lines,
 * ensuring that each line is at most max_width characters.
 * It uses a heuristic: it searches backward and forward for a break character.
 */
char *wrap_text(const char *text, int max_width) {
    size_t len = strlen(text);
    if (len <= (size_t)max_width)
        return strdup(text);
    int found = -1;
    const char *break_chars = " ,.:;";
    for (int i = max_width; i >= 0; i--) {
        if (strchr(break_chars, text[i])) {
            found = i;
            break;
        }
    }
    if (found == -1) {
        for (int i = max_width + 1; i < (int)len && i < max_width + 10; i++) {
            if (strchr(break_chars, text[i])) {
                found = i;
                break;
            }
        }
    }
    if (found == -1)
        found = max_width;
    char *first_part = strndup(text, found);
    int skip = 0;
    while (found + skip < (int)len && strchr(break_chars, text[found + skip]))
        skip++;
    while (found + skip < (int)len && isspace((unsigned char)text[found + skip]))
        skip++;
    char *remaining = strdup(text + found + skip);
    char *wrapped_remaining = wrap_text(remaining, max_width);
    free(remaining);
    size_t total_size = strlen(first_part) + 2 + strlen(wrapped_remaining) + 1;
    char *result = malloc(total_size);
    if (!result) {
        free(first_part);
        free(wrapped_remaining);
        return NULL;
    }
    snprintf(result, total_size, "%s\n%s", first_part, wrapped_remaining);
    free(first_part);
    free(wrapped_remaining);
    return result;
}

// ----------------- Processing Rules -----------------

/* Rule A: Process a commented-out print statement.
 * If a full-line comment starts with "print(" (after '#' and whitespace) and the line exceeds MAX_LEN,
 * this function extracts the code (removing the '#' and spaces), writes it to a temporary file,
 * runs Black on it, reads back the formatted output, and then wraps the result in a triple-quoted block.
 * Returns a newly allocated string (with trailing newline) if modified, or NULL otherwise.
 */
char *process_commented_print_line(const char *original_line) {
    char buffer[BUFFER_SIZE];
    strncpy(buffer, original_line, BUFFER_SIZE-1);
    buffer[BUFFER_SIZE-1] = '\0';
    strip_crlf(buffer);
    if (strlen(buffer) <= MAX_LEN)
        return NULL;
    if (!is_commented_print(buffer))
        return NULL;
    char *hash_ptr = strchr(buffer, '#');
    if (!hash_ptr)
        return NULL;
    int code_len = hash_ptr - buffer;
    if (code_len >= MAX_LEN)
        return NULL;
    char *comment_content = hash_ptr + 1;
    while (*comment_content && isspace((unsigned char)*comment_content))
        comment_content++;
    if (strncmp(comment_content, "def ", 4) == 0)
        return NULL;
    int indent = 0;
    while (buffer[indent] && isspace((unsigned char)buffer[indent]))
        indent++;
    char *code = hash_ptr + 1;
    while (*code && isspace((unsigned char)*code))
        code++;
    char tmp_filename[] = "/tmp/blacktmpXXXXXX";
    int fd = mkstemp(tmp_filename);
    if (fd == -1) {
        perror("mkstemp");
        return NULL;
    }
    FILE *tmp_file = fdopen(fd, "w");
    if (!tmp_file) {
        perror("fdopen");
        close(fd);
        return NULL;
    }
    fprintf(tmp_file, "%s\n", code);
    fclose(tmp_file);
    if (!run_black(tmp_filename)) {
        fprintf(stderr, "Error: Failed to run black. Ensure it is in your PATH.\n");
        remove(tmp_filename);
        return NULL;
    }
    FILE *fp = fopen(tmp_filename, "r");
    if (!fp) {
        perror("fopen temporary file");
        remove(tmp_filename);
        return NULL;
    }
    char *formatted = read_file_contents(fp);
    fclose(fp);
    remove(tmp_filename);
    if (!formatted)
        return NULL;
    strip_crlf(formatted);
    if (formatted[0] == '#') {
        memmove(formatted, formatted+1, strlen(formatted));
        while (*formatted && isspace((unsigned char)*formatted))
            memmove(formatted, formatted+1, strlen(formatted));
    }
    // Build the triple-quoted block.
    size_t new_capacity = BUFFER_SIZE;
    char *new_content = malloc(new_capacity);
    if (!new_content) {
        free(formatted);
        return NULL;
    }
    size_t offset = 0;
#define ENSURE_CAP(extra) \
    do { \
        if (offset + (extra) >= new_capacity) { \
            new_capacity = (offset + (extra)) * 2; \
            char *tmp = realloc(new_content, new_capacity); \
            if (!tmp) { free(new_content); free(formatted); return NULL; } \
            new_content = tmp; \
        } \
    } while (0)
    ENSURE_CAP(indent + 5);
    for (int i = 0; i < indent; i++)
        new_content[offset++] = ' ';
    offset += snprintf(new_content+offset, new_capacity-offset, "\"\"\"\n");
    char *saveptr;
    char *line_ptr = strtok_r(formatted, "\n", &saveptr);
    while (line_ptr) {
        rtrim(line_ptr); // Remove trailing whitespace.
        ENSURE_CAP(indent + strlen(line_ptr) + 5);
        for (int i = 0; i < indent; i++)
            new_content[offset++] = ' ';
        offset += snprintf(new_content+offset, new_capacity-offset, "%s\n", line_ptr);
        line_ptr = strtok_r(NULL, "\n", &saveptr);
    }
    ENSURE_CAP(indent + 5);
    for (int i = 0; i < indent; i++)
        new_content[offset++] = ' ';
    offset += snprintf(new_content+offset, new_capacity-offset, "\"\"\"\n");
    new_content[offset] = '\0';
#undef ENSURE_CAP
    free(formatted);
    return new_content;
}

/* Rule B: Process an inline comment on a code line.
 * If a line contains code followed by an inline comment (i.e. '#' is not the first non-space)
 * and the total length exceeds MAX_LEN, split it into two lines:
 *   - The first line is the comment (moved above with "# " prefix and same indentation).
 *   - The second line is the code portion.
 * Returns a new string if modified, or NULL if no change is needed.
 */
char *split_inline_comment(const char *original_line) {
    char buffer[BUFFER_SIZE];
    strncpy(buffer, original_line, BUFFER_SIZE-1);
    buffer[BUFFER_SIZE-1] = '\0';
    strip_crlf(buffer);
    if (strlen(buffer) <= MAX_LEN)
        return NULL;
    char *hash_ptr = strchr(buffer, '#');
    if (!hash_ptr)
        return NULL;
    const char *p = buffer;
    while (*p && isspace((unsigned char)*p))
        p++;
    if (*p == '#')
        return NULL;  // Already a full-line comment.
    int code_len = hash_ptr - buffer;
    char code_part[BUFFER_SIZE];
    strncpy(code_part, buffer, code_len);
    code_part[code_len] = '\0';
    rtrim(code_part);
    char *comment_part = hash_ptr;
    if (*comment_part == '#')
        comment_part++;
    while (*comment_part && isspace((unsigned char)*comment_part))
        comment_part++;
    int indent = 0;
    while (buffer[indent] && isspace((unsigned char)buffer[indent]))
        indent++;
    size_t new_capacity = BUFFER_SIZE;
    char *new_content = malloc(new_capacity);
    if (!new_content)
        return NULL;
    size_t offset = 0;
#define ENSURE_SPLIT(extra) \
    do { \
        if (offset + (extra) >= new_capacity) { \
            new_capacity = (offset + (extra)) * 2; \
            char *tmp = realloc(new_content, new_capacity); \
            if (!tmp) { free(new_content); return NULL; } \
            new_content = tmp; \
        } \
    } while (0)
    ENSURE_SPLIT(indent + strlen(comment_part) + 5);
    for (int i = 0; i < indent; i++)
        new_content[offset++] = ' ';
    offset += snprintf(new_content+offset, new_capacity-offset, "# %s\n", comment_part);
    ENSURE_SPLIT(strlen(code_part) + 5);
    offset += snprintf(new_content+offset, new_capacity-offset, "%s\n", code_part);
    new_content[offset] = '\0';
#undef ENSURE_SPLIT
    return new_content;
}

/* Rule C: Merge consecutive full-line comments into a single block.
 * Merges comment lines from index 'start' until the first non-comment line.
 * Flattens the merged content, rewraps it using wrap_text (available width = MAX_LEN - common_indent),
 * and encloses it in a triple-quoted block (""" ... """) with the common indentation.
 * Updates *end_index to the index after the merged block.
 */
char *merge_comment_block(char **lines, size_t start, size_t count, size_t *end_index) {
    int common_indent = 1000;
    size_t i;
    for (i = start; i < count; i++) {
        const char *line = lines[i];
        const char *p = line;
        while (*p && isspace((unsigned char)*p))
            p++;
        if (*p != '#')
            break;
        int indent = p - line;
        if (indent < common_indent)
            common_indent = indent;
    }
    *end_index = i;
    size_t merged_capacity = BUFFER_SIZE;
    char *merged = malloc(merged_capacity);
    if (!merged) return NULL;
    merged[0] = '\0';
    for (size_t j = start; j < i; j++) {
        char *line = lines[j];
        char *content = line + common_indent;
        if (*content == '#') content++;
        while (*content && isspace((unsigned char)*content))
            content++;
        size_t needed = strlen(content) + 2;
        if (strlen(merged) + needed >= merged_capacity) {
            merged_capacity = (strlen(merged) + needed) * 2;
            char *tmp = realloc(merged, merged_capacity);
            if (!tmp) { free(merged); return NULL; }
            merged = tmp;
        }
        strcat(merged, content);
        strcat(merged, " ");
    }
    rtrim(merged);
    int avail_width = MAX_LEN - common_indent;
    char *wrapped = wrap_text(merged, avail_width);
    free(merged);
    if (!wrapped)
        return NULL;
    // Remove any extra leading whitespace/newlines.
    ltrim(wrapped);
    size_t final_capacity = BUFFER_SIZE;
    char *final_content = malloc(final_capacity);
    if (!final_content) { free(wrapped); return NULL; }
    size_t final_offset = 0;
#define ENSURE_FINAL(extra) \
    do { \
        if (final_offset + (extra) >= final_capacity) { \
            final_capacity = (final_offset + (extra)) * 2; \
            char *tmp = realloc(final_content, final_capacity); \
            if (!tmp) { free(final_content); free(wrapped); return NULL; } \
            final_content = tmp; \
        } \
    } while (0)
    ENSURE_FINAL(common_indent + 5);
    for (int k = 0; k < common_indent; k++)
        final_content[final_offset++] = ' ';
    final_offset += snprintf(final_content+final_offset, final_capacity-final_offset, "\"\"\"\n");
    char *saveptr;
    char *token = strtok_r(wrapped, "\n", &saveptr);
    while (token) {
        rtrim(token); // trim trailing whitespace on each wrapped line.
        ENSURE_FINAL(common_indent + strlen(token) + 5);
        for (int k = 0; k < common_indent; k++)
            final_content[final_offset++] = ' ';
        final_offset += snprintf(final_content+final_offset, final_capacity-final_offset, "%s\n", token);
        token = strtok_r(NULL, "\n", &saveptr);
    }
    ENSURE_FINAL(common_indent + 5);
    for (int k = 0; k < common_indent; k++)
        final_content[final_offset++] = ' ';
    final_offset += snprintf(final_content+final_offset, final_capacity-final_offset, "\"\"\"\n");
    final_content[final_offset] = '\0';
#undef ENSURE_FINAL
    free(wrapped);
    return final_content;
}

/* Rule D: Process an existing triple-quoted comment block.
 * If a line (after indentation) starts with """ then it is the beginning of a block.
 * The function gathers all lines until the closing """ is found, merges the inner content,
 * reflows it (using wrap_text with available width = MAX_LEN - common_indent),
 * trims trailing whitespace from each rewrapped line, and reassembles the block with opening
 * and closing triple quotes.
 * Updates *end_index to be the index after the block.
 */
char *process_triple_quote_block(char **lines, size_t start, size_t count, size_t *end_index) {
    const char *line = lines[start];
    int common_indent = 0;
    while (line[common_indent] && isspace((unsigned char)line[common_indent]))
        common_indent++;
    if (strstr(line, "\"\"\"") == NULL)
        return NULL;
    char *open_ptr = strstr(lines[start], "\"\"\"");
    open_ptr += 3; // Skip the opening triple quotes.
    size_t content_capacity = BUFFER_SIZE, content_total = 0;
    char *content = malloc(content_capacity);
    if (!content) return NULL;
    content[0] = '\0';
    if (*open_ptr) {
        strcat(content, open_ptr);
        strcat(content, " ");
        content_total = strlen(content);
    }
    size_t i;
    for (i = start + 1; i < count; i++) {
        if (strstr(lines[i], "\"\"\"") != NULL) {
            char *close_ptr = strstr(lines[i], "\"\"\"");
            size_t len = close_ptr - lines[i];
            char temp[BUFFER_SIZE];
            strncpy(temp, lines[i], len);
            temp[len] = '\0';
            if (strlen(temp) > 0) {
                if (content_total + strlen(temp) + 2 > content_capacity) {
                    content_capacity = (content_total + strlen(temp) + 2) * 2;
                    char *tmp = realloc(content, content_capacity);
                    if (!tmp) { free(content); return NULL; }
                    content = tmp;
                }
                strcat(content, temp);
                strcat(content, " ");
                content_total = strlen(content);
            }
            i++;
            break;
        } else {
            char temp[BUFFER_SIZE];
            strcpy(temp, lines[i]);
            strip_crlf(temp);
            if (content_total + strlen(temp) + 2 > content_capacity) {
                content_capacity = (content_total + strlen(temp) + 2) * 2;
                char *tmp = realloc(content, content_capacity);
                if (!tmp) { free(content); return NULL; }
                content = tmp;
            }
            strcat(content, temp);
            strcat(content, " ");
            content_total = strlen(content);
        }
    }
    *end_index = i;
    int avail_width = MAX_LEN - common_indent;
    char *wrapped = wrap_text(content, avail_width);
    free(content);
    if (!wrapped)
        return NULL;
    ltrim(wrapped);
    size_t final_capacity = BUFFER_SIZE;
    char *final_block = malloc(final_capacity);
    if (!final_block) { free(wrapped); return NULL; }
    size_t final_offset = 0;
#define ENSURE_FINAL(extra) \
    do { \
        if (final_offset + (extra) >= final_capacity) { \
            final_capacity = (final_offset + (extra)) * 2; \
            char *tmp = realloc(final_block, final_capacity); \
            if (!tmp) { free(final_block); free(wrapped); return NULL; } \
            final_block = tmp; \
        } \
    } while (0)
    ENSURE_FINAL(common_indent + 5);
    for (int k = 0; k < common_indent; k++)
        final_block[final_offset++] = ' ';
    final_offset += snprintf(final_block+final_offset, final_capacity-final_offset, "\"\"\"\n");
    char *saveptr2;
    char *token = strtok_r(wrapped, "\n", &saveptr2);
    while (token) {
        rtrim(token);
        ENSURE_FINAL(common_indent + strlen(token) + 5);
        for (int k = 0; k < common_indent; k++)
            final_block[final_offset++] = ' ';
        final_offset += snprintf(final_block+final_offset, final_capacity-final_offset, "%s\n", token);
        token = strtok_r(NULL, "\n", &saveptr2);
    }
    ENSURE_FINAL(common_indent + 5);
    for (int k = 0; k < common_indent; k++)
        final_block[final_offset++] = ' ';
    final_offset += snprintf(final_block+final_offset, final_capacity-final_offset, "\"\"\"\n");
    final_block[final_offset] = '\0';
#undef ENSURE_FINAL
    free(wrapped);
    return final_block;
}

// ----------------- File/Directory Processing -----------------

/* process_file()
 * Processes a single Python file by reading its lines into memory, applying the transformation rules,
 * and then writing the modified content back to the file.
 */
void process_file(const char *filename) {
    FILE *fin = fopen(filename, "r");
    if (!fin) {
        perror(filename);
        return;
    }
    char **lines = NULL;
    size_t count = 0, capacity = 0;
    char buf[BUFFER_SIZE];
    while (fgets(buf, sizeof(buf), fin)) {
        if (count >= capacity) {
            capacity = (capacity == 0) ? 256 : capacity * 2;
            char **tmp = realloc(lines, capacity * sizeof(char *));
            if (!tmp) {
                perror("realloc");
                fclose(fin);
                return;
            }
            lines = tmp;
        }
        lines[count] = strdup(buf);
        if (!lines[count]) {
            perror("strdup");
            fclose(fin);
            return;
        }
        count++;
    }
    fclose(fin);

    char tmp_out[] = "/tmp/reflow_outputXXXXXX";
    int fd_out = mkstemp(tmp_out);
    if (fd_out == -1) {
        perror("mkstemp output");
        return;
    }
    FILE *fout = fdopen(fd_out, "w");
    if (!fout) {
        perror("fdopen output");
        close(fd_out);
        return;
    }

    int changes = 0;
    for (size_t i = 0; i < count; i++) {
        char *new_line = NULL;
        // Rule D: Process existing triple-quoted blocks.
        const char *trimmed = lines[i];
        while (*trimmed && isspace((unsigned char)*trimmed))
            trimmed++;
        if (strncmp(trimmed, "\"\"\"", 3) == 0) {
            size_t end_index;
            new_line = process_triple_quote_block(lines, i, count, &end_index);
            if (new_line) {
                printf("Processed triple-quoted block in %s (lines %zu-%zu).\n", filename, i+1, end_index);
                fputs(new_line, fout);
                free(new_line);
                changes++;
                i = end_index - 1;
                continue;
            }
        }
        // Rule A: Process commented-out print statements.
        new_line = process_commented_print_line(lines[i]);
        if (new_line) {
            printf("Modified commented-out print in %s at line %zu.\n", filename, i+1);
            fputs(new_line, fout);
            free(new_line);
            changes++;
            continue;
        }
        // Rule B: Split inline comments.
        new_line = split_inline_comment(lines[i]);
        if (new_line) {
            printf("Split inline comment in %s at line %zu.\n", filename, i+1);
            fputs(new_line, fout);
            free(new_line);
            changes++;
            continue;
        }
        // Rule C: Merge consecutive full-line comments.
        if (is_full_line_comment(lines[i]) && strlen(lines[i]) > MAX_LEN) {
            size_t end_index;
            new_line = merge_comment_block(lines, i, count, &end_index);
            if (new_line) {
                printf("Merged comment block in %s from line %zu to %zu.\n", filename, i+1, end_index);
                fputs(new_line, fout);
                free(new_line);
                changes++;
                i = end_index - 1;
                continue;
            }
        }
        // Otherwise, write the line unchanged.
        fputs(lines[i], fout);
    }
    for (size_t i = 0; i < count; i++)
        free(lines[i]);
    free(lines);
    fclose(fout);

    if (rename(tmp_out, filename) != 0) {
        perror("rename");
        return;
    }
    printf("Processed %s: %d modification(s) made.\n", filename, changes);
}

/* process_directory()
 * Recursively processes all files with a ".py" extension in the given directory.
 */
void process_directory(const char *dir_path) {
    DIR *dir = opendir(dir_path);
    if (!dir) {
        perror(dir_path);
        return;
    }
    struct dirent *entry;
    char path[BUFFER_SIZE];
    while ((entry = readdir(dir)) != NULL) {
        // Skip "." and ".."
        if (strcmp(entry->d_name, ".") == 0 || strcmp(entry->d_name, "..") == 0)
            continue;
        snprintf(path, sizeof(path), "%s/%s", dir_path, entry->d_name);
        struct stat st;
        if (stat(path, &st) == -1) {
            perror(path);
            continue;
        }
        if (S_ISDIR(st.st_mode)) {
            process_directory(path);
        } else if (S_ISREG(st.st_mode)) {
            // Process only .py files.
            const char *ext = strrchr(entry->d_name, '.');
            if (ext && strcmp(ext, ".py") == 0) {
                process_file(path);
            }
        }
    }
    closedir(dir);
}

// ----------------- Main -----------------

/* main()
 * Usage: reformat_print <path>
 * If <path> is a file, process that file.
 * If <path> is a directory, recursively process all ".py" files within.
 */
int main(int argc, char *argv[]) {
    if (argc != 2) {
        fprintf(stderr, "Usage: %s <path>\n", argv[0]);
        return 1;
    }
    if (!check_black_available()) {
        fprintf(stderr, "Error: 'black' is not available in your PATH. Please install it (e.g., pip install black).\n");
        return 1;
    }
    struct stat st;
    if (stat(argv[1], &st) == -1) {
        perror(argv[1]);
        return 1;
    }
    if (S_ISDIR(st.st_mode)) {
        process_directory(argv[1]);
    } else if (S_ISREG(st.st_mode)) {
        process_file(argv[1]);
    } else {
        fprintf(stderr, "Error: %s is not a regular file or directory.\n", argv[1]);
        return 1;
    }
    return 0;
}

