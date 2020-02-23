/* Main program that uses pipes to connect the three filters as follows:
 *
 * ./gen_numbers n | ./filter_pos_nums | ./calc_avg
 * 
 * Author: FIXME 
 * Date created: FIXME
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
    int pfd_njs76[2];

    if pipe(pfd_njs76) == 1) {
        perror("pipe");
        exit(EXIT_FAILURE);
    }


    /* FIXME: Complete the functionality */

    exit (EXIT_SUCCESS);
}

