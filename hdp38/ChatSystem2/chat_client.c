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
int sigKill = 0;

void 
print_main_menu (void)
{
    printf ("\n'B'roadcast message\n");
    printf ("'P'rivate message\n");
    printf ("'E'xit\n");
    return;
}

static void setup_notification (mqd_t *mqdp);
static void custom_signal_handler (int signalNumber);

static void thread_func (union sigval sv) {
    ssize_t nr;
    mqd_t *mqdp;
    void* msg_buffer;
    struct server_msg *buffer;
    struct mq_attr attr;

    mqdp = sv.sival_ptr;

    if (mq_getattr (*mqdp, &attr) == -1) {
        perror ("mq_getattr");
        exit (EXIT_FAILURE);
    }

    msg_buffer = malloc (attr.mq_msgsize);
    if (msg_buffer == NULL) {
        perror ("malloc");
        exit (EXIT_FAILURE);
    }

    /* Reenable notification */
    setup_notification (mqdp);

    /* Drain the queue empty */
    while ((nr = mq_receive (*mqdp, msg_buffer, attr.mq_msgsize, NULL)) >= 0) {
        buffer = (struct server_msg *) msg_buffer;
        if (strcmp(buffer->sender_name, SERVER_NAME) != 0) { /* Print if not null message */
            printf ("%s: %s\n", buffer-> sender_name, buffer->msg);
        }
        
    }
    free(msg_buffer);
        

    return;
}


static void 
setup_notification (mqd_t *mqdp)
{
    struct sigevent sev;

    sev.sigev_notify = SIGEV_THREAD;            /* Notify via thread */
    sev.sigev_notify_function = thread_func;
    sev.sigev_notify_attributes = NULL;
    sev.sigev_value.sival_ptr = mqdp;           /* Argument to thread_func() */

    if (mq_notify (*mqdp, &sev) == -1) {
        perror ("mq_notify");
        exit (EXIT_FAILURE);
    }

    return;
}

static void 
mq_terminate (mqd_t mqd, char* client_name, struct client_msg msg)
{
    printf ("Chat client exiting\n");
    /* Client MQ cleanup */
    if (mq_send (mqd, (char *) &msg, sizeof (msg), 0) == -1) {
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
    //struct client_msg server_msg;
    msg.client_pid = getpid();
    strcpy (msg.user_name, user_name);
    char option, dummy;
    char recipient[USER_NAME_LEN];
    char message[MESSAGE_LEN];
    msg.control = 1;
    msg.broadcast = 2;

    /* Client MQs */
    int flags;
    mode_t perms;
    mqd_t mqd;
    struct mq_attr attr;
    static char client_name[MESSAGE_LEN];

    /* Set the default message queue attributes. */
    attr.mq_maxmsg = 10;    /* Maximum number of messages on queue */
    attr.mq_msgsize = sizeof(msg); /* Maximum message size in bytes */
    flags = O_RDWR;         /* Create or open the queue for reading and writing */
    flags |= O_CREAT;
    flags |= O_NONBLOCK;
    
    perms = S_IRUSR | S_IWUSR;  /* rw------- permissions on the queue */
    snprintf (client_name, MESSAGE_LEN, "/hdp38_njs76_client_%s", (char*) user_name);
    mqd = mq_open (client_name, flags, perms, &attr);
    if (mqd == (mqd_t)-1) {
        perror ("mq_open");
        exit (EXIT_FAILURE);
    }

    /* Set up notification */
    setup_notification (&mqd);

    /* Handle signal */
    signal (SIGINT, custom_signal_handler);
    signal (SIGUSR1, custom_signal_handler);

    mqd_t mqd_server;
    printf ("User %s connecting to server\n", user_name);
    mqd_server = mq_open (SERVER_NAME, O_WRONLY);
    if (mqd_server == (mqd_t)-1) {
        perror ("Server does not exist");
        mq_terminate(mqd, client_name, msg);
        exit (EXIT_FAILURE);
    }
    
    /* Operational menu for client */
    if (mq_send (mqd_server, (char *) &msg, sizeof (msg), 0) == -1) {
        perror ("mq_send");
        exit (EXIT_FAILURE);
    }
   
    msg.control = 2;
    sleep(2); /* Wait for response from server if is kill */
    
    if (sigKill == 1) {
        msg.control = 0;
        msg.broadcast = 2;
        if (mq_send (mqd_server, (char *) &msg, sizeof (msg), 0) == -1) {
            perror ("mq_send");
            exit (EXIT_FAILURE);
        }
        printf("Server is full.\n");
        mq_terminate(mqd, client_name, msg);
    }
    while (1) {
        print_main_menu ();
        option = getchar ();
        /* Read dummy character to consume the \n left behind in STDIN */
        dummy = getchar ();

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
                //message[strcspn(message, "\n")] = 0;
                snprintf(msg.msg, sizeof(msg.msg), "%s", message);
                //strcpy (msg.msg, message);
                if (mq_send (mqd_server, (const char *) &msg, sizeof (msg), 0) == -1) {
                    perror ("mq_send");
                    exit (EXIT_FAILURE);
                }
                msg.broadcast = 2;
                break;

            case 'P':
                /* FIXME: Get name of private user and send the private 
                 * message to server to be sent to private user */
                msg.broadcast = 0;
                printf("Username of recipient (Ctrl-d to cancel operation): ");
                if (fgets(recipient, USER_NAME_LEN, stdin) == NULL)
                {
                    printf("Invalid User\n");
                    break;
                };
                //recipient[strcspn(recipient, "\n")] = 0;

                printf("Message to user (Ctrl-d to cancel operation): ");
                if (fgets(message, MESSAGE_LEN, stdin) == NULL)
                {
                    printf("Operation cancelled\n");
                    break;
                };
                //message[strcspn(message, "\n")] = 0;
                snprintf(msg.msg, sizeof(msg.msg), "%s", message);
                snprintf(msg.priv_user_name, sizeof(msg.priv_user_name), "%s", recipient);
                if (mq_send (mqd_server, (char *) &msg, sizeof (msg), 0) == -1) {
                    perror ("mq_send");
                    exit (EXIT_FAILURE);
                }
                msg.broadcast = 2;
                break;

            case 'E':    
                msg.control = 0;
                msg.broadcast = 2;
                if (mq_send (mqd_server, (char *) &msg, sizeof (msg), 0) == -1) {
                    perror ("mq_send");
                    exit (EXIT_FAILURE);
                }
                mq_terminate(mqd, client_name, msg);
            default:
                printf ("Unknown option\n");
                break;
        }
    }
    exit (EXIT_SUCCESS);
}


static void 
custom_signal_handler (int signalNumber)
{
    switch (signalNumber){
        /*case SIGINT:
            printf ("Caught the Control+C signal. \n");
            exit (EXIT_SUCCESS);*/

        case SIGUSR1:
            sigKill = 1;
            //exit (EXIT_SUCCESS);
            break;
        /*case SIGINT:
            sigKill = 1;
            //exit (EXIT_SUCCESS);
            break;*/
         default: 
             break;

    }
}