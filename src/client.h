#ifndef CLIENT_H
#define CLIENT_H

#include <stdio.h>
#include <string.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <stdlib.h>
#include <unistd.h>

#include "../library/lib/player.h"
#include "../library/lib/game.h"
#include "../library/lib/server.h"
#include "../library/lib/protocol.h"
#include "../library/lib/util.h"

#include "getKey.h"

#define MAX_MESSAGE_LENGTH 100

/*Based on description in spec*/
#define MIN_RID_LENGTH 5

#define MAX_SECTIONS 7
#define MAX_LENGTH_OF_SECTION 100

/**
 * Struct storing GameState, file descriptors
 */
struct SessionInfo {
    FILE* toServer; 
    FILE* fromServer;
    int sockFd;
    struct GameState* game;
    int port;
    char* name;
    char* gameName;
    char* reconnectId;
};

enum PlayerExitCode {
    ALL_IS_GOOD = 0,
    ARG_COUNT = 1,
    BAD_KEY_FILE = 2,
    INVALID_NAME = 3,
    CONNECTION_ERROR = 5,
    AUTH_ERROR = 6,
    BAD_RID = 7,
    COMM_ERROR = 8,
    DISCONNECT = 9,
    INVALID_MESSAGE = 10,
};


/**
 * For exiting the game with correct message and exit status
 */
void exit_game(enum PlayerExitCode exitCode, int player);


/**
 * Checks the arguments given when player program is started
 */
void check_args(int argc, char** argv);


/**
 * Connects to the server, opens FILE*s
 */
void connect_to_server(int port, struct SessionInfo* sessionInfo, int* sockfd);


/**
 * Joins a game then plays in it
 */
void join_and_play_game(char** argv);


/**
 * Reconnects to an already-existing game
 */
enum PlayerExitCode reconnect(struct SessionInfo* sessionInfo, char** argv);


/**
 * Actually plays the game. Interacts with the user by prompting for input,
 * giving game state, etc.
 */
enum PlayerExitCode play_game(struct SessionInfo* sessionInfo);


/**
 * The main loop function for the game. 
 * This function will process all of the messages from server, other than
 * handshake messages, RID, playinfo
 */
enum PlayerExitCode main_game_loop(struct SessionInfo* sessionInfo);


/**
 * When a player reconnects, the server will send info about all the players
 * In the game. This function parses one of those messages
 */
void process_player_message(char* message, struct SessionInfo* sessionInfo);


/**
 * Frees all the arrays of strings that were created in the
 * process_player_message function
 */
void free_sections(char** firstSections, char** secondSections, 
        char** thirdSections);


/**
 * Parses the amount of tokens of each type a particular player has,
 * based off a "player" message
 */
int process_third_split(char** thirdSections, 
        struct SessionInfo* sessionInfo, int playerToProcess);


/**
 * Parses the section in a player message that outlines how many discounts
 * of each colour a particular player has
 */
int process_second_split(char** secondSections, struct SessionInfo*
        sessionInfo, int playerId);


/**
 * Parses which player this message is referring to, 
 * The points for this player
 * returns the playerID for this message, or -1 if some aspect of the message
 * is found to be invalid.
 */
int process_first_split(char** firstSections, 
        struct SessionInfo* sessionInfo, int* score);


/**
 * Given one message from server, process it and return any error values
 * if necessary
 */
enum PlayerExitCode process_message(enum MessageFromHub messageType,
        char* message, struct SessionInfo* sessionInfo);


/**
 * Mallocs an array of strings appropriately
 */
void malloc_string_arrays(char** sections);


/**
 * Prints the message when a player reconnects with the info of that player
 */
void print_message(struct Player player);


/**
 * If the result of a process message attempt is not zero,
 * this indicates something went wrong. This function deals with that
 */
void bad_handle_result(enum ErrorCode handleResult);


/**
 * Processes a "tokens" message. i.e. initialises the amount of non-wild
 * tokens in each pile
 */
int process_tokens_message(struct SessionInfo* sessionInfo, char* message);


/**
 * Prompts the user for their desired move
 */
enum ErrorCode handle_do_what(struct SessionInfo* sessionInfo);


/**
 * Prompts the user for the parameters of a purchase move.
 * i.e. Which card, and how many of each tokens to use in the purchase
 */
void get_purchase_move(struct SessionInfo* sessionInfo);


/**
 * When the program prompts the user for a given amount of tokens for a
 * purchase, this function is used to check the amount is valid.
 * Returns 0 on success and 1 on failure.
 */
int check_token_count(char* userInput, struct Player self, int index);


/**
 * Validates a card number when prompting user for a purchase move
 * Returns 0 on success, 1 on failure.
 */
int check_card_num(char* input);


/**
 * Prompts the user for parameters of a take move
 */
void get_take_move(struct SessionInfo* sessionInfo);


/**
 * Handles a disconnect message, cleans up, then exits
 */
void handle_disco_message(struct SessionInfo* sessionInfo, char* message);


/**
 * Exits the game if another player disconnected
 */
void player_disconnect(char* discoMessage);


/**
 * Handles an invalid message, cleans up, then exits
 */
void handle_invalid_message(struct SessionInfo* sessionInfo, char* message);


/**
 * If a process_message() call goes wrong, deal with appropriately
 */
void process_error(struct SessionInfo* sessionInfo, 
        enum PlayerExitCode processedMessage);


/**
 * Validates the reconnect ID sent by server to client
 */
enum PlayerExitCode process_rid(char* message,
        struct SessionInfo* sessionInfo);


/**
 * When a playinfo message is sent from server, validate it and process it
 */
enum PlayerExitCode process_play_info(char* message, 
        struct SessionInfo* sessionInfo);


/**
 * Splits a string based on a delimiter.
 * Input: string to split, array of strings that will be changed, 
 * the delimiter to split on.
 * Returns: Number of sections
 */
int split_string(char* message, char** sections, char delimiter);


/**
 * Splits the rid into sections, then checks them
 */
enum PlayerExitCode check_rid_sections(char* message, 
        struct SessionInfo* sessionInfo);


/**
 * Splits the playinfo message into sections, then validates each of them
 */
enum PlayerExitCode check_playinfo_sections(char* message,
        struct SessionInfo* sessionInfo);
#endif //CLIENT_H
