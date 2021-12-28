/*
- Multiuser chat with socket, threads and curses
- Client side
- Version 1.0
- Developed by Alessio Rubicini, Andrea Malloni, Daniele Monaldi, Alessio Cinti
*/

/* Libraries */
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <curses.h>
#include <pthread.h>
#include <signal.h>
#include <termios.h>

/* Utility functions for messages */
#include "utilitychat.h"


/* Socket descriptor */
int sock = 0;

/* Client username */
char username[30] = {0};

/* Variables for curses windows */
WINDOW *top;      /* Ncurses output window descriptor */
WINDOW *bottom;   /* Ncurses input window descriptor */
int tline = 0;    /* tline position in top window */
int bline = 1;    /* bline position in bottom window */
int nline;        /* number of lines for single message */
int maxx, maxy;   /* Screen dimensions */
int stop = 0;     /* Disconnection flag (used by watchdog) */


/* Functions prototypes */
void *sender(void *name);
void *listener();
void *watchdog();
void sig_handler(int signum){};




/* Main program */
int main(int argc, char const *argv[])
 { 
  signal(SIGINT, sig_handler);

  /* Thread creation */
  pthread_t threads[3];
  char str[5];

  /* Terminal resize */
  printf("\e[8;30;90;t");
  fflush(stdout);
  tcdrain(STDOUT_FILENO);

  /* Connection information struct */
  struct sockaddr_in serv_addr;
  serv_addr.sin_family = AF_INET; 
  serv_addr.sin_port = htons(PORT);

  /* Socket setup */
  sock = socket(AF_INET, SOCK_STREAM, 0);
  if(sock < 0)
   { 
    perror("socket error");
    return (-1);
   }
  
  if(inet_pton(AF_INET, IP, &serv_addr.sin_addr) <= 0)
   { 
    perror("inet_pton error");
    return (-2); 
   }  
   
  /* Connection to server */
  if(connect(sock, (struct sockaddr *)&serv_addr, sizeof(serv_addr)) < 0)
   { 
    perror("socket connect error");
    return (-3); 
   }

  /* Username input */
  do
   {
    printf("[USERNAME]: ");
    scanf("%s", username); 

    if(strlen(username) > 30)
     {
      logmsg("ERROR"); 
      printf(" Your username is too long, no more than 30 character allowed!\n"); 
     }
   }
  while(strlen(username) > 30);


  /* Window setup */
  initscr();
  getmaxyx(stdscr, maxy, maxx);

  top = newwin(maxy/5 * 4 - 1, maxx, 0, 0);       /* Chat window */
  bottom = newwin(maxy/5, maxx, maxy/5 * 4, 0);   /* Input window */

  scrollok(top, TRUE);
  scrollok(bottom, TRUE);
  box(top, '|', '=');
  box(bottom, '|', '=');

  mvwprintw(top, 0, 2, " Your chat ");
  mvwprintw(bottom, 0, 2, " Type here "); 

  wsetscrreg(top, 1, maxy/5 * 4 - 3);
  wsetscrreg(bottom, 1, maxy/5 - 1);


  /* Threads creation */  
  pthread_create(&threads[0], NULL, &sender, (void *)username);
  pthread_create(&threads[1], NULL, &listener, NULL);
  pthread_create(&threads[2], NULL, &watchdog, NULL);
  
  /* Terminal resize */
  printf("\e[8;30;90;t");
  fflush(stdout);
  tcdrain(STDOUT_FILENO);

  /* Wait for threads to finish */
  pthread_join(threads[0], NULL);
  pthread_join(threads[1], NULL);
  pthread_join(threads[2], NULL);
  
  /* Close ncurses windows */
  delwin(top);
  delwin(bottom);
  endwin();     

  /* Program's end */
  return 0; 
}





/*
* Function: sender
* ---------------------
* Wait for user input on bottom window and send the message to the server
* updating the window and scrolling it if necessary
*
* name: client username
*
* return: NULL
*/
void *sender(void *name)
 {
  char input[BODY_SIZE];    /* Buffer for message input */
  char msg[BUFSIZE];        /* Message buffer */

  system("");   /* Window bug fix */

  while(1)
   {
    bline = 1; 

    /* Clear */
    strcpy(input, "");
    strcpy(msg, "");

    /* Get user's message */
    mvwgetnstr(bottom, bline, 3, input, BODY_SIZE);
     
    /* Check for quitting */
    if(strcmp(input, "/exit") == 0)
     {      
      strcpy(msg, get_msg(EXIT, username, "closing", "0"));
      
      /* Send exit message to server */
      if(write(sock, msg, strlen(msg)) < 0)
       {
        perror("error while sending message to server");
        stop = 1; 
       }
      
      /* Exit thread */
      stop = 1; 
      pthread_exit(NULL);
     }  

    /* Check if message is empty and message size */
    if(strcmp(input, "") != 0)
     {
      /* Compose the message: "type|username|message|color" */    
      strcpy(msg, get_msg(MSG, name, input, "0"));

      /* Send message to server */    
      if(msg != "\0")
       {
        if(write(sock, msg, strlen(msg)) < 0)
         {
          perror("error while sending message to server");
         }
       }
     }

    /* Bottom window re-build */

    bottom = newwin(maxy/5, maxx, maxy/5 * 4, 0); /* Input window */

    werase(bottom);
    scrollok(bottom, TRUE);
    box(bottom, '|', '=');
    mvwprintw(bottom, 0, 2, " Type here ");

    wsetscrreg(bottom, 1, maxy/5 - 1);
   }
}





/*
* Function: listener
* ---------------------
* Listen for messages from server and print them on ncurses top window
* updating and scrolling it if necessary
*
* return: NULL
*/
void *listener()
 {
  char msg_rcv[BUFSIZE];      /* Buffer for received message */
  char msg_type[30];          /* Message type */
  char msg_user[30];          /* Message author */
  char msg_body[BODY_SIZE];   /* Message body */
  char msg_color[3];          /* Color corresponding to message author */
  char label[BUFSIZE];        /* Message displayed on top window */
  char buffer[BUFSIZE];       /* Temporary buffer */
  int pair;                   /* Color number */
  int curr = 0;               /* Used for multi-lines messages loop */
  int msg_len;                /* Message lenght */

  /* Ncurses color pair */
  start_color();

  init_pair(1, COLOR_BLUE, COLOR_BLACK);
  init_pair(2, COLOR_GREEN, COLOR_BLACK);
  init_pair(3, COLOR_CYAN, COLOR_BLACK);
  init_pair(4, COLOR_RED, COLOR_BLACK);
  init_pair(5, COLOR_MAGENTA, COLOR_BLACK);
  init_pair(6, COLOR_YELLOW, COLOR_BLACK);
  init_pair(7, COLOR_WHITE, COLOR_BLACK);
  
  while(1)
   {
 
    /* Clear buffers */
    memset(msg_rcv, 0, sizeof msg_rcv);
    memset(msg_type, 0, sizeof msg_type);
    memset(msg_user, 0, sizeof msg_user);
    memset(msg_body, 0, sizeof msg_body);
    memset(label, 0, sizeof label);
    memset(buffer, 0, sizeof buffer);
    
    /* Refresh windows */
    wrefresh(top);

    /* Receive message from server */
    if(read(sock, msg_rcv, sizeof(msg_rcv)) < 0)
     {
      perror("error while reading message from server");
      pthread_exit(NULL);
     }

    /* Extract type from message */
    strcpy(buffer, msg_rcv);
    strcpy(msg_type, get_field(buffer, 1));

    /* Extract username from message */
    strcpy(buffer, msg_rcv);
    strcpy(msg_user, get_field(buffer, 2));

    /* Check if current user is the author */
    if(strcmp(msg_user, username) == 0)
     {
      strcpy(msg_user, "You"); 
     }

    /* Extract body from message */
    strcpy(buffer, msg_rcv);
    strcpy(msg_body, get_field(buffer, 3));
    
    /* Extract color from message */
    strcpy(buffer, msg_rcv);
    strcpy(msg_color, get_field(buffer, 4));

    /* Casts the color to an integer */
    pair = atoi(msg_color);

    /* Check if it's a disconnection confirmation from server */
    if(strcmp(msg_type, EXIT) == 0)
     {
      close(sock);
      pthread_exit(NULL); 
     }

    /* Check if it's a broadcasted message from another client */
    if(strcmp(msg_type, MSG) == 0)
     {
      /* Create the message to display */
      strcpy(label, "");
      strcat(label, msg_user);
      strcat(label, ": \0");
      strcat(label, msg_body);

      /* Gets message lenght */
      msg_len = strlen(label);

      /* Gets number of lines required to print the message */
      nline = msg_len/(maxx - 6) + 1; 

      if(nline > 1)
       { 
        /* Repeat for every line required */ 
        for(int i=0; i<nline; i++)
         {
          /* Clear temporary buffer */ 
          memset(buffer, 0, sizeof buffer);

          /* 
          * Copies 1 line of character at a time 
          * from label into a buffer, until end of label 
          */
          for(int k=0; k<(maxx - 6) && curr<msg_len; k++)
           {
            buffer[k] = label[curr];            
            curr++;
           }
          /* Scrolls top window */
          if(tline == maxy/5 * 4 - 4)
           {
            scroll(top); 
            mvwprintw(top, tline, 0, "|");
            mvwprintw(top, tline, maxx - 1, "|"); 
            //tline--;
           }
          else
           {
            tline++;
           } 

          /* Prints extracted line on top window */ 
          mvwprintw(top, tline, 3, buffer);  
         }
        curr = 0; 
       }
      else
       { 
        /* Scrolls top window */
        if(tline == maxy/5 * 4 - 4)
         {
          scroll(top); 
          mvwprintw(top, tline, 0, "|");
          mvwprintw(top, tline, maxx - 1, "|"); 
          //tline--;
         }
        else
         {
          tline++;
         } 

        /* Prints label on top window */ 
        mvwprintw(top, tline, 3, label);   
       }


       /* Print received message on top window */
      if(pair <= 7)
       {
        if(nline > 1)
         {
          /* Activate color */
          wattron(top, COLOR_PAIR(pair)); 

          /* Print received message on top window */
          mvwprintw(top, tline - nline + 1, 3, msg_user); 

          /* Deactivate color */
          wattroff(top, COLOR_PAIR(pair));
         }
        else
         {
          /* Activate color */
          wattron(top, COLOR_PAIR(pair));

          /* Print received message on top window */
          mvwprintw(top, tline, 3, msg_user); 

          /* Deactivate color */
          wattroff(top, COLOR_PAIR(pair));
         } 
       }
      else
       {
        if(nline > 1)
         {
          /* Activate color */
          wattron(top, COLOR_PAIR(pair - 7));

          /* Activate bold*/
          wattron(top, A_BOLD);

          /* Print received message on top window */
          mvwprintw(top, tline - nline + 1, 3, msg_user);

          /* Deactivate bold */
          wattroff(top, A_BOLD);

          /* Deactivate color */
          wattroff(top, COLOR_PAIR(pair - 7));
         }
        else
         {
          /* Activate color */
          wattron(top, COLOR_PAIR(pair - 7));

          /* Activate bold*/
          wattron(top, A_BOLD);

          /* Print received message on top window */
          mvwprintw(top, tline, 3, msg_user);

          /* Deactivate bold */
          wattroff(top, A_BOLD);

          /* Deactivate color */
          wattroff(top, COLOR_PAIR(pair - 7));
         } 
       }

      strcpy(label, "");
     }
   }
 }




/*
* Function: watchdog
* ---------------------
* Check cursor position in bottom window
* If cursor reaches end of line is moved to next line
*
* return: NULL
*/
void *watchdog()
 {
  int cursorY = 0;
  int cursorX = 0;

  while(stop != 1)
   {
    getyx(bottom, cursorY, cursorX);
    if(cursorX >= maxx - 3)
     {
      if(bline != maxy/5 - 1)
       {
        bline++;
        wmove(bottom, bline, 3); 
       }
     }
   } 
  pthread_exit(NULL); 
 }
