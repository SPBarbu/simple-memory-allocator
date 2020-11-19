CC=gcc
CFLAGS=-fsanitize=signed-integer-overflow -fsanitize=undefined -g -std=gnu99 -O0 -Wall -Wextra -Wno-sign-compare -Wno-unused-parameter -Wno-unused-variable -Wshadow -Wno-unused-result

sma:
	$(CC) tester.c sma.c -o SMA $(CFLAGS)
clean:
	rm -rf SMA