CC = gcc
CFLAGS = -g -Wall -pedantic

############################exe############################

all: minrush

minrush: minrush.o minero.o pow.o monitor.o

#########################.o################################

minrush.o: minrush.c 
	$(CC) $(CFLAGS) -c $^

pow.o: pow.c pow.h
	$(CC) $(CFLAGS) -c $^

minero.o: minero.c minero.h pow.h
	$(CC) $(CFLAGS) -c $^

minitor.o: monitor.c monitor.h
	$(CC) $(CFLAGS) -c $^

#########################run###############################

run: 
		./minrush 50 3 10

runv:
		valgrind  ./minrush 0 5 3

########################clean##############################

clean: 
		rm *.o *.gch minrush