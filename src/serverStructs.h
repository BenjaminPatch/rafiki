#ifndef SERVER_STRUCTS_H
#define SERVER_STRUCTS_H

#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <unistd.h>
#include <ctype.h>
#include <stdbool.h>
#include <signal.h>

#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <pthread.h>
#include <semaphore.h>

#include <util.h>
#include <server.h>
#include <deck.h>

#define BUFF 256

/**
 * Error Codes for argument checking and server exiting
 * The library does not provide an enum for all of these so I made a new one
 */
enum ServerExitCode {
    NORMAL = 0,
    ARG_COUNT = 1,
    KEY_FILE = 2,
    DECK_FILE = 3,
    STAT_FILE = 4,
    TIMEOUT = 5,
    LISTEN_PORT = 6,
    SYS_CALL = 10,
};


/**
 * Information required to start a game.
 * i.e. how many players, tokens at the start, capacity, the port to start it
 * from
 */
struct StartGame {
    /* The port to start the game on, i.e. When a player on this port makes
     * a new game, these are the rules used. */
    int port;

    /* The initial socket when binding to port in server setup */
    int sockFd;

    /* Used for setting up port listening */
    struct sockaddr_in serverAddress;

    /* Game rules */
    int startTokens;
    int pointsRequired;
    int numPlayers; /*Capacity*/

    /* Linked list for setting up games */
    struct StartGame* nextStartGame;

    /* So we can refer back to the server from within StartGame */
    struct ServerData* server;
};

/* All the info about a game, including rules, capacity, players and the
 * Game struct itself
 */
struct GameFull {
    /* Info to start game */
    struct StartGame* gameRules;
    
    /* Game struct in provided library */
    struct Game* game;

    /* To tell if the game has started */
    bool hasStarted;

    /* Checking if capacity has been reached */
    bool isFull;
    int capacity;
    
    /* How many people are waiting for game to start */
    int currentPlayerCount;
    
    /* The first player to join a game, used in setting up the new game */
    struct ClientInfo* player;

    /* next linked list node */
    struct GameFull* nextGame;

    /* Which number this game is (relative to number of games with this name)*/
    int count;
};


struct ServerData {
    /* Linked list nodes */
    struct GameFull* headGame;
    struct GameFull* tailGame;
    struct StartGame* headStartGame;
    
    /* Array of ports being listened on */
    int* ports;
    int numPorts;

    /* Key for server. Used in handshake with client */
    char* key;

    char* deckFileName;

    /* Time to wait, in seconds, 
     * before ending a game after a player disconnects*/
    int timeout;
    
    char* statFile;

    /* Used to signal the main game Operator that a game is ready*/
    sem_t gameReady;

    pthread_t threads[BUFF];

    /*For locking the linked list in ServerData struct*/
    pthread_mutex_t gameLinkedList;
};


/**
 * Info about a client that has connected
 * Mostly used for adding them to a game, before file descriptors and such are
 * added to library structs
 */
struct ClientInfo {
    FILE* readFP; 
    FILE* writeFP;
    int sockFd;
    struct StartGame* game;
    struct ServerData* server;
    struct GameFull* gameFull;
    int port;

    bool connected;

    /*Used to set the name of the player once game name has been sent*/
    struct GamePlayer* player;
};

struct PlayerScore {
    char* name;
    int tokens;
    int points;
    struct PlayerScore* nextScore;
};

#endif // SERVER_STRUCTS_H
