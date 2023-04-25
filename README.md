# project_3
2023 - Rutgers - cs214 - project3

jn689 & tmb240

INTRO:

This is a simple online tic tac toe game that supports multiple game sessions at once.

IMPLEMENTATION:

Sockets are used to create a connection between server and client. The open_listener function initialize a socket and sets it to listen on a specified port (we use port 16000). The server accepts connection and assigns the client to a tic tac toe game session. Sessions are assigned every time two clients connect to the server.

In the loop, pthread_create is use to create new thread for each game session. It calls the ttt_session function which initializes and start the game. Each game session is given a session id, which is allocated in memory. Pthread allows multiple games to run concurrently.

The function ttt_session contain the game logic. It uses write() to send prompts to the clients and read() to get user inptus such as move,ff, or draw etc. The sockets for each player is set to nonblocking flag, which prevent the client from blocking information when their opponent make commit an action.

After each action/input is made from the client such as connecting to the server or making a move, a protocol is printed. 

For instance turn|bytes|x|enter row,column|

STRING FORMATTING:

A buffer "content" is being used to create a message that can be sent to the server/clients. It uses several strings that are formatted and concateenated togther with sprintf() and strcat(). Strlen() is use to get the bytes of the strings. Additionally for user inputs commands like move, strlen(cmd)-1 is used to remove the \n from the string.


RUNNING THE CODE:

In a linux environment, call 'make' in the terminal which has the files are stored. Simply start the server with ./server. Use your own client to connect to the game. 
Example ./client domain 16000


TEST PLAN:

This is a broad overview of what to examine and test. Please note that messages/prompts will appear after the completion of each item listed below, confirming that the function is working as intended.

Testing network connectivity:
 - Compile and start up the server with ./server 
 Expected output: "listening for incoming connections is printed"
 - Verify that connection works and gamestart with two players 
 Expected output: "Waiting for 2nd player..." & after 2nd player connects, the server will send a message to the client assigning them a role, and prompting them to enter a name.
 - Verify that two or more players can connect to the server and play the game. (Test for multithreading)
 Expected result: should work

Testing gameplay:
 - Enter moves for both players and verify that the game board updates accordingly
 - Verify that the game ends when a player wins or the game is tied
 - Verify that players cannot make moves outside the game board
 - Verify each options work, i.e. request for draw, surrender, move
 
Expected output messages (just examples):
MOVE|6|X|2,2|
MOVD|16|X|2,2|....X....|
INVL|24|That space is occupied.|
DRAW|2|S|

Test performance:
 - Test the game with a large number of players and verify that the server can handle the load (test having like 10+ games running concurrently)
  Expected results: server should be able to handle
 
Of course, this is just bare minimum, in the real-world, we would have more complex measures such as security to protect the user data from being leaked such as their ip/connections, and test the game with slow network connections and verify that the game remains playable etc (perhaps with some sort of network emulator tool).




