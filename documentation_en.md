# Development report and chat software in C language documentation

## **Contents**

- [Product requirements](#product-requirements)
- [Software architecture](#Software-architecture)
- [Technical specifications](#Technical-specifications)
- [Common issues](#Common-issues)


## **Product requirements**

The request is to develop, using C language, a real-time chat software with terminal clients where 2 or more users are able to communicate each other.


## **Team description**

**Alessio Rubicini**: server and routines to manage messages development, client bug fixing and documentation writing

**Andrea Malloni**: software design, graphic interface development, documentation writing, bug fixes and optimizations    

**Daniele Monaldi**: client development, graphic interface bug fixes and various optimizations  

**Alessio Cinti**: interface development, documentation writing and various bug fixes

**Nicolas Burocchi**: commenting the code, documentation writing and translation and beta testing

---

## **Software architecture**

The sowftware is based on the classic Client/Server architecture.
It's composed by 3 files: *server.c*, *client.c* and *utilitychat.h*.

### Server
The server manages the entire platform dealing about clients connection/disconnection, messages broadcast and a part of client interface management.
It keeps track of connected clients using a 14x2 matrix, containing socket descriptors and colors assigned to them (only for an aesthetic matter that will be analyzed later) expressed in the form of integers (1-14). Moreover, an integer variable store the number of clients currently online.

After having initialized the server socket on local IP address and 8080 port, it waits for a client connection.
When a client connects the matrix is updated memorizing the connection descriptor and its corresponding color selected among the available ones. Then a thread is started in order to execute the *ClientThread* client management function. In this way the server continues waiting for other connections on the 8080 port.

*ClientThread* function executes an infinite loop where tries to read an incoming message from the client.
In particular, using *get_field* funcion (contained in *utilitychat.h* file) checks the type of the received message:
- If the message is an exit request (*exit*) the server proceeds with the client disconnection.
- Otherwise, if the message is a text sent on the chat (*message*) it's forwarded to all the other clients.


### Client
The client is the software part that the user uses to connect to the chat and send/receive messages. Due to the fact that taking an input the message on the terminal is a blocking operation, it has been choosen to take advantage of threads for reading/writing operations, in order to make the two things simultaneous, and to use a [graphic interface](#graphic-interface) to visualize messages, because otherwise the classic terminal would have experienced bugs.
The source code is composed of 4 processes, the *main* and 3 *threads* (1 for reading, one for writing and 1 for control).

The **main** focuses on starting the GUI and the 3 threads. It then waits for their execution to end and closes the program.

The **sender** reads, instead, input messages from the user through the graphic interface and sends them to the server. Also, it checks the written message lenght and its content; if it is `/exit` the thread stops its execution, after having communicated it to the server.

The **listener** continiously reads messages from the server, printing them on the interface window. It also checks message type and, like the sender, if it recives an exit message stops its execution.

The **watchdog** is merely a support thread, because the only job it does is to update the cursor position during the input.


### Utilitychat.h
The file name is self-explaing: it's a library containing utility functions used in the other 2 files.
It's composed of:
- A dedicated section for the inclusion of other external libraries
- A dedicated section for costant definition. This includes ANSI color, message types and the buffer standard size.
- A dedicated section for function declaration.
This includes:
*get_msg*, which returns a message in the **type|name|body|color** format built using strings given as parameters;
*get_field*, that, given a message and a property, returns 1 of 4 parameters of it.
*logmsg*, a function used only by the server to print log messages on the terminal.

More detailed informations about the structure functioning and its implementation are given in the following sections.

---


## **Technical specifications**

### Message structure

Messages exchanged between client and server are strings split in textual fields by a pipeline (`|`). They are generated by the *get_msg* function, which requires 4 strings as parameters:
1 type, 1 sender (name), 1 body and 1 color.
- **Type**: the 2 types of sendable messages are defined in [utilityChat.h](#utilityChat.h) as `EXIT` and `MSG`, respectively for exit messages and broadcast messages.
- **Sender**: is the username that the client types at the connection moment.
- **Body**: is the string inserted by the user when he wants to send a message on the chat.
- **Color**: is used during message printing operation in order to visualize client username on the  interface with univocal colors. The server assigns a color to each client, which is choosen among the available ones.

The compositions happens copying all the parameters and the separatory pipelines in a unique buffer through the *strcat* function. The default buffer size is specified in [utilityChat.h](#utilityChat.h) and is 2KB.

### Server connection
Connection is quite a simple operation to carry out for the client because, after setting all the socket parameters, it only executes the *connect* function, assuming that the server is already listening. If the connection succeeds, the client *main* process takes in input the username, that can't be longer than 30 characters, then it starts the graphic interface. The last operation is to start the 3 threads that, using the newly generated connection, can start doing their job. The *main* keeps waiting for their end.

### Disconnection
Client can disconnect from the chat in 2 ways:
- Typing `/exit` command
- Forcefully closing the terminal

The second solution in quite extreme and is expected to be used as less as possible, however it works. Closing the terminal the client process is closed and the connection is terminated, without warning the server that remains perfectly active. During the implementation phase it was found a problem reguarding sending a void message from the client involuntary during the forced closing which the server wasn't able to manage generating a crash. The solution was to make the client check the body size of every outgoing message and stop the void ones, in order to avoid accidental misunderstandings. As a consequence, the only situation where the server receives a void message is the forced closing one, that is managed automatically disconnecting the client who sent the message.


The first solution instead is the classic one. The user only has to type `/exit` to end the program. The *sendMessage* thread, who controls the outgoing messages body, notices this and send to the server an `EXIT` type message (defined in [utilityChat.h](#utilityChat.h)) to communicate the disconnection. Moreover it sets the global variable `stop` at 1, in order to make the control thread *watchdog* stop, then the sender thread *SendMessage* ends its execution. The server, receiving this message, realizes that the client is disconnecting and sends an exit message as confirmation, then closes the connection deleting the descriptor assignated to the client from the matrix making it newly available. From the client side, also the listening thread *listener* receives the closing message from the server and ends its execution. When all the threads are closed the *main* process is resumed, which destroys the grapgic interface windows cleaning the terminal and returns 0. This confirmations exchange between client and server is due to the fact that threads from the client process can't communicate between them, so the server does somehow a "bridge".


Precisation: the `ctrl+c` key combination has been disabled catching the signal `SIGINT` and managing it through a void function, making available only the previous 2 metods to close the connection.

### Messages exchange
Broadly the messages exchange among the various clients happens starting from a user, who sends a message to the server. The server proceeds forwarding the message to all the other connected users iterating on the socket descriptor matrix.
The detailed execution of the procedure is the following:
1. The *sender* thread takes as input the user message through the interface
2. The thread effectuates a control on the input to verify if the inserted body is void or an exit command
3. Passed the controls, the thread calls the message creation function and sends the message to the server
4. The server receives the broadcast message from the user and effectuates controls on the message type and on its size too
5. If the received message is vaild the connected user matrix is iterated and the message us forwarded to everyone
6. The client *listener* threads receive the forwarded message and effectuate controls on its type
7. If the message is a broadcast one, it's printed in the output window.   


### Graphic Interface

For the input and output simultaneous management have been used threads, without particular problems. However messages visualization in the terminal made a graphic interface necessary, beacause the threads simultaneity in the terminal brought several visual bugs. To solve this problem *curses.h* was used, a library for the terminal management in UNIX-like systems.
In this case the terminal was split in 2 parts:
- the *top* where incoming messages from the server are printed
- the *bottom* where input messages from the user are taken

The base idea was the following: the user is able to write his messages in the *bottom* window whenever he wants, using more than a line if necessary, meanwhile all the messages being exchanged in the group are being visualized in the *top* window following the arrival order.Obviously, once the message is sent, the *bottom* window has to be cleaned, while the *top* one, when is completely filled, has to scroll up to make possible to print new incoming messages.

At the implementation level, the interface is started and ended in the *main* process, respectively whit the creation and the destruction of the 2 windows. Once they're created, through the *box* function a frame made of characters is drawn in each window in order to make its effective size visible. Later, through the *scrollock* function the scroll is abilitated on the specified window, while with *wsetscrreg* an area inside the window where to execute the scroll is specified. Windows are managed by a rhread for each one, that starts later in the same *main* process. Only when threads have ended their execution the *main* is able to destroy the windows and then close the program.
- **Top window management**: the *top* window is only managed by the *listner* thread. Once a message is recieved and the controls are done, this is printed in the window through the *mvwprintw* function, specifying the value of `tline` variable as y coordinate, wich represents the current void line.
Before doing the print, the variable is tested, and if its value corresponds to the end of the window this one scrolls through the *scroll* function, otherwise its value is incrementated. Moreover, if the message to print has a larger size then the window, it's segmented in more lines printed in succession through the same procedure.
- **Bottom window management**: the *bottom* window is managed by the *sender* thread. But in this case *watchdog* thread joins the game, because: the user writes its message in the *bottom* window through the *mvwgetnstr* function, that takes as input a string wich size is equal to the specified one, in this case the maximum message body lenght. Because the function is a blocking one, until the user doesn't send the message the *sende* cannot execute other operations, so the *watchdog* has to continuosly check in a loop the cursor position inside the input window and, if it reaches the end of the line, it moves it at the beginning of the line.
- **Color management**: colors are used by the *listener* during the messages print, in order to visualize each username with the color assigned to it. With *ncurses.h* library the available colors are 15, 7 of them are standard colors and 7 are the corresponding bold version (the last color is black), and they're defined in the *listner* thrugh the *init_pair* function, that associates an integer number to a color pair (foreground/background). When the *listner* receives a message, the color is extracted and, because it is a char type, a cast to integer is effectuated, in order to make possibile to use the color as a reference to a color pair. However the library doen't allow to print a message only partially colored, indeed is needed to use the *wattron* function to activate/deactivate the colored print mode. While thise mode is active everything is printed will be colored according to the specified color pair, consequentially the solution was to prit a colored username on the same coordinates of the white one for each message, in order to overwrite the default one.

---

## Common issues

### Ncurses and threads
The biggest problem in the software development was the *ncurses* library.
The library in its 5.6 version is not thread-safe. This creates conflicts when it's managed by more then one thread at the same time, like in the case of this software. In its 5.7 version instead it has a rough POSIX thread support but to make advantage of it it's necessary to configure it from scratch with the `-pthread` option.
During the development it was tried to configure the library but it failed because of the lack of other necessary libraries.
Particoularly this conflict brought to an incorrect initialization of the graphic interface that appeared distorted at the program launch but as soon as the teminal was resized and a random key was pressed the terminal arranged correctly. Because it wasn't possible to find a solution it was necessary to automatize the terminal resizing through a an apposite command with make possible to set the default terminal size. For the key pressure it was used a *system* call without a specific command, because in C language doesn't exist an appropriate way to simulate a key pressure, and this appeared to solve the problem despite it doesn't have an effective logic correlation with the library.


### Messages management on more lines
The management of multiline input was discussed in the graphic interface section and it didn't cause particoular problems. Instead the multiline print in the *top* bottom did. Indeed this operation was implemented in the following way:

The listner first calculates the lenght of the message that has to be printed and, if this exceeds the window width, proceeds calculating the number of needed lines dividing it for the width itself.
Then a segment of the message wich lenght is equal tothe terminal width is copied in a buffer, then it's printed after the controls on `tline` value, that as said before is updated or scrolled. This procedure is repeated as many times as the number of calculated necessary lines, in order to print the entire message.

###  Unresolved issues

- **Graphic problems**: despite most of the bugs have been solved, problems may happen to the graphic interface due to the instability of the library *ncurses*.
- **Top window scroll**: one of the few problems that still are unresolved happens when messages shown in the *top* window reach the lower edge, so it has to scroll down. Due to the fact that the scroll involves the lateral edges of the window too, they need to be newly printed. The problem occours at the moment of the print of the new edge, indeed the second last line will result lacking of edge. Several attempts to resolve the problem only brought new bugs.

## Conclusions
Despite the  issues and difficulties written before, the software is perfectly able to carry out the operations asked in the request.