all: feed broker

feed: feed.c feed.h utils.h
	gcc -o feed feed.c -pthread

broker: manager.c manager.h utils.h
	gcc -o broker manager.c -pthread

clean:
	$(RM) feed broker
