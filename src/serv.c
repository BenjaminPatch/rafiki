#include "serv.h"

/* Global for signal handling*/
sem_t sigintCalled;

/* Used to let port threads know to shut down */ 
int flag = 0;

int main(int argc, char** argv) {
    pthread_t sigintThread;
    struct ServerData* server;
    server = malloc(sizeof(struct ServerData));
    check_args(argc, argv, server);
    setup_sigterm();
    //setup_sigint();
    ignore_sigpipe();
    server->headGame = NULL;
    server->deckFileName = argv[2];
    server->timeout = atoi(argv[4]);
    pthread_mutex_init(&server->gameLinkedList, NULL);
    sem_init(&server->gameReady, 2, 0);
    sem_init(&sigintCalled, 1, 0);
    server->statFile = argv[3];
    initialise_server(server, argv[3]);
    if (pthread_create(&sigintThread, NULL, wait_on_sigint, server) 
            != 0) {
        exit_server(SYS_CALL);
    }
    while (1) {
        lobby(server);
    }
    return 0;
}


/**
 * Waits on sigint to be called then deals with it properly
 */
void* wait_on_sigint(void* thisServer) {
    struct ServerData* server = (struct ServerData*) thisServer;
    while (1) {
        //int sem_wait_nointr(&sigintCalled); 
        while (sem_wait(&sigintCalled)) {
            if (errno == EINTR) {
                errno = 0;
            } else {
                break;
            }
        }
        initialise_server(server, server->statFile);
    }
}


/**
 * Frees all mallocs
 */
void clean_up(struct ServerData* server) {
    struct GameFull* current = server->headGame;
    struct GameFull* temp;
    while (1) {
        if (current == NULL) {
            break;
        }
        struct ClientInfo** clients = 
                (struct ClientInfo**)current->game->data;
        for (int i = 0; i < current->capacity; i++) {
            fclose(clients[i]->readFP);
            fclose(clients[i]->writeFP);
            close(clients[i]->sockFd);
        }
        temp = current;
        current = current->nextGame;
        free(temp);
    }
    struct StartGame* currentStart = server->headStartGame;
    struct StartGame* tempStart;
    for (int i = 0; i < server->numPorts; i++) {
        pthread_kill(server->threads[i], SIGTERM);
        tempStart = currentStart;
        currentStart = currentStart->nextStartGame;
        free(tempStart);
    }
    free(server);
}


/**
 * Takes an enum ServerExitCode, and exits with the appropriate message
 */
void exit_server(enum ServerExitCode exitCode) {
    switch (exitCode) {     
        case NORMAL:
            exit(exitCode);
        case ARG_COUNT:
            fprintf(stderr, 
                    "Usage: rafiki keyfile deckfile statfile timeout\n");
            exit(exitCode);
        case KEY_FILE:
            fprintf(stderr, "Bad keyfile\n");
            exit(exitCode);
        case DECK_FILE:
            fprintf(stderr, "Bad deckfile\n");
            exit(exitCode);
        case STAT_FILE:
            fprintf(stderr, "Bad statfile\n");
            exit(exitCode);
        case TIMEOUT:
            fprintf(stderr, "Bad timeout\n");
            exit(exitCode);
        case LISTEN_PORT:
            fprintf(stderr, "Failed listen\n");
            exit(exitCode);
        case SYS_CALL:
            fprintf(stderr, "System error\n");
            exit(exitCode);
    }
}


/**
 * Sets up the signal handler to ignore SIGPIPE
 */
void ignore_sigpipe() {
    struct sigaction action;
    action.sa_handler = do_nothing;
    sigemptyset(&action.sa_mask);
    action.sa_flags = 0;
    sigaction(SIGPIPE, &action, NULL);
}


/**
 * Sets up the signal handling for SIGINT
 */
void setup_sigint() {
    struct sigaction sigint;
    memset(&sigint, 0, sizeof(struct sigaction));
    sigint.sa_handler = sigint_handler;
    sigaction(SIGINT, &sigint, NULL);
}


/**
 * Signal handler for when the server receives SIGINT
 */
void sigint_handler() {
    flag = 1;
    sem_post(&sigintCalled);
}

/**
 * Sets up the signal handler for SIGTERM
 */
void setup_sigterm() {
    struct sigaction sigterm;
    memset(&sigterm, 0, sizeof(struct sigaction));
    sigterm.sa_handler = sigterm_handler;
    sigaction(SIGTERM, &sigterm, NULL);
}


/**
 * Cleans up then shuts down the server, as described in spec
 */
void sigterm_handler(int sigNum) {
    //clean_up();
    exit_server(NORMAL);
}


/**
 * Used to ignore signals, since you have to assign a function to the signal-
 * handler.
 */
void do_nothing() {

}


/**
 * Checks the validity of the arguments
 */
void check_args(int argc, char** argv, struct ServerData* server) {
    enum ServerExitCode exitCode;
    if (argc != 5) {
        exitCode = ARG_COUNT;
        exit_server(exitCode);
    }
    int keyValid;
    server->key = get_key(argv[1], &keyValid);
    if (!keyValid) {
        exit_server(KEY_FILE);
    }
    if (!strcmp(argv[4], "0")) {
        return;
    }
    if (atoi(argv[4]) <= 0) {
        exitCode = TIMEOUT;
        exit_server(exitCode);
    }
    int deckStatus; 
    
    struct GameFull* newGame = malloc(sizeof(struct GameFull));
    newGame->game = malloc(sizeof(struct Game));
    deckStatus = parse_deck_file(&newGame->game->deckSize, 
            &newGame->game->deck, argv[2]);
    //free(newGame->game->deck);
    free(newGame->game);
    free(newGame);
    if (deckStatus) {
        exit_server(DECK_FILE);
    }
}


/**
 * The waiting room/lobby for games. Uses a semaphore to get notificed when a
 * game is ready
 */
void lobby(struct ServerData* server) {
    //int sem_wait_nointr(&server->gameReady); 
    while (sem_wait(&server->gameReady)) {
        if (errno == EINTR) {
            errno = 0;
        } else {
            break;
        }
    }
    find_game(server);
}


/**
 * Finds a game that is full so we can start the game
 */
void find_game(struct ServerData* server) {
    pthread_t gameThread;    
    struct GameFull* current = server->headGame;
    if (current->isFull && !current->hasStarted) {
        if (pthread_create(&gameThread, NULL, initiate_game, (void*)current)
                != 0) {
            fprintf(stderr, "Error starting game\n");
        }
    }

}


/**
 * Starts the server. i.e. starts listening on all ports before creating games
 */
void initialise_server(struct ServerData* server, char* statFile) {
    int flag = 0;
    FILE* stats = fopen(statFile, "r");
    enum ServerExitCode exitCode = NORMAL;
    if (stats == NULL || !stats) {
        exitCode = STAT_FILE;
        exit_server(exitCode);
    }  
    int count = 0;
    while (!feof(stats)) {
        char* line;
        int length;
        if ((length = read_line(stats, &line, 0)) < 0) {
            exit_server(SYS_CALL);
        }
        if (length == 0) {
            continue; 
        }
        if (flag) {
            exitCode = STAT_FILE;
        }
        if (length == 0) { 
            flag = 1;
            free(line);
            continue;
        }
        struct StartGame* gameInfo;
        gameInfo = parse_stat_line(line);
        gameInfo->server = server;
        if (!count) {
            server->headStartGame = gameInfo;
        } else {
            append_start_game(server, gameInfo);
        }
        count++;
        initialise_port(gameInfo, count - 1);
        free(line);
    }
    server->numPorts = count;
    print_port(server);
    if (fclose(stats) == EOF) {
        exit_server(SYS_CALL);
    }
}


/**
 * Prints the ports to stderr
 */
void print_port(struct ServerData* server) {
    struct StartGame* current = server->headStartGame;
    enum ServerExitCode exitCode = NORMAL;
    if (current == NULL) {
        exitCode = STAT_FILE;
    }
    while (1) {
        if (current == NULL) {  
            break;
        }
        fprintf(stderr, "%d", current->port);
        current = current->nextStartGame;
        if (current == NULL) {  
            break;
        }
        fprintf(stderr, " ");
    }
    fprintf(stderr, "\n");
    if (exitCode) {
        exit_server(exitCode);
    }

}


/**
 * Appends a StartGame struct to the linked list
 */
void append_start_game(struct ServerData* server, struct StartGame* gameInfo) {
    struct StartGame* current = server->headStartGame;
    if (current == NULL) {
        server->headStartGame = gameInfo;
    }
    /*if (current->nextStartGame == NULL) {
        current->nextStartGame = gameInfo;
        return;
    }*/
    while (1) {
        if (current->nextStartGame == NULL) {
            current->nextStartGame = gameInfo;
            break;
        }
        current = current->nextStartGame;
    }
}


/**
 * Initialises the information for a particular port to listen on
 */
void initialise_port(struct StartGame* gameInfo, int count) {
    pthread_t portThread;
    gameInfo->sockFd = socket(AF_INET, SOCK_STREAM, 0); 
    int optVal = 1;
    if (setsockopt(gameInfo->sockFd, SOL_SOCKET, SO_REUSEADDR, &optVal,
            sizeof(int)) < 0) {
        exit_server(SYS_CALL);
    }
    gameInfo->serverAddress.sin_family = AF_INET;
    gameInfo->serverAddress.sin_port = htons(gameInfo->port);
    gameInfo->serverAddress.sin_addr.s_addr = htonl(INADDR_ANY);
    bind(gameInfo->sockFd,
            (struct sockaddr*) &(gameInfo->serverAddress), 
            sizeof(struct sockaddr_in));
    listen(gameInfo->sockFd, SOMAXCONN);
    socklen_t saLen = sizeof(gameInfo->serverAddress);
    getsockname(gameInfo->sockFd,
            (struct sockaddr*) &(gameInfo->serverAddress), 
            &saLen);
    gameInfo->port = ntohs(gameInfo->serverAddress.sin_port);
    if (pthread_create(&portThread, NULL, listen_port, (void*)gameInfo) != 0) {
        exit_server(SYS_CALL);
    }
    gameInfo->server->threads[count] = portThread;
    pthread_detach(portThread);
}

/**
 * Listens on a particular port as designated by the statfile
 */
void* listen_port(void* gameInfo) {
    struct StartGame* game = (struct StartGame*) gameInfo;
    struct ServerData* temp = (struct ServerData*)game->server;
    pthread_t client;
    //int count = 0;
    int sockFd = game->sockFd;
    struct sockaddr_in newAddr;
    socklen_t addrSize;
    int clientSocket;
    addrSize = sizeof(struct sockaddr_in); 

    while (1) {
        if (flag) {
            flag = 0;
            pthread_exit(NULL);
        }
        if ((clientSocket = accept(sockFd, (struct sockaddr*)&newAddr,
                &addrSize)) == -1) {
            exit_server(SYS_CALL);
        }
        struct ClientInfo* thisClient = malloc(sizeof(struct ClientInfo));
        thisClient->connected = true;
        thisClient->sockFd = clientSocket;
        FILE* writeFP = fdopen(clientSocket, "w");
        thisClient->writeFP = writeFP;
        thisClient->port = game->port;
        if (writeFP == NULL) {
            exit_server(SYS_CALL);
        }
        int clientSocketRead = dup(clientSocket);
        FILE* readFP = fdopen(clientSocketRead, "r");
        thisClient->readFP = readFP;
        if (readFP == NULL) { 
            exit_server(SYS_CALL);
        }
        thisClient->game = game;
        thisClient->server = temp;
        if (pthread_create(&client, NULL, handle_client, 
                (void*) thisClient) != 0) {
            exit_server(SYS_CALL);
        }
    }
}

/**
 * When a client connects, handle their requests
 */
void* handle_client(void* client) {
    struct ClientInfo* clientInfo = (struct ClientInfo*) client;
    while (1) {
        char* input;
        //memset(input, '\0', 256);
        int result;
        if ((result = read_line(clientInfo->readFP, &input, 0)) <= 0) {
            fclose(clientInfo->readFP);
            exit_server(SYS_CALL);
            break;
        }
        new_connection(input, clientInfo);
        break;
    }
    pthread_exit(NULL);
}
        

/**
 * Handles a new connection from client, as opposed to reconnection
 */
void new_connection(char* input, struct ClientInfo* clientInfo) {
    char key[strlen(input) - 4];
    if (sscanf(input, "play%s", key) == 1) {
        if (!strcmp(key, clientInfo->server->key)) {
            fprintf(clientInfo->writeFP, "yes\n"); 
            fflush(clientInfo->writeFP);
            get_game_and_name(clientInfo);
            return;
        } 
    } else if (sscanf(input, "reconnect%s", key) == 1) {
        validate_reconnect(clientInfo, key);
        return;
    } else if (!strcmp(input, "scores")) {
        fprintf(clientInfo->writeFP, "yes\n");
        fflush(clientInfo->writeFP);
        send_scores(clientInfo); 
        return;
    }
    fprintf(clientInfo->writeFP, "no\n"); 
    fflush(clientInfo->writeFP);
    fclose(clientInfo->writeFP);
    fclose(clientInfo->readFP);
    close(clientInfo->sockFd);
}


/**
 * Sorts the scores based on points, then total tokens
 */
void order_scores(struct PlayerScore* firstScore) {
    int hasSwapped;
    struct PlayerScore* temp;
    struct PlayerScore* otherTemp = NULL;
    
    do {
        hasSwapped = 0;
        temp = firstScore;
        while (temp->nextScore != otherTemp) {
            if (temp->points < temp->nextScore->points) {
                swap_two_scores(temp, temp->nextScore);
                hasSwapped = 1;
            }
            temp = temp->nextScore;
        }
        otherTemp = temp;
    } while (hasSwapped); 
    struct PlayerScore* secondOtherTemp = NULL; 
    do {
        hasSwapped = 0;
        temp = firstScore;
        while (temp->nextScore != secondOtherTemp) {
            if (temp->tokens > temp->nextScore->tokens) {
                swap_two_scores(temp, temp->nextScore);
                hasSwapped = 1;
            }
            temp = temp->nextScore;
        }
        secondOtherTemp = temp;
    } while (hasSwapped);
}


/**
 * Swaps the data of two PlayerScore structs, used in sorting
 */
void swap_two_scores(struct PlayerScore* first, struct PlayerScore* second) {
    /* Swap the points */
    int temp = first->points;
    first->points = second->points;
    second->points = temp;
    
    /* Swap the names */
    char* tempString = first->name;
    first->name = second->name;
    second->name = tempString;
    
    /* Swap the tokens */
    int tokens = first->tokens;
    first->tokens = second->tokens;
    second->tokens = tokens;
}


/**
 * Figures out the scores of all players,
 * then sends them to the client
 */
void send_scores(struct ClientInfo* clientInfo) {
    /* Gather data on all players by name */    
    struct GameFull* current = clientInfo->server->headGame;
    struct PlayerScore* firstPlayer = malloc(sizeof(struct PlayerScore));
    firstPlayer->points = -1;
    firstPlayer->nextScore = NULL;
    if (current == NULL) {
        fprintf(clientInfo->writeFP, 
                "Player Name,Total Tokens,Total Points\n");
        fflush(clientInfo->writeFP);
        fclose(clientInfo->writeFP);
        fclose(clientInfo->readFP);
        close(clientInfo->sockFd);
        return;
    }
    while (1) {
        if (current == NULL) {
            order_scores(firstPlayer);
            print_scores_message(firstPlayer, clientInfo->writeFP);
            fclose(clientInfo->writeFP);
            fclose(clientInfo->readFP);
            close(clientInfo->sockFd);
            return;
        }
        if (current->capacity != current->currentPlayerCount) {
            current = current->nextGame;
            continue;
        }
        struct PlayerScore newPlayer = {.name = "", .points = -1, .tokens = -1,
                .nextScore = NULL};
        for (int i = 0; i < current->currentPlayerCount; i++) {
            if (!name_already_made(&newPlayer, 
                    current->game->players[i].state.name,
                    clientInfo, firstPlayer)) {
                append_player_score(current->game->players[i], &newPlayer,
                        firstPlayer);
            }
            for (int j = 0; j < TOKEN_MAX; j++) {
                newPlayer.tokens += current->game->players[i].state.tokens[j];
            }
            newPlayer.points += current->game->players[i].state.score;
        }
        current = current->nextGame;
    }

}


/**
 * Prints the messages to the scores client
 */
void print_scores_message(struct PlayerScore* firstPlayer, FILE* writeFp) {
    struct PlayerScore* current = firstPlayer;
    fprintf(writeFp, "Player Name,Total Tokens,Total Points\n");
    fflush(writeFp);
    while (current != NULL) {
        fprintf(writeFp, "%s,", current->name);
        fprintf(writeFp, "%d,", current->tokens);
        fprintf(writeFp, "%d\n", current->points);
        fflush(writeFp);
        current = current->nextScore;
    }
}


/**
 * Appends a player score to linked list
 */
void append_player_score(struct GamePlayer player, 
        struct PlayerScore* newPlayer, struct PlayerScore* headPlayer) {
    if (headPlayer->points == -1) {
        headPlayer->points = player.state.score;
        headPlayer->tokens = 0;
        for (int i = 0; i < TOKEN_MAX; i++) {
            headPlayer->tokens += player.state.tokens[i];
        }
        headPlayer->name = player.state.name;
        return;
    }
    while (1) {
        if (headPlayer->nextScore == NULL) {
            headPlayer->nextScore = malloc(sizeof(struct PlayerScore));
            headPlayer->nextScore->points = player.state.score;
            headPlayer->nextScore->tokens = 0;
            for (int i = 0; i < TOKEN_MAX; i++) {
                headPlayer->nextScore->tokens += player.state.tokens[i];
            }
            headPlayer->nextScore->name = player.state.name;
            return;
        }
        headPlayer = headPlayer->nextScore;
    }
}
            

/**
 * If the scores struct for a player of a particular name has already been
 * made, return 1. Else, return 0.
 */
int name_already_made(struct PlayerScore* newPlayer, char* name, 
        struct ClientInfo* clientInfo, struct PlayerScore* firstPlayer) {
    struct PlayerScore* current = firstPlayer;
    while (1) {
        if (current == NULL || current->name == NULL) {
            return 0;
        }
        if (!strcmp(current->name, name)) {
            newPlayer = current;
            return 1;
        }
        current = current->nextScore;
    }
}


/**
 * Validates the reconnection ID when a player attempts to reconnect
 */
void validate_reconnect(struct ClientInfo* clientInfo, char* rid) {
    char gameName[BUFFER];
    int gameCounter;
    int playerId;
    char dummyVariable[BUFFER];
    if (sscanf(rid, "%s,%d,%d%s", gameName, &gameCounter, &playerId, 
            dummyVariable) != 3) {
        fprintf(clientInfo->writeFP, "no\n");
        fflush(clientInfo->writeFP);
    }
    struct GameFull* current = clientInfo->server->headGame;
    while (1) {
        if (current == NULL) {
            fprintf(clientInfo->writeFP, "no\n");
            fflush(clientInfo->writeFP);
            pthread_exit(NULL);
        }
        if (!strcmp(current->game->name, "gameName")) {
            struct ClientInfo** client = 
                    (struct ClientInfo**)current->game->data;
            if (current->count == gameCounter && 
                    client[playerId]->connected == false) {
                current->game->players[playerId].toPlayer =
                        clientInfo->writeFP;
                current->game->players[playerId].fromPlayer =
                        clientInfo->readFP;
                client[playerId]->connected = true;
                return;
            }
        }
        current = current->nextGame;
    }
}


/**
 * Gets the name of the game that the client wants to connect to, as well
 * as the name of the client him/herself
 */
void get_game_and_name(struct ClientInfo* clientInfo) {
    char* input;
    int count;
    if ((count = read_line(clientInfo->readFP, &input, 0)) < 0) {
        exit_server(SYS_CALL);
    }
    process_game_name(clientInfo, input);
    count = read_line(clientInfo->readFP, &input, 0);
    clientInfo->player->state.name = malloc(sizeof(input));
    clientInfo->player->state.name = input;
    struct GameFull* currentGame = clientInfo->gameFull;
    if (currentGame->currentPlayerCount == currentGame->capacity) {
        currentGame->isFull = true;
        sem_post(&clientInfo->server->gameReady);
    }
}


/**
 * Processes a game name, if game is full, either join another with same name
 * or make a new game
 */
void process_game_name(struct ClientInfo* clientInfo, char* gameName) {
    struct GameFull* currentGame = (clientInfo->server->headGame);
    /*used for determining which game of 
    * a particular name we are dealing with*/
    int count = 1;
    while (1) {
        if (currentGame == NULL) {
            break;
        }
        if (!strcmp(gameName, currentGame->game->name)) {
            if (!(currentGame->isFull)) {
                add_player_to_game(currentGame, clientInfo);
                return;
            } else {
                count = currentGame->count + 1;
            }
        }
        currentGame = currentGame->nextGame;
    }
    make_new_game(clientInfo, gameName, count); 
}


/**
 * Adds a player to an already-existing game.
 * Closes the thread, but stores the file pointers used to communicate with
 * this client.
 */
void add_player_to_game(struct GameFull* currentGame,
        struct ClientInfo* clientInfo) {
    int index;
    for (int i = 0; i < currentGame->game->playerCount; i++) {
        if (currentGame->game->players[i].fileDescriptor == -1) {
            setup_player(currentGame, clientInfo, i); 
            index = i;
            break;
        }
    }
    currentGame->currentPlayerCount++;
    clientInfo->gameFull = currentGame;
    currentGame->game->players[index + 1].fileDescriptor = -1;
}


/**
 * Sets up the info for a game player in game
 */
void setup_player(struct GameFull* newGame, struct ClientInfo* clientInfo, 
        int index) {

    //newGame->game->players[index];
    newGame->game->players[index].toPlayer = clientInfo->writeFP;
    newGame->game->players[index].fromPlayer = clientInfo->readFP;
    struct Player state = newGame->game->players[index].state;
    initialize_player(&state, index);
    newGame->game->players[index].state = state;
    clientInfo->player = &newGame->game->players[index];
    struct ClientInfo** clients;
    clients = newGame->game->data;
    clients[index] = clientInfo;
}


/**
 * Creates a new game if there isn't a suitable one already-made.
 * Stores the initiating players info in it.
 */
void make_new_game(struct ClientInfo* clientInfo, char* gameName, int count) {
    struct GameFull* newGame = malloc(sizeof(struct GameFull));
    struct ClientInfo** clients;
    newGame->count = count;
    newGame->gameRules = find_right_port(clientInfo);
    newGame->game = malloc(sizeof(struct Game));
    newGame->game->data =
            (struct ClientInfo**)malloc(sizeof(struct ClientInfo*)
            * newGame->gameRules->numPlayers);
    clients = newGame->game->data;
    clients[0] = clientInfo;
    newGame->game->name = gameName;
    newGame->game->winScore = newGame->gameRules->pointsRequired;
    newGame->game->playerCount = newGame->gameRules->numPlayers;
    newGame->game->players = malloc(sizeof(struct GamePlayer) * 
            newGame->game->playerCount);
    newGame->currentPlayerCount = 1;
    newGame->capacity = newGame->gameRules->numPlayers;
    setup_player(newGame, clientInfo, 0); 
    for (int i = 0; i < TOKEN_MAX - 1; i++) {
        newGame->game->tokenCount[i] = newGame->gameRules->startTokens;
    }
    newGame->game->boardSize = 0;
    
    enum DeckStatus deckStatus = VALID;
    deckStatus = parse_deck_file(&newGame->game->deckSize,
            &newGame->game->deck, clientInfo->server->deckFileName);
    if (deckStatus) {
        exit_server(DECK_FILE);
    }
    
    pthread_mutex_lock(&clientInfo->server->gameLinkedList);
    append_game_full(clientInfo->server, newGame);
    pthread_mutex_unlock(&clientInfo->server->gameLinkedList);
    newGame->isFull = false;
    newGame->hasStarted = false;
    newGame->game->name = gameName;
    clientInfo->gameFull = newGame;
    newGame->game->players[1].fileDescriptor = -1;
} 


/**
 * Appends a GameFull struct to the linked list in server
 */
void append_game_full(struct ServerData* server, struct GameFull* newGame) {
    if (server->headGame == NULL) {
        server->headGame = newGame;
    } else {
        server->tailGame->nextGame = newGame;
    }
    server->tailGame = newGame;
}


/**
 * Finds the right Game Rules based off the port a user connected to
 * i.e. if a player is starting a new game and they connected on port
 * 3000, look at the statfile line for port 3000
 */
struct StartGame* find_right_port(struct ClientInfo* clientInfo) {
    struct StartGame* current = clientInfo->server->headStartGame;
    while (1) {
        if (current->port == clientInfo->port) {
            return current;
        }
        current = current->nextStartGame;
        if (current == NULL) {
            exit_server(SYS_CALL);
        }
    }
    return NULL;
}

/**
 * Parses a line in the statfile
 */
struct StartGame* parse_stat_line(char* line) {
    struct StartGame* gameInfo = (struct StartGame*)malloc(sizeof(
            struct StartGame));
    memset(gameInfo, 0, sizeof(struct StartGame));
    char** sections = split_string(line, ',');
    check_sections(sections);
    gameInfo->port = atoi(sections[0]);
    gameInfo->startTokens = atoi(sections[1]);
    gameInfo->pointsRequired = atoi(sections[2]);
    gameInfo->numPlayers = atoi(sections[3]);
    for (int i = 0; i < MAX_SECTIONS; i++) {
        free(sections[i]);
    }
    free(sections);
    return gameInfo;
}

/**
 * Checks the validity of the sections of a particular line in the
 * statfile
 */
void check_sections(char** sections) {
    for (int i = 0; i < MAX_SECTIONS; i++) {
        /*Checks if any sections are empty (bad)*/
        if (sections[i][0] == '\0') {
            exit_server(STAT_FILE);
        }
        for (int j = 0; j < strlen(sections[i]); j++) {
            if (!isdigit(sections[i][j])) {
                exit_server(STAT_FILE);
            }
        }
        switch (i) {
            case 0:
                if((atoi(sections[i]) < 0 || atoi(sections[i]) > 65535)) {
                    exit_server(STAT_FILE); 
                }
                break;
            case 1:
                if (atoi(sections[i]) <= 0) {
                    exit_server(STAT_FILE);
                }
                break;
            case 2:
                if (atoi(sections[i]) <= 0) {
                    exit_server(STAT_FILE);
                }
                break;
            case 3:
                if (atoi(sections[i]) < 2 || atoi(sections[i]) > 26) {
                    exit_server(STAT_FILE);
                }
                break;
        }
    }
}

/**
 * Splits string into array of strings, split on a delimiter
 */
char** split_string(char* toSplit, char delimiter) {
    int sectionCounter = 0;
    int indexInSection = 0;
    char** sections = malloc(sizeof(char*) * (MAX_SECTIONS));
    for (int i = 0; i < MAX_SECTIONS; i++) {
        sections[i] = malloc(sizeof(char) * MAX_LENGTH_INT);
        memset(sections[i], '\0', MAX_LENGTH_INT);
    }
    for (int i = 0; i < strlen(toSplit); i++) {
        if (sectionCounter > MAX_SECTIONS - 1) {
            exit(1);
        }
        if (toSplit[i] == delimiter) {
            sectionCounter++;
            indexInSection = 0;
            continue;
        }
        sections[sectionCounter][indexInSection] = toSplit[i];
        indexInSection++;
    }
    return sections;
}
