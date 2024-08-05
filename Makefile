CC = gcc
CFLAGS = -Wall -Wextra -std=c99

SRCS = mysh.c readline.c commandlist.c commandhandler.c utils.c 
OBJS = $(SRCS:.c=.o)
HDRS = $(wildcard *.h)

mysh: $(OBJS)
	$(CC) $(CFLAGS) -o $@ $^

%.o: %.c $(HDRS)
	$(CC) $(CFLAGS) -c -g $< -o $@

clean:
	rm -f $(OBJS) mysh