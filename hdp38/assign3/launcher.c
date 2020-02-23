/* Program to launch the barber and customer threads. 
 *
 * Compile as follows: gcc -o launcher launcher.c -std=c99 -Wall -lpthread -lm
 *
 * Run as follows: ./launcher 
 *
 * Author: Naga Kandasamy
 * Date: February 7, 2020
 *
 * Student/team: Hoang Pham, Nicholas Syrylo
 * Date: February 23, 2020
 */

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


#define MIN_CUSTOMERS 10
#define MAX_CUSTOMERS 20
#define WAITING_ROOM_SIZE 8

/* Definition of semaphores */
// sem_t hdp38_njs76_waiting_room;             /* Signal that the waiting room can accommodate  customers */
// sem_t hdp38_njs76_barber_seat;              /* Signal to ensure exclusive access to the barber seat */
// sem_t hdp38_njs76_done_with_customer;       /* Signals the customer that the barber is done with him/her */
// sem_t hdp38_njs76_barber_bed;               /* Signal to wake up the barber */

/* Returns a random integer value, uniformly distributed between [min, max] */ 
int
rand_int (int min, int max)
{
    return ((int) floor ((double) (min + ((max - min + 1) * ((float) rand ()/(float) RAND_MAX)))));
}
    
int 
main (int argc, char **argv)
{
    int i, pid, num_customers;
    char arg[20];
    int flags;

    /* FIXME: create the needed named semaphores and initialize them to the correct values */
    flags = O_CREAT;
    sem_t *waiting_room = sem_open ("/hdp38_njs76_waiting_room", flags, S_IRUSR | S_IWUSR, WAITING_ROOM_SIZE);
    sem_t *barber_seat = sem_open ("/hdp38_njs76_barber_seat", flags, S_IRUSR | S_IWUSR, 1);
    sem_t *done_with_customer = sem_open ("/hdp38_njs76_done_with_customer", flags, S_IRUSR | S_IWUSR, 0);
    sem_t *barber_bed = sem_open ("/hdp38_njs76_barber_bed", flags, S_IRUSR | S_IWUSR, 0);

    /* Create the barber process. */
    printf ("Launcher: creating barber proccess\n"); 
    pid = fork ();
    switch (pid) {
        case -1:
            perror ("fork");
            exit (EXIT_FAILURE);

        case 0: /* Child process */
            /* Execute the barber process, passing the waiting room size as the argument. 
             * FIXME: Also, pass the names of the ncessary semaphores as additional command-line 
             * arguments. 
             */
            snprintf (arg, sizeof (arg), "%d", WAITING_ROOM_SIZE);
            execlp ("./barber", "barber", arg, "/hdp38_njs76_barber_bed", "/hdp38_njs76_done_with_customer", (char *) NULL);
            perror ("exec"); /* If exec succeeds, we should get here */
            exit (EXIT_FAILURE);
        
        default:
            break;
    }

    /* Create the customer processes */
    srand (time (NULL));
    num_customers = rand_int (MIN_CUSTOMERS, MAX_CUSTOMERS);
    printf ("Launcher: creating %d customers\n", num_customers);

    for (i = 0; i < num_customers; i++){
        pid = fork ();
        switch (pid) {
            case -1:
                perror ("fork");
                exit (EXIT_FAILURE);

            case 0: /* Child code */
                /* Pass the customer number as the first argument.
                 * FIXME: pass the names of the necessary semaphores as additional 
                 * command-line arguments.
                 */
                snprintf (arg, sizeof (arg), "%d", i);
                execlp ("./customer", "customer", arg, "/hdp38_njs76_barber_bed", "/hdp38_njs76_waiting_room", "/hdp38_njs76_barber_seat", "/hdp38_njs76_done_with_customer",  (char *) NULL);
                perror ("exec");
                exit (EXIT_FAILURE);

            default:
                break;
        }
    }

    /* Wait for the barber and customer processes to finish */
    for (i = 0; i < (num_customers + 1); i++)
        wait (NULL);

    /* FIXME: Unlink all the semaphores */
    sem_close(waiting_room);
    sem_close(barber_seat);
    sem_close(done_with_customer);
    sem_close(barber_bed);

    sem_unlink("/hdp38_njs76_barber_bed");
    sem_unlink("/hdp38_njs76_waiting_room");
    sem_unlink("/hdp38_njs76_barber_seat");
    sem_unlink("/hdp38_njs76_done_with_customer");

    exit (EXIT_SUCCESS);
}
