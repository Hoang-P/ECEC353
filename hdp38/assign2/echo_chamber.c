/*
Write a program that uses two pipes to enable bidirectional communication between a parent and
child process, and implements the following functionality:
• The parent process should loop reading a block of text from standard input until EOF (Control+D) is received from the keyboard.
• The parent uses one of the pipes to send the received text to the child.
• The child converts the text received from the parent to uppercase and sends it back to the
parent via the other pipe.
• The parent reads the data coming back from the child and echoes it on standard output before
continuing around the loop once more.

Author: Nicholas Syrylo and Hoang Pham

Use to build:  gcc -o echo_chamber echo_chamber.c -std=c99 -Wall


*/



/* TODO:  Need to fix the issue where it handles two EOFs before printing */
#include <stdio.h>
#include <string.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <unistd.h>
#include <stdlib.h>
#include <errno.h>
#include <ctype.h>

#define TEXT_LEN 1024

int main ( int arc, char **argv) {
    while(1) {
        int pid;
        int fdParentToChild[2];
        int fdChildToParent[2];
        char blockOfText[TEXT_LEN];
        // char word[TEXT_LEN];
        int n;
        int status;

        /* Create the two pipes, one for parent to child, and one for child to parent */
        if (pipe(fdParentToChild) < 0) {
            perror("pipe");
            exit(EXIT_FAILURE);
        }

        if (pipe(fdChildToParent) < 0) {
            perror("pipe");
            exit(EXIT_FAILURE);
        }

        if ((pid = fork()) < 0) {
            perror("fork");
            exit(EXIT_FAILURE);
        }

        /* Parent code to read and then send data */
        if (pid > 0) {
            close(fdParentToChild[0]);  // Close parent's parent to child pipe read end
            close(fdChildToParent[1]);  // Close parent's child to parent write end
            printf("PARENT: Enter your text: ");

            fgets(blockOfText, TEXT_LEN, stdin);
            
            printf("\nRead in sentence: %s \n", blockOfText);
        
            write(fdParentToChild[1], blockOfText, strlen(blockOfText));

        } 
        
        else {        /* Child code to read in the data and then convert to upper */
            close(fdParentToChild[1]);  // Close child's parent to child pipe write end
            close(fdChildToParent[0]);  // Close child's child to parent read end

            n = read(fdParentToChild[0], blockOfText, TEXT_LEN);     // Reading in n bytes
            blockOfText[n] = '\0';                                   // Terminate the string

            /* Capitalize each char */
            int i; 
            while(blockOfText[i]) {
                blockOfText[i] = toupper(blockOfText[i]);
                i++;
            }

            /* now to write the data back */
            write(fdChildToParent[1], blockOfText, strlen(blockOfText));

            exit(EXIT_SUCCESS);
        }
        
        /* Final parent code */
        pid = waitpid(pid, &status, 0);
        /* print out what was read from pipe */
        n = read(fdChildToParent[0], blockOfText, TEXT_LEN);
        blockOfText[n] = '\0';
        
        printf("PARENT: Response from child: %s \n", blockOfText);
    }



}