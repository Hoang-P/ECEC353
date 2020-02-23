/* Skeleton code for the customer program. 
 *
 * Compile as follows: gcc -o customer customer.c -std=c99 -Wall -lpthread -lm 
 *
 * Author: Naga Kandasamy
 * Date: February 7, 2020
 *
 * */

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <semaphore.h>
#include <math.h>
#include <time.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <sys/stat.h>
#include <fcntl.h>


#define MIN_TIME 2
#define MAX_TIME 10
#define WAITING_ROOM_SIZE 8

// sem_t hdp38_njs76_test_sem; 

/* Simulate walking to barber shop, by sleeping for some time between [min. max] in seconds */
void
walk (int min, int max) 
{
    sleep ((int) floor((double) (min + ((max - min + 1) * ((float) rand ()/(float) RAND_MAX)))));
    return;
}

int 
main (int argc, char **argv)
{
    srand (time (NULL)); 

    int customer_number = atoi (argv[1]);
    /* FIXME: unpack the semaphore names from the rest of the arguments */
    sem_t *barber_bed = sem_open(argv[2], O_RDWR);
    sem_t *waiting_room = sem_open(argv[3], O_RDWR);
    sem_t *barber_seat = sem_open(argv[4], O_RDWR);
    sem_t *done_with_customer = sem_open(argv[5], O_RDWR);

    printf ("Customer %d: Walking to barber shop\n", customer_number);
    walk (MIN_TIME, MAX_TIME);

    /* FIXME: Get hair cut by barber and go home. */
    /* Check if empty seats in waiting room */
    int free_seats;
    sem_getvalue(waiting_room, &free_seats);


    // if (sem_trywait(&waiting_room) != 0)  
    if (free_seats > 0)
    {
        printf("Customer %d entered waiting room.\n", customer_number);
        sem_wait (waiting_room);           /* Wait in barber shop */
        sem_wait (barber_seat);            /* Wait for the barber to become free */
        printf("Customer %d getting haircut.\n", customer_number);
        sem_post (waiting_room);           /* Let people waiting outside the shop know */
        sem_post (barber_bed);             /* Wake up barber */\
        printf("Customer %d woke up barber.\n", customer_number);
        sem_wait (done_with_customer);     /* Wait until hair is cut */
        sem_post (barber_seat);             /* Give up seat */
        printf ("Customer %d: all done\n", customer_number);
    }
    else
    {
        printf ("Waiting room full, customer %d exiting.\n", customer_number);
    }

    exit (EXIT_SUCCESS);
}
