#include "gopher.h"

/**
 * Checks the arguments passed to gopher
 */
void check_args(int argc, char** argv) {
    if (argc != 2) {
        fprintf(stderr, "Usage: gopher port\n");
        exit(ARG_COUNT);
    }
    if (strcmp(argv[1], "0")) {
        if (!atoi(argv[1])) {
            fprintf(stderr, "Failed to connect\n");
            exit(CONNECTION);
        }
    }

}

int main(int argc, char** argv) {
    check_args(argc, argv);
    struct SessionInfo* sessionInfo = malloc(sizeof(struct SessionInfo));
    int sockfd, status;
    sockfd = socket(AF_INET, SOCK_STREAM, 0);
    char* buffer;
    int port;
    if (!strcmp(argv[1], "0")) {
        port = 0;
    } else {
        port = atoi(argv[1]);
    }
    // specify an address for the socket
    struct sockaddr_in serverAddress;
    serverAddress.sin_family = AF_INET; 
    serverAddress.sin_port = htons(port);
    serverAddress.sin_addr.s_addr = INADDR_ANY;
    
    if ((status = connect(sockfd, (struct sockaddr*) &serverAddress, 
            sizeof(serverAddress))) == -1) {
        fprintf(stderr, "Failed to connect\n");
        exit(CONNECTION);
    }
    sessionInfo->fromServer = fdopen(sockfd, "r");
    sessionInfo->toServer = fdopen(sockfd, "w");

    fprintf(sessionInfo->toServer, "scores\n");
    fflush(sessionInfo->toServer);
    int count;
    if ((count = read_line(sessionInfo->fromServer, &buffer, 0)) <= 0) {
        fprintf(stderr, "Invalid server\n");
        exit(INVALID_SERVER);
    }
    enum ErrorCode result = GOOD;
    if (!strcmp(buffer, "yes")) {
        result = get_scores(sessionInfo);
    } else {
        fprintf(stderr, "Invalid server\n");
        exit(INVALID_SERVER);
    } 
    if (result) {
        exit(GOOD);
    }
}


/**
 * Receives score info from server and prints them to stdout
 */
enum ErrorCode get_scores(struct SessionInfo* sessionInfo) {
    int c;
    int count = 0;
    int indexInLine = 0;
    char currentLine[BUF];
    memset(currentLine, '\0', BUF - 1);
    if ((c = fgetc(sessionInfo->fromServer)) == EOF) {
        exit(GOOD);
    }
    while(1) {
        if (feof(sessionInfo->fromServer)) {
            fclose(sessionInfo->fromServer);
            fclose(sessionInfo->toServer);
            exit(GOOD);
        }
        count++;
        if (c == '\n') {
            currentLine[indexInLine] = c;
            indexInLine = 0;
            printf("%s", currentLine);
            fflush(stdout);
            memset(currentLine, '\0', BUF - 1);
            c = fgetc(sessionInfo->fromServer);
            continue;
        }
        currentLine[indexInLine] = c;
        c = fgetc(sessionInfo->fromServer);
        indexInLine++;
    }
    return GOOD;
}







