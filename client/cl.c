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
#include <sys/ioctl.h>

#define IP_ADDRESS "127.0.0.1"
#define PORT 57300


void getInput(char *input) {
    struct winsize w;
    ioctl(STDOUT_FILENO, TIOCGWINSZ, &w);

    for(int i = 0; i < w.ws_col; i++) printf("-");
    printf("\n| send: ");
    fgets(input, BUFSIZ - 1, stdin);
    for(int i = 0; i < w.ws_col; i++) printf("-");
    printf("\n");
    fflush(stdout);
}


int main(int argc, char *argv[]) {
    char *message;

    if(argc == 2){
        message = malloc(strlen("HELLO\n") + strlen(argv[1]) + 1);
        strcpy(message, "HELLO");
        strcat(message, "\n");
        strcat(message, argv[1]);
    }
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


    // if(message)
    //         send(serverFd, message, strlen(message), 0);

    // // real implementations shouldn't assume the entire message will be present in 1 read
    // int bytesRead = read(serverFd, response, BUFSIZ - 1);
    // response[bytesRead] = 0;
    // printf("%s", response);
    //                                                      // THIS IS FOR DEBUG ONLY
    // bytesRead = read(serverFd, response, BUFSIZ - 1);
    // response[bytesRead] = 0;
    // printf("%s", response);
    // fflush(stdout);

    for(;;) {
        char *command = malloc(sizeof(char) * BUFSIZ);

        char *response = malloc(sizeof(char) * BUFSIZ);
        char *response_len = malloc(sizeof(char) * BUFSIZ);
        // print 25 dashes 
        getInput(command);
        if(command)
            send(serverFd, command, strlen(command), 0);

        // real implementations shouldn't assume the entire message will be present in 1 read
        int bytesRead = read(serverFd, response_len, BUFSIZ - 1);
        response_len[bytesRead] = 0;
        // printf("bytesRead: %d\n", bytesRead);
        // printf("response_len: %s\n", response_len);
        // fflush(stdout);
        bytesRead = 0;
        while(bytesRead < atoi(response_len)){
            bytesRead += read(serverFd, &response[bytesRead], BUFSIZ - 1);
            response[bytesRead] = 0;
        }
        // printf("%d bytes read\n", bytesRead);
        printf("%s", response);
        
        free(command);
        free(response);
        free(response_len);

        if (strcmp(command, ">EXIT\n") == 0) break;
        // else sleep(1);
    }

    close(serverFd);
    return 0;
}