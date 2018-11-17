#ifndef SERVER_H
#define SERVER_H


#include "serverGamePlay.h" 
#include "serverStructs.h"

/*Maximum/only amount of "sections" each line in the statfile can have*/
#define MAX_SECTIONS 4

/*Maximum amount of digits in an integer on this system*/
#define MAX_LENGTH_INT 10

#define BUFFER 256


/**
 * Takes an enum ServerExitCode, and exits with the appropriate message
 */
void exit_server(enum ServerExitCode exitCode);


/**
 * Frees all mallocs
 */
void clean_up(struct ServerData* server);


/**
 * Checks the validity of the arguments
 */
void check_args(int argc, char** argv, struct ServerData* server);


/**
 * Sets up the signal handler for SIGTERM
 */
void setup_sigterm();


/**
 * Sets up the signal handling for SIGINT
 */
void setup_sigint();


/**
 * Cleans up then shuts down the server, as described in spec
 */
void sigterm_handler(int sigNum);


/**
 * Sets up the signal handler to ignore SIGPIPE
 */
void ignore_sigpipe();


/**
 * Used to ignore signals
 */
void do_nothing();


/**
 * Gets the server key from the keyfile
 */
char* get_key(char* fileName, int* valid);


/**
 * Validates the reconnection ID when a player attempts to reconnect
 */
void validate_reconnect(struct ClientInfo* clientInfo, char* rid);


/**
 * The waiting room/lobby for games. Uses a semaphore to get notificed when a
 * game is ready
 */
void lobby(struct ServerData* server);


/**
 * Finds a game that is full so we can start the game
 */
void find_game(struct ServerData* server);


/**
 * Figures out the scores of all players,
 * then sends them to the client
 */
void send_scores(struct ClientInfo* clientInfo);


/**
 * If the scores struct for a player of a particular name has already been
 * made, return 1. Else, return 0.
 */
int name_already_made(struct PlayerScore* newPlayer, char* name, 
        struct ClientInfo* clientInfo, struct PlayerScore* firstPlayer);


/**
 * Appends a player score to linked list
 */
void append_player_score(struct GamePlayer player, 
        struct PlayerScore* newPlayer, struct PlayerScore* headPlayer);


/**
 * Prints the messages to the scores client
 */
void print_scores_message(struct PlayerScore* firstPlayer, FILE* writeFp);


/**
 * Sorts the scores based on points, then total tokens
 */
void order_scores(struct PlayerScore* firstScore);


/**
 * Sorts the scores based on points, then total tokens
 */
void order_scores(struct PlayerScore* firstScore);


/**
 * Swaps the data of two PlayerScore structs, used in sorting
 */
void swap_two_scores(struct PlayerScore* first, struct PlayerScore* second);


/**
 * Waits on sigint to be called then deals with it properly
 */
void* wait_on_sigint(void*);


/**
 * Prints the ports to stderr
 */
void print_port(struct ServerData* server);


/**
 * Signal handler for when the server receives SIGINT
 */
void sigint_handler();


/**
 * Starts the server. i.e. starts listening on all ports before creating games
 */
void initialise_server(struct ServerData* server, char* statFile);

/**
 * read_line function I made before the a3 library was released
 */
//char* read_line(FILE* file);

/**
 * Parses a line in the statfile
 */
struct StartGame* parse_stat_line(char* line);

/**
 * Splits string into array of strings, split on a delimiter
 */
char** split_string(char* toSplit, char Delimiter);


/**
 * Checks the validity of the sections of a particular line in the
 * statfile
 */
void check_sections(char** sections);


/**
 * Initialises the information for a particular port to listen on
 */
void initialise_port(struct StartGame* gameInfo, int count);


/**
 * Appends a StartGame struct to the linked list
 */
void append_start_game(struct ServerData* server, struct StartGame* gameInfo);


/**
 * When a client connects, parse their message and deal with appropriately
 */
void process_client_message(char* input, struct ClientInfo* clientInfo);


/**
 * Processes a game name, if game is full, either join another with same name
 * or make a new game
 */
void process_game_name(struct ClientInfo* clientInfo, char* input);

/**
 * When a client connects, handle their requests
 */
void* handle_client(void* client);


/**
 * Adds a player to an already-existing game.
 * Closes the thread, but stores the file pointers used to communicate with
 * this client.
 */
void add_player_to_game(struct GameFull* currentGame,
        struct ClientInfo* clientInfo);


/**
 * Creates a new game if there isn't a suitable one already-made.
 * Stores the initiating players info in it.
 */
void make_new_game(struct ClientInfo* clientInfo, char* gameName, int count);


/**
 * Sets up the info for a game player in game
 */
void setup_player(struct GameFull* game, struct ClientInfo* clientInfo,
        int index);


/**
 * Appends a GameFull struct to the linked list in server
 */
void append_game_full(struct ServerData* server, struct GameFull* newGame);


/**
 * Finds the right Game Rules based off the port a user connected to
 * i.e. if a player is starting a new game and they connected on port
 * 3000, look at the statfile line for port 3000
 */
struct StartGame* find_right_port(struct ClientInfo* clientInfo);


/**
 * Listens on a particular port as designated by the statfile
 */
void* listen_port(void* gameInfo);


/**
 * Handles a new connection from client, as opposed to reconnection
 */
void new_connection(char* input, struct ClientInfo* clientInfo);
void get_game_and_name(struct ClientInfo* clientInfo);

#endif //SERVER_H
