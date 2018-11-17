#ifndef SERVER_GAME_PLAY_H
#define SERVER_GAME_PLAY_H

#include <stdio.h>
#include <pthread.h>
#include <errno.h>

#include <util.h>
#include <server.h>
#include <game.h>
#include <protocol.h>
#include <deck.h>

#include "serverStructs.h"
#include "getKey.h"


#define MAX_INT_LENGTH 10

/**
 * The thread function that sets up a game.
 * Includes re-ordering the players and sending the reconnect IDs
 * Calls play_game which will actually play the game
 */
void* initiate_game(void* newGame);


/**
 * Main game-playing function. Handles a single game
 */
enum ErrorCode play_game(struct GameFull* gameFull);


/**
 * Sends the do_what message to a player, waits for their response,
 * then processes their message and updates the game
 */
enum ErrorCode do_what(struct GameFull* gameFull, 
        struct Game* game, int playerId);


/**
 * Orders the player IDs in lexographical order
 */
void order_players(struct GameFull* game);


/**
 * Changes a string to lower case
 */
char* change_to_lower(char* toChange);


/**
 * Sends the Reconnect IDs to all players in a game
 */
void send_reconnect_ids(struct GameFull* gameFull);


/**
 * Given a Game and a player number, processes the reconnect ID for that player
 */
char* get_reconnect_id(struct GameFull* game, int index);


/**
 * Sends the playinfo message to all the players
 * e.g. playinfo2/3 would indicate a player is of ID 2 and the capacity
 * of that game is 3
 */
void send_playinfo(struct GameFull* gameFull);

#endif //SERVER_GAME_PLAY_H
