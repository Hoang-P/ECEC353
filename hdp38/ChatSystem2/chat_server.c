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
 * 
 * Server will only send a message FROM the server if the server is full 
 * or if the client tries to connect with same name as server.
 * Otherwise, message "sender" is the client who originated the message. 
 */
#define _POSIX_C_SOURCE 1

#define SERVER_NAME  "/hdp38_njs76_chat_server"
#define MAX_CLIENTS 5
#define CTRLC 10
#define HEARTBEAT 20

#include <mqueue.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <stdlib.h>
#include <unistd.h>
#include <stdio.h>
#include <errno.h>
#include "msg_structure.h"
#include <stdio.h>
#include <string.h>
#include <signal.h>
#include <setjmp.h>

/* FIXME: Establish signal handler to handle CTRL+C signal and 
 * shut down gracefully. 
 */
static void custom_signal_handler (int signalNumber);
static void alarm_handler (int); 
static sigjmp_buf env;

int 
main (int argc, char **argv)
{
    int flags;
    mode_t perms;
    mqd_t mqd; /* Server MQ */
    struct mq_attr attr;
    struct client_msg msg_buffer;
    char connected_clients[MAX_CLIENTS][USER_NAME_LEN];
    memset(connected_clients, '\0', sizeof(connected_clients[0][0]) * MAX_CLIENTS * USER_NAME_LEN);

    int alarm_interval = 5;
    mqd_t priv_mq; /* mq for private user */
    mqd_t broadcast_mq; /* for broadcasts */
    mqd_t heartbeat_mq; /* for the heartbeat */
    struct server_msg server_buffer;
    /* Build the dest_user mq */
    static char dest_user_mq[MESSAGE_LEN];
    int num_clients = 0;


    /* Set the default message queue attributes. */
    attr.mq_maxmsg = 10;    /* Maximum number of messages on queue */
    attr.mq_msgsize = sizeof(msg_buffer); /* Maximum message size in bytes */
    flags = O_RDWR;         /* Create or open the queue for reading and writing */
    flags |= O_CREAT;
    flags |= O_NONBLOCK;
    
    perms = S_IRUSR | S_IWUSR;  /* rw------- permissions on the queue */

    mqd = mq_open (SERVER_NAME, flags, perms, &attr);
    if (mqd == (mqd_t)-1) {
        perror ("mq_open");
        exit (EXIT_FAILURE);
    }
    /* Handle signals */
    signal (SIGINT, custom_signal_handler);
    signal (SIGALRM, alarm_handler);

    unsigned int priority;
    //struct mq_attr attr;
    ssize_t nr;

    int ret;
    ret = sigsetjmp (env, 1);

    /* Switch statement for sigsetjmp */
    switch (ret)
    {
        case 0:
            break;

        case CTRLC:
            printf("\nServer gracefully shutting down.\n");
            if (mq_close(mqd) == (mqd_t)-1) {
                perror ("mq_close");
                exit (EXIT_FAILURE);
            }
            
            if (mq_unlink(SERVER_NAME) == (mqd_t)-1) {
                perror ("mq_unlink");
                exit (EXIT_FAILURE);
            }
            exit(EXIT_SUCCESS);
        
        case HEARTBEAT:
            printf("Sending a heartbeat\n");
            /* Send message to clients to re-establish a beat */
            for (int i = 0; i < MAX_CLIENTS; i++) {
                if (connected_clients[i][0] != 0) { 
                    snprintf (dest_user_mq, MESSAGE_LEN, "/hdp38_njs76_client_%s", (char *) connected_clients[i]);
                    dest_user_mq[strcspn(dest_user_mq, "\n")] = 0; // remove newline from client name
                    // printf("Sending to mq : %s\n", dest_user_mq);
                    heartbeat_mq = mq_open (dest_user_mq, O_WRONLY);

                    /* Open priv user mq and send */
                    if (heartbeat_mq == (mqd_t)-1) {
                        perror ("Could not find message recipient");
                        continue;
                    }

                    strcpy(server_buffer.sender_name, SERVER_NAME);
                    strcpy(server_buffer.msg, "\0");
                    
                    if (mq_send (heartbeat_mq, (char *) &server_buffer, sizeof (server_buffer), 0) == -1) {
                        perror ("mq_send");
                        exit (EXIT_FAILURE);
                    }
                }
            }
            break;
        
        default:
            break;
    }

    alarm (alarm_interval);

    while (1) {
        /* FIXME: Server code here */

        /* Set the alarm up */

        /* Get the attributes of the MQ */
        if (mq_getattr (mqd, &attr) == -1) {
            perror ("mq_getattr");
            exit (EXIT_FAILURE);
        }

        /* Allocate local buffer to store the received message from the MQ */
        nr = mq_receive (mqd, (char *) &msg_buffer, sizeof(msg_buffer), &priority);
        if (nr == -1) {
            //perror ("mq_receive");
            continue;
        }


        /* Add user to the connected clients array */
        switch(msg_buffer.control) {
            case 0: /* User leaves */
                printf("DEBUG: User leaving\n");
                for (int i = 0; i < MAX_CLIENTS; i++) {
                    // printf("%s: %s\n", connected_clients[i], msg_buffer.user_name);
                    if (strcmp(connected_clients[i], msg_buffer.user_name) == 0) {
                        num_clients--;
                        printf("NumClients : %i\n", num_clients);
                        printf("DEBUG: The user that left was %s\n", msg_buffer.user_name);
                        memset(&connected_clients[i], '\0', sizeof(connected_clients[i]));
                        
                        // for (int ii = 0; ii < MAX_CLIENTS; ii++) {
                        //     printf("DEBUG: Client %i: %s\n", ii, connected_clients[ii]);
                        // }
                        break;
                    }
                }
                break;
            case 1: /* User joins */
                // printf("DEBUG: User joins\n");             
                /* Check to see if server is full */
                num_clients++;
                if (num_clients > MAX_CLIENTS) {
                    printf("Server full! \nKilling client: %s\n", msg_buffer.user_name);
                    num_clients--;
                    kill(msg_buffer.client_pid, SIGUSR1);
                    break;
                }

                for (int i = 0; i < MAX_CLIENTS; i++) {
                    if (connected_clients[i][0] == 0) {
                        snprintf(connected_clients[i], sizeof(connected_clients[i]), "%s", msg_buffer.user_name);
                        printf("DEBUG: The user that joined was %s\n", connected_clients[i]);
                        for (int ii = 0; ii < MAX_CLIENTS; ii++) {
                            printf("DEBUG: Client %i: %s\n", ii, connected_clients[ii]);
                        }
                        printf("NumClients : %i\n", num_clients);
                        break;
                    }
                }
                break;
            default:
                printf("DEBUG: Unknown case for control: %d\n", msg_buffer.control);
                break;
        }

        switch (msg_buffer.broadcast) {
            case 0: /* private message */
                /* find the user to whisper */
                printf("private message\n");
                snprintf (dest_user_mq, MESSAGE_LEN, "/hdp38_njs76_client_%s", (char *) msg_buffer.priv_user_name);
                dest_user_mq[strcspn(dest_user_mq, "\n")] = 0; // remove newline from client name
                printf("Opened priv_mq of : %s\n", dest_user_mq);
                priv_mq = mq_open (dest_user_mq, O_WRONLY);
                
                /* Open priv user mq and send */
                if (priv_mq == (mqd_t)-1) {
                    perror ("Could not find private message recipient");
                    // exit (EXIT_FAILURE);
                    break;
                }

                strcpy(server_buffer.sender_name, msg_buffer.user_name);
                strcpy(server_buffer.msg, msg_buffer.msg);
                if (mq_send (priv_mq, (char *) &server_buffer, sizeof (server_buffer), 0) == -1) {
                    perror ("mq_send");
                    exit (EXIT_FAILURE);
                }
                printf("Message from %s sent to other person %s.\nContents: %s\n", server_buffer.sender_name, \
                                                              msg_buffer.priv_user_name, server_buffer.msg);

                break;

            case 1: /* broadcast message */
                printf("Broadcast message from %s\n", msg_buffer.user_name);
                /* loop over each client that currently exists. Also ignore the client that's sending the message. */
                for (int i = 0; i < MAX_CLIENTS; i++) {
                    if (connected_clients[i][0] != 0 && strcmp(connected_clients[i], msg_buffer.user_name) != 0) { 
                        snprintf (dest_user_mq, MESSAGE_LEN, "/hdp38_njs76_client_%s", (char *) connected_clients[i]);
                        dest_user_mq[strcspn(dest_user_mq, "\n")] = 0; // remove newline from client name
                        printf("Sending to mq : %s\n", dest_user_mq);
                        broadcast_mq = mq_open (dest_user_mq, O_WRONLY);

                        /* Open priv user mq and send */
                        if (broadcast_mq == (mqd_t)-1) {
                            perror ("Could not find message recipient");
                            continue;
                        }

                        strcpy(server_buffer.sender_name, msg_buffer.user_name);
                        strcpy(server_buffer.msg, msg_buffer.msg);
                        if (mq_send (broadcast_mq, (char *) &server_buffer, sizeof (server_buffer), 0) == -1) {
                            perror ("mq_send");
                            exit (EXIT_FAILURE);
                        }
                        /* printf("Message from %s sent to other person %s.\nContents: %s\n", server_buffer.sender_name, \
                                                                     connected_clients[i], server_buffer.msg); */

                    }

                }
                break;
            default:
                // printf("DEBUG: Unknown case for broadcast: %d\n", msg_buffer.broadcast);
                break;

        }


        // if (msg_buffer.control == 2)
        // {
        //     printf ("Read %ld bytes; priority = %u \n", (long) nr, priority);
        //     printf ("%s \n", msg_buffer.msg);
        // }
    }
    exit (EXIT_SUCCESS);
}


static void 
custom_signal_handler (int signalNumber)
{
    siglongjmp (env, CTRLC);
}

static void 
alarm_handler (int sig)
{
    signal (SIGALRM, alarm_handler); /* Restablish handler for next occurrence */
    siglongjmp (env, HEARTBEAT);
    return;
}