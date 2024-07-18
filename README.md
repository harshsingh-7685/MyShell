Name: Harsh Singh; NETID: hks59
Assignment 3: My Shell Implementation(mysh.c)

findPath(const char* cmd):
Finds the full path of a command by searching in the directories listed in the PATH variable

redirectInputOutput(char** args, int* inFd, int* outFd):
Handles input and output redirection for commands

expandWildcards(char* arg, char** expandedArgs, int* expandedArgCount):
Expands wildacrds in a command.

executeCommand(char** args):
A funcion to execute built in functions (cd, exit, which) only

executeExternalCommand(char** args, int inFd, int outFd):
A function to execute external commands using execv()

parseAndExecute(char* input):
Parses a single input and executes the command.

runShell(FILE* input):
Handles pipelines, and calls parseAndExecute() for each command in the pipeline. Takes up to two inputs in a pipeline. If there are more inputs, it will not run them, nor will it print any errors.

main(int argc, char* argv[]):
The entry point of the program. It sets up the shell and processes input from either an interactive session or batch.

Test Cases:
Batch mode test cases are included in myscript.sh file. They can be run using ./mysh myscript.sh
Interactice mode test cases are the same as the batch mode test cases. These are just entered manually.
In these cases, I tested mysh's ability to handle wildcards in ls, built-in commands, redirection, and conditionals.

Bugs:
There is a wierd bug where "mysh> " is printed multiple times in a row in batch mode. Other logic is completely sound.