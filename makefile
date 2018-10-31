OBJS_MIRRORSERVER = MirrorServer.o help_functions.o Buffer_Queue.o
SOURCE_MIRRORSERVER = MirrorServer.c help_functions.c Buffer_Queue.c
OUT_MIRRORSERVER = MirrorServer

OBJS_MIRRORINITIATOR = MirrorInitiator.o help_functions.o Buffer_Queue.o
SOURCE_MIRRORINITIATOR = MirrorInitiator.c help_functions.c Buffer_Queue.c
OUT_MIRRORINITIATOR = MirrorInitiator

OBJS_CONTENTSERVER = ContentServer.o help_functions.o Buffer_Queue.o
SOURCE_CONTENTSERVER = ContentServer.c help_functions.c Buffer_Queue.c
OUT_CONTENTSERVER = ContentServer


LIBS = -lpthread
FLAGS = -g -c -Wall 

all: server client contentserver

server: $(OBJS_MIRRORSERVER)
	gcc $(OBJS_MIRRORSERVER) -o $(OUT_MIRRORSERVER) $(LIBS)

client: $(OBJS_MIRRORINITIATOR)
	gcc $(OBJS_MIRRORINITIATOR) -o $(OUT_MIRRORINITIATOR)

contentserver: $(OBJS_CONTENTSERVER)
	gcc $(OBJS_CONTENTSERVER) -o $(OUT_CONTENTSERVER)

MirrorServer.o: MirrorServer.c
	gcc $(FLAGS) MirrorServer.c

MirrorInitiator.o: MirrorInitiator.c
	gcc $(FLAGS) MirrorInitiator.c

help_functions.o: help_functions.c
	gcc $(FLAGS) help_functions.c

Buffer_Queue.o: Buffer_Queue.c
	gcc $(FLAGS) Buffer_Queue.c	


clean:
	rm -f $(OBJS_MIRRORSERVER) $(OBJS_MIRRORINITIATOR) $(OBJS_CONTENTSERVER)
	rm -f $(OUT_MIRRORSERVER) $(OUT_MIRRORINITIATOR) $(OUT_CONTENTSERVER)


