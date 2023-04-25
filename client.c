#define _POSIX_C_SOURCE 200809L

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/socket.h>
#include <netdb.h>
#include <string.h>
#include <stdbool.h>
#include <fcntl.h>
#include <errno.h>

#define BUFSIZE 4096

int connect_inet(char *host, char *service) {
    struct addrinfo hints, *info_list, *info;
    int sock, error;

    // look up remote host
    memset(&hints, 0, sizeof(hints));
    hints.ai_family = AF_UNSPEC;  // in practice, this means give us IPv4 or IPv6
    hints.ai_socktype = SOCK_STREAM; // indicate we want a streaming socket

    error = getaddrinfo(host, service, &hints, &info_list);
    if (error) {
        fprintf(stderr, "error looking up %s:%s: %s\n", host, service, gai_strerror(error));
        return -1;
    }

    for (info = info_list; info != NULL; info = info->ai_next) {
        sock = socket(info->ai_family, info->ai_socktype, info->ai_protocol);
        if (sock < 0) continue;

        error = connect(sock, info->ai_addr, info->ai_addrlen);
        if (error) {
            close(sock);
            continue;
        }

        break;
    }
    freeaddrinfo(info_list);

    if (info == NULL) {
        fprintf(stderr, "Unable to connect to %s:%s\n", host, service);
        return -1;
    }

    return sock;
}

int main(int argc, char **argv) {
    if (argc != 3) {
        printf("Specify host and service\n");
        exit(EXIT_FAILURE);
    }
    int sock = connect_inet(argv[1], argv[2]);
    if (sock < 0) exit(EXIT_FAILURE);
    //while (true) {

    char inputBuf[BUFSIZE];
    memset(inputBuf, 0, BUFSIZE * sizeof(char));
    int inputBytesRead;

    char fromServBuf[BUFSIZE];
    memset(fromServBuf, 0, BUFSIZE * sizeof(char));
    int servBytesRead;

    int clientBytesSent;

    int sockFlags = fcntl(sock, F_GETFL);
    fcntl(sock, F_SETFL, sockFlags | O_NONBLOCK);
    int inputFlags = fcntl(STDIN_FILENO, F_GETFL);
    fcntl(STDIN_FILENO, F_SETFL, inputFlags | O_NONBLOCK);

    while (true) {
        if ((servBytesRead = read(sock, fromServBuf, BUFSIZE)) < 0) {
            if (servBytesRead == -1 && errno == EWOULDBLOCK) {
                sleep((unsigned int) 0.1);
            } else if(servBytesRead == -1) {
                perror("Read error");
            }
        } else {
            if (servBytesRead > 0) {
                printf("[Bytes received: %d]\n%s\n", servBytesRead, fromServBuf);
                memset(fromServBuf, 0, BUFSIZE * sizeof(char));
            }
        }
        if ((inputBytesRead = read(STDIN_FILENO, inputBuf, BUFSIZE)) < 0) {
            if (errno == EWOULDBLOCK) {
                sleep((unsigned int) 0.01);
            } else {
                perror("Error");
                break;
            }
        } else {
            write(sock, inputBuf, inputBytesRead);
            memset(inputBuf, 0, BUFSIZE * sizeof(char));
        }
    }
    close(sock);
    return EXIT_SUCCESS;
}
