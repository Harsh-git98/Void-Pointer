// mini_interpreter.c
// Corrected minimal interpreter for the tiny custom language described.

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>

#define NAME_LEN 32
#define MAX_VARS 100

typedef struct {
    char name[NAME_LEN];
    long value;
    int initialized; // new: track initialization
} Var;

static Var vars[MAX_VARS];
static int var_count = 0;

/* ---------- Helpers ---------- */

static void die(const char *fmt, ...) {
    va_list ap;
    va_start(ap, fmt);
    vfprintf(stderr, fmt, ap);
    va_end(ap);
    fprintf(stderr, "\n");
    exit(1);
}

static void skip_spaces_ptr(const char **p) {
    while (**p && isspace((unsigned char)**p)) (*p)++;
}

static int is_name_char(char c) {
    return isalnum((unsigned char)c) || c == '_';
}

static void copy_name_trimmed(char *dst, const char *start, size_t len) {
    // Trim leading and trailing spaces inside the [start, start+len) region,
    // then copy up to NAME_LEN-1 bytes.
    size_t i = 0, j = len ? len - 1 : 0;
    // trim leading
    while (i < len && isspace((unsigned char)start[i])) i++;
    // trim trailing
    while (len > 0 && j > i && isspace((unsigned char)start[j])) j--;
    size_t newlen = (j >= i) ? (j - i + 1) : 0;
    if (newlen >= NAME_LEN) die("Error: variable name too long (max %d)", NAME_LEN - 1);
    if (newlen > 0) memcpy(dst, start + i, newlen);
    dst[newlen] = '\0';
}

/* ---------- Variable table ---------- */

static Var* search_variable(const char *name) {
    for (int i = 0; i < var_count; ++i) if (strcmp(vars[i].name, name) == 0) return &vars[i];
    return NULL;
}

static Var* create_variable(const char *name) {
    if (var_count >= MAX_VARS) die("Error: maximum variable limit reached (%d)", MAX_VARS);
    if (search_variable(name) != NULL) die("Error: variable '%s' already exists", name);
    strncpy(vars[var_count].name, name, NAME_LEN-1);
    vars[var_count].name[NAME_LEN-1] = '\0';
    vars[var_count].value = 0;
    vars[var_count].initialized = 0;
    return &vars[var_count++];
}

/* ---------- Expression parser (recursive, supports + - * / and @var and integers) ---------- */

static long evaluate_expression(const char **ptr); // forward
static long evaluate_term(const char **ptr);

static long parse_operand(const char **ptr) {
    skip_spaces_ptr(ptr);
    if (**ptr == '@') {
        (*ptr)++;
        // parse name
        char name[NAME_LEN];
        int i = 0;
        while (**ptr && is_name_char(**ptr) && i < NAME_LEN - 1) {
            name[i++] = **ptr;
            (*ptr)++;
        }
        name[i] = '\0';
        if (i == 0) die("Error: expected variable name after '@'");
        Var *v = search_variable(name);
        if (!v) die("Error: undefined variable '%s'", name);
        if (!v->initialized) die("Error: variable '%s' used before initialization", name);
        return v->value;
    } else if (**ptr == '(') {
        // parentheses
        (*ptr)++; // consume '('
        long val = evaluate_expression(ptr);
        skip_spaces_ptr(ptr);
        if (**ptr != ')') die("Error: missing ')' in expression near '%s'", *ptr);
        (*ptr)++; // consume ')'
        return val;
    } else {
        // integer (supports optional + or -)
        char *end = NULL;
        long val = strtol(*ptr, &end, 10);
        if (end == *ptr) die("Error: invalid number in expression near '%s'", *ptr);
        *ptr = end;
        return val;
    }
}

static long evaluate_term(const char **ptr) {
    long result = parse_operand(ptr);
    skip_spaces_ptr(ptr);
    while (**ptr == '*' || **ptr == '/') {
        char op = **ptr; (*ptr)++;
        long rhs = parse_operand(ptr);
        if (op == '*') result = result * rhs;
        else {
            if (rhs == 0) die("Error: division by zero");
            result = result / rhs;
        }
        skip_spaces_ptr(ptr);
    }
    return result;
}

static long evaluate_expression(const char **ptr) {
    long result = evaluate_term(ptr);
    skip_spaces_ptr(ptr);
    while (**ptr == '+' || **ptr == '-') {
        char op = **ptr; (*ptr)++;
        long rhs = evaluate_term(ptr);
        if (op == '+') result += rhs;
        else result -= rhs;
        skip_spaces_ptr(ptr);
    }
    return result;
}

/* ---------- Parser / Executor ---------- */

static void expect_char_or_die(const char **p, char c, const char *context) {
    skip_spaces_ptr(p);
    if (**p != c) die("Error: expected '%c' %s (near '%s')", c, context, *p);
    (*p)++;
}

static void execute_code(const char *src) {
    const char *ptr = src;
    char tmp_name[NAME_LEN];

    while (*ptr) {
        skip_spaces_ptr(&ptr);
        if (!*ptr) break;

        // declaration: &name;
        if (*ptr == '&') {
            ptr++;
            skip_spaces_ptr(&ptr);
            // find semicolon
            const char *semi = strchr(ptr, ';');
            if (!semi) die("Error: missing ';' after declaration");
            size_t len = (size_t)(semi - ptr);
            copy_name_trimmed(tmp_name, ptr, len);
            // check name validity: must start with letter or underscore
            if (!tmp_name[0] || !(isalpha((unsigned char)tmp_name[0]) || tmp_name[0] == '_'))
                die("Error: invalid variable name '%s'", tmp_name);
            // also ensure chars are valid
            for (size_t i = 1; i < strlen(tmp_name); ++i) if (!is_name_char(tmp_name[i])) die("Error: invalid character in variable name '%s'", tmp_name);
            create_variable(tmp_name);
            ptr = semi + 1;
            continue;
        }

        // assignment or bare @; both start with '@'
        if (*ptr == '@') {
            ptr++;
            skip_spaces_ptr(&ptr);

            // find semicolon
            const char *semi = strchr(ptr, ';');
            if (!semi) die("Error: missing ';' after statement starting at '%s'", ptr);

            // find equals sign between ptr and semi (if any)
            const char *eq = NULL;
            const char *scan = ptr;
            while (scan < semi) {
                if (*scan == '=') { eq = scan; break; }
                scan++;
            }

            if (eq) {
                // assignment: @name = expr;
                size_t namelen = (size_t)(eq - ptr);
                copy_name_trimmed(tmp_name, ptr, namelen);
                if (!tmp_name[0]) die("Error: empty variable name in assignment");
                Var *v = search_variable(tmp_name);
                if (!v) die("Error: variable '%s' not declared", tmp_name);
                // move ptr to after '=' and evaluate expression (which will advance ptr)
                ptr = eq + 1;
                long val = evaluate_expression(&ptr);
                // after evaluation ptr should be at or before semicolon; skip spaces and require semicolon
                skip_spaces_ptr(&ptr);
                if (*ptr != ';') die("Error: missing ';' after assignment expression near '%s'", ptr);
                ptr++; // consume ';'
                v->value = val;
                v->initialized = 1;
            } else {
                // bare @something; just skip (no-op) but still validate name
                size_t namelen = (size_t)(semi - ptr);
                copy_name_trimmed(tmp_name, ptr, namelen);
                if (!tmp_name[0]) die("Error: empty variable reference");
                // ensure variable exists
                Var *v = search_variable(tmp_name);
                if (!v) die("Error: variable '%s' not declared", tmp_name);
                ptr = semi + 1;
            }
            continue;
        }

        // input: >>@name;
        if (*ptr == '>' && *(ptr + 1) == '>') {
            ptr += 2;
            skip_spaces_ptr(&ptr);
            if (*ptr != '@') die("Error: expected '@' after '>>'");
            ptr++;
            const char *semi = strchr(ptr, ';');
            if (!semi) die("Error: missing ';' after input statement");
            size_t len = (size_t)(semi - ptr);
            copy_name_trimmed(tmp_name, ptr, len);
            Var *v = search_variable(tmp_name);
            if (!v) die("Error: variable '%s' not declared for input", tmp_name);
            printf("Enter value for '%s': ", tmp_name);
            fflush(stdout);
            if (scanf("%ld", &v->value) != 1) die("Error: invalid input for variable '%s'", tmp_name);
            v->initialized = 1;
            ptr = semi + 1;
            continue;
        }

        // output: <<@name;
        if (*ptr == '<' && *(ptr + 1) == '<') {
            ptr += 2;
            skip_spaces_ptr(&ptr);
            if (*ptr != '@') die("Error: expected '@' after '<<'");
            ptr++;
            const char *semi = strchr(ptr, ';');
            if (!semi) die("Error: missing ';' after output statement");
            size_t len = (size_t)(semi - ptr);
            copy_name_trimmed(tmp_name, ptr, len);
            Var *v = search_variable(tmp_name);
            if (!v) die("Error: variable '%s' not declared for output", tmp_name);
            if (!v->initialized) die("Error: variable '%s' used before initialization", tmp_name);
            printf("%ld\n", v->value);
            ptr = semi + 1;
            continue;
        }

        // any other character is an error
        die("Error: unrecognized token starting at '%s'", ptr);
    }
}

/* ---------- file reading ---------- */

static char *read_file(const char *filename) {
    FILE *f = fopen(filename, "rb");
    if (!f) die("Error: could not open file '%s'", filename);
    if (fseek(f, 0, SEEK_END) != 0) die("Error: fseek failed");
    long sz = ftell(f);
    if (sz < 0) die("Error: ftell failed");
    rewind(f);
    char *buf = malloc((size_t)sz + 1);
    if (!buf) die("Error: malloc failed");
    size_t r = fread(buf, 1, (size_t)sz, f);
    if (r != (size_t)sz) {
        // it's okay if file changed between ftell and fread, but warn and continue with bytes read.
    }
    buf[r] = '\0';
    fclose(f);
    return buf;
}

/* ---------- main ---------- */

int main(int argc, char **argv) {
    if (argc < 2) {
        fprintf(stderr, "Usage: %s <source.txt>\n", argv[0]);
        return 1;
    }
    char *code = read_file(argv[1]);
    execute_code(code);
    free(code);
    return 0;
}
