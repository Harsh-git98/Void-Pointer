#include <stdio.h>
#include <stdlib.h>
#include <ctype.h>
#include <string.h>
#include "interpreter.h"

Var vars[MAX_VARS];
int var_count = 0;

Var* search_variable(const char *name) {
    for (int i = 0; i < var_count; i++) {
        if (strcmp(vars[i].name, name) == 0) {
            return &vars[i];
        }
    }
    return NULL;
}

void create_variable(const char *name) {
    if (var_count >= MAX_VARS) {
        fprintf(stderr, "Error: Maximum variable limit reached.\n");
        exit(1);
    }
    if (search_variable(name) != NULL) {
        fprintf(stderr, "Error: Variable '%s' already exists.\n", name);
        exit(1);
    }
    strncpy(vars[var_count].name, name, NAME_LEN - 1);
    vars[var_count].name[NAME_LEN - 1] = '\0'; // Ensure null-termination
    vars[var_count].value = 0;
    var_count++;
}

// Reads the content of a file into a dynamically allocated string
char* read_file(const char *filename) {
    FILE *file = fopen(filename, "r");
    if (!file) {
        fprintf(stderr, "Error: Could not open file '%s'\n", filename);
        exit(1);
    }

    fseek(file, 0, SEEK_END);
    long size = ftell(file);
    fseek(file, 0, SEEK_SET);

    char *buffer = (char *)malloc(size + 1);
    if (!buffer) {
        fprintf(stderr, "Error: Memory allocation failed.\n");
        fclose(file);
        exit(1);
    }

    fread(buffer, 1, size, file);
    buffer[size] = '\0';
    fclose(file);
    return buffer;
}

// Parses a number or a variable from the source code
long parse_operand(const char **ptr) {
    long value = 0;
    while (isspace(**ptr)) (*ptr)++;
    
    if (**ptr == '@') {
        (*ptr)++;
        char var_name[NAME_LEN];
        int i = 0;
        while (isalnum(**ptr) && i < NAME_LEN - 1) {
            var_name[i++] = **ptr;
            (*ptr)++;
        }
        var_name[i] = '\0';
        
        Var *var = search_variable(var_name);
        if (!var) {
            fprintf(stderr, "Error: Undefined variable '%s' in expression.\n", var_name);
            exit(1);
        }
        return var->value;
    } else if (isdigit(**ptr) || **ptr == '-') { // Handle negative numbers
        value = strtol(*ptr, (char**)ptr, 10);
        return value;
    } else {
        fprintf(stderr, "Error: Invalid operand in expression near '%s'\n", *ptr);
        exit(1);
    }
}

// Evaluates a term (multiplication and division)
long evaluate_term(const char **ptr) {
    long result = parse_operand(ptr);
    while (isspace(**ptr)) (*ptr)++;
    while (**ptr == '*' || **ptr == '/') {
        char op = **ptr;
        (*ptr)++;
        long next_val = parse_operand(ptr);
        if (op == '*') {
            result *= next_val;
        } else if (op == '/') {
            if (next_val == 0) {
                fprintf(stderr, "Error: Division by zero.\n");
                exit(1);
            }
            result /= next_val;
        }
        while (isspace(**ptr)) (*ptr)++;
    }
    return result;
}

// Evaluates an expression (addition and subtraction)
long evaluate_expression(const char **ptr) {
    long result = evaluate_term(ptr);
    while (isspace(**ptr)) (*ptr)++;
    while (**ptr == '+' || **ptr == '-') {
        char op = **ptr;
        (*ptr)++;
        long next_val = evaluate_term(ptr);
        if (op == '+') {
            result += next_val;
        } else if (op == '-') {
            result -= next_val;
        }
        while (isspace(**ptr)) (*ptr)++;
    }
    return result;
}
