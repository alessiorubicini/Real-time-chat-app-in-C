/*
- Multiuser chat with sockets, threads and ncurses
- Server side
- Version 1.0
- Developed by Alessio Rubicini, Andrea Malloni, Daniele Monaldi, Alessio Cinti
*/

/* Libraries */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <netinet/in.h>
#include <unistd.h>
#include <sys/types.h>
#include <pthread.h>

/* Utility functions for messages */
#include "utilitychat.h"


int users[MAX_USERS][2];    /* Array of connected users socket descriptors */
int colors[MAX_USERS];      /* Colors array */
int o_users = 0;            /* Numbers of users connected */
char buffer[BUFSIZE];       /* Temporary buffer */


/* 
* Function: client_t
* ----------------------
* Function executed as thread for every client.
* Listen for messages from client and performs broadcasting or disconnection.
*
* return: NULL
*/
void* client_t(void *argc)
 {
  char msg_rcv[BUFSIZE];      /* Client message buffer */
  char msg_user[30];          /* Username of the client who sent the message */
  char msg_type[15];          /* Type of client message */
  char msg_body[BODY_SIZE];   /* Body of client message */
  char msg_color[3];
  int sock = (int)(intptr_t)argc;   /* Client socket from function parameters */
  int i = 0;                        /* Counter for broadcasting loop */

  while(1)
   {    
    /* Try to read message from client */
    if(read(sock , msg_rcv, BUFSIZE) < 0)
     {
      logmsg("ERROR");
      fflush(stdout);
      perror(" cannot read message from client ");
      pthread_exit(NULL);
     }
    else
     {      
      /* Check if message is empty */
      if(strlen(msg_rcv) != 0)
       {
        logmsg("EVENT");
        printf(" A client sent this message: %s\n", msg_rcv);

        /* Read message type */
        strcpy(buffer, msg_rcv);
        strcpy(msg_type, get_field(buffer, 1));

        /* Read client username */
        strcpy(buffer, msg_rcv);
        strcpy(msg_user, get_field(buffer, 2));

        /* Read message body */
        strcpy(buffer, msg_rcv);
        strcpy(msg_body, get_field(buffer, 3));

        for(i=0; i<MAX_USERS; i++)
         {
          if(users[i][0] == sock)
           {
            sprintf(msg_color, "%d", users[i][1]);
            strcpy(msg_rcv, get_msg(msg_type, msg_user, msg_body, msg_color));
           }
         }

        /* Check if it's an exit warning */
        if(strcmp(msg_type, "exit") == 0)
         {
          /* Remove socket descriptor from array */
          for(i = 0; i < MAX_USERS; i++)
           {
            if(users[i][0] == sock)
            {
              strcpy(buffer, get_msg(EXIT, HOST, "closing", "0"));
              if(send(users[i][0], buffer, strlen(buffer), 0) < 0)
              {
                logmsg("ERROR");
                fflush(stdout);
                perror(" handshake failed "); 
              } 
              
              /* Close socket connection */
              close(sock);
              users[i][0] = 0;
              users[i][1] = 0;
              colors[i] = 0;
              
              logmsg("DONE");
              printf(" Client n.%d disconnected\n", i + 1);
            }
           }
          
          /* Decrease number of connected users */
          o_users--;
          
          return NULL;
         }

        /* Check if it's a message on broadcast */
        if(strcmp(msg_type, "message") == 0)
         {
          /* Broadast message to all clients */
          for(i = 0; i < MAX_USERS; i++)
           {        
            /* Check if socket descriptor is valid */
            if(users[i][0] != 0)
             {
              /* Send message to client */
              if(send(users[i][0], msg_rcv, strlen(msg_rcv), 0) < 0)
               {
                logmsg("ERROR");
                fflush(stdout);
                perror(" broadcasting failed ");

                close(users[i][0]);
                users[i][0] = 0;
                users[i][1] = 0;
                colors[i] = 0;
               }
             }
           }
         }
       }
      else
       {
        /* Remove socket descriptor from array */
        for(i = 0; i < MAX_USERS; i++)
         {
          if(users[i][0] == sock)
           {
            strcpy(buffer, get_msg(EXIT, HOST, "closing", "0"));
            if(send(users[i][0], buffer, strlen(buffer), 0) < 0)
             {
              logmsg("ERROR");
              fflush(stdout);
              perror(" handshake failed ");
             } 
              
            /* Close socket connection */
            close(sock);
            users[i][0] = 0;
            users[i][1] = 0;
            colors[i] = 0;

            logmsg("DONE");
            printf(" Client n.%d disconnected\n", i + 1);
           }
         }
          
        /* Decrease number of connected users */
        o_users--;
          
        return NULL;
       }
      
      /* Clear buffer */
      bzero(msg_rcv, strlen(msg_rcv));
     }
   }
  return NULL;
 }



/* Main program */
int main(int argc, char const *argv[])
 {   
  /* Client thread descriptor */ 
  pthread_t thread;   

  /* Variables declaration */
  int server_fd;                  /* Socket descriptor */
  int new_sock;                   /* Returned client socket */
  struct sockaddr_in address;     /* Connection information struct */
  int addrlen = sizeof(address);  /* Address size */
  int i;                          /* Counter */

  /* Array initialization */
  for(i = 0; i < MAX_USERS; i++)
   {
    users[i][0] = 0;
    colors[i] = 0;
   }

  /* Socket setup */
  if((server_fd = socket(AF_INET, SOCK_STREAM, 0)) == 0)
   {
    logmsg("ERROR");
    fflush(stdout);
    perror(" socket creation failed "); 
    return (-1);
   } 

  address.sin_family = AF_INET; 
  address.sin_addr.s_addr = INADDR_ANY; 
  address.sin_port = htons(PORT); 

  if(bind(server_fd, (struct sockaddr *)&address, sizeof(address)) < 0)
   {
    logmsg("ERROR");
    fflush(stdout);
    perror(" bind failed "); 
    close(server_fd);
    return (-2);
   }

  /* Listen for clients connection */
  if(listen(server_fd, 15) < 0)
   {
    logmsg("ERROR");
    fflush(stdout);
    perror(" cannot listen for connections ");
    close(server_fd);
    return (-3);
   }


  logmsg("STARTING");
  printf(" Server started, waiting for clients connection....\n");

  /* Server while loop */
  while(1)
   {
    /* Trying to accept client connection */
    if((new_sock = accept(server_fd, (struct sockaddr *)&address,(socklen_t*)&addrlen)) < 0) 
     {
      /* Print error message */
      logmsg("ERROR");
      fflush(stdout);
      perror(" failed to accept client connection");

      /* Close connection */
      close(server_fd);
      return (-4);
     }
    else
     {
      /* Check if server reached max number of client */
      if(o_users < MAX_USERS)
       {
        for(i=0; i<MAX_USERS; i++) 
         {
          if(users[i][0] == 0)
           {
            /* Accept client connection and start thread */
            users[i][0] = new_sock;

            for(int k=0; k<MAX_USERS; k++)
             {
              if(colors[k] == 0)
               {
                users[i][1] = k + 1;
                colors[k] = 1;
                break; 
               } 
             }
            break; 
           } 
         }
        
        o_users++;

        logmsg("EVENT");
        printf(" Client n.%d connected\n", o_users);

        /* Start thread */
        if (pthread_create(&thread, NULL, &client_t, (void*)(size_t)new_sock)< 0)
         {
          logmsg("ERROR");
          fflush(stdout);
          perror(" cannot create client thread "); 
         }
       }
      else
       {
        logmsg("WARNING");
        printf(" A client tried to connect: max users reached!\n");
       } 
     }
   }
   
  /* Program's end */
  close(server_fd);
  return 0;

 }
