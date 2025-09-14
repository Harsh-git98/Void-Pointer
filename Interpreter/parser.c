#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include "interpreter.h"

// The main function to tokenize and parse the source code
void execute_code(const char *src) {
    const char *ptr = src;
    char var_name[NAME_LEN];
    Var *var;
    int brace_flag=0;

    // Skip leading whitespace before the opening brace
    while (isspace(*ptr)) {
        ptr++;
    }

    // Check for the opening brace
    if (*ptr != '{') {
        fprintf(stderr, "Error: Source code must begin with a '{'.\n");
        exit(1);
    }
    else{
        brace_flag=1;
    }
    ptr++; // Move past the '{'

    while (*ptr) {
        // Skip whitespace
        while (isspace(*ptr)) {
            ptr++;
        }
        
        // Check for the closing brace
        if (*ptr == '}') {
            ptr++;
            // Check for any extra characters after the closing brace
            while (isspace(*ptr)) {
                ptr++;
            }
            if (*ptr != '\0') {
                fprintf(stderr, "Error: Unexpected characters after closing '}'.\n");
                exit(1);
            }
            brace_flag=0;
            break; // Exit the loop
        }

       // --- Handle hardcoded print: #message ---
        if (*ptr == '#') {
            ptr++; // Move past the '#'
            // Print characters until a newline or end of file is reached
            while (*ptr != '\n' && *ptr != '\0') {
                putchar(*ptr);
                ptr++;
            }
            putchar('\n'); // Print a newline at the end
        }
        // Handle variable declaration: &var;
        else if (*ptr == '&') {
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
            printf("%ld\n",var->value);
            ptr = semicolon + 1;
        }
        
        else {
            fprintf(stderr, "Error: Unrecognized statement near '%s'\n", ptr);
            exit(1);
        }
    }
    if(brace_flag==1)
    {
        fprintf(stderr, "Expected } at End. \n");
        exit(1);

    }
}
