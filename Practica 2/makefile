CC = gcc
CFLAGS = -g -Wall -pedantic 

############################exe############################

all: voting

voting: voting.o func.o -lm

#########################.o################################

voting.o: voting.c 
	$(CC) $(CFLAGS) -c $^

func.o: func.h func.c
	$(CC) $(CFLAGS) -c $^


#########################run###############################

run: 
		./voting 20 5 

runv:
		valgrind --leak-check=full --show-leak-kinds=all --track-origins=yes ./voting 10 5

########################clean##############################

clean: 
		rm *.o *.gch *.txt voting 