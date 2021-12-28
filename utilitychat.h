/*
- Multiuser chat with socket, threads and curses
- Utility functions
- Version 1.0
- Developed by Alessio Rubicini, Andrea Malloni, Daniele Monaldi, Alessio Cinti
*/

/* Libraries */
#include <stdio.h>w
#include <string.h>

/* ANSI colors definition */
#define ANSI_COLOR_RED     "\x1b[31m"
#define ANSI_COLOR_GREEN   "\x1b[32m"
#define ANSI_COLOR_YELLOW  "\x1b[33m"
#define ANSI_COLOR_BLUE    "\x1b[34m"
#define ANSI_COLOR_MAGENTA "\x1b[35m"
#define ANSI_COLOR_CYAN    "\x1b[36m"
#define ANSI_COLOR_RESET   "\x1b[0m"

/* Message types */
#define MSG "message"
#define EXIT "exit"


#define BUFSIZE 2048                     /* Standard buffer size */
#define BODY_SIZE BUFSIZE - 30 - 15 - 3  /* Message body size */
#define MAX_USERS 15                     /* Max number of clients */

#define HOST "server"   /* Host name */
#define IP "79.26.66.206"  /* IP address */
#define PORT 8080       /* Socket port */


/*
Function: getMessageField
------------------------
Get the specified message field from the given message

returns: the message field
*/
char* get_field(char message[], int property)
 {  
  char *split;
  int i = 1;

  split = strtok(message, "|");

  if(property == 1)
   {
    return split;
   }
  else
   {
    while(split != NULL)
     {
      i++;
      
      split = strtok(NULL, "|");

      if(i == property)
       {
        return split;
       }
     }
   }
 }



/*
Function: composeMessage
------------------------
Compose a message from the given parts

returns: the complete formatted message ready to be sent
*/
char* get_msg(char type[], char name[], char body[], char color[])
 {
  char *msg;
  msg = malloc(2048 * sizeof(char));

  /* Build the message: "type|username|message" */  
  strcpy(msg, "");

  strcat(msg, type);
  strcat(msg, "|");
  strcat(msg, name);
  strcat(msg, "|");
  strcat(msg, body);
  strcat(msg, "|");
  strcat(msg, color);

  return msg;
 }


void logmsg(char type[])
 {
  char color[10];

  if(strcmp(type, "ERROR") == 0) { strcpy(color, ANSI_COLOR_RED); }
  if(strcmp(type, "STARTING") == 0) { strcpy(color, ANSI_COLOR_GREEN); }
  if(strcmp(type, "EVENT") == 0) { strcpy(color, ANSI_COLOR_YELLOW); }
  if(strcmp(type, "WARNING") == 0) { strcpy(color, ANSI_COLOR_MAGENTA); }
  if(strcmp(type, "DONE") == 0) { strcpy(color, ANSI_COLOR_CYAN); }
  
  printf("> ");
  printf("%s[%s]"ANSI_COLOR_RESET, color, type);
 }
 
 
 
