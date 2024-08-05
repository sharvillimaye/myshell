/**
 * This is a general implementation of readline. It takes in an input and uses read() to 
 * provide the next line of the file.
*/

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>

#define BUFFER_SIZE 512

/**
 * Standard readline implementation using lseek. Does not use any additional memory.
*/
char *readline_std(int fd) {
    char buffer[BUFFER_SIZE];
    char *line = NULL;
    ssize_t bytes_read;
    int line_length = 0;

    while ((bytes_read = read(fd, buffer, BUFFER_SIZE)) > 0) {
        ssize_t newline_index = -1;
        for (ssize_t i = 0; i < BUFFER_SIZE; i++) {
            line_length++;
            if (buffer[i] == '\n' || buffer[i] == '\0') {
                newline_index = i;
                break;
            }
        }

        if (newline_index != -1) {
            lseek(fd, newline_index - bytes_read + 1, SEEK_CUR);
            line = malloc((line_length + 1) * sizeof(char));
            if (line == NULL) {
                perror("Malloc failed");
                exit(EXIT_FAILURE);
            }
            strncpy(line, buffer, newline_index + 1);
            line[line_length - 1] = '\0';
            memset(buffer, '\0', sizeof(buffer));
            break;
        }
    }

    if (bytes_read == -1) {
        perror("Error in reading file");
        exit(EXIT_FAILURE);
    }

    return line;
}

/**
 * Readline implementation specifically made for pipe since lseek implementation does not work. Inefficient due to a high amount of read() calls.
*/
int line_length = BUFFER_SIZE;
char *readline_pipe(int fd) {
    char *line = malloc(line_length);
    if (line == NULL) {
        perror("Error in malloc");
        exit(EXIT_FAILURE);
    }

    char character;
    int i = 0;
    ssize_t bytes_read;
    while ((bytes_read = read(fd, &character, 1)) > 0) {
        line[i] = character;

        if (character == '\n') {
            break; 
        }

        i++;
        
        if (i >= line_length - 1) {
            line_length *= 2;
            line = realloc(line, line_length);
            if (line == NULL) {
                perror("Error with realloc");
                exit(EXIT_FAILURE);
            }
        }
    }

    if (bytes_read == -1) {
        perror("Error while reading");
        exit(EXIT_FAILURE);
    }

    line[i] = '\0';

    if (bytes_read == 0 && i == 0) {
        free(line);
        return NULL;
    }

    line_length = BUFFER_SIZE;
    return line;
}

/**
 * Wrapper over the two implementations in this file.
*/
char *readline(int fd) {
    char *line = NULL;
    if (fd == STDIN_FILENO && !isatty(fd)) {
        line = readline_pipe(fd);
    } else {
        line = readline_std(fd);
    }
    return line;
}