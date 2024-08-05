#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <readline/readline.h>
#include <fcntl.h>
#include <unistd.h>
#include <sys/wait.h>
#include <glob.h>
#include <limits.h>

typedef struct Node{
	char* data;
	struct Node* next;
} Node;

struct command{
	char** arguments;
	int argc;
	char* input;
	char* output;
	char** spaceSepWords;
};

int prevPipe = 0;
int hasPipe = 0;
int exitStatus = 0;
int conditional;
int hasSlash (char* str){
	for (int i=0; i<strlen(str); i++){
		if(str[i]=='/'){
			return 1;
		}
	}
	return 0;
}
char* findPathToProgram(char* prog){
	char* completePath = malloc(15+strlen(prog));
	if(hasSlash(prog)){
		if(access(prog, X_OK) == 0){
			//file exists at path and is excecutable
			strcpy(completePath, prog);
			return completePath;
		}
	}
	else{
		//no slash means we need to search for the program in the standard search directories
	//	char* completePath = malloc(15+strlen(prog));
		char *searchDirectories[] = {"/usr/local/bin", "/usr/bin", "/bin"};
		for(int i=0; i<3; i++){
			sprintf(completePath, "%s/%s", searchDirectories[i], prog);
			if(access(completePath, X_OK) == 0){
				//program found at directory and is excecutable
				return completePath;
				//need to free in caller function; free after returning//////////////////////////////////
			}
		}

	}
	free(completePath);
	return NULL;

}

int breakPipe(char* input, char** w1, char** w2, char** ans){
	char p = '|';
	*w1 = strsep(&input, &p);
	if(*w1!=NULL){
		*w2 = strsep(&input, &p);
		//printf("\nw2 is NULL\n");
	}
	if(*w2==NULL){
		return -1;
	}
	ans[0]=*w1; ans[1]=*w2;// = {w1,w2};
	return 1;
}
int breakInput(char* input, char** w1, char** w2, char** ans){
        char p = '<';
        *w1 = strsep(&input, &p);
        if(*w1!=NULL){
                *w2 = strsep(&input, &p);
                //printf("\nw2 is NULL\n");
        }
        if(*w2==NULL){
                return -1;
        }
        ans[0]=*w1; ans[1]=*w2;// = {w1,w2};
        return 1;

}
int breakOutput(char* input, char** w1, char** w2, char** ans){
        char p = '>';
        *w1 = strsep(&input, &p);
        if(*w1!=NULL){
                *w2 = strsep(&input, &p);
        }
        if(*w2==NULL){
                return -1;
        }
        ans[0]=*w1; ans[1]=*w2;
        return 1;
}
void redirect(char* input, char* output){
        if(input != NULL){
                int fdInput = open(input, O_RDONLY);
                if(fdInput == -1){
                        printf("Error: Failed to open input file. \n");
                        exit(-1);
                }
                dup2(fdInput, STDIN_FILENO);
                close(fdInput);
        }
        if(output != NULL){
		int fdOutput = open(output, O_WRONLY | O_CREAT | O_TRUNC, S_IRUSR | S_IWUSR | S_IRGRP);
                if(fdOutput == -1){
                        printf("Error: failed to open/create output file");
                        exit(-1);
                }
                dup2(fdOutput, STDOUT_FILENO);
                close(fdOutput);
        }
}

void expandWildCards(char* args, Node* ptr, int* numArg){
    glob_t pglob;
    if (glob(args, GLOB_NOCHECK | GLOB_TILDE, NULL, &pglob) == 0) {
        for (int i = 0; i < pglob.gl_pathc; i++) {
            ptr->data = strdup(pglob.gl_pathv[i]);
            (*numArg)++;
            if (i < pglob.gl_pathc - 1) { // Check if not last element
                ptr-> next = (Node*)malloc(sizeof(Node));
                ptr = ptr->next;
            } else {
                ptr->next = NULL; // Set the next of the last node to NULL
            }
        }
    }
    globfree(&pglob);
}


void freeNode(Node* head){
    Node* next;
	//not freeing strings because the char** is pointing to that now
    while (head != NULL) {
        next = head->next; // Save the reference to the next node
        free(head); // Free the current node
        head = next; // Move to the next node
    }
}

char** readSpaceCommands(char* input, int* numArg) {
	struct Node* head = (Node*) malloc(sizeof(Node));
	head->data = NULL;
    head->next = NULL;
	struct Node* ptr = head;
	char* token = strtok(input, " ");
	while(token != NULL){
		
		if (strchr(token, '*')) {
            expandWildCards(token, ptr, numArg);
        } 
        else {
            // Store the argument and increment the argument count
			ptr->data = strdup(token);
			ptr->next = (Node*)malloc(sizeof(Node));
			ptr = ptr->next;
			ptr->data = NULL;  // Initialize the new node
            ptr->next = NULL;
			(*numArg)++;
        }
        token = strtok(NULL, " ");  // Get the next token
    }
	ptr = head;
	char** argv = malloc(sizeof(char*) * (*numArg + 1));
	for(int i = 0; i < *numArg; i++){
		argv[i] = strdup(ptr->data);
		ptr = ptr->next;
	}
	argv[(*numArg)] = NULL;
	freeNode(head);
	return argv;
}


int isPlainCommand(char* output, char* command){
	//check for output redirection or pipe
	if(hasPipe){
		return -1;
	}
	if(strcmp(command, "cd") == 0){
		return 1;
	}
	if(output == NULL){
                if(strcmp(command, "pwd") == 0){
                        return 2;
                }
                if(strcmp(command, "which") == 0){
                        return 3;
                }
	}
	return 0;
}



int valid(char* str){
//      if(str[0] == '<' || str[0] == '>' || str[0] == '|'){
//              printf("Error: no program to excecute was given. \n");
//      }
        int foundI=0;
        int foundO=0;
        int foundP=0;
        for (int i=0; i<strlen(str); i++){
                if(str[i] == '<'){
                        if(foundI){
                                printf("Error: each command can have at most 1 input redirection. \n");
                                return -1;
                        }
                        else{
                                foundI=1;
                        }
                }
                else if(str[i] == '>'){
                        if(foundO){
                                printf("Error: each command can have at most 1 output redirection. \n");
                                return -1;
                        }
                        else{
                                foundO=1;
                        }
                }
                else if(str[i]=='|'){
                        if(foundP){
                                printf("Error: each command can have at most 1 pipe. \n");
                                return -1;
                        }
                        else{
                                foundP=1;
                        }
                }
        }
        return 0;
}
int runCD(char** arguments, int argc){
        if (argc<2){ //first element of arguments is command, one argument means 2 elements are used
                printf("Error: No arguments given to command cd. \n");
                return -1;
        }
        if(argc>2){
                printf("Error: too many arguents given to command cd. \n");
                return -1;
        }
        if(chdir(arguments[1]) == 0){
		return 1;
	}
	printf("Error changing directory\n");
        return -1;
}

int runPWD(){
//	printf("running pwd\n");
    	size_t maxLen = pathconf(".", _PC_PATH_MAX);
	char* str = malloc(maxLen);
	if(str == NULL){
		printf("Error: Failed to allocate memory for pwd. \n");
		return -1;
	}
        if (getcwd(str, maxLen) == NULL) {
		printf("Error: pwd failed. \n");
		return -1;
	}
	printf("Current Working Directory is: %s\n",str);
	free(str);
	return 1;
}
int runWhich(char** arguments, int argc){
	if(argc == 1){
		printf("Error: no arguments given for the command which. \n");
		return -1;
	}
	if(argc>2){
		printf("Error: too many arguments for the command whih. \n");
		return -1;
	}
	if(strcmp(arguments[1],"cd") == 0 || strcmp(arguments[1],"which") == 0 || strcmp(arguments[1],"pwd") == 0 ){
		printf("Error: passed name of a built-in as argument to command which. \n");
		return -1;
	}
	char* path = findPathToProgram(arguments[1]);
	if(path == NULL){
		printf("Error: Could not find valid path. \n");
		return -1;
	}
	printf("path to program: %s\n",path);
	free(path);
	return 1;

}
int runProgram(char** arguments, int argc, char* input, char* output){
        if(arguments[0] == NULL){// || arguments[0]==' '||arguments[0]=='\0'){
	        printf("Error: no command\n");
                return -1;
        }
	int status = isPlainCommand(output, arguments[0]);
	if(status==1){
		return runCD(arguments, argc);
	}
	else if(status == 2){
		return runPWD();
	}
	else if(status == 3){
		return runWhich(arguments, argc);
	}

	char* execPath = findPathToProgram(arguments[0]);
        if(execPath==NULL){
                printf("error, could not find valid path to program to excecute. \n");
                return -1;
        }

	pid_t pid = fork();
	if(pid == -1){
		printf("Error: fork failed. \n");
		free(execPath);
		return -1;
	}
	if(pid == 0){
		arguments[0] = execPath;
		redirect(input, output);
		int status = execv(arguments[0], arguments);
		if(status == -1){
			printf("Error: Failed to excecute program. \n");
			return -1;
		}
	}
	else{
		int s = 0;
		int* address= &s;
		int waitPid = 0;
		waitpid(pid, address, waitPid);
	}
        free(execPath);
	return 0;

}

int runPipeProgram(char** beforePipeArgs, char** afterPipeArgs, char* input1, char* output1, char* input2, char* output2){
	int pipefd[2];
	int pipeStatus = pipe(pipefd);
	if(pipeStatus < 0){
		printf("Error: failed to initialize the pipe. \n");
		return -1;
	}
	pid_t pipe1pid = fork();
	if(pipe1pid == -1){
		printf("Error: failed to fork. \n");
		return -1;
	}
	if(pipe1pid == 0){
		close(pipefd[0]);
		if(output1==NULL){
		        dup2(pipefd[1], STDOUT_FILENO);
		}
		redirect(input1, output1);

		close(pipefd[1]);
//		redirect(input1, output1);
		char* execPath = findPathToProgram(beforePipeArgs[0]);
        	if(execPath==NULL){
                	printf("error, could not find valid path to program to excecute. \n");
                	return -1;
        	}
//		redirect(input1, output1);
		beforePipeArgs[0] = execPath;
                int status = execv(beforePipeArgs[0], beforePipeArgs);
                if(status == -1){
                        printf("Error: Failed to excecute first program. \n");
                        //free(execPath);
			return -1;
                }
	}
	else{
		pid_t pipe2pid = fork();
		if (pipe2pid<0){
			printf("Error: failed to fork. \n");
			return -1;
		}
		if(pipe2pid == 0){
            		close(pipefd[1]);
			if(input2==NULL){
				dup2(pipefd[0], STDIN_FILENO);
			}
                        redirect(input2, output2);
			close(pipefd[0]);

	                char* execPathPipe = findPathToProgram(afterPipeArgs[0]);
	                if(execPathPipe==NULL){
	                        printf("Error, could not find valid path to program to excecute. \n");
	                        return -1;
	                }
//			redirect(input2, output2);
	                afterPipeArgs[0] = execPathPipe;
	                int status = execv(afterPipeArgs[0], afterPipeArgs);
	                if(status == -1){
	                        printf("Error: Failed to excecute second program. \n");
	                        free(execPathPipe);
	                        return -1;
			}
		}
		else{
			close(pipefd[0]);
			close(pipefd[1]);
			waitpid(pipe1pid,NULL,0);
			waitpid(pipe2pid,NULL,0);
		}
	}
	return 1;
}

int breakInputComponents(char* i, struct command* com){
	conditional = 0;
	char* beforePipe = NULL;
	char* afterPipe = NULL;
	char* ans[2];
	int pipeStatus = breakPipe(i,&beforePipe,&afterPipe, ans);
	int numWords = 0;
	char* input = NULL;
	char* output = NULL;
	char** spaceSeperatedWords;
	char** arguments;
        char* a1=NULL;
	char* b1=NULL;
	char* a2=NULL;
	char* b2=NULL;
        char* outputs[2];
        char* a11=NULL;
	char* b11=NULL;
	char* a = NULL;
	char* b = NULL;
	char* inputs[2];

	if(pipeStatus == -1){
		//NO PIPE
		spaceSeperatedWords = readSpaceCommands(i, &numWords);
		if(strcmp(spaceSeperatedWords[0],"then") == 0){
			conditional = 1;
			spaceSeperatedWords++;
			if(exitStatus != 0 || prevPipe){
				return -1;
			}
		} else if (strcmp(spaceSeperatedWords[0], "else") == 0){
			conditional = 1;
			spaceSeperatedWords++;
			if(exitStatus != -1 || prevPipe){
				return -1;
			}
		}

		if(strcmp(spaceSeperatedWords[0],"exit")==0){
			return -2;
		}
		arguments = malloc(sizeof(char*)*(numWords+2));
		int ptr = 0;
		char* word;
		for(int i=0; i<numWords; i++){
			word = spaceSeperatedWords[i];
			int inputSeperatedWordStatus = breakInput(word,&a,&b,inputs);

			if(inputSeperatedWordStatus!=-1){
				int outputSeperatedWordStatus = breakOutput(inputs[0],&a1,&b1,outputs);
				if(outputSeperatedWordStatus!=-1){
					//output, then input
					//hello>cya<hey; hello>cya<   hey; heyyy   >hello<cya; cya   >hello<  hey;
					output = outputs[1];
					if(outputs[0][0]!='\0'){
						arguments[ptr]=outputs[0];
						ptr+=1;
					}
					if(outputs[1][0]=='\0'){
						printf("Error, no argument after \">\"\n");
						return -1;
					}
					if(inputs[1][0]!='\0'){
						input = inputs[1];
					}
					else{
						if(i+1==numWords){
							printf("Error: no argument after \"<\"\n");
							return -1;
						}
						else{
							input = spaceSeperatedWords[i+1];
							i+=1;
						}
					}
				}
				else{
					//output not in first half, check second half
					outputSeperatedWordStatus = breakOutput(inputs[1],&a2,&b2,outputs);
					if(outputSeperatedWordStatus!=-1){
						//input then output
						//hello<byebye>cya
	                                        input = inputs[1];
        	                                if(inputs[0][0]!='\0'){
                	                                arguments[ptr]=inputs[0];
                        	                        ptr+=1;
                                	        }
	                                        if(inputs[1][0]=='\0'){
        	                                        printf("Error, no argument after \"<\"\n");
                	                                return -1;
                        	                }
                                	        if(outputs[1][0]!='\0'){
                                        	        output = outputs[1];
                                        	}
                                        	else{
                                        	        if(i+1==numWords){
                                        	                printf("Error: no argument after \">\"\n");
                                        	                return -1;
                                        	        }
                                        	        else{
                                        	                output = spaceSeperatedWords[i+1];
                                        	                i+=1;
                                        	        }
                                        	}
					}
					else{
						//only input, no output
						//hello<bye
						if(inputs[0][0]!='\0'){
							arguments[ptr] = inputs[0];
							ptr+=1;
						}
						if(inputs[1][0]!='\0'){
							input = inputs[1];
						}
						else{//input is at end of word, read next word
							if(i+1==numWords){
								printf("Error: no argument after \"<\"\n");
								return -1;
							}
							char* word2 = spaceSeperatedWords[i+1];
							if(output!=NULL){
								input=word2;
								i+=1;
							}
							else{
								outputSeperatedWordStatus = breakOutput(word2,&a2,&b2,outputs);
                                        			if(outputSeperatedWordStatus!=-1){
									//output in second word
									if(outputs[0][0]=='\0'){
										printf("Error: No argument after \"<\"\n");
										return -1;
									}
									input=outputs[0];
									if(outputs[1][0]!='\0'){
										output=outputs[1];
									}
									else{
										if(i+1==numWords){
											printf("Error: no argument after \"<\"\n");
											return -1;
										}
										output=spaceSeperatedWords[i+2];
										i++;
									}
								i++;
								}
								else{ //no output in second word
									input = word2;
									i+=1;
								}
							}
						}
					}
				}
				
			}
			else{//no input, check for output
                    int outputSeperatedWordStatus = breakOutput(word,&a11,&b11,outputs);
                    if(outputSeperatedWordStatus!=-1){
					//only output
					//hello<bye
					if(outputs[0][0]!='\0'){
                                                arguments[ptr] = outputs[0];
                                                ptr+=1;
                                        }
                                        if(outputs[1][0]!='\0'){
                                                output = outputs[1];
                                        }
                                        else{//output is at end of word, read next word
                                                 if(i+1==numWords){
                                                        printf("Error: no argument after \">\"\n");
                                                        return -1;
                                                 }
                                                 char* word2 = spaceSeperatedWords[i+1];
                                                 if(input!=NULL){
       		                                          output=word2;
                                                          i+=1;
               	                                 }
                		                 else{
                                		        int inputSeperatedWordStatus = breakInput(word2,&a2,&b2,inputs);
                                                        if(inputSeperatedWordStatus!=-1){
                                                                if(inputs[0][0]=='\0'){
                                                                        printf("Error: No argument after \">\"\n");
                                                                        return -1;
                                                                }
                                                                output=inputs[0];
                                                                if(inputs[1][0]!='\0'){
	                                                                input=inputs[1];
                                                                }
                                                                else{
                                                                        if(i+1==numWords){
                                                                                printf("Error: no argument after \">\"\n");
										return -1;
                                                                       }
                                                                        input=spaceSeperatedWords[i+2];
									i++;
                                                                }
        	                                                i++;
							}
                                                        else{ //no input in second word
                                                                output = word2;
                                                                i+=1;
                                                        }
                                                }
					}
				}
				else{
					//no input no output
					arguments[ptr] = word;
					ptr+=1;
				}
			}
                }

		(*com).arguments = arguments;
		(*com).argc = ptr;
		(*com).input = input;
		(*com).output = output;
		(*com).spaceSepWords = spaceSeperatedWords;
		return 1;
	}
	return -1;
}
int parseAndRun(char* i){
		prevPipe = hasPipe;
        char* beforePipe = NULL;
        char* afterPipe = NULL;
        char* ans[2];
        int pipeStatus = breakPipe(i,&beforePipe,&afterPipe, ans);
        if(pipeStatus == -1){
		if(valid(i) < 0){
			return -1;
		}
		hasPipe=0;
		struct command com;
		int s = breakInputComponents(i,&com);
		if(s<1){
			return s;
		}
		runProgram(com.arguments, com.argc, com.input, com.output);

		free(com.arguments);
		if(conditional){
			free(com.spaceSepWords - 1);
		} else {
			free(com.spaceSepWords);
		}
		
	}
	else{
		hasPipe = 1;
		struct command bPipe;
		struct command aPipe;
		//PIPE FOUND AND SPLIT TO ans[0] AND ans[1].
		//if(ans[1]){}
		if(valid(ans[0]) == -1 || valid(ans[1]) == -1){
			return -1;
		}
		if (*ans[0]=='\0'){
			printf("Error: no arguments given before pipe. \n");
			return -1;
		}
		else if(*ans[1]=='\0'){
			printf("Error: no arguments given after pipe. \n");
			return -1;
		}
		else{
			int s = breakInputComponents(ans[0],&bPipe);

			if(s<1){
				return s;
			}
			s =  breakInputComponents(ans[1],&aPipe);
			if(s<1){
				return s;
			}
			s = runPipeProgram(bPipe.arguments, aPipe.arguments, bPipe.input, bPipe.output, aPipe.input, aPipe.output);
			if(s<1){
				exit(-1);
                                return s;
                        }
			if(bPipe.arguments[0]==NULL){
	                        printf("Error: no arguments given before pipe. \n");
		//		printf("Error: no argument before pipe\n");
                                return -1;
			}
			if(aPipe.arguments[0] == NULL){
	                        printf("Error: no arguments given after pipe. \n");
//				printf("Error: no argument after pipe\n");
				return -1;
			}
			free(aPipe.arguments);
			free(aPipe.spaceSepWords);
			free(bPipe.arguments);
            free(bPipe.spaceSepWords);
		}
	}
	return 0;
}


char* readLineInput(FILE* filePointer){
	char* buf;
        size_t currentLen= 32;
	size_t jumpSize = 32;
	buf = malloc(currentLen);
	if(buf==NULL){
		printf("Failed to allocated memory to read instructions.\n");
		return NULL;
	}
	char* str = buf;
	int status = fgets(str,jumpSize-1,filePointer) != NULL;
	if(!status){
		free(buf);
		return NULL;
	}
	while(status){
		if(strchr(str, '\n')){
			break;
		}
		currentLen+=jumpSize;
		if(buf==NULL){
			free(buf);
			return NULL;
		}
		buf = realloc(buf,currentLen);
		if(buf==NULL){//if realloc fails
			printf("Failed to allocate memory to read instructions.\n");
			free(buf);
			return NULL;
		}
		str=&(buf[strlen(buf)]);
		status = fgets(str,jumpSize-1,stdin) != NULL;
	}
	buf[strcspn(buf, "\n")]='\0';
	return buf;
}

int main(int argc, char* argv[]){
	if (argc==1){
		//INTERACTIVE MODE
		printf("Welcome to My Shell interactive mode! Enter your commands when prompted and they will be excecuted! \n");
		printf("mysh> ");
		char* inputLine;
		int status;
		while(1){
			inputLine = readLineInput(stdin);
 			if(inputLine[0]=='\0'){
				printf("ERROR: no input\n");
				status = -1;
			}
			else{
				status = parseAndRun(inputLine);
			}
			if(status == -2){
				break;
			}
			free(inputLine);
			printf("mysh> ");
			exitStatus = status;
		}
		printf("Thank you for using My Shell! I hope you enjoyed. My Shell is now terminating. \n");
		return 0;
	}
	else if (argc==2){
                //1 argument passed in by user
                //BATCH MODE
		int status;
		char* inputLine;
		int fd = open(argv[1],O_RDONLY);
		if(fd==-1){
			printf("Error opening file");
			return -1;
		}
    		FILE* fp=fdopen(fd,"r");
		while(1){
			inputLine = readLineInput(fp);
			if(inputLine==NULL){
				break;
			}
                        if(inputLine[0]=='\0'){
                                printf("ERROR: no input\n");
                                status = -1;
                        }
                        else{
                                status = parseAndRun(inputLine);
                        }
                        if(status == -2){
                                break;
                        }
			free(inputLine);
			exitStatus = status;
		}
		fd = close(fd);
		if(fd==-1){
			printf("Error closing file.");
			return -1;
		}
		return 0;
	}
	else{
		printf("Too many arguments\nUsage: mysh <filename>\n");
		return -1;
	}
}