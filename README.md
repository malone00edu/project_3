# project_3
2023 - Rutgers - cs214 - project3

jn689 & tmb240

INTRO:
A basic online tic tac toe game which supports multiple games to be run concurrently.

IMPLEMENTATION:

Sockets are used to create a connection between server and client. The open_listener function initialize a socket and sets it to listen on a specified port (we use port 16000). The server accepts connection and assigns the client to a tic tac toe game session. Sessions are assigned every time two clients connect to the server.

In the loop, pthread_create is use to create new thread for each game session. It calls the ttt_session function which initializes and start the game. Each game session is given a session id, which is allocated in memory. Pthread allows multiple games to run concurrently.

The function ttt_session contain the game logic. It uses write() to send prompts to the clients and read() to get user inptus such as move,ff, or draw etc. The sockets for each player is set to nonblocking flag, which prevent the client from blocking information when their opponent make commit an action.

After each action/input is made from the client such as connecting to the server or making a move, a protocol is printed. 

For instance turn|bytes|x|enter row,column|

String formatting

A buffer "content" is being used to create a message that can be sent to the server/clients. It uses several strings that are formatted and concateenated togther with sprintf() and strcat(). Strlen() is use to get the bytes of the strings. Additionally for user inputs commands like move, strlen(cmd)-1 is used to remove the \n from the string.


RUNNING THE CODE:
In a linux environment, call 'make' in the terminal which has the files are stored. Simply start the server with ./server. Use your own client to connect to the game.
Example ./client domain 16000


TEST PLAN

