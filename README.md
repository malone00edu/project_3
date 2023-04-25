# project_3
2023 - Rutgers - cs214 - project3

jn689 & tmb240

INTRO:

This is a simple online tic tac toe game that supports multiple game sessions at once.

IMPLEMENTATION:

Sockets are used to create a connection between server and client. The open_listener function initialize a socket and sets it to listen on a specified port (we use port 16000). The server accepts connection and assigns the client to a tic tac toe game session. Sessions are assigned every time two clients connect to the server.

In the loop, pthread_create is use to create new thread for each game session. It calls the ttt_session function which initializes and start the game. Each game session is given a session id, which is allocated in memory. Pthread allows multiple games to run concurrently.

The function ttt_session contain the game logic. It uses write() to send prompts to the clients and read() to get user inptus such as move,ff, or draw etc. The sockets for each player is set to nonblocking flag, which prevent the client from blocking information when their opponent make commit an action.

STRING FORMATTING:

After each action/input is made from the client such as connecting to the server or making a move, a protocol is printed. 

For instance turn|bytes|x|enter row,column|

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
 Expected output: Enter name prompt will display
 - Verify that two or more players can connect to the server and play the game. (Test for multithreading)
 Expected result: should work

Testing gameplay:
 - Enter moves for both players and verify that the game board updates accordingly
 - Verify that the game ends when a player wins or the game is tied
 - Verify that players cannot make moves outside the game board
 - Verify each options work, i.e. request for draw, surrender, move

 
EXAMPlE OFOUTPUT MSGES FROM THE SERVER SIDE:
Listening for incoming connections...
PlAY|5|john|
Waiting for 2nd player...
PlAY|4|bob|
Got 2 players!
[After calling pthread_create; getpid: 4054922, getpthread_self: 140575537379072]
Session 1 is active.
MOVE|6|X|1,2|
DRAW|2|S|
DRAW|2|R|
MOVE|6|O|1,3|
RSGN|2|X|


Testing performance:
 - Test the game with a large number of players and verify that the server can handle the load (test having like 5+ games running concurrently)
  Expected results: server should be able to handle
 - Repeatedly do a command like draw, while doing other things simulatiousley like connecting another client to the server. (Stress test)
  Expected results: server should be able to handle
  
Other tests:
 - Check to make sure all the errors display correctly when inputting wrong messages, names that are taken, invalid moves etc.
 - Test what happens when a client disconnects after being matched or game ended (doesn't matter when, at the start of the game,middle, after etc)
  Expected results: Current game session should just stop and memory is freed for future use.
  
 
Of course, this is just bare minimum, in the real-world, we would have more complex measures such as security to protect the user data from being leaked such as their ip/connections, and test the game with slow network connections and verify that the game remains playable etc (perhaps with some sort of network emulator tool).




