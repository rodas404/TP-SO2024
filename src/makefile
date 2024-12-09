all: feed broker

feed: feed.o feed.h utils.h
	gcc -o feed feed.c -pthread

feed.o: feed.c
	gcc -c feed.c

broker: manager.o manager.h utils.h
	gcc -o broker manager.c -pthread

broker.o: manager.c
	gcc -c manager.c

clean:
	rm -f feed broker feed.o broker.o

clean-feed:
	rm -f feed feed.o

clean-broker:
	rm -f broker broker.o
