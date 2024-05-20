/*
  ARTHUR VIEGAS EGUIA
  shell208.c
  Template originally written by
  Jeff Ondich on 16 February 2022
  and updated on 16 February 2023

  This program reads one line of text from stdin while giving the user useful feedback
  about that line of text. It returns errors if the line is too long, if
  there is something wrong with the input or if the user inputs an invalid command. In this case
  the code prints an error message and asks the user for another command. If the user inputs a
  valid command, the program runs the command and asks for another command once it is done running it.

  Note that this program has a weird issue. If you hit Ctrl-D while running
  it, you'll get an infinite loop of error messages. (You can hit Ctrl-C
  to stop the program.) Any idea why this happens? Any idea how to fix it?
*/

#include <stdio.h>
#include <signal.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <ctype.h>
#include <fcntl.h>

// The command buffer will need to have room to hold the
// command, the \n at the end of the command, and the \0.
// That's why the maximum command size is 2 less than the
// command buffer size.
#define COMMAND_BUFFER_SIZE 102
#define MAX_COMMAND_SIZE COMMAND_BUFFER_SIZE - 2

// Return values for get_command
#define COMMAND_INPUT_SUCCEEDED 0
#define COMMAND_INPUT_FAILED 1
#define COMMAND_END_OF_FILE 2
#define COMMAND_TOO_LONG 3

int get_command(char *command_buffer, int buffer_size);
void execute_command(char *command_line);
void helpCommand();
void interrupt_handler(int);


//Runs the shell. Prompts the user to type a command
// It Handles errors or executes the command.
//Ignores signals and exits if the user types exit
int main() {
    const char *prompt = "ASh (Arthur's Shell)> ";
    char command_line[COMMAND_BUFFER_SIZE];

    //Handles signals. It prints ^C if the user types it, as was
    //required in the activity. Ignores the signal.
    if (signal(SIGINT, interrupt_handler) != SIG_DFL) {
        fprintf(stdout, "^C.\n");
    }

    // The main infinite loop. It asks the user
    //For input, if it is valid runs the input otherwise
    //Prints an error message
    while (1) {
        printf("%s", prompt);
        fflush(stdout);
        

        int result = get_command(command_line, COMMAND_BUFFER_SIZE);
        if (result == COMMAND_END_OF_FILE) {
            // stdin has reached EOF, so it's time to be done. This often happens
            // when the user hits Ctrl-D.
            break;
        }
        else if (result == COMMAND_INPUT_FAILED) {
            fprintf(stderr, "There was a problem reading your command. Please try again.\n");
            // we could try to analyze the error using ferror and respond in different
            // ways depending on the error, but instead, let's just bail.
            break;
        }
        else if (result == COMMAND_TOO_LONG) {
            fprintf(stderr, "Commands are limited to length %d. Please try again.\n", MAX_COMMAND_SIZE);
        }
        else if(!strcmp(command_line, "help")){
            //Prints a help message to the user
            helpCommand();
        }
        else if(!strcmp("exit", command_line)){
            //Exits the program only if the user types exit
            exit(0);
        }
        else {
            execute_command(command_line);
        }
    }

    return 0;
}

//Prints ^C to stderr if the user types it.
void interrupt_handler(int sig){
    fprintf(stderr, "^C\n");
    fflush(stderr);
    fprintf(stdout, "ASh (Arthur's Shell)> ");
    fflush(stdout);
}

//Prints a message aiming to help the user to learn about the program
//Prints what the shell does in stdout.
void helpCommand(){
    printf("This is a shell made for CS208 - Introduction to Computer Systems taught by Jeff Ondich\n");
    printf("So far the user can type one word unix commands and the shell will run them, or print an error message\n");
    printf("in case the error command is too long, invalid, or there is something wrong with the input.\n");
    printf("Examples of one word commands are ls, date, wc among others. The shell also supports command line arguments\n");
    printf("like -l.\nIt also supports redirection of input/output with > and <. The shell also\n");
    printf("supports a single pipe to link commands together.\n");
}

/*
    Retrieves the next line of input from stdin (where, typically, the user
    has typed a command) and stores it in command_buffer.

    The newline character (\n, ASCII 10) character at the end of the input
    line will be read from stdin but not stored in command_buffer.

    The input stored in command_buffer will be \0-terminated.

    Returns:
        COMMAND_TOO_LONG if the number of chars in the input line
            (including the \n), is greater than or equal to buffer_size
        COMMAND_INPUT_FAILED if the input operation fails with an error
        COMMAND_END_OF_FILE if the input operation fails with feof(stdin) == true
        COMMAND_INPUT_SUCCEEDED otherwise

    Preconditions:
        - buffer_size > 0
        - command_buffer != NULL
        - command_buffer points to a buffer large enough for at least buffer_size chars
*/
int get_command(char *command_buffer, int buffer_size) {
    assert(buffer_size > 0);
    assert(command_buffer != NULL);

    if (fgets(command_buffer, buffer_size, stdin) == NULL) {
        if (feof(stdin)) {
            return COMMAND_END_OF_FILE;
        }
        else {
            return COMMAND_INPUT_FAILED;
        }
    }

    int command_length = strlen(command_buffer);
    if (command_buffer[command_length - 1] != '\n') {
        // If we get here, the input line hasn't been fully read yet.
        // We need to read the rest of the input line so the unread portion
        // of the line doesn't corrupt the next command the user types.
        char ch = getchar();
        while (ch != '\n' && ch != EOF) {
            ch = getchar();
        }

        return COMMAND_TOO_LONG;
    }

    // remove the newline character
    command_buffer[command_length - 1] = '\0';
    return COMMAND_INPUT_SUCCEEDED;
}


//Given a string representing a command typed in command line
//and an array of commands/parameters, this functions separates
//Each parameter in the string and puts them in
//separate positions in the array of args
void handle_arguments(char *command_line, char *cmdArr[]) {
    int i = 0, cmd = 1;
    cmdArr[0] = command_line;
    while(command_line[i] != '\0'){
        //This functions considers that a space is what divides parameters
        if(isspace(command_line[i])){
            command_line[i] = '\0';
            //further error checking for strings finishing in spaces or with
            //multiple spaces between parameters
            if(command_line[i + 1] != '\0' && !isspace(command_line[i + 1])){
                cmdArr[cmd] =  command_line + i + 1;
                cmd++;
            } 
        }
        i++;
    }
    cmdArr[cmd] = NULL;
}


//Given a command line with multiple commands separated by pipes, separates
//each command and puts every one of them in a specific position in 
//array myCommands
void parsePipes(char *command_line, char *myCommands[]){
    int i = 0, pos = 1;
    myCommands[0] = command_line;
    while(command_line[i] != '\0'){
        //This functions considers that a pipe divides different commands
        if(command_line[i] == '|'){
            command_line[i] = '\0';
            if(!isspace(command_line[i + 1])){
                fprintf(stderr, "Error: pkease input pipe correctly, there must be a speace and a command after it");
            }
            do{
                i++;
            } while(isspace(command_line[i]));
            myCommands[pos] = &command_line[i];
            pos++;
        }
        i++;
    }
    myCommands[pos] = NULL;
}

//Redirects output to file if necessary
void redirectOutput(char *myArgs[]){
    int i = 0;
    while(myArgs[i] != NULL){
        if(!strcmp(myArgs[i], ">")){
            if(myArgs[i + 1] != NULL){
                int fd = open(myArgs[i + 1], O_WRONLY | O_CREAT | O_TRUNC, 0644);
                if(fd < 0){
                    //Exit the current subprocess if unable to open file
                    perror("Trouble opening file\n");
                    exit(1);
                }
                if(dup2(fd, 1) < 0){
                    //Exiting the current subprocess if dup2 fails
                    perror("Trouble redirecting to file\n");
                    exit(1);
                } 
                close(fd);
                myArgs[i] = NULL;
            }
            else{
                fprintf(stderr, "Error: wrong output file address\n");
            }
        }
        i++;
    }
}

//Redirects standard input to file if necessary
void redirectInput(char *myArgs[]){
    int i = 0;
    while(myArgs[i] != NULL){
        if(!strcmp(myArgs[i], "<")){
            if(myArgs[i + 1] != NULL){
                int fd = open(myArgs[i + 1], O_RDONLY);
                if(fd < 0){
                    //Exit the current subprocess if unable to open file
                    perror("Trouble opening file\n");
                    exit(1);
                }
                if(dup2(fd, 0) < 0){
                    //Exits the subprocess if redirecting the input fails
                    perror("Error redirecting to file\n");
                    exit(1);
                }
                close(fd);
                myArgs[i] = NULL;
            }
            else{
                fprintf(stderr, "Error: input output file address\n");
            }
        }
        i++;
    }
}

//Executes a command from a command line which uses
//A single pipe
void helperExecutePiping(char *commands[]){
    char *myArgs[COMMAND_BUFFER_SIZE];
    int fd[2], status;
    if(pipe(fd) < 0){
        perror("Error creating pipe\n");
        exit(1);
    }
    if(fork() != 0){
        if(fork() != 0){
            //Parent. Closes connection and waits
            close(fd[0]);
            close(fd[1]);
            wait(&status);
            wait(&status);
        }
        else {
            //child 2. Receives command from child 1, and executes
            //based on input received from previous command.
            close(fd[1]);
            if(dup2(fd[0], STDIN_FILENO) < 0){
                perror("Trouble creating pipe\n");
                exit(1);
            }
            close(fd[0]);
            handle_arguments(commands[1], myArgs);
            redirectOutput(myArgs);
            execvp(myArgs[0], myArgs);
            perror("Error: Unknown command 2\n");
            fflush(stdout);

        }
    }
    else{
        //child 1. Executes command and sends it as parameter for
        //child 2
        close(fd[0]);
        if(dup2(fd[1], STDOUT_FILENO) < 0){
            perror("Trouble creating pipe\n");
            exit(1);
        }
        close(fd[1]);
        handle_arguments(commands[0], myArgs);
        redirectInput(myArgs);
        execvp(myArgs[0], myArgs);
        perror("Error: Unknown command 1\n");
        fflush(stdout);
    }
}


//Executes a single command from a command line.
//i. e. one that does not use pipes
void helperExecuteNoPipe(char *command_line){
    int status;
    if(fork() != 0) {
        wait(&status);
    }
    else {
        char *myArgs[COMMAND_BUFFER_SIZE];
        handle_arguments(command_line, myArgs);
        redirectInput(myArgs);
        redirectOutput(myArgs);
        execvp(myArgs[0], myArgs);
    }
}

//Forks. In the child subprocess, separates the arguments from
//the commands per se, and runs them using execvp. If an error happens
//Prints an error message. The parent waits for the child to be done executing.
//Redirects output if necessary. Accepts up to 2 commands separated by |.
//Calls helper functions to do run commands depending on the existence of pipes
void execute_command(char *command_line) {
    char *commands[COMMAND_BUFFER_SIZE];
    parsePipes(command_line, commands);
    if(commands[1] != NULL){
        helperExecutePiping(commands);
    }
    else{
        helperExecuteNoPipe(commands[0]);
    }
}
