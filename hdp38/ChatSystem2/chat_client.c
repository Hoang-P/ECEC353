/* Skeleton code for the client side code. 
 *
 * Compile as follows: gcc -o chat_client chat_client.c -std=c99 -Wall -lrt
 *
 * Author: Naga Kandsamy
 * Date created: January 28, 2020
 * Date modified:
 *
 * Student/team name: FIXME
 * Date created: FIXME 
 *
*/

#define _POSIX_C_SOURCE 2 // For getopt()
#define TEXT_LEN 2048
#define SERVER_NAME  "/hdp38_njs76_chat_server"

#include <mqueue.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <stdlib.h>
#include <unistd.h>
#include <stdio.h>
#include <errno.h>
#include <string.h>
#include "msg_structure.h"
#include <signal.h>

void 
print_main_menu (void)
{
    printf ("\n'B'roadcast message\n");
    printf ("'P'rivate message\n");
    printf ("'E'xit\n");
    return;
}

int 
main (int argc, char **argv)
{
    char user_name[USER_NAME_LEN];

    if (argc != 2) {
        printf ("Usage: %s user-name\n", argv[0]);
        exit (EXIT_FAILURE);
    }

    strcpy (user_name, argv[1]); /* Get the client user name */

    struct client_msg msg;
    msg.client_pid = getpid();
    strcpy (msg.user_name, user_name);
    char option, dummy;
    char recipient[TEXT_LEN];
    char message[MESSAGE_LEN];
    msg.control = 1;

    /* Client MQs */
    int flags;
    mode_t perms;
    mqd_t mqd;
    struct mq_attr attr;
    static char client_name[MESSAGE_LEN];
    int c;

    /* Set the default message queue attributes. */
    attr.mq_maxmsg = 10;    /* Maximum number of messages on queue */
    attr.mq_msgsize = 2048; /* Maximum message size in bytes */
    flags = O_RDWR;         /* Create or open the queue for reading and writing */
    flags |= O_CREAT;
    flags |= O_NONBLOCK;
    
    perms = S_IRUSR | S_IWUSR;  /* rw------- permissions on the queue */
    snprintf (client_name, MESSAGE_LEN, "/hdp38_njs76_client_%s.%i", (char*) user_name, (int) msg.client_pid);
    mqd = mq_open (client_name, flags, perms, &attr);
    if (mqd == (mqd_t)-1) {
        perror ("mq_open");
        exit (EXIT_FAILURE);
    }

    /* FIXME: Connect to server */
    mqd_t mqd_server;
    printf ("User %s connecting to server\n", user_name);
    mqd_server = mq_open (SERVER_NAME, O_WRONLY);
    if (mqd_server == (mqd_t)-1) {
        perror ("Server does not exist");
        exit (EXIT_FAILURE);
    }
    
    /* Operational menu for client */
    if (mq_send (mqd_server, (char *) &msg, sizeof (msg), 0) == -1) {
        perror ("mq_send");
        exit (EXIT_FAILURE);
    }
    msg.control = 2;

    while (1) {
        print_main_menu ();
        option = getchar ();
        /* discard all characters up to and including newline */
        while ((c = getchar()) != '\n' && c != EOF); 

        switch (option) {
            case 'B':
                /* FIXME: Send message to server to be broadcast */ 
                msg.broadcast = 1;
                printf("Message to broadcast (Ctrl-d to cancel operation): ");
                //scanf("%s", message);
                if (fgets(message, MESSAGE_LEN, stdin) == NULL)
                {
                    printf("\nOperation cancelled\n");
                    break;
                };
                message[strcspn(message, "\n")] = 0;
                snprintf(msg.msg, sizeof(msg.msg), "%s", message);
                //strcpy (msg.msg, message);
                if (mq_send (mqd_server, (const char *) &msg, sizeof (msg), 0) == -1) {
                    perror ("mq_send");
                    exit (EXIT_FAILURE);
                }
                break;

            case 'P':
                /* FIXME: Get name of private user and send the private 
                 * message to server to be sent to private user */
                msg.broadcast = 0;
                printf("Username of recipient (Ctrl-d to cancel operation): ");
                if (fgets(recipient, TEXT_LEN, stdin) == NULL)
                {
                    printf("Invalid User\n");
                    break;
                };
                recipient[strcspn(recipient, "\n")] = 0;

                printf("Message to user (Ctrl-d to cancel operation): ");
                if (fgets(message, TEXT_LEN, stdin) == NULL)
                {
                    printf("Operation cancelled\n");
                    break;
                };
                message[strcspn(message, "\n")] = 0;
                snprintf(msg.msg, sizeof(msg.msg), "%s", message);
                snprintf(msg.priv_user_name, sizeof(msg.priv_user_name), "%s", recipient);
                if (mq_send (mqd, (char *) &msg, sizeof (msg), 1) == -1) {
                    perror ("mq_send");
                    exit (EXIT_FAILURE);
                }
                break;

            case 'E':
                printf ("Chat client exiting\n");
                /* FIXME: Send message to server that we are exiting */
                msg.control = 0;
                if (mq_send (mqd, (char *) &msg, sizeof (msg), 1) == -1) {
                    perror ("mq_send");
                    exit (EXIT_FAILURE);
                }
                if (mq_close(mqd) == (mqd_t)-1) {
                    perror ("mq_close");
                    exit (EXIT_FAILURE);
                }
                if (mq_unlink(client_name) == (mqd_t)-1) {
                    perror ("mq_unlink");
                    exit (EXIT_FAILURE);
                }
                exit (EXIT_SUCCESS);

            default:
                printf ("Unknown option\n");
                break;
        /* Read dummy character to consume the \n left behind in STDIN */
        //dummy = getchar ();
        }
    }
         
    exit (EXIT_SUCCESS);
}


static void 
custom_signal_handler (int signalNumber, int pid, char* client_name)
{
    unsigned int priority;
    void *buffer;
    struct mq_attr attr;
    ssize_t nr;
    mqd_t mqd;
    mqd = mq_open (client_name, O_RDONLY);
    if (mqd == (mqd_t)-1) {
        perror ("mq_open");
        exit (EXIT_FAILURE);
    }

    switch (signalNumber){
        case SIGINT:
            printf ("Caught the Control+C signal. \n");
            exit (EXIT_SUCCESS);

        case SIGQUIT: 
            printf ("Caught the Quit signal. \n");
            printf ("Going through an orderly shutdown process. \n");
            exit (EXIT_SUCCESS);

        case SIGUSR1:
            /* Get the attributes of the MQ */
            if (mq_getattr (mqd, &attr) == -1) {
                perror ("mq_getattr");
                exit (EXIT_FAILURE);
            }

            /* Allocate local buffer to store the received message from the MQ */
            buffer = malloc (sizeof(attr.mq_msgsize));
            if (buffer == NULL) {
                perror ("malloc");
                exit (EXIT_FAILURE);
            }

            nr = mq_receive (mqd, buffer, attr.mq_msgsize, &priority);
            if (nr == -1) {
                perror ("mq_receive");
                exit (EXIT_FAILURE);
            }

            printf ("Read %ld bytes; priority = %u \n", (long) nr, priority);
            if (write (STDOUT_FILENO, buffer, nr) == -1) {
                perror ("write");
                exit (EXIT_FAILURE);
            }

         default: 
             break;

    }
}