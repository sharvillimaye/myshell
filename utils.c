/**
 * This class provides some general helper functions that allow the code to be cleaner. 
*/

#include <stdlib.h>
#include <stdio.h>
#include <string.h>

#include "utils.h"
#include "commandlist.h"

/**
 * Takes a string and outputs an array of tokens.
*/
char **parse_tokens(char *line) {
    char *modified_line = NULL;
    int i = 0;
    int size = 0;
    while (line[i] != '\0') {
        if (line[i] == '>' || line[i] == '<' || line[i] == '|') {
            size += 3;
            modified_line = realloc(modified_line, size);
            if (modified_line == NULL) {
                perror("realloc");
                exit(EXIT_FAILURE);
            }
            char characters[] = {' ', line[i], ' '};
            strcat(modified_line, characters);
        } else {
            size++;
            modified_line = realloc(modified_line, size);
            if (modified_line == NULL) {
                perror("realloc");
                exit(EXIT_FAILURE);
            }
            modified_line[size - 1] = line[i];
        }
        i++;
    }

    free(line);
    modified_line[size] = '\0';
    line = modified_line;
    int number_of_commands = 0;
    char **commands = NULL;
    char *token;
    const char *delimiters = " \t\n\v\f\r";    
    token = strtok(line, delimiters);
    while (token != NULL) {
        number_of_commands++;
        commands = realloc(commands, number_of_commands * sizeof(char *));
        if (commands == NULL) {
            perror("realloc");
            exit(EXIT_FAILURE);
        }
        commands[number_of_commands - 1] = strdup(token);
        if (commands[number_of_commands - 1] == NULL) {
            perror("strdup");
            exit(EXIT_FAILURE);
        }
        token = strtok(NULL, delimiters);
    }

    free(modified_line);
    
    commands = realloc(commands, (number_of_commands + 1) * sizeof(char *));
    if (commands == NULL) {
        perror("realloc");
        exit(EXIT_FAILURE);
    }
    commands[number_of_commands] = NULL;
    
    return commands;
}