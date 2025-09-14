#ifndef INTERPRETER_H
#define INTERPRETER_H

#define NAME_LEN 32
#define MAX_VARS 100

typedef struct {
    char name[NAME_LEN];
    long value;
} Var;

extern Var vars[MAX_VARS];
extern int var_count;

Var* search_variable(const char *name);
void create_variable(const char *name);
char* read_file(const char *filename);
long parse_operand(const char **ptr);
long evaluate_term(const char **ptr);
long evaluate_expression(const char **ptr);
void execute_code(const char *src);

#endif
