CFLAGS = -Wall -pedantic -std=gnu99 -I/local/courses/csse2310/include
THREADFLAGS = -pthread -Wall -pedantic -std=gnu99 -I/local/courses/csse2310/include

LIBRARYLINK = -L/local/courses/csse2310/lib -la4

PTHREAD = CFLAGS += -pthread
DEBUG = CFLAGS += PTHREAD

all: serv.o serverGamePlay.o getKey.o client.o gopher.o rafiki zazu gopher

serverGamePlay.o: serverGamePlay.c
	gcc -c -g $(THREADFLAGS) $(LIBRARYLINK) serverGamePlay.c -o serverGamePlay.o

getKey.o: getKey.c
	gcc -c -g $(THREADFLAGS) $(LIBRARYLINK) getKey.c -o getKey.o

noLib: server.c
	gcc -g -Wall -pedantic -std=gnu99 -pthread server.c -o server 

serv.o: serv.c
	gcc -c -g $(THREADFLAGS) $(LIBRARYLINK) serv.c -o serv.o

client.o: client.c
	gcc -c -g $(THREADFLAGS) $(LIBRARYLINK) client.c -o client.o

gopher.o: gopher.c
	gcc -c -g $(THREADFLAGS) $(LIBRARYLINK) gopher.c -o gopher.o

gopher: gopher.o
	gcc -Wall -pedantic -std=gnu99 $(LIBRARYLINK) gopher.o -o gopher

rafiki: serv.o serverGamePlay.o getKey.o
	gcc -Wall -pedantic -std=gnu99 $(LIBRARYLINK) -pthread serverGamePlay.o serv.o getKey.o -o rafiki

zazu: client.o getKey.o
	gcc -Wall -pedantic -std=gnu99 $(LIBRARYLINK) client.o getKey.o -o zazu
