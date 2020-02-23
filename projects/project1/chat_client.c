/* Skeleton code for the client side code. 
 *
 * Compile as follows: gcc -o chat_client chat_client.c -std=c99 -Wall -lrt
 *
 * Author: Naga Kandsamy
 * Date created: January 28, 2020
 * Date modified:
 *
 * Student/team name: Hoang Pham, Nicholas Syrylo
 * Date created: Feburary 16, 2020
 *
*/

#define _POSIX_C_SOURCE 2 // For getopt()

#include <mqueue.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <stdlib.h>
#include <unistd.h>
#include <stdio.h>
#include <errno.h>
#include <string.h>
#include "msg_structure.h"

#define SERVER_QUEUE_NAME  "/mq_server_njs76"


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

    /* Create or open server and client message queue */ 
    mqd_t server_mq;
    mqd_t client_mq;
    int flags;
    mode_t perms; 
    struct mq_attr attr;
    attr.mq_maxmsg = 10; /* Maximum number of messages on MQ */ 
    attr.mq_msgsize = 2048; /* Maximum message size in bytes */

    flags = O_RDWR;
    perms = S_IRUSR | S_IWUSR;
    
    server_mq = mq_open(SERVER_QUEUE_NAME, flags, perms, &attr);


    if (argc != 2) {
        printf ("Usage: %s user-name\n", argv[0]);
        exit (EXIT_FAILURE);
    }

    strcpy (user_name, argv[1]); /* Get the client user name */

    /* FIXME: Connect to server */
    printf ("User %s connecting to server\n", user_name);
    
    /* Build a connection request message */
    struct client_msg connection_request;

    connection_request.client_pid = getpid();
    strcpy(connection_request.user_name, user_name);
    connection_request.broadcast = 0;
    connection_request.control = 1; // 1 is connect, 0 is disconnect


    
    





    /* Operational menu for client */

    char option, dummy;
    char dest_user_name[USER_NAME_LEN];
    while (1) {
        print_main_menu ();
        option = getchar ();

        switch (option) {
            case 'B':
               /* FIXME: Send message to server to be broadcast */ 
                break;

            case 'P':
                /* FIXME: Get name of private user and send the private 
                 * message to server to be sent to private user */


                printf("Whisper user : ");
                scanf("%s", dest_user_name);

                break;

            case 'E':
                printf ("Chat client exiting\n");
                /* FIXME: Send message to server that we are exiting */
                exit (EXIT_SUCCESS);

            default:
                printf ("Unknown option\n");
                break;
                
        }
        /* Read dummy character to consume the \n left behind in STDIN */
        dummy = getchar ();
    }
         
    exit (EXIT_SUCCESS);
}
