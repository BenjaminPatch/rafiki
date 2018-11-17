#include "client.h"


/**
 * For exiting the game with correct message and exit status
 */
void exit_game(enum PlayerExitCode exitCode, int player) {
    switch (exitCode) {
        case ALL_IS_GOOD:
            exit(ALL_IS_GOOD);
        case ARG_COUNT:
            fprintf(stderr, "Usage: zazu keyfile port game pname\n");
            exit(ARG_COUNT);
        case BAD_KEY_FILE:
            fprintf(stderr, "Bad key file\n");
            exit(BAD_KEY_FILE);
        case INVALID_NAME:
            fprintf(stderr, "Bad name\n");
            exit(INVALID_NAME);
        case CONNECTION_ERROR:
            fprintf(stderr, "Failed to connect\n");
            exit(CONNECTION_ERROR);
        case AUTH_ERROR:
            fprintf(stderr, "Bad auth\n");
            exit(AUTH_ERROR);
        case BAD_RID:
            fprintf(stderr, "Bad reconnect id\n");
            exit(BAD_RID);
        case COMM_ERROR:
            fprintf(stderr, "Communication Error\n");
            exit(COMM_ERROR);
        case DISCONNECT:
            /*Not reached because a different function is used
             * So as to be able to free disco message after fprintf
             * without having to have another variable in this function
             */
        case INVALID_MESSAGE:
            fprintf(stderr, "Player %c sent an invalid message\n",
                    player + 'A');
            exit(INVALID_MESSAGE);
    }
}

int main(int argc, char** argv) {
    check_args(argc, argv);
    join_and_play_game(argv);
    
    
    //struct sockaddr_in serv_addr, cli_addr;
    //int n;
    return 0;
}


/**
 * Checks the arguments given
 */
void check_args(int argc, char** argv) {
    enum PlayerExitCode exitCode;
    if (argc != 5) {
        exitCode = ARG_COUNT;
        exit_game(exitCode, 0);
    }
    if (strcmp(argv[2], "0")) {
        if (!atoi(argv[2])) {
            exitCode = CONNECTION_ERROR;
            exit_game(exitCode, 0);
        }
    }
    if (!strcmp(argv[3], "reconnect")) {
        return;
    }
    for (int i = 0; i < strlen(argv[3]); i++) {
        if (argv[3][i] == '\n' || argv[3][i] == ',') {
            exitCode = INVALID_NAME;
            exit_game(exitCode, 0);
        }
    }
    for (int i = 0; i < strlen(argv[4]); i++) {
        if (argv[4][i] == '\n' || argv[4][i] == ',') {
            exitCode = INVALID_NAME;
            exit_game(exitCode, 0);
        }
    }
}


/**
 * Joins a game then plays in it
 */
void join_and_play_game(char** argv) {
    struct SessionInfo* sessionInfo = malloc(sizeof(struct SessionInfo));
    struct GameState* gameInfo = malloc(sizeof(struct GameState));
    sessionInfo->game = gameInfo;
    sessionInfo->gameName = argv[3];
    sessionInfo->name = argv[4];
    int sockfd;
    int keyValid;
    char* key = get_key(argv[1], &keyValid);
    if (!keyValid) {
        exit_game(BAD_KEY_FILE, 0);
    }
    int port;
    /* Parse the port argv */
    if (!strcmp(argv[2], "0")) {
        port = 0;
    } else {
        port = atoi(argv[2]);
    }
    connect_to_server(port, sessionInfo, &sockfd);
    if (!strcmp(argv[3], "reconnect")) {
        fprintf(sessionInfo->toServer, "reconnect%s\n", key);
        fflush(sessionInfo->toServer);
    } else {
        fprintf(sessionInfo->toServer, "play%s\n", key);
        fflush(sessionInfo->toServer);
    }
    int count;
    char* buffer;
    if ((count = read_line(sessionInfo->fromServer, &buffer, 0)) <= 0) {
        exit_game(COMMUNICATION_ERROR, 0);
    }
    enum ErrorCode result = ALL_IS_GOOD;
    if (!strcmp(buffer, "yes")) {
        if (!strcmp(argv[3], "reconnect")) {
            fprintf(sessionInfo->toServer, "rid%s\n", argv[4]);
            fflush(sessionInfo->toServer);
            result = reconnect(sessionInfo, argv);
        } else {
            result = play_game(sessionInfo);
        }
    } else {
        exit_game(AUTH_ERROR, 0);
    } 
    exit_game(result, 0);
    close(sockfd);
}


/**
 * Connects to the server, opens FILE*s
 */
void connect_to_server(int port, struct SessionInfo* sessionInfo, 
        int* sockfd) {
    int status;
    *sockfd = socket(AF_INET, SOCK_STREAM, 0);
    // specify an address for the socket
    struct sockaddr_in serverAddress;
    serverAddress.sin_family = AF_INET; 
    serverAddress.sin_port = htons(port);
    serverAddress.sin_addr.s_addr = INADDR_ANY;
    
    if ((status = connect(*sockfd, (struct sockaddr*) &serverAddress, 
            sizeof(serverAddress))) == -1) {
        exit_game(CONNECTION_ERROR, 0);
    }
    sessionInfo->fromServer = fdopen(*sockfd, "r");
    sessionInfo->toServer = fdopen(*sockfd, "w");
}


/**
 * Reconnects to an already-existing game
 */
enum PlayerExitCode reconnect(struct SessionInfo* sessionInfo, char** argv) {
    char* message; 
    int count;
    enum PlayerExitCode err = ALL_IS_GOOD;
    if ((count = read_line(sessionInfo->fromServer, &message, 0)) < 0) {
        exit_game(COMM_ERROR, 8);
    }
    if (!strncmp(message, "playinfo", 8)) {
        err = process_play_info(message, sessionInfo);
    } else if (!strcmp(message, "no")) {
        return BAD_RID;
    } else {
        return COMM_ERROR; 
    }
    sessionInfo->game->players = malloc(sizeof(struct Player*));
    count = read_line(sessionInfo->fromServer, &message, 0);
    if (!strncmp(message, "tokens", 6)) {
        err = process_tokens_message(sessionInfo, message);
        if (err) {
            return COMM_ERROR;
        }
    } else {
        return COMM_ERROR;
    }
    err = main_game_loop(sessionInfo);
    return err;
}


/**
 * Actually plays the game. Interacts with the user by prompting for input,
 * giving game state, etc.
 */
enum PlayerExitCode play_game(struct SessionInfo* sessionInfo) {
    fprintf(sessionInfo->toServer, "%s\n", sessionInfo->gameName);
    fflush(sessionInfo->toServer);
    fprintf(sessionInfo->toServer, "%s\n", sessionInfo->name);
    fflush(sessionInfo->toServer);
    char* message;
    int count;
    if ((count = read_line(sessionInfo->fromServer, &message, 0)) <= 0) {
        exit_game(COMMUNICATION_ERROR, 0);
    }
    enum PlayerExitCode err;
    if (!strncmp(message, "rid", 3)) {
        err = process_rid(message, sessionInfo);
    } else {
        exit_game(COMM_ERROR, 0);
    }
    if (err) {
        exit_game(COMM_ERROR, 0);
    }
    count = read_line(sessionInfo->fromServer, &message, 0);
    if (!strncmp(message, "playinfo", 8)) {
        err = process_play_info(message, sessionInfo);
    } else {
        exit_game(COMM_ERROR, 0);
    }
    sessionInfo->game->players = malloc(sizeof(struct Player*));
    for (int i = 0; i < sessionInfo->game->playerCount; i++) {
        sessionInfo->game->players[i].playerId = i;
        for (int j = 0; j < TOKEN_MAX; j++) {
            sessionInfo->game->players[i].tokens[j] = 0;
            if (j < TOKEN_MAX) {
                sessionInfo->game->players[j].discounts[j] = 0;
            }
        }
        sessionInfo->game->players[i].score = 0;
    }
    if (err) {
        exit_game(COMM_ERROR, 0);
    }
    err = main_game_loop(sessionInfo);
    return err;
}


/**
 * The main loop function for the game. 
 * This function will process all of the messages from server, other than
 * handshake messages, RID, playinfo
 */
enum PlayerExitCode main_game_loop(struct SessionInfo* sessionInfo) {
    char* message;
    display_turn_info(sessionInfo->game);
    while (1) {
        int count = read_line(sessionInfo->fromServer, &message, 0);
        if (count <= 0) {
            exit_game(COMMUNICATION_ERROR, 0);
        }
        enum MessageFromHub messageType = classify_from_hub(message);
        enum PlayerExitCode processedMessage = process_message(messageType,
                message, sessionInfo);
        if (processedMessage != ALL_IS_GOOD) {
            process_error(sessionInfo, processedMessage);
        }
    }
}


/**
 * If a process_message() call goes wrong, deal with appropriately
 */
void process_error(struct SessionInfo* sessionInfo, 
        enum PlayerExitCode processedMessage) {

}

/**
 * Given one message from server, process it and return any error values
 * if necessary
 */
enum PlayerExitCode process_message(enum MessageFromHub messageType,
        char* message, struct SessionInfo* sessionInfo) {
    enum ErrorCode handleResult = NOTHING_WRONG;
    int tokensResult;
    switch (messageType) {
        case END_OF_GAME:
            display_eog_info(sessionInfo->game);
            exit_game(ALL_IS_GOOD, 0);
        case DO_WHAT:
            handleResult = handle_do_what(sessionInfo);
            return ALL_IS_GOOD;
        case PURCHASED:
            handleResult = handle_purchased_message(sessionInfo->game,
                    message);
            break;
        case TOOK:
            handleResult = handle_took_message(sessionInfo->game,
                    message);
            break;
        case TOOK_WILD:
            handleResult = handle_took_wild_message(sessionInfo->game,
                    message);
            break;
        case NEW_CARD:
            handleResult = handle_new_card_message(sessionInfo->game,
                    message);
            break;
        case TOKENS:
            tokensResult = process_tokens_message(sessionInfo, message);
            if (tokensResult) {
                exit_game(COMMUNICATION_ERROR, 0);
            }
            break;
        case DISCO:
            handle_disco_message(sessionInfo, message);
        case INVALID:
            exit_game(INVALID_MESSAGE, 0);
        default:
            if (!strncmp(message, "player", 6)) {
                process_player_message(message, sessionInfo);
            } else {
                exit_game(COMMUNICATION_ERROR, 0);
            }
    }
    if (handleResult) {
        bad_handle_result(handleResult); 
    }
    display_turn_info(sessionInfo->game);
    return ALL_IS_GOOD;
}


/**
 * Mallocs an array of strings appropriately
 */
void malloc_string_arrays(char** sections) {
    for (int i = 0; i < MAX_SECTIONS; i++) {
        sections[i] = malloc(sizeof(char) * MAX_LENGTH_OF_SECTION);
        memset(sections[i], '\0', MAX_LENGTH_OF_SECTION);
    }
}


/**
 * When a player reconnects, the server will send info about all the players
 * In the game. This function parses one of those messages
 */
void process_player_message(char* message, struct SessionInfo* sessionInfo) {
    int playerToProcess = -1;
    int playerScore;
    /* Deals with the colon-separated sections of the player message */
    char** firstSections = malloc(sizeof(char*) * MAX_SECTIONS);
    malloc_string_arrays(firstSections);
    int sectionsNum = split_string(message, firstSections, ':');
    if (sectionsNum != 4) {
        exit_game(COMM_ERROR, 0);
    } else {
        playerToProcess = process_first_split(firstSections, sessionInfo,
                &playerScore);
    }
    if (playerToProcess == -1) {
        exit_game(COMM_ERROR, 0);
    } 

    char** secondSections = malloc(sizeof(char*) * MAX_SECTIONS);
    malloc_string_arrays(secondSections);
    sectionsNum = split_string(firstSections[2], secondSections, ',');
    if (sectionsNum != 4) {
        exit_game(COMM_ERROR, 0);
    } else {
        sectionsNum = process_second_split(secondSections, sessionInfo,
                playerToProcess);
    }
    if (!sectionsNum) {
        exit_game(COMM_ERROR, 0);
    }
    char** thirdSections = malloc(sizeof(char*) * MAX_SECTIONS);
    malloc_string_arrays(thirdSections);
    sectionsNum = split_string(firstSections[3], thirdSections, ',');
    if (sectionsNum != 5) {
        exit_game(COMM_ERROR, 0);
    } else {
        if (!(sectionsNum = process_third_split(thirdSections, sessionInfo,
                playerToProcess))) {
            exit_game(COMM_ERROR, 0);
        }
    }
    print_message(sessionInfo->game->players[playerToProcess]);
    free_sections(firstSections, secondSections, thirdSections);
}    


/**
 * Prints the message when a player reconnects with the info of that player
 */
void print_message(struct Player player) {
    printf("Player %c:%d:Discounts=%d,%d,%d,%d:Tokens=%d,%d,%d,%d,%d\n",
            player.playerId + 'A', player.score, player.discounts[0],
            player.discounts[1], player.discounts[2], player.discounts[3],
            player.tokens[0], player.tokens[1], player.tokens[2], 
            player.tokens[3], player.tokens[4]);
    fflush(stdout);
}


/**
 * Frees all the arrays of strings that were created in the
 * process_player_message function
 */
void free_sections(char** firstSections, char** secondSections, 
        char** thirdSections) {
    for (int i = 0; i < MAX_SECTIONS; i++) {
        free(thirdSections[i]);
    }
    free(thirdSections);

    for (int i = 0; i < MAX_SECTIONS; i++) {
        free(secondSections[i]);
    }
    free(secondSections);

    for (int i = 0; i < MAX_SECTIONS; i++) {
        free(firstSections[i]);
    }
    free(firstSections);
}


/**
 * Parses the amount of tokens of each type a particular player has,
 * based off a "player" message
 */
int process_third_split(char** thirdSections, 
        struct SessionInfo* sessionInfo, int playerToProcess) {
    char dummy[MAX_MESSAGE_LENGTH];
    if (sscanf(thirdSections[0], "t=%d%s", 
            &sessionInfo->game->players[playerToProcess].tokens[0], 
            dummy) != 1) {
        return 0;
    }
    for (int i = 1; i < TOKEN_MAX; i++) {
        if (sscanf(thirdSections[i], "%d%s", 
                &sessionInfo->game->players[playerToProcess].tokens[i], 
                dummy) != 1) {
            return 0;
        }
    }
    return 1;
}


/**
 * Parses the section in a player message that outlines how many discounts
 * of each colour a particular player has
 */
int process_second_split(char** secondSections, struct SessionInfo*
        sessionInfo, int playerId) {
    char dummy[MAX_MESSAGE_LENGTH];
    if (sscanf(secondSections[0], "d=%d%s", 
            &sessionInfo->game->players[playerId].discounts[0], dummy) != 1) {
        return 0;
    }
    for (int i = 1; i < TOKEN_MAX - 1; i++) {
        if (sscanf(secondSections[i], "%d%s", 
                &sessionInfo->game->players[playerId].discounts[i], 
                dummy) != 1) {
            return 0;
        }
    }
    return 1;
}


/**
 * Parses which player this message is referring to, 
 * The points for this player
 * returns the playerID for this message, or -1 if some aspect of the message
 * is found to be invalid.
 */
int process_first_split(char** firstSections, 
        struct SessionInfo* sessionInfo, int* score) {
    int thisPlayer;
    if (strlen(firstSections[0]) != 7) {
        return -1;
    }
    thisPlayer = firstSections[0][6] - 'A';
    if (thisPlayer < 0 || thisPlayer >= sessionInfo->game->playerCount) {
        return -1;
    }
    if (strcmp(firstSections[1], "0")) {
        if (!atoi(firstSections[1])) {
            return -1;
        }
    }
    *score = atoi(firstSections[1]);
    sessionInfo->game->players[thisPlayer].score = atoi(firstSections[1]);
    return thisPlayer;
}


/**
 * If the result of a process message attempt is not zero,
 * this indicates something went wrong. This function deals with that
 */
void bad_handle_result(enum ErrorCode handleResult) {
    switch (handleResult) {
        case NOTHING_WRONG:
            return; /*Won't enter function if this is true*/
        case PLAYER_CLOSED:
            exit_game(CONNECTION_ERROR, 0);
        default:
            exit_game(COMMUNICATION_ERROR, 0);
    }
}

/**
 * Processes a "tokens" message. i.e. initialises the amount of non-wild
 * tokens in each pile
 */
int process_tokens_message(struct SessionInfo* sessionInfo, char* message) {
 
    int tokenCount;
    int tokensValid = parse_tokens_message(&tokenCount, message); 
    for (int i = 0; i < TOKEN_MAX; i++) {
        sessionInfo->game->tokenCount[i] = tokenCount;
    }
    //char* output = print_tokens_message(tokenCount);
    //printf("tokensMessage: %s\n", output);
    //fflush(stdout);
    return tokensValid;
}

/**
 * Prompts the user for their desired move
 */
enum ErrorCode handle_do_what(struct SessionInfo* sessionInfo) {
    char* userInput;
    printf("Received dowhat\n");
    while (1) {
        printf("Action> ");
        read_line(stdin, &userInput, 0);
        if (!strcmp(userInput, "purchase")) {
            get_purchase_move(sessionInfo);
            break;
        } else if (!strcmp(userInput, "take")) {
            get_take_move(sessionInfo);
            break;
        } else if (!strcmp(userInput, "wild")) {
            fprintf(sessionInfo->toServer, "wild\n");
            fflush(sessionInfo->toServer);
            break;
        } else {
            continue;
        }
    }
    return NOTHING_WRONG;
}


/**
 * Prompts the user for the parameters of a purchase move.
 * i.e. Which card, and how many of each tokens to use in the purchase
 */
void get_purchase_move(struct SessionInfo* sessionInfo) {
    char* userInput;
    struct PurchaseMessage* messageContents = 
            malloc(sizeof(struct PurchaseMessage));
    while (1) {
        printf("Card> ");
        int result;
        if ((result = read_line(stdin, &userInput, 0)) <= 0) {
            exit_game(COMMUNICATION_ERROR, 0);
        }
        result = check_card_num(userInput);
        if (result) {
            continue;
        }
        messageContents->cardNumber = atoi(userInput);
        break;
    }
    char tokens[5] = {'P', 'B', 'Y', 'R', 'W'};
    struct Player self = sessionInfo->game->players[sessionInfo->game->selfId];
    for (int i = 0; i < TOKEN_MAX; i++) {
        while (1) {
            if (!self.tokens[i]) {
                messageContents->costSpent[i] = 0;
                break;
            }
            printf("Token-%c> ", tokens[i]);
            int read; 
            if ((read = read_line(stdin, &userInput, 0)) <= 0) {
                exit_game(COMMUNICATION_ERROR, 0);
            }
            int result = check_token_count(userInput, self, i);
            if (result) {
                continue;
            }
            messageContents->costSpent[i] = atoi(userInput);
            break;
        }
    }
    fprintf(sessionInfo->toServer, "purchase%d:%d,%d,%d,%d,%d\n",
            messageContents->cardNumber, messageContents->costSpent[0],
            messageContents->costSpent[1], messageContents->costSpent[2],
            messageContents->costSpent[3], messageContents->costSpent[4]);
    fflush(sessionInfo->toServer);
    free(messageContents);
}


/**
 * When the program prompts the user for a given amount of tokens for a
 * purchase, this function is used to check the amount is valid.
 * Returns 0 on success and 1 on failure.
 */
int check_token_count(char* userInput, struct Player self, int index) {
    if (!strcmp(userInput, "0")) {
        return 0;
    }
    if (!atoi(userInput)) {
        return 1;
    }
    int amount = atoi(userInput);
    if (amount < 0 || amount > self.tokens[index]) {
        return 1;
    }
    return 0;
}


/**
 * Validates a card number when prompting user for a purchase move
 * Returns 0 on success, 1 on failure.
 */
int check_card_num(char* input) {
    if (strlen(input) != 1) {
        return 1;
    }
    if (input[0] < '0' || input[0] > '7') {
        return 1; 
    }
    return 0;
}


/**
 * Prompts the user for parameters of a take move
 */
void get_take_move(struct SessionInfo* sessionInfo) {
    char* userInput;
    struct TakeMessage* messageContents = malloc(sizeof(struct TakeMessage));
    char tokens[5] = {'P', 'B', 'Y', 'R', 'W'};
    struct GameState* game = sessionInfo->game;
    for (int i = 0; i < TOKEN_MAX - 1; i++) {
        while (1) {
            if (!game->tokenCount[i]) {
                messageContents->tokens[i] = 0;
                break;
            }
            printf("Token-%c> ", tokens[i]);
            int read; 
            if ((read = read_line(stdin, &userInput, 0)) <= 0) {
                exit_game(COMMUNICATION_ERROR, 0);
            }
            if (!strcmp(userInput, "0")) {
                messageContents->tokens[i] = 0;
                break;
            }
            if (!atoi(userInput)) {
                continue;
            }
            if (atoi(userInput) <=
                    sessionInfo->game->tokenCount[i]) {
                messageContents->tokens[i] = atoi(userInput);
            } else {
                continue;
            }
            break;
        }
    }
    fprintf(sessionInfo->toServer, "take%d,%d,%d,%d\n", 
            messageContents->tokens[0], messageContents->tokens[1], 
            messageContents->tokens[2], messageContents->tokens[3]);
    fflush(sessionInfo->toServer);

}


/**
 * Handles a disconnect message, cleans up, then exits
 */
void handle_disco_message(struct SessionInfo* sessionInfo, char* message) {
    int disconnectedPlayer;
    int discoResult;
    discoResult = parse_disco_message(&disconnectedPlayer, message);
    if (discoResult == -1) {
        exit_game(COMMUNICATION_ERROR, 0);
    } else {
        char* discoMessage = 
                print_disco_message(disconnectedPlayer);
        printf(discoMessage);
        fflush(stdout);
        player_disconnect(discoMessage);
        
    }
    
}


/**
 * Exits the game if another player disconnected
 */
void player_disconnect(char* discoMessage) {
    fprintf(stderr, "Player %c disconnected\n", discoMessage[5]);
    free(discoMessage);
    exit(DISCONNECT);
}


/**
 * Handles an invalid message, cleans up, then exits
 */
void handle_invalid_message(struct SessionInfo* sessionInfo, char* message) {
    int invalidMessagePlayer;
    int invalidResult = 
            parse_invalid_message(&invalidMessagePlayer, message);
    if (invalidResult == -1) {
        exit_game(COMM_ERROR, 0);
    } else {
        char* invalidMessage = 
                print_invalid_message(invalidMessagePlayer);
        printf(invalidMessage);
        fflush(stdout);
        free(invalidMessage);
    }
}


/**
 * Validates the reconnect ID sent by server to client
 */
enum PlayerExitCode process_rid(char* message, 
        struct SessionInfo* sessionInfo) {
    char rid[MAX_MESSAGE_LENGTH];
    memset(rid, '\0', MAX_MESSAGE_LENGTH - 1);
    if (strlen(message) < MIN_RID_LENGTH) {
        exit_game(COMM_ERROR, 0);
    }
    enum PlayerExitCode err = check_rid_sections(message, sessionInfo); 
    if (err) {
        exit_game(COMM_ERROR, 0);
    }
    if (sscanf(message, "rid%s", rid) != 1) {
        exit_game(COMM_ERROR, 0);
    }
    printf("%s\n", rid);
    fflush(stdout);
    return ALL_IS_GOOD;

}


/**
 * When a playinfo message is sent from server, validate it and process it
 */
enum PlayerExitCode process_play_info(char* message, 
        struct SessionInfo* sessionInfo) {
    enum PlayerExitCode err = check_playinfo_sections(message, sessionInfo);
    if (err) {
        return COMM_ERROR;
    }
    return ALL_IS_GOOD;
}


/**
 * Splits a string based on a delimiter.
 * Input: string to split, array of strings that will be changed, 
 * the delimiter to split on.
 * Returns: Number of sections
 */
int split_string(char* message, char** sections, char delimiter) {
    int sectionCounter = 0;
    int indexInSection = 0;
    for (int i = 0; i < strlen(message); i++) {
        if (sectionCounter >= MAX_SECTIONS) {
            exit_game(COMM_ERROR, 0);
        }
        if (message[i] == delimiter) {
            sectionCounter++;
            indexInSection = 0;
            continue;
        }
        sections[sectionCounter][indexInSection] = message[i];
        indexInSection++;
    }
    return sectionCounter;
}


/**
 * Splits the rid into sections, then checks each section
 */
enum PlayerExitCode check_rid_sections(char* message, 
        struct SessionInfo* sessionInfo) {
    char** sections = malloc(sizeof(char*) * MAX_SECTIONS);
    for (int i = 0; i < MAX_SECTIONS; i++) {
        sections[i] = malloc(sizeof(char) * MAX_LENGTH_OF_SECTION);
        memset(sections[i], '\0', MAX_LENGTH_OF_SECTION);
    }
    split_string(message, sections, ',');
    if (sscanf(sections[0], "rid%s", sessionInfo->gameName) != 1) {
        exit_game(COMM_ERROR, 0);
    }
    if (atoi(sections[1]) == 0) {
        exit_game(COMM_ERROR, 0);
    }
    if (strcmp(sections[2], "0")) {
        if (atoi(sections[2]) == 0) {
            exit_game(COMM_ERROR, 0);
        } else if (atoi(sections[2]) < 0 || atoi(sections[2]) > 25) {
            exit_game(COMM_ERROR, 0);
        }
    }
    for (int i = 0; i < MAX_SECTIONS; i++) {
        free(sections[i]);
    }
    free(sections);
    return ALL_IS_GOOD;
}


/**
 * Splits the playinfo message into sections, then validates each of them
 */
enum PlayerExitCode check_playinfo_sections(char* message,
        struct SessionInfo* sessionInfo) {
    char** sections = malloc(sizeof(char*) * MAX_SECTIONS);
    for (int i = 0; i < MAX_SECTIONS; i++) {
        sections[i] = malloc(sizeof(char) * MAX_LENGTH_OF_SECTION);
        memset(sections[i], '\0', MAX_LENGTH_OF_SECTION);
    }
    split_string(message, sections, '/');
    if (strlen(sections[0]) != 9) {
        exit_game(COMM_ERROR, 0);
    }
    sessionInfo->game->selfId = sections[0][8] - 'A';
    char dummy[MAX_MESSAGE_LENGTH];
    if (sscanf(sections[1], "%d%s", &sessionInfo->game->playerCount,
            dummy) != 1) {
        exit_game(COMM_ERROR, 0);
    }
    
    return ALL_IS_GOOD;

}
