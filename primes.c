 /* Skeleton code from primes.
  *
  * Author: Naga Kandasamy
  *
  * Date created: June 28, 2018
  * Date updated: January 16, 2020 
  *
  * Build your code as follows: gcc -o primes primes.c -std=c99 -Wall
  *
  * */

#include <stdlib.h>
#include <unistd.h>
#include <stdio.h>
#include <limits.h>
#include <signal.h>
#include <errno.h>

#define FALSE 0
#define TRUE !FALSE

unsigned long int num_found; /* Number of prime numbers found */
unsigned long int idx;
unsigned long int buf[5];
static void custom_signal_handler (int);

int 
is_prime (unsigned int num)
{
    unsigned long int i;

    if (num < 2) {
        return FALSE;
    }
    else if (num == 2) {
        return TRUE;
    }
    else if (num % 2 == 0) {
        return FALSE;
    }
    else {
        for (i = 3; (i*i) <= num; i += 2) {
            if (num % i == 0) {
                return FALSE;
            }
        }
        return TRUE;
    }
}

/* Complete the function to display the last five prime numbers found by the 
 *  * program, either upon normal program termination or upon being terminated 
 *   * via a SINGINT or SIGQUIT signal. 
 *    */
void 
report (int num)
{
	if (num == 0)
	{
		printf("Last 5 primes found:\n");
		for (int i = 0; i < 5; i++)
		{
			printf ("%lu \n", buf[i]);
		}
	}
	else if (idx != sizeof(buf))
	{
		buf[idx] = num;
		idx++;
	}
	else
	{
		idx = 0;
	}
}

int 
main (int argc, char** argv)
{
    unsigned long int num;
    num_found = 0;
	signal (SIGINT, custom_signal_handler);
	signal (SIGQUIT, custom_signal_handler);

    printf ("Beginning search for primes between 1 and %lu. \n", LONG_MAX);
    for (num = 1; num < LONG_MAX; num++) {
        if (is_prime (num)) {
            num_found++;
			report(num);
            printf ("%lu \n", num);
        }
    }
	report(0);
    exit (EXIT_SUCCESS);
}

static void 
custom_signal_handler (int signalNumber)
{
    switch (signalNumber){
        case SIGINT:
             signal (SIGINT, custom_signal_handler); /* Restablish the signal handler for the next occurrence */
             printf ("Caught the Control+C signal. \n");
			 report(0);
             exit (EXIT_SUCCESS);

         case SIGQUIT: 
             printf ("Caught the Quit signal. \n");
             printf ("Going through an orderly shutdown process. \n");
			 report(0);
             exit (EXIT_SUCCESS);

         default: 
             break;

    }
}
