#ifndef GOPHER_H
#define GOPHER_H

#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <unistd.h>
#include <ctype.h>

#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>

#include <game.h>
#include <util.h>

#define LENGTH_OF_FIRST_LINE 37
#define BUF 256

/**
 * Exit values for Gopher scores client
 */
enum ScoresExitCode {
    GOOD = 0,
    ARG_COUNT = 1,
    CONNECTION = 3,
    INVALID_SERVER = 4,
};


/**
 * For the scores client to store information about it's current session
 * Mostly pertaining to server connection info
 */
struct SessionInfo {
    FILE* toServer;
    FILE* fromServer; 
};


/**
 * Receives score info from server and prints them to stdout
 */
enum ErrorCode get_scores(struct SessionInfo* sessionInfo);


#endif //GOPHER_H
