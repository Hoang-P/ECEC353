/* Main program that uses pipes to connect the three filters as follows:
 *
 * ./gen_numbers n | ./filter_pos_nums | ./calc_avg
 * 
 * Author: Hoang Pham and Nicholas Syrylo
 * Date created: 2/2/20
 *
 * Compile as follows: 
 * gcc -o pipes_gen_filter_avg pipes_gen_filter_avg.c -std=c99 -Wall -lm
 * */

#include <stdio.h>
#include <string.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <unistd.h>
#include <stdlib.h>
#include <errno.h>

int 
main (int argc, char **argv)
{
    /* FIXME: Complete the functionality */
    if (argc != 2 || strcmp (argv[1], "--help") == 0) {
        printf ("Usage: %s num-integers \n", argv[0]);
        exit (EXIT_FAILURE);
    }

    int pfd[2];
    int pfd1[2];

    if (pipe (pfd) ==  -1) {
        perror ("pipe");
        exit (EXIT_FAILURE);
    }
    
    // FIRST CHILD
    switch (fork ()) {
        case -1:
            perror ("fork");
            exit (EXIT_FAILURE);

        case 0:                 /* Child code */
            if (close (pfd[0]) == -1) {
                perror ("close 1");
                exit (EXIT_FAILURE);
            }

            /* Duplicate stdout on write end of pipe; close the duplicated 
             * descriptor. */
            if (pfd[1] != STDOUT_FILENO) {
                if (dup2 (pfd[1], STDOUT_FILENO) == -1) {
                    perror ("dup2 1");
                    exit (EXIT_FAILURE);
                }
            
                if (close (pfd[1]) == -1) {
                    perror ("close 2");
                    exit (EXIT_FAILURE);
                }
            }
            
            /* Execute the first command. Write to pipe. */
            execlp ("./gen_numbers", "./gen_numbers", argv[1], (char *) NULL);
            exit (EXIT_FAILURE);    /* Should not get here unless execlp returns an error */

        default:
            break; /* Parent falls through to create the next child */
    }

    if (pipe (pfd1) ==  -1) {
        perror ("pipe");
        exit (EXIT_FAILURE);
    }

    // SECOND CHILD
    switch (fork ()) {
        case -1:
            perror ("fork");
            exit (EXIT_FAILURE);

        case 0:                 /* Child code */
            if (close (pfd[1]) == -1) {
                perror ("close 3");
                exit (EXIT_FAILURE);
            }

            /* Duplicate stdin on read end of pipe; close the duplicated 
             * descriptor. */
            if (pfd[0] != STDIN_FILENO){
                if (dup2 (pfd[0], STDIN_FILENO) == -1) {
                    perror ("dup2 2");
                    exit (EXIT_FAILURE);
                }

                if (close (pfd[0]) == -1){
                    perror ("close 4");
                    exit (EXIT_FAILURE);
                }
            }

            if (close (pfd1[0]) == -1) {
                perror ("close 5");
                exit (EXIT_FAILURE);
            }

            /* Duplicate stdout on write end of pipe; close the duplicated 
             * descriptor. */
            if (pfd1[1] != STDOUT_FILENO){
                if (dup2 (pfd1[1], STDOUT_FILENO) == -1) {
                    perror ("dup2 3");
                    exit (EXIT_FAILURE);
                }

                if (close (pfd1[1]) == -1){
                    perror ("close 6");
                    exit (EXIT_FAILURE);
                }
            }
            /* Execute the second command. Reads from pipe. */
            execlp ("./filter_pos_nums", "./filter_pos_nums", (char *) NULL);
            exit (EXIT_FAILURE);

        default:
            break; /* Parent falls through. */
    }

    if (close (pfd[0]) == -1) {
        perror ("close 7");
        exit (EXIT_FAILURE);
    }
    
    if (close (pfd[1]) == -1) {
        perror ("close 8");
        exit (EXIT_FAILURE);
    }

    // THIRD CHILD
    switch (fork ()) {
        case -1:
            perror ("fork");
            exit (EXIT_FAILURE);

        case 0:                 /* Child code */
            if (close (pfd1[1]) == -1) {
                perror ("close 9");
                exit (EXIT_FAILURE);
            }

            /* Duplicate stdin on read end of pipe; close the duplicated 
             * descriptor. */
            if (pfd1[0] != STDIN_FILENO){
                if (dup2 (pfd1[0], STDIN_FILENO) == -1) {
                    perror ("dup2 4");
                    exit (EXIT_FAILURE);
                }
                if (close (pfd1[0]) == -1){
                    perror ("close 10");
                    exit (EXIT_FAILURE);
                }
            }
            
            /* Execute the third command. Reads from pipe. */
            execlp ("./calc_avg", "./calc_avg", (char *) NULL);
            exit (EXIT_FAILURE);

        default:
            break; /* Parent falls through. */
    }


    /* Parent closes unused file descriptors for pipe and waits for the 
     * children to terminate. */
    if (close (pfd1[0]) == -1) {
        perror ("close 11");
        exit (EXIT_FAILURE);
    }
    
    if (close (pfd1[1]) == -1) {
        perror ("close 12");
        exit (EXIT_FAILURE);
    }

    if (wait (NULL) == -1) {
        perror ("wait 1");
        exit (EXIT_FAILURE);
    }

    if (wait (NULL) == -1) {
        perror ("wait 2");
        exit (EXIT_FAILURE);
    }

    if (wait (NULL) == -1) {
        perror ("wait 3");
        exit (EXIT_FAILURE);
    }
    
    exit (EXIT_SUCCESS);
}
