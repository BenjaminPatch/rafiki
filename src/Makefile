CFLAGS = -Wall -pedantic -std=gnu99 
THREADFLAGS = -pthread -Wall -pedantic -std=gnu99 

LIBLINK = ../library/lib/librafiki.a

LIBRARYLINK = -L/home/benpa/gitRepos/rafiki/library/lib/libSrc -lRafiki-shared

PTHREAD = CFLAGS += -pthread
DEBUG = CFLAGS += PTHREAD

all: serv.o serverGamePlay.o getKey.o client.o gopher.o rafiki zazu gopher

serverGamePlay.o: serverGamePlay.c
	gcc -c -g $(THREADFLAGS) serverGamePlay.c -o serverGamePlay.o $(LIBLINK)

getKey.o: getKey.c
	gcc -c -g $(THREADFLAGS) getKey.c -o getKey.o $(LIBLINK)


serv.o: serv.c
	gcc -c -g $(THREADFLAGS) serv.c -o serv.o $(LIBLINK)

client.o: client.c
	gcc -c -g $(THREADFLAGS) client.c -o client.o $(LIBLINK)

gopher.o: gopher.c
	gcc -c -g $(THREADFLAGS) gopher.c -o gopher.o $(LIBLINK)

gopher: gopher.o
	gcc -Wall -pedantic -std=gnu99 gopher.o -o gopher $(LIBLINK) 

rafiki: serv.o serverGamePlay.o getKey.o
	gcc -Wall -pedantic -std=gnu99 -pthread serverGamePlay.o serv.o getKey.o -o rafiki $(LIBLINK) 

zazu: client.o getKey.o
	gcc -Wall -pedantic -std=gnu99 client.o getKey.o -o zazu $(LIBLINK) 
