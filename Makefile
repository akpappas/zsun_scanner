CC=mips-openwrt-linux-uclibc-gcc
OPT=-g -std=gnu99 -O0 -Wall -Wextra -DDEBUG
#CC=gcc
all: proc_limits.o myhash.o timer_thread.o
	$(CC) main.c myhash.o proc_limits.o timer_thread.o -o parser $(OPT) -lpthread

proc_limits.o: proc_limits.c myhash.c
	$(CC) -c proc_limits.c  $(OPT)

myhash.o: myhash.c
	$(CC) -c myhash.c $(OPT)

timer_thread.o: timer_thread.c
	$(CC) -c timer_thread.c $(OPT)

clean:
	rm *.o
