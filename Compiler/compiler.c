#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>

#define NAME_LEN 32
#define MAX_VARS 100

// Struct to store a variable's name and value
typedef struct {
    char name[NAME_LEN];
    long value;
} Var;

// Global array to store our variables and a counter
Var vars[MAX_VARS];
int var_count = 0;

// Forward declarations for expression evaluation
long evaluate_expression(const char **ptr);
long evaluate_term(const char **ptr);

// --- Helper Functions ---

// Searches for a variable by name and returns a pointer to it
// Returns NULL if the variable is not found
Var* search_variable(const char *name) {
    for (int i = 0; i < var_count; i++) {
        if (strcmp(vars[i].name, name) == 0) {
            return &vars[i];
        }
    }
    return NULL;
}

// Creates a new variable with the given name and initializes its value to 0
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

// --- Main Parser Function ---

void execute_code(const char *src) {
    const char *ptr = src;
    char var_name[NAME_LEN];
    Var *var;
    
    while (*ptr) {
        // Skip whitespace
        while (isspace(*ptr)) {
            ptr++;
        }
        if (!*ptr) break; // End of file
        
        // Handle variable declaration: &var;
        if (*ptr == '&') {
            ptr++;
            char *semicolon = strchr(ptr, ';');
            if (!semicolon) {
                fprintf(stderr, "Error: Missing semicolon after variable declaration.\n");
                exit(1);
            }
            int len = semicolon - ptr;
            if (len >= NAME_LEN) {
                 fprintf(stderr, "Error: Variable name too long.\n");
                 exit(1);
            }
            strncpy(var_name, ptr, len);
            var_name[len] = '\0';
            create_variable(var_name);
            ptr = semicolon + 1;
        }
        
        // Handle assignment or just a variable retrieval
        else if (*ptr == '@') {
            ptr++;
            char *equals_sign = strchr(ptr, '=');
            char *semicolon = strchr(ptr, ';');
            
            if (equals_sign && equals_sign < semicolon) { // Assignment: @var = ...;
                int name_len = equals_sign - ptr;
                strncpy(var_name, ptr, name_len);
                var_name[name_len] = '\0';
                
                var = search_variable(var_name);
                if (!var) {
                    fprintf(stderr, "Error: Variable '%s' not declared.\n", var_name);
                    exit(1);
                }
                
                ptr = equals_sign + 1;
                long result = evaluate_expression(&ptr);
                var->value = result;
                
                if (strchr(ptr, ';') == NULL) {
                     fprintf(stderr, "Error: Missing semicolon after assignment.\n");
                     exit(1);
                }
                 ptr = strchr(ptr, ';') + 1;
                
            } else if (semicolon) { // No assignment, just a semicolon
                ptr = semicolon + 1;
            } else {
                 fprintf(stderr, "Error: Invalid syntax.\n");
                 exit(1);
            }
        }
        
        // Handle I/O
        else if (*ptr == '>' && *(ptr + 1) == '>') { // Input: >>@var;
            ptr += 2;
            if (*ptr != '@') {
                fprintf(stderr, "Error: Expected '@' after '>>'.\n");
                exit(1);
            }
            ptr++;
            char *semicolon = strchr(ptr, ';');
            if (!semicolon) {
                 fprintf(stderr, "Error: Missing semicolon after input statement.\n");
                 exit(1);
            }
            int len = semicolon - ptr;
            strncpy(var_name, ptr, len);
            var_name[len] = '\0';
            
            var = search_variable(var_name);
            if (!var) {
                fprintf(stderr, "Error: Variable '%s' not declared.\n", var_name);
                exit(1);
            }
            printf("Enter value for '%s': ", var_name);
            if (scanf("%ld", &var->value) != 1) {
                fprintf(stderr, "Error: Invalid input.\n");
                exit(1);
            }
            ptr = semicolon + 1;
        }
        
        else if (*ptr == '<' && *(ptr + 1) == '<') { // Output: <<@var;
            ptr += 2;
            if (*ptr != '@') {
                fprintf(stderr, "Error: Expected '@' after '<<'.\n");
                exit(1);
            }
            ptr++;
            char *semicolon = strchr(ptr, ';');
            if (!semicolon) {
                 fprintf(stderr, "Error: Missing semicolon after output statement.\n");
                 exit(1);
            }
            int len = semicolon - ptr;
            strncpy(var_name, ptr, len);
            var_name[len] = '\0';
            
            var = search_variable(var_name);
            if (!var) {
                fprintf(stderr, "Error: Variable '%s' not declared.\n", var_name);
                exit(1);
            }
            printf("Value of '%s' is: %ld\n", var->name, var->value);
            ptr = semicolon + 1;
        }
        
        else {
            fprintf(stderr, "Error: Unrecognized statement near '%s'\n", ptr);
            exit(1);
        }
    }
}

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