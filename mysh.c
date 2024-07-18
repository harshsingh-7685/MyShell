#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/wait.h>
#include <sys/types.h>
#include <fcntl.h>
#include <stdbool.h>
#include <glob.h>
#include <errno.h>

#define MAXLENGTH 1024
#define MAXARGS 64

//Global variable to store the last exit status
int lastExitStatus = 0;

//Function to find the full path of a command
char* findPath(const char* cmd){
    if (strchr(cmd, '/') != NULL) 
        return strdup(cmd);

    const char* pathEnv = getenv("PATH");
    if (!pathEnv)
        return NULL;

    char* path = strdup(pathEnv);
    char* saveptr;
    
    for (char *p = strtok_r(path, ":", &saveptr); p != NULL; p = strtok_r(NULL, ":", &saveptr)){
        char fullPath[MAXLENGTH];
        snprintf(fullPath, sizeof(fullPath), "%s/%s", p, cmd);
        
        if (access(fullPath, X_OK) == 0){
            free(path);
            return strdup(fullPath);
        }
    }

    free(path);
    return NULL;
}

//Function to redirect input and output
void redirectInputOutput(char** args, int* inFd, int* outFd){
    *inFd = STDIN_FILENO;   //set default input
    *outFd = STDOUT_FILENO;   //set default output

    for (int i = 0; args[i] != NULL; i++){
        if (strcmp(args[i], "<") == 0){
            if (args[i + 1] != NULL) {
                *inFd = open(args[i + 1], O_RDONLY);   //open input file

                if (*inFd < 0){
                    perror("Failed to open input file");
                    lastExitStatus = EXIT_FAILURE;
                    exit(EXIT_FAILURE);
                }

                args[i] = NULL;
            }
        }
        else if (strcmp(args[i], ">") == 0){
            if (args[i + 1] != NULL){
                *outFd = open(args[i + 1], O_WRONLY | O_CREAT | O_TRUNC, 0640);   //create file if it doesn't exist

                if (*outFd < 0){
                    perror("Failed to open output file");
                    lastExitStatus = EXIT_FAILURE;
                    exit(EXIT_FAILURE);
                }

                args[i] = NULL;
            }
        }
    }
}

// Function to expand wildcards
void expandWildcards(char* arg, char** expandedArgs, int* expandedArgCount){
    glob_t globbuf;   //buffer to hold the results of the glob function
    int result = glob(arg, GLOB_NOCHECK | GLOB_TILDE, NULL, &globbuf);   //expand the wildcard

    if (result == 0 && globbuf.gl_pathc > 0){
        for (size_t i = 0; i < globbuf.gl_pathc && *expandedArgCount < MAXARGS - 1; ++i)
            expandedArgs[(*expandedArgCount)++] = strdup(globbuf.gl_pathv[i]);
    }
    else
        expandedArgs[(*expandedArgCount)++] = strdup(arg);

    globfree(&globbuf);   //free the glob buffer
}

//Function to execute built-in commands
void executeCommand(char** args){
    //If no command, return
    if (!args[0])
        return; 

    //cd
    if (strcmp(args[0], "cd") == 0){
        if (args[1] && chdir(args[1]) != 0){
            printf("cd: No such file or directory\n");
            lastExitStatus = EXIT_FAILURE;
        }
        return;
    }

    //exit
    if (strcmp(args[0], "exit") == 0){
        printf("exiting\n");
        exit(0);
    }

    //which
    if (strcmp(args[0], "which") == 0){
        if (args[1] == NULL){
            printf("which: missing argument\n");
            lastExitStatus = EXIT_FAILURE;
            return;
        }
        char* path = findPath(args[1]);
        if (path){
            printf("%s\n", path);
            free(path);
        }
        else{
            printf("%s: command not found\n", args[1]);
            lastExitStatus = EXIT_FAILURE;
        }
        return;
    }
}

//Function to execute external commands
void executeExternalCommand(char** args, int inFd, int outFd){
    pid_t pid = fork();   //fork a child process

    if (pid == 0) {
        //Child process: apply redirection and execute the command
        if (inFd != STDIN_FILENO){
            dup2(inFd, STDIN_FILENO);
            close(inFd);
        }

        if (outFd != STDOUT_FILENO){
            dup2(outFd, STDOUT_FILENO);
            close(outFd);
        }

        char* path = findPath(args[0]);
        if (path) {
            execv(path, args);
            free(path);
        }

        printf("%s: Command not found\n", args[0]);
        lastExitStatus = EXIT_FAILURE;
        exit(EXIT_FAILURE);
    }
    else if (pid > 0){
        // Parent process: wait for the child and capture its exit status
        int status;
        waitpid(pid, &status, 0);

        if (WIFEXITED(status))
            lastExitStatus = WEXITSTATUS(status);
        if (inFd != STDIN_FILENO)
            close(inFd);
        if (outFd != STDOUT_FILENO)
            close(outFd);
    }
    else{
        perror("fork failed");
        lastExitStatus = EXIT_FAILURE;
    }
}

void parseAndExecute(char* input){
    char* args[MAXARGS] = {0};
    char* tempArgs[MAXARGS] = {0}; //use a temporary array to store the initial split
    int argCount = 0;
    char* token = strtok(input, " \n");
    
    while (token != NULL && argCount < MAXARGS - 1){
        tempArgs[argCount++] = token;
        token = strtok(NULL, " \n");
    }
    tempArgs[argCount] = NULL; //null-terminate the tempArgs array

    //Handle conditionals directly in tempArgs
    char** commandArgs = tempArgs;
    if (strcmp(tempArgs[0], "then") == 0){
        //Skip if the last command failed
        if (lastExitStatus != 0) return;
        commandArgs++;
    }
    else if (strcmp(tempArgs[0], "else") == 0){
        //Skip if the last command succeeded
        if (lastExitStatus == 0) return;
        commandArgs++;
    }

    //Expand wildcards for the resulting commandArgs
    argCount = 0; // Reset argCount

    for (int i = 0; commandArgs[i] != NULL && argCount < MAXARGS - 1; i++)
        expandWildcards(commandArgs[i], args, &argCount);

    args[argCount] = NULL; //null-terminate the args array

    //Execute command with potential redirection handling
    if (strcmp(args[0], "cd") == 0 || strcmp(args[0], "exit") == 0 || strcmp(args[0], "which") == 0)
        executeCommand(args);
    else{
        int inFd = STDIN_FILENO, outFd = STDOUT_FILENO;
        redirectInputOutput(args, &inFd, &outFd);
        executeExternalCommand(args, inFd, outFd);
    }
}


void runShell(FILE* input){
    char command[MAXLENGTH];
    bool isInteractiveMode = isatty(fileno(stdin));

    while (1) {
        // Only print the prompt in interactive mode
        if (isInteractiveMode){
            printf("mysh> ");
            fflush(stdout);
        }

        if (!fgets(command, MAXLENGTH, input)){
            if (feof(input)) {
                if (isInteractiveMode)
                    printf("\n");
                break;
            }
            else
                continue;
        }

        command[strcspn(command, "\n")] = '\0';  //remove newline at end

        //Handle upto 2 commands in a pipeline
        char* pipeline[2] = {NULL, NULL};
        int pipeFd[2] = {-1, -1};

        pipeline[0] = strtok(command, "|");

        if (pipeline[0] != NULL)
            pipeline[1] = strtok(NULL, "|");

        if (pipeline[1]){
            if (pipe(pipeFd) == -1){
                perror("pipe failed");
                continue; //Skip this command if pipe creation fails
            }
        }

        //Execute the first part of the pipeline or single command
        if (pipeline[0]) {
            char* trimmedCmd = strtok(pipeline[0], "\n");

            if (trimmedCmd) {
                if (pipeline[1]) {
                    int stdoutBackup = dup(STDOUT_FILENO);
                    dup2(pipeFd[1], STDOUT_FILENO);

                    parseAndExecute(trimmedCmd);
                    dup2(stdoutBackup, STDOUT_FILENO);
                    close(stdoutBackup);
                    close(pipeFd[1]);
                }
                else
                    parseAndExecute(trimmedCmd);
            }
        }

        //Execute the second part of the pipeline, if present
        if (pipeline[1]) {
            char* trimmedCmd = strtok(pipeline[1], "\n");
            if (trimmedCmd) {
                int stdinBackup = dup(STDIN_FILENO);
                dup2(pipeFd[0], STDIN_FILENO);

                parseAndExecute(trimmedCmd);
                dup2(stdinBackup, STDIN_FILENO);
                close(stdinBackup);

                close(pipeFd[0]);
            }
        }
        else if (pipeline[0] && pipeFd[1] != -1)
            close(pipeFd[1]);

        // Ensure both ends of the pipe are closed in case they were opened
        if (pipeFd[0] != -1) close(pipeFd[0]);
        if (pipeFd[1] != -1) close(pipeFd[1]);
    }
}

int main(int argc, char* argv[]){
    FILE* input = stdin;

    if (argc == 2){
        input = fopen(argv[1], "r");
        if (!input) {
            perror("Error opening script file");
            lastExitStatus = EXIT_FAILURE;
            exit(EXIT_FAILURE);
        }
    }
    else if (argc > 2){
        printf("Usage: %s [script file]\n", argv[0]);
        lastExitStatus = EXIT_FAILURE;
        exit(EXIT_FAILURE);
    }
    else if (isatty(fileno(stdin)))
        printf("Welcome to my shell!\n");

    runShell(input);

    //Close the input file if it was opened
    if (input != stdin)
        fclose(input);

    return 0;
}