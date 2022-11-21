#include <stdio.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <string.h>
#include <unistd.h>
#include <signal.h>
#include <netinet/in.h>
#include <stdlib.h>
#include <arpa/inet.h>
#include <time.h>

#define IP_ADDRESS "127.0.0.1"
#define PORT 57300


int main() {
    struct sockaddr_in serverAddress;
    serverAddress.sin_family = AF_INET;
    serverAddress.sin_port = htons(PORT);
    inet_aton(IP_ADDRESS, &serverAddress.sin_addr);

    int serverFd = socket(AF_INET, SOCK_STREAM, 0);

    if(serverFd == -1 ) {
        perror("socket");
        return -1;
    }

    if( connect(serverFd, (struct sockaddr *) &serverAddress, sizeof(struct sockaddr_in) ) == -1 ) {
        perror("connect");
        return -1;
    }

    char *command = malloc(sizeof(char) * BUFSIZ);

    char *response = malloc(sizeof(char) * BUFSIZ);

    for(;;) {
        // createRandomMsg(randomMsg, RANDOM_MSG_LENGTH);
        printf("send: ");
        fgets(command, BUFSIZ - 1, stdin);
        printf("\n");
        fflush(stdout);
        if(command)
            send(serverFd, command, strlen(command), 0);

        // real implementations shouldn't assume the entire message will be present in 1 read
        int bytesRead = read(serverFd, response, BUFSIZ - 1);
        response[bytesRead] = 0;
        printf("%s\n", response);

        if (strcmp(command, ">EXIT\n") == 0) break;
        else sleep(1);
    }

    close(serverFd);
    return 0;
}