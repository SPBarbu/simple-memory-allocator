CC=gcc
CFLAGS=-fsanitize=signed-integer-overflow -g -std=gnu99 -O0 -Wextra -Wno-sign-compare -Wno-unused-parameter -Wno-unused-variable -Wshadow -Wno-unused-result

sma:
	$(CC) a3_test.c sma.c -o SMA -lm $(CFLAGS)
clean:
	rm -rf SMA