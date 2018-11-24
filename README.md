# Rafiki (Client-Server Network Program)
A multithreaded server which hosts many games of the card game in my "austerityCardGame" repo. This time with humans playing by using the client program to connect to the server over localhost.
### Skills Tested
- C Programming
- Network programming
- Multithreading
- Advanced file IO
### Getting Started
##### Prerequisites
To run this program on a unix system, you must have gcc (or some other valid compiler). If you are using a compiler other than gcc, you will need to set up the compilation process accordingly.
##### Installing/Compiling
The Makefile in /src outlines which files should be compiled then linked to other object files. Running the "make" command should be valid for most unix systems. This project was made on Ubuntu and compiles with the provided makefile on that.
To compile the library, you will also need to go to library/lib and run "make" to produce the .a file.

### Game Walkthrough/Instructions
**All required files have examples in src/exampleDeckKeysStats/**
##### Server-side
Once compiled, initiate the server by executing ./rafiki. The usage instructions are: rafiki keyfile deckfile statfile timeout
    - keyfile: The file used for verifying client connections.
    - deckfile: Stores data about the cards which are to be used in games.
    - statfile: The ports which the program is to listen on (if port = 0, use ephemeral port). Each line has a port then information about the associated with that port. When a player connects to the server with an unused game name, then the server will start a game based on the rules associated with the port the player connected on.
    - timeout: How long the server should wait for when a client disconnects before ending the game.
![Server Initialisation](https://i.gyazo.com/dedc6497d467817d60f3de99fe042ccb.png)

**Figure 1: the port is initialised and is listening on ports 63654 and 3004**
![SIGINT](https://i.gyazo.com/33b3305dfddf71083408c402b740ee6e.png)

**Figure 2: SIGINT causes the server to close listening on all ports, re-read the statfile and start listening on those ports.**
**If you want to shut down the server, SIGTERM or SIGKILL must be sent to the server.**

From this point onwards, the server will not print anything else to stdout, except for error messages, or the new ports if SIGINT is sent.
It will, however, be sending many messages to the player client or scores client.
##### Client-side (Players)
Start a player by executing ./zazu. The usage instructions are: zazu keyfile port game pname
    - keyfile: The file used for verifying client connections.
    - port: One of the ports that the server is listening on.
    - game: The name of the game to connect to. If no game with that name exists, a new one will be created.
    - pname: Name of the player
**_OR_**, if reconnecting
zazu keyfile port "reconnect" reconnectID
    - reconnect: the literal word "reconnect"
    - reconnectID: The reconnect ID sent by server at start of game

![Game Started](https://i.gyazo.com/d88fae94b6c21c45a941972320f7d54b.png)

**Figure 3: When enough players have joined a given game, the server will begin sending data about the game.**
**The first message sent will be the reconnect ID, which the player can save to use for reconnecting, should player disconnect.**
**After RID is sent, basic player info and info on each card is output until all the starting cards have been drawn.**

The format for the card info is: Card NUM:ColourDiscount/Points/pricePurp,priceBlue,priceYellow,PriceRed

![prompting User](https://i.gyazo.com/46b1978401b1213cce460a290b0ca1cf.png)

**Figure 4: Now that all the cards have been drawn, the first player will be prompted for a move**

Valid moves are: *purchase*, *take*, or *wild*
For a purchase action, the user will then be prompted for the card they wish to buy:
Card>
The user must then enter a number from 0-7, inclusive. After this, the user will be prompted for the tokens to
use. The player will then prompt the user for how many tokens of each colour to use for this purchase, in the
order: P, B, Y, R, W. The prompt for each of these will be in the form:
Token-?>
where ? is replaced with the appropriate token colour letter from above. The player will not prompt a user
for a colour if they do not have any tokens of that colour. A valid response from the user is a non-negative
number of tokens, up to the number of tokens of that colour currently held by the player. After all information
is received, the player will send a purchase message to the server.
For a take action, the user will then be prompted how many of each colour token they would like to take,
in the order: P, B, Y, R. The prompt for each of these will be in the form:
Token-?>
where ? is replaced with the appropriate token colour letter from above. A valid response from the user is
a non-negative number of tokens up to the maximum number available in the game. After all information is
received, the player will send a take message to the server.
No additional prompting will be done if the user performs a wild action.
At any of the prompts above, if the user input is invalid, the player should display that prompt again to
obtain a valid value. There is no limit to how many times the user will be reprompted by the player. The
player should not send an action message to the server until a valid message (in terms of structure) has been
constructed from the user input. The server may still reprompt the player for a move by
sending a dowhat message again.

### Author(s)
Benjamin Patch (myself)
