/*
    ARTHUR VIEGAS EGUIA
    
    Started by Jeff Ondich on 3/26/96
    Last modified 16 February 2022
    
    This program gives a simple example of
    exec, and how a parent process can wait for
    a child process to return.
*/

#include    <stdio.h>
#include    <unistd.h>
#include    <sys/types.h>
#include    <sys/wait.h>

int main(int argc, char *argv[]) {
    pid_t pid = fork();

    if (pid != 0) {
        /* Parent */
        printf("Parent just created child, ID %d\n", pid);
        fflush(stdout);

        int status;
        pid = wait(&status);
        printf("My child ID %d just exited with status %d\n", pid, WEXITSTATUS(status));
        fflush(stdout);

    } else {
        /* Child */
        printf("Process %d is about to execute 'getanumber'\n", getpid());
        fflush(stdout);
        //execlp("getanumber", "getanumber", (char *)0);
        execlp("ls", "ls", "-l", (char *)0);
        perror("exec failed");
        fflush(stdout);
    }

    return 0;
}
