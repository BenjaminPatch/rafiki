all: deck.o game.o player.o protocol.o server.o token.o util.o librafiki.a

deck.o: deck.c
	gcc -c -Wall deck.c
game.o: game.c
	gcc -c -Wall game.c
player.o: player.c
	gcc -c -Wall player.c
protocol.o: protocol.c
	gcc -c -Wall protocol.c
server.o: server.c
	gcc -c -Wall server.c
token.o: token.c
	gcc -c -Wall token.c
util.o: util.c
	gcc -c -Wall util.c
librafiki.a: deck.o  game.o  player.o  protocol.o  server.o  token.o  util.o
	ar -cvq librafiki.a *.o
