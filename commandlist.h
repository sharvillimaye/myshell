typedef struct node {
    struct node *prev;
    struct node *next;
    int argc;
    char **argv;
    char *output;
    char *input;
    int status;
} Command;

typedef struct list {
    struct node *head;
    char **tokens;
} CommandList;

Command *create_command(char *);
CommandList *create_list(char **tokens);
void print_list();
void add_command(Command *);
void remove_list();