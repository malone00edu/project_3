#define _POSIX_C_SOURCE 200809L

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <stdbool.h>
#include <limits.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <netdb.h>
#include <signal.h>
#include <errno.h>
#include <pthread.h>
#include <assert.h>
#include <fcntl.h>
#include <ctype.h>

#define SOCKETERROR (-1)
#define QUEUE_SIZE 8
#define BUFSIZE 255
#define MAXCLIENTS 99
#define CONTENTSIZE 1024

volatile int active = 1;

void install_handlers(void);

int open_listener(char *service, int queue_size);

void *ttt_session(void *);

int check(int exp, const char *msg);

void get_o_name(int gameID, char *playerXPoolName);

void get_x_name(int gameID, char *playerOPoolName);

bool get_move(char player, int socket, int otherplayersock, char board[3][3], bool gameOver);

void get_options(char player, int socket, int gameID, char *otherplayer, char *cmdBuf);

char *strtrim(char *s);

bool reqDraw(int socket, int otherplayersock);

int playerPool[MAXCLIENTS]; // Total amount of game allowed on the server. (Starts at 0. Using array logic.)

typedef struct game {
    int gameID; // Identifies session
    int playerX; // Socket number
    char xName[BUFSIZE];
    bool firstMoveX;
    int playerO; // Socket number
    char oName[BUFSIZE];
    bool firstMoveO;

} game;

game *tttArray;
char **playerInPoolArr;


int main(int argc, char **argv) {

    struct sockaddr_storage remote_host;
    socklen_t remote_host_len;

    char *service = argc == 2 ? argv[1] : "16007";

    playerInPoolArr = calloc(MAXCLIENTS, sizeof(char *));
    tttArray = malloc(50 * sizeof(*tttArray)); // Max 50 sessions w/ a max of two game in each session.

    for (int i = 0; i < 50; i++) {
        tttArray[i].gameID = -1; // Session doesn't exist unless a positive number is assigned.
        memset(tttArray[i].xName, 0, BUFSIZE); // Clearing xName buffer.
        memset(tttArray[i].oName, 0, BUFSIZE); // Clearing oName buffer.
    }

    install_handlers();

    int listener = open_listener(service, QUEUE_SIZE);
    if (listener < 0) exit(EXIT_FAILURE);

    puts("Listening for incoming connections...");
    int currSocket = 0;
    int sessionIndex = 0;
    int currPlayerCount = 4;
    int initialBytes;
    int lenErrMsg;

    bool validName = false;
    bool nameExist = false;

    char nameBuf[BUFSIZE];
    char content[CONTENTSIZE];
    char package[BUFSIZE];

    char invlErrMsg[] = "|Player's name is taken. Try again.|";

    memset(nameBuf, 0, BUFSIZE * sizeof(char));
    memset(content, 0, CONTENTSIZE * sizeof(char));
    memset(package, 0, BUFSIZE * sizeof(char));

    while (active) {
        remote_host_len = sizeof(remote_host);
        check(playerPool[currSocket] = accept(listener, (struct sockaddr *) &remote_host, &remote_host_len),
              "Accept failed");

        //printf("Bytes: %d, %s\n", initialBytes, nameBuf);
        validName = false;
        while (!validName) {
            nameExist = false;
            initialBytes = check(read(playerPool[currSocket], nameBuf, BUFSIZE), "Read failed");
            if (initialBytes > 0) {
                nameBuf[strlen(nameBuf) - 1] = '\0';
                // Starts at 4 because 0 to 3 is used already. 3 being the server.
                for (int j = 4; j < currPlayerCount; j++) {
                    if (strcmp(playerInPoolArr[j], nameBuf) == 0) {
                        lenErrMsg = strlen(invlErrMsg);
                        sprintf(content, "%d|%s", lenErrMsg, invlErrMsg);
                        strcpy(package, "INVL|");
                        strcat(package, content);
                        check(write(playerPool[currSocket], package, strlen(package)), "Send failed");
                        nameExist = true;
                        memset(nameBuf, 0, BUFSIZE * sizeof(char));
                        memset(content, 0, CONTENTSIZE * sizeof(char));
                        memset(package, 0, BUFSIZE * sizeof(char));
                        break;
                    }
                }
                if (!nameExist) {
                    playerInPoolArr[playerPool[currSocket]] = strdup(nameBuf);
                    printf("NAME|%lu|%s|\n", strlen(nameBuf) + 1, playerInPoolArr[playerPool[currSocket]]);
                    strcpy(package, "WAIT|0|");
                    check(write(playerPool[currSocket], package, strlen(package)), "Write failed");
                    validName = true;
                    currPlayerCount++;
                    memset(package, 0, BUFSIZE * sizeof(char));
                }
            }
        }
        // Count starts at 0. Notice: 1st two game will put the currSocket count at 1. (Using array index logic)
        if (currSocket % 2 != 0) {
            printf("Got 2 players!\n");
            tttArray[sessionIndex].gameID = sessionIndex;
            tttArray[sessionIndex].playerX = playerPool[currSocket - 1];
            tttArray[sessionIndex].playerO = playerPool[currSocket];
            tttArray[sessionIndex].firstMoveX = true;
            tttArray[sessionIndex].firstMoveO = true;

            pthread_t t;
            int *sessionID = malloc(sizeof(int));
            *sessionID = tttArray[sessionIndex].gameID;
            if (pthread_create(&t, NULL, ttt_session, (void *) sessionID) != 0) {
                perror("Could not create thread");
                exit(1);
            }
            sessionIndex++;
        } else {
            printf("Waiting for 2nd player...\n");
        }
        currSocket++;
    }

    free(playerInPoolArr);
    return 0;
}

// Check if errno was set to -1
int check(int exp, const char *msg) {
    if (exp == SOCKETERROR) {
        perror(msg);
        exit(1);
    }
    return exp;
}

void handler(int signum) {
    active = 0;
}

void install_handlers(void) {
    struct sigaction act;
    act.sa_handler = handler;
    act.sa_flags = 0;
    sigemptyset(&act.sa_mask);

    sigaction(SIGINT, &act, NULL);
    sigaction(SIGTERM, &act, NULL);
}

int open_listener(char *service, int queue_size) {
    struct addrinfo hint, *info_list, *info;
    int error, sock;

    // initialize hints
    memset(&hint, 0, sizeof(struct addrinfo));
    hint.ai_family = AF_UNSPEC;
    hint.ai_socktype = SOCK_STREAM;
    hint.ai_flags = AI_PASSIVE;

    // obtain information for listening socket
    error = getaddrinfo(NULL, service, &hint, &info_list);
    if (error) {
        fprintf(stderr, "getaddrinfo: %s\n", gai_strerror(error));
        return -1;
    }

    // attempt to create socket
    for (info = info_list; info != NULL; info = info->ai_next) {
        sock = socket(info->ai_family, info->ai_socktype, info->ai_protocol);
        // if we could not create the socket, try the next method
        if (sock == -1) continue;
        // bind socket to requested port
        error = bind(sock, info->ai_addr, info->ai_addrlen);
        if (error) {
            close(sock);
            continue;
        }
        // enable listening for incoming connection requests
        error = listen(sock, queue_size);
        if (error) {
            close(sock);
            continue;
        }
        // if we got this far, we have opened the socket
        break;
    }

    freeaddrinfo(info_list);
    // info will be NULL if no method succeeded
    if (info == NULL) {
        fprintf(stderr, "Could not bind\n");
        return -1;
    }
    return sock;
}

void *ttt_session(void *sessionID) {
    int gameID = *((int *) sessionID);
    assert(gameID != -1);

    printf("[After calling pthread_create; getpid: %d, getpthread_self: %lu]\n", getpid(), pthread_self());
    printf("Session %d is active.\n", tttArray[gameID].gameID + 1);

    int xBytesReceived = 0, oBytesReceived = 0;
    char xMessageBuf[BUFSIZE], oMessageBuf[BUFSIZE], sMessageBuf[BUFSIZE];
    bool xNameAssigned = false, oNameAssigned = false;
    int playerXSocket = tttArray[gameID].playerX; // Player X's socket number
    int playerOSocket = tttArray[gameID].playerO; // player O's socket number
    int flagsX = fcntl(playerXSocket, F_GETFL);
    int flagsO = fcntl(playerOSocket, F_GETFL);

    memset(xMessageBuf, 0, BUFSIZE * sizeof(char));
    memset(oMessageBuf, 0, BUFSIZE * sizeof(char));
    memset(sMessageBuf, 0, BUFSIZE * sizeof(char));

    fcntl(playerXSocket, F_SETFL, flagsX | O_NONBLOCK);
    fcntl(playerOSocket, F_SETFL, flagsO | O_NONBLOCK);

    char introBuffX[] = "\nWelcome player X!\n\nYou will go 1st.\nIf there is any wait time, then player O is deciding,\n"
                        "or a connection has been dropped. =/ (You will be alerted.)";
    char introBuffO[] = "\nWelcome player O!\n\nYou will go 2nd.\nIf there is any wait time, then player X is deciding,\n"
                        "or a connection has been dropped. =/ (You will be alerted.)";

    // Greet players.
    check(write(playerXSocket, introBuffX, strlen(introBuffX)), "Send failed");
    check(write(playerOSocket, introBuffO, strlen(introBuffO)), "Send failed");

    // Get player X and O's name from pool of array names.
    get_x_name(gameID, playerInPoolArr[playerXSocket]);
    get_o_name(gameID, playerInPoolArr[playerOSocket]);

    // Extract playerX's name from struct array.
    char playerXName[BUFSIZE];
    memset(playerXName, 0, BUFSIZE);
    strcpy(playerXName, tttArray[gameID].xName);
    int lenX = strlen(playerXName);
    if (lenX > 0 && playerXName[lenX - 1] == '\n') playerXName[lenX - 1] = '\0';

    // Extract playerO's name from struct array.
    char playerOName[BUFSIZE];
    memset(playerOName, 0, BUFSIZE);
    strcpy(playerOName, tttArray[gameID].oName);
    int lenO = strlen(playerOName);
    if (lenO > 0 && playerOName[lenO - 1] == '\n') playerOName[lenO - 1] = '\0';

    char sessionMsg[] = "\nIn session with ";

    strcpy(oMessageBuf, sessionMsg);
    strcat(oMessageBuf, playerXName);
    check(write(playerOSocket, oMessageBuf, strlen(oMessageBuf)), "Send failed");
    memset(oMessageBuf, 0, BUFSIZE);

    strcpy(xMessageBuf, sessionMsg);
    strcat(xMessageBuf, playerOName);
    check(write(playerXSocket, xMessageBuf, strlen(xMessageBuf)), "Send failed");
    memset(xMessageBuf, 0, BUFSIZE);


    char board[3][3] = {{'.', '.', '.'},
                        {'.', '.', '.'},
                        {'.', '.', '.'}};


    //write code to send a blank board to the clients here
    char boardString[BUFSIZE];
    sprintf(boardString,
            "\n   1   2   3\n1  %c | %c | %c \n  ---+---+---\n2  %c | %c | %c \n  ---+---+---\n3  %c | %c | %c \n",
            board[0][0], board[0][1], board[0][2],
            board[1][0], board[1][1], board[1][2],
            board[2][0], board[2][1], board[2][2]);

    check(write(playerXSocket, boardString, strlen(boardString)), "Send failed");
    check(write(playerOSocket, boardString, strlen(boardString)), "Send failed");

    //create loop here that will get move from player x first then player o
    //write code here

// Initialize the game over flag
    bool gameOver = false;

    fcntl(playerXSocket, F_SETFL, flagsX);
    fcntl(playerOSocket, F_SETFL, flagsO);

    char winLINE[] = "W|You've won! Reason: By completing a line.|\n";
    char loseLINE[] = "L|You've lost. Reason: Other player completed a line.|\n";
    char loseRSGN[] = "L|You've lost. Reason: Surrendered.|\n";
    char draw[] = "D|Game Tied!|\n";
    int rounds = 0;

    char xFF[] = "W|You've won! Reason: Player X has surrendered.|\n";
    char oFF[] = "W|You've won! Reason: Player O has surrendered.|\n";

    char drawaccept[] = "D|Both players accepted the draw! The game is tied.|\n";
    char drawreject[] = "D|Your opponent rejected the draw, the game will continue.|\n";

    char serverMsgO[] = "OVER|";

    // Loop until the game is over
    while (!gameOver) {

        char package[BUFSIZE];
        char content[CONTENTSIZE];
        memset(package, 0, BUFSIZE * sizeof(char));
        memset(content, 0, CONTENTSIZE * sizeof(char));
        char cmd[BUFSIZE];

        get_options('X', playerXSocket, gameID, tttArray[gameID].oName, cmd);

        if (strcmp(cmd, "move") == 0) {
            // Get a move from player X
            gameOver = get_move('X', playerXSocket, playerOSocket, board, gameOver);
            if (gameOver) {
                memset(package, 0, BUFSIZE * sizeof(char));
                memset(content, 0, CONTENTSIZE * sizeof(char));
                sprintf(content, "%lu|%s", strlen(winLINE) - 1, winLINE);
                strcat(package, serverMsgO);
                strcat(package, content);
                check(write(playerXSocket, package, strlen(package)), "Send failed");
                memset(package, 0, BUFSIZE * sizeof(char));
                memset(content, 0, CONTENTSIZE * sizeof(char));
                sprintf(content, "%lu|%s", strlen(loseLINE) - 1, loseLINE);
                strcat(package, serverMsgO);
                strcat(package, content);
                check(write(playerOSocket, package, strlen(package)), "Send failed");
                break;
            }
            rounds++;
        } else if (strcmp(cmd, "ff") == 0) {
            printf("RSGN|%d|X|\n", 2);
            memset(package, 0, BUFSIZE * sizeof(char));
            memset(content, 0, CONTENTSIZE * sizeof(char));
            sprintf(content, "%lu|%s", strlen(loseRSGN) - 1, loseRSGN);
            strcat(package, serverMsgO);
            strcat(package, content);
            check(write(playerXSocket, package, strlen(package)), "Send failed");
            memset(package, 0, BUFSIZE * sizeof(char));
            memset(content, 0, CONTENTSIZE * sizeof(char));
            sprintf(content, "%lu|%s", strlen(xFF) - 1, xFF);
            strcat(package, serverMsgO);
            strcat(package, content);
            check(write(playerOSocket, package, strlen(package)), "Send failed");
            break;
        } else {
            printf("DRAW|%d|S|\n", 2);
            bool acceptDraw = reqDraw(playerXSocket, playerOSocket);
            if (acceptDraw) {
                printf("DRAW|%d|A|\n", 2);
                memset(package, 0, BUFSIZE * sizeof(char));
                memset(content, 0, CONTENTSIZE * sizeof(char));
                sprintf(content, "%lu|%s", strlen(drawaccept) - 1, drawaccept);
                strcat(package, serverMsgO);
                strcat(package, content);
                check(send(playerXSocket, package, strlen(package), O_NONBLOCK), "Send failed");
                check(send(playerOSocket, package, strlen(package), O_NONBLOCK), "Send failed");
                break;
            } else {
                //continue the game
                printf("DRAW|%d|R|\n", 2);
                memset(package, 0, BUFSIZE * sizeof(char));
                memset(content, 0, CONTENTSIZE * sizeof(char));
                sprintf(content, "%lu|%s", strlen(drawreject) - 1, drawreject);
                strcat(package, serverMsgO);
                strcat(package, content);
                check(write(playerXSocket, package, strlen(package)), "Send failed");

                gameOver = get_move('X', playerXSocket, playerOSocket, board, gameOver);
                if (gameOver) {
                    memset(package, 0, BUFSIZE * sizeof(char));
                    memset(content, 0, CONTENTSIZE * sizeof(char));
                    sprintf(content, "%lu|%s", strlen(winLINE) - 1, winLINE);
                    strcat(package, serverMsgO);
                    strcat(package, content);
                    check(write(playerXSocket, package, strlen(package)), "Send failed");
                    memset(package, 0, BUFSIZE * sizeof(char));
                    memset(content, 0, CONTENTSIZE * sizeof(char));
                    sprintf(content, "%lu|%s", strlen(loseLINE) - 1, loseLINE);
                    strcat(package, serverMsgO);
                    strcat(package, content);
                    check(write(playerOSocket, package, strlen(package)), "Send failed");
                    break;
                }
                rounds++;
            }
        }

        //draw
        if (rounds == 9) {
            memset(package, 0, BUFSIZE * sizeof(char));
            memset(content, 0, CONTENTSIZE * sizeof(char));
            sprintf(content, "%lu|%s", strlen(draw) - 1, draw);
            strcat(package, serverMsgO);
            strcat(package, content);
            check(write(playerXSocket, package, strlen(package)), "Send failed");
            check(write(playerOSocket, package, strlen(package)), "Send failed");
            break;
        }

        //do the same for player O
        get_options('O', playerOSocket, gameID, tttArray[gameID].xName, cmd);

        if (strcmp(cmd, "move") == 0) {
            // Get a move from player X
            gameOver = get_move('O', playerOSocket, playerXSocket, board, gameOver);
            if (gameOver) {
                memset(package, 0, BUFSIZE * sizeof(char));
                memset(content, 0, CONTENTSIZE * sizeof(char));
                sprintf(content, "%lu|%s", strlen(winLINE) - 1, winLINE);
                strcat(package, serverMsgO);
                strcat(package, content);
                check(write(playerOSocket, package, strlen(package)), "Send failed");
                memset(package, 0, BUFSIZE * sizeof(char));
                memset(content, 0, CONTENTSIZE * sizeof(char));
                sprintf(content, "%lu|%s", strlen(loseLINE) - 1, loseLINE);
                strcat(package, serverMsgO);
                strcat(package, content);
                check(write(playerXSocket, package, strlen(package)), "Send failed");
                break;
            }
            rounds++;
        } else if (strcmp(cmd, "ff") == 0) {
            printf("RSGN|%d|O|\n", 2);
            memset(package, 0, BUFSIZE * sizeof(char));
            memset(content, 0, CONTENTSIZE * sizeof(char));
            sprintf(content, "%lu|%s", strlen(loseRSGN) - 1, loseRSGN);
            strcat(package, serverMsgO);
            strcat(package, content);
            check(write(playerOSocket, package, strlen(package)), "Send failed");
            memset(package, 0, BUFSIZE * sizeof(char));
            memset(content, 0, CONTENTSIZE * sizeof(char));
            sprintf(content, "%lu|%s", strlen(oFF) - 1, oFF);
            strcat(package, serverMsgO);
            strcat(package, content);
            check(write(playerXSocket, package, strlen(package)), "Send failed");
            break;
        } else {
            printf("DRAW|%d|S|\n", 2);
            bool acceptDraw = reqDraw(playerOSocket, playerXSocket);
            if (acceptDraw) {
                memset(package, 0, BUFSIZE * sizeof(char));
                memset(content, 0, CONTENTSIZE * sizeof(char));
                sprintf(content, "%lu|%s", strlen(drawaccept) - 1, drawaccept);
                strcat(package, serverMsgO);
                strcat(package, content);
                printf("DRAW|%d|A|\n", 2);
                check(write(playerOSocket, package, strlen(package)), "Send failed");
                check(write(playerXSocket, package, strlen(package)), "Send failed");
                break;
            } else {
                //continue the game
                memset(package, 0, BUFSIZE * sizeof(char));
                memset(content, 0, CONTENTSIZE * sizeof(char));
                sprintf(content, "%lu|%s", strlen(drawreject) - 1, drawreject);
                strcat(package, serverMsgO);
                strcat(package, content);
                printf("DRAW|%d|R|\n", 2);
                check(write(playerOSocket, package, strlen(package)), "Send failed");

                gameOver = get_move('O', playerOSocket, playerXSocket, board, gameOver);
                if (gameOver) {
                    memset(package, 0, BUFSIZE * sizeof(char));
                    memset(content, 0, CONTENTSIZE * sizeof(char));
                    sprintf(content, "%lu|%s", strlen(winLINE) - 1, winLINE);
                    strcat(package, serverMsgO);
                    strcat(package, content);
                    check(write(playerOSocket, package, strlen(package)), "Send failed");
                    memset(package, 0, BUFSIZE * sizeof(char));
                    memset(content, 0, CONTENTSIZE * sizeof(char));
                    sprintf(content, "%lu|%s", strlen(loseLINE) - 1, loseLINE);
                    strcat(package, serverMsgO);
                    strcat(package, content);
                    check(write(playerXSocket, package, strlen(package)), "Send failed");
                    break;
                }
                rounds++;
            }
        }
    }

    free(sessionID);
    close(playerXSocket);
    close(playerOSocket);
    return NULL;
}


char *strtrim(char *s) {
    // Trim leading whitespace
    while (isspace(*s)) {
        s++;
    }

    // Trim trailing whitespace
    char *end = s + strlen(s) - 1;
    while (end > s && isspace(*end)) {
        end--;
    }
    *(end + 1) = '\0';

    return s;
}

bool reqDraw(int socket, int otherplayersock) {
    bool validReply = false;

    char package[BUFSIZE];
    char content[CONTENTSIZE];
    memset(package, 0, BUFSIZE * sizeof(char));
    memset(content, 0, CONTENTSIZE * sizeof(char));
    char serverMsg[] = "DRAW|";
    char updatePlayer1[] = "Requesting draw ...waiting...|";
    sprintf(content, "%ld|%s", strlen(updatePlayer1), updatePlayer1);
    strcat(package, serverMsg);
    strcat(package, content);
    check(write(socket, package, strlen(package)), "Send failed");

    bool acceptDraw = false;
    char optionBuf[BUFSIZE];
    while (!validReply) {

        memset(package, 0, BUFSIZE * sizeof(char));
        memset(content, 0, CONTENTSIZE * sizeof(char));
        char updatePlayer2[] = "\"Your opponent requested for a draw, accept request? Y or N|";
        sprintf(content, "%ld|%s", strlen(updatePlayer2), updatePlayer2);
        strcat(package, serverMsg);
        strcat(package, content);
        check(write(otherplayersock, package, strlen(package)), "Send failed");


        memset(optionBuf, 0, BUFSIZE * sizeof(char));
        int bytesReceived = read(otherplayersock, optionBuf, BUFSIZE);
        if (bytesReceived <= 0) {
            continue;
        }

        char *trimmedBuf = strtrim(optionBuf);

        if (strcmp(trimmedBuf, "Y") == 0) {
            acceptDraw = true;
            validReply = true;
        } else if (strcmp(trimmedBuf, "N") == 0) {
            acceptDraw = false;
            validReply = true;
        } else {
            memset(package, 0, BUFSIZE * sizeof(char));
            memset(content, 0, CONTENTSIZE * sizeof(char));
            char serverMsgINVL[] = "INVL|";
            char updatePlayerRETRY[] = "RETRY|\n";
            sprintf(content, "%ld|%s", strlen(updatePlayerRETRY) - 1, updatePlayerRETRY);
            strcat(package, serverMsgINVL);
            strcat(package, content);
            check(write(otherplayersock, package, strlen(package)), "Send failed");
            continue;
        }
    }
    return acceptDraw;
}

void get_options(char player, int socket, int gameID, char *otherplayer, char *cmdBuf) {
    //send options to player X
    bool validCMD = false;
    char package[BUFSIZE];
    char content[CONTENTSIZE];
    char *playerX = "X";
    char *playerO = "O";
    char firstPlayMsg[] = "BEGN|";
    char serverMsg1[] = "TURN|";
    char updatePlayer1[] = "Your options are: move, ff, draw|";
    char serverMsg2[] = "INVL|";
    char updatePlayer2[] = "RETRY|\n";
    while (!validCMD) {
        if ((socket == tttArray[gameID].playerX) && (tttArray[gameID].firstMoveX)) {
            memset(package, 0, BUFSIZE * sizeof(char));
            memset(content, 0, CONTENTSIZE * sizeof(char));
            sprintf(content, "%lu|%c|%s|\n", strlen(otherplayer) + 3, player, otherplayer);
            strcat(package, firstPlayMsg);
            strcat(package, content);
            check(write(socket, package, strlen(package)), "Send failed");
            tttArray[gameID].firstMoveX = false;
        }
        if ((socket == tttArray[gameID].playerO) && (tttArray[gameID].firstMoveO)) {
            memset(package, 0, BUFSIZE * sizeof(char));
            memset(content, 0, CONTENTSIZE * sizeof(char));
            sprintf(content, "%lu|%c|%s|\n", strlen(otherplayer) + 2, player, otherplayer);
            strcat(package, firstPlayMsg);
            strcat(package, content);
            check(write(socket, package, strlen(package)), "Send failed");
            tttArray[gameID].firstMoveO = false;
        }
        memset(package, 0, BUFSIZE * sizeof(char));
        memset(content, 0, CONTENTSIZE * sizeof(char));
        sprintf(content, "%lu|%c|%s", strlen(updatePlayer1) + 2, player, updatePlayer1);
        strcat(package, serverMsg1);
        strcat(package, content);

        check(write(socket, package, strlen(package)), "Send failed");

        //clear buffer
        memset(cmdBuf, 0, BUFSIZE * sizeof(char));
        //if invalid read then reenter
        int bytesReceived = read(socket, cmdBuf, BUFSIZE);
        if (bytesReceived <= 0) {
            sleep(1);
            continue;
        }

        char *trimmedBuf = strtrim(cmdBuf);

        if (strcmp(trimmedBuf, "move") == 0 || strcmp(trimmedBuf, "ff") == 0 || strcmp(trimmedBuf, "draw") == 0) {
            validCMD = true;
        } else {
            memset(package, 0, BUFSIZE * sizeof(char));
            memset(content, 0, CONTENTSIZE * sizeof(char));
            sprintf(content, "%ld|%s", strlen(updatePlayer2) - 1, updatePlayer2); // Not including '\n' in byte count.
            strcat(package, serverMsg2);
            strcat(package, content);
            check(write(socket, package, strlen(package)), "Send failed");
            continue;
        }
    }
}

bool get_move(char player, int socket, int otherplayersock, char board[3][3], bool gameOver) {

    bool validMove = false;
    char movebuf[BUFSIZE];
    char boardString[BUFSIZE];

    while (!validMove) {
        char package[BUFSIZE];
        char content[CONTENTSIZE];
        memset(package, 0, BUFSIZE * sizeof(char));
        memset(content, 0, CONTENTSIZE * sizeof(char));
        char serverMsg1[] = "TURN|";
        char updatePlayer1[] = "Enter your move as row,column|";
        sprintf(content, "%lu|%c|%s", strlen(updatePlayer1) + 2, player, updatePlayer1);
        strcat(package, serverMsg1);
        strcat(package, content);
        check(write(socket, package, strlen(package)), "Send failed");

        // Clear the buffer before receiving input from Player O
        memset(movebuf, 0, BUFSIZE * sizeof(char));

        int bytesReceived = read(socket, movebuf, BUFSIZE);
        if (bytesReceived <= 0) {
            continue;
        }

        // Remove '\n' from string
        movebuf[strlen(movebuf) - 1] = '\0';
        int row = 0, col = 0;
        if (sscanf(movebuf, "%d,%d", &row, &col) != 2 || row < 1 || row > 3 || col < 1 || col > 3 ||
            board[row - 1][col - 1] != '.') {
            memset(package, 0, BUFSIZE * sizeof(char));
            memset(content, 0, CONTENTSIZE * sizeof(char));
            char serverMsg2[] = "INVL|";
            char updatePlayer2[] = "Not a valid move. Try again.|\n";
            sprintf(content, "%ld|%s", strlen(updatePlayer2) - 1, updatePlayer2); // Not including '\n' in byte count.
            strcat(package, serverMsg2);
            strcat(package, content);
            check(write(socket, package, strlen(package)), "Send failed");

        } else {

            row--;
            col--;

            board[row][col] = player;
            validMove = true;
            //updates board
            sprintf(boardString,
                    "%c%c%c%c%c%c%c%c%c",
                    board[0][0], board[0][1], board[0][2],
                    board[1][0], board[1][1], board[1][2],
                    board[2][0], board[2][1], board[2][2]);

            //insert win condition here

            //horizontal win
            if (board[row][col] == board[0][0] && board[0][0] == board[0][1] && board[0][0] == board[0][2]) {

                gameOver = true;
            } else if (board[row][col] == board[1][0] && board[1][0] == board[1][1] && board[1][0] == board[1][2]) {

                gameOver = true;
            } else if (board[row][col] == board[2][0] && board[2][0] == board[2][1] && board[2][0] == board[2][2]) {

                gameOver = true;
            }

                //vertical win
            else if (board[row][col] == board[0][0] && board[0][0] == board[1][0] && board[0][0] == board[2][0]) {

                gameOver = true;
            } else if (board[row][col] == board[0][1] && board[0][1] == board[1][1] && board[0][1] == board[2][1]) {

                gameOver = true;
            } else if (board[row][col] == board[0][2] && board[0][2] == board[1][2] && board[0][2] == board[2][2]) {

                gameOver = true;
            }

                //diagnol win
            else if (board[row][col] == board[0][0] && board[0][0] == board[1][1] && board[0][0] == board[2][2]) {

                gameOver = true;
            } else if (board[row][col] == board[0][2] && board[0][2] == board[1][1] && board[0][2] == board[2][0]) {

                gameOver = true;
            }
        }
    }
    //prints updated board
    printf("MOVE|%lu|%c|%s|\n", strlen(movebuf) + 3, player, movebuf);
    char serverMsg[] = "MOVD|";
    char package[BUFSIZE];
    char content[CONTENTSIZE];
    memset(package, 0, BUFSIZE * sizeof(char));
    memset(content, 0, CONTENTSIZE * sizeof(char));
    sprintf(content, "%ld|%c|%s|%s|\n", strlen(movebuf) + strlen(boardString) + 4, player, movebuf, boardString);
    strcat(package, serverMsg);
    strcat(package, content);
    check(write(socket, package, strlen(package)), "Send failed");
    check(write(otherplayersock, package, strlen(package)), "Send failed");
    return gameOver;
}

// Get player X's name
void get_x_name(int gameID, char *playerOPoolName) {
    strcpy(tttArray[gameID].xName, playerOPoolName);
}

// Get player O's name
void get_o_name(int gameID, char *playerXPoolName) {
    strcpy(tttArray[gameID].oName, playerXPoolName);
}
