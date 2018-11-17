#include "serverGamePlay.h"

/**
 * The thread function that sets up a game.
 * Includes re-ordering the players and sending the reconnect IDs
 * Calls play_game which will actually play the game
 */
void* initiate_game(void* newGame) {
    struct GameFull* gameFull = (struct GameFull*) newGame;
    struct Game* game = (struct Game*)gameFull->game;
   
    order_players(gameFull);
    send_reconnect_ids(gameFull);
    send_playinfo(gameFull);
    for (int i = 0; i < game->playerCount; i++) {
        
        fprintf(game->players[i].toPlayer, "tokens%d\n", 
                gameFull->gameRules->startTokens);
        fflush(game->players[i].toPlayer);
    }
    for (int i = 0; i < BOARD_SIZE; i++) {
        if (!cards_left(gameFull->game)) {
            break;
        } else {
            draw_card(gameFull->game);
        }
    }
    enum ErrorCode exitCode = play_game(gameFull);
    if (exitCode != PROTOCOL_ERROR) {
        for (int i = 0; i < game->playerCount; i++) {
            fprintf(game->players[i].toPlayer, "eog\n");
            fflush(game->players[i].toPlayer);
        }
    }
    pthread_exit(NULL);
}


/**
 * Main game-playing function. Handles a single game
 */
enum ErrorCode play_game(struct GameFull* gameFull) {
    struct Game* game = gameFull->game;
    enum ErrorCode err = 0;
    while (is_game_over(gameFull->game) == false) {
        for (int i = 0; i < game->playerCount; i++) {
            err = do_what(gameFull, game, i);
            if (err == PROTOCOL_ERROR) {
                err = do_what(gameFull, game, i);
            }
            if (err) {
                for (int j = 0; j < game->playerCount; j++) {
                    fprintf(game->players[j].toPlayer, "invalid%c\n", i + 'A');
                    fflush(game->players[j].toPlayer);
                }
                return err;
            }
        }
    }
    return 0;
}


/**
 * Sends the do_what message to a player, waits for their response,
 * then processes their message and updates the game
 */
enum ErrorCode do_what(struct GameFull* gameFull, 
        struct Game* game, int playerId) {
    FILE* toPlayer = game->players[playerId].toPlayer;
    FILE* fromPlayer = game->players[playerId].fromPlayer;

    enum ErrorCode err = 0;
    
    fprintf(toPlayer, "dowhat\n");
    fflush(toPlayer);
    
    char* message;
    int readBytes = read_line(fromPlayer, &message, 0);
    if (readBytes <= 0) {
        struct ClientInfo** clients = 
                (struct ClientInfo**)game->data;
        clients[playerId]->connected = false;        
        for (int i = 0; i < gameFull->gameRules->server->timeout; i++) {
            sleep(1);
            readBytes = read_line(game->players[playerId].toPlayer,
                    &message, 0);
            if (readBytes > 0) {
                break;
            }
            if (i == gameFull->gameRules->server->timeout - 1) {
                return PLAYER_CLOSED;
            }
        }
        if (ferror(game->players[playerId].fromPlayer) && errno == EINTR) {
            free(message);
            return INTERRUPTED;
        } else if (feof(game->players[playerId].fromPlayer)) {
            free(message);
            return PLAYER_CLOSED;
        } 
    }
    enum MessageFromPlayer messageType = classify_from_player(message);
    switch(messageType) {
        case PURCHASE:
            err = handle_purchase_message(playerId, game, message);
            break;
        case WILD:
            handle_wild_message(playerId, game);
            break;
        case TAKE:
            err = handle_take_message(playerId, game, message);
            break;
    }
    return err;
}


/**
 * Orders the player IDs in lexographical order
 */
void order_players(struct GameFull* currentGame) {
    int flag;
    struct Game* game = currentGame->game;
    while (1) {
        flag = 0;
        for (int i = 0; i < game->playerCount - 1; i++) {

            // Kept in case the case of letters doesn't actually make a
            // difference. i.e. Ben is the same as ben.
            // Apparently this is not the case.
            /*
            char* lowerCaseVersion1 = 
                    change_to_lower(game->players[i].state.name);
            char* lowerCaseVersion2 = 
                    change_to_lower(game->players[i + 1].state.name);
            */

            // I did this instead of using qsort() because this way
            // I don't have to put every name into an array of strings
            if (strcmp(game->players[i].state.name, 
                    game->players[i + 1].state.name) > 0) {
                struct GamePlayer temp = game->players[i + 1];
                game->players[i + 1] = game->players[i];
                game->players[i] = temp;
                flag = 1;
            }
        }
        if (!flag) {
            break;
        }
    }
    for (int i = 0; i < game->playerCount; i++) {
        game->players[i].state.playerId = i;
    }
}


/**
 * Changes a string to lower case
 */
char* change_to_lower(char* toChange) {
    char* newString = (char*)malloc(sizeof(toChange));
    for (int i = 0; i < strlen(toChange); i++) {
        newString[i] = tolower(toChange[i]);
    }
    return newString;
}


/**
 * Sends the Reconnect IDs to all players in a game
 */
void send_reconnect_ids(struct GameFull* gameFull) {
    for (int i = 0; i < gameFull->capacity; i++) {
        fprintf(gameFull->game->players[i].toPlayer, "rid%s\n",
                get_reconnect_id(gameFull, i));
        fflush(gameFull->game->players[i].toPlayer);
    }
}


/**
 * Given a Game and a player number, processes the reconnect ID for that player
 */
char* get_reconnect_id(struct GameFull* gameFull, int index) {
    int maxLengthOfGameCount = 2; //Since 26 is 2 digits
    char* rid = malloc(sizeof(char) * strlen(gameFull->game->name) + 
            MAX_INT_LENGTH + maxLengthOfGameCount * 2);
    sprintf(rid, "%s,%d,%d", gameFull->game->name, gameFull->count, index);
    return rid;
}


/**
 * Sends the playinfo message to all the players
 * e.g. playinfo2/3 would indicate a player is of ID 2 and the capacity
 * of that game is 3
 */
void send_playinfo(struct GameFull* gameFull) {
    struct Game* game = gameFull->game;
    for (int i = 0; i < gameFull->capacity; i++) {
        fprintf(game->players[i].toPlayer, "playinfo%c/%d\n", 
                i + 65, gameFull->capacity);
        fflush(game->players[i].toPlayer);
    }
}
