CC = gcc
CFLAGS = -g -Wall -pedantic 

############################exe############################

all: miner monitor

miner: miner.o pow.o -lm
monitor: monitor.o pow.o -lm

#########################.o################################

voting.o: voting.c 
	$(CC) $(CFLAGS) -c $^

func.o: func.h func.c
	$(CC) $(CFLAGS) -c $^

pow.o: pow.h pow.c
	$(CC) $(CFLAGS) -c $^


#########################run###############################

run1: 
		./miner 8 100

run2:
		./monitor 0

run3:
		./monitor 500

runv1:
		valgrind --leak-check=full --show-leak-kinds=all --track-origins=yes ./miner 15 50

runv2:
		valgrind --leak-check=full --show-leak-kinds=all --track-origins=yes ./monitor 500

runv3:
		valgrind --leak-check=full --show-leak-kinds=all --track-origins=yes ./monitor 100

########################clean##############################

clean: 
		rm *.o *.gch monitor miner