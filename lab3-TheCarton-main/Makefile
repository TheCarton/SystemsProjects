LDLIBS=-lz -lpthread
CFLAGS=-ggdb3 -Wall

EXES = dbserver dbclient

all: $(EXES)

dbclient: dbclient.o

dbserver: dbserver.o stats.o queue.o kvstore.o worker.o

stats.o: stats.h stats.c lab3.h

queue.o: queue.h queue.c lab3.h

kvstore.o: kvstore.h kvstore.c lab3.h

worker.o: worker.h worker.c lab3.h

clean:
	rm -f $(EXES) *.o data.[0-9]*
