/* Skeleton code for the server side code. 
 * 
 * Compile as follows: gcc -o hdp38_njs76_chat_server chat_server.c -std=c99 -Wall -lrt
 *
 * Author: Naga Kandasamy
 * Date created: January 28, 2020
 *
 * Student/team name: FIXME
 * Date created: FIXME  
 *
 */

#define SERVER_NAME  "/hdp38_njs76_chat_server"

#include <mqueue.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <stdlib.h>
#include <unistd.h>
#include <stdio.h>
#include <errno.h>
#include "msg_structure.h"

/* FIXME: Establish signal handler to handle CTRL+C signal and 
 * shut down gracefully. 
 */

int 
main (int argc, char **argv)
{
    int flags;
    mode_t perms;
    mqd_t mqd;
    struct mq_attr attr;
    struct client_msg buffer;

    /* Set the default message queue attributes. */
    attr.mq_maxmsg = 10;    /* Maximum number of messages on queue */
    attr.mq_msgsize = sizeof(buffer); /* Maximum message size in bytes */
    flags = O_RDWR;         /* Create or open the queue for reading and writing */
    flags |= O_CREAT;
    flags |= O_NONBLOCK;
    
    perms = S_IRUSR | S_IWUSR;  /* rw------- permissions on the queue */

    mqd = mq_open (SERVER_NAME, flags, perms, &attr);
    if (mqd == (mqd_t)-1) {
        perror ("mq_open");
        exit (EXIT_FAILURE);
    }

    //mqd_t mqd;
    unsigned int priority;
    //struct mq_attr attr;
    ssize_t nr;
    while (1) {
        /* FIXME: Server code here */
        /* Get the attributes of the MQ */
        if (mq_getattr (mqd, &attr) == -1) {
            perror ("mq_getattr");
            exit (EXIT_FAILURE);
        }

        /* Allocate local buffer to store the received message from the MQ */
        nr = mq_receive (mqd, (char *) &buffer, sizeof(buffer), &priority);
        if (nr == -1) {
            //perror ("mq_receive");
            continue;
        }

        if (buffer.control != 1)
        {
            printf ("Read %ld bytes; priority = %u \n", (long) nr, priority);
            printf ("%s \n", buffer.msg);
        }
    }
    exit (EXIT_SUCCESS);
}
