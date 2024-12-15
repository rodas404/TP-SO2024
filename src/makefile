all: feed broker

feed: feed.o feed_comandos.o utils.o
	gcc -o feed feed.c feed_comandos.o utils.o  -pthread

broker: manager.o manager_users.o manager_topics.o manager_lifecycle.o manager_messages.o utils.o
	gcc -o broker manager.o manager_users.o manager_topics.o manager_lifecycle.o manager_messages.o utils.o -pthread

feed.o: feed.c feed.h utils.h 
	gcc -c feed.c

feed_comandos.o: feed_comandos.c feed_comandos.h
	gcc -c feed_comandos.c

manager_users.o: manager_users.c manager_users.h manager_messages.h manager.h
	gcc -c manager_users.c 

manager_topics.o: manager_topics.c manager_topics.h manager_messages.h
	gcc -c manager_topics.c

manager_lifecycle.o: manager_lifecycle.c manager_lifecycle.h manager_topics.h
	gcc -c manager_lifecycle.c	

manager_messages.o: manager_messages.c manager_messages.h manager.h manager_topics.h
	gcc -c manager_messages.c

broker.o: manager.c manager.h
	gcc -c manager.c

utils.o: utils.c utils.h
	gcc -c utils.c

clean:
	rm -f feed broker feed.o broker.o manager_lifecycle.o manager_topics.o manager_users.o manager_messages.o utils.o feed_comandos.o

clean-feed:
	rm -f feed feed.o utils.o feed_comandos.o

clean-broker:
	rm -f broker broker.o utils.o manager_lifecycle.o manager_topics.o manager_users.o manager_messages.o manager.o

