#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <readline/readline.h>
#include <fcntl.h>
#include <unistd.h>
#include <sys/wait.h>
#include <glob.h>
#include <limits.h>

#include "readline.h"

typedef struct node {
    char *data;
    struct node *next;
} Node;

struct command {
    char **argv;
    int argc;
    char *input;
    char *output;
    char **space_seperated_words;
};

int prev_pipe = 0;
int has_pipe = 0;
int exit_status = 0;
int conditional;

int hasSlash(char* str) {
	for (int i = 0; i < strlen(str); i++){
		if(str[i] == '/') {
			return 1;
		}
	}
    return 0;
}