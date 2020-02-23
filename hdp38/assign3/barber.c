/* Skeleton code for the barber program. 
 *
 * Compile as follows: gcc -o barber barber.c -std=c99 -Wall -lpthread -lm 
 *
 * Author: Naga Kandasamy
 * Date: February 7, 2020
 *
 * */
#define _POSIX_C_SOURCE 1

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <semaphore.h>
#include <math.h>
#include <errno.h>
#include <time.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <signal.h>
#include <setjmp.h>
static sigjmp_buf env;

#define MIN_TIME 2 // 2
#define MAX_TIME 5 // 5
#define TIME_OUT 10

 

static void alarm_handler (int); /* Signal handler to catch the ALRM signal */
static sigjmp_buf env;


/* Simulate cutting hair, by sleeping for some time between [min. max] in seconds */
void
cut_hair (int min, int max) 
{
    sleep ((int) floor((double) (min + ((max - min + 1) * ((float) rand ()/(float) RAND_MAX)))));
    return;
}

int 
main (int argc, char **argv)
{
    signal(SIGALRM, alarm_handler);
    srand (time (NULL));

    printf ("Barber: Opening up the shop\n");
    
    int waiting_room_size = atoi (argv[1]);
    printf ("Barber: size of waiting room = %d\n", waiting_room_size);


    /* FIXME: Unpack the semaphore names from the rest of the command-line arguments 
     * and open them for use.
     */
    sem_t *barber_bed = sem_open(argv[2], O_RDWR);
    sem_t *done_with_customer = sem_open(argv[3], O_RDWR);

    /* FIXME: Stay in a loop, cutting hair for customers as they arrive. 
     * If no customer wakes the barber within TIME_OUT seconds, barber
     * closes shop and goes home.
     */
    int ret;
    ret = sigsetjmp(env, 1);
    switch (ret) {
        case 0:
            break;
        case TIME_OUT:
            printf ("10s passed. Barber is done for the day. \n");
            exit (EXIT_SUCCESS);
    }

    while (1) {
        /* If no customers in 10s, quit */
        /* Else cut hair */
        sem_wait (barber_bed);

        printf("Barber: Cutting hair\n");
        cut_hair (MIN_TIME, MAX_TIME);
        
        sem_post (done_with_customer); /* Indicate that chair is free */
        printf ("Customer going home \n");
        alarm (TIME_OUT);
    }
    
    exit (EXIT_SUCCESS);
}

static void 
alarm_handler (int sig)
{
    
    // signal (SIGALRM, alarm_handler); /* Restablish handler for next occurrence */
    siglongjmp(env, TIME_OUT);
    
    return;
}