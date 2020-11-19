#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>
#include <inttypes.h>
#include "sma.h"

/**
 * Modified from https://gist.github.com/domnikl/af00cc154e3da1c5d965
 */
void hexDump(void* addr, int len) {
    int i;
    unsigned char buff[17];
    unsigned char* pc = (unsigned char*)addr;
    char str[20];

    // Process every byte in the data.
    for (i = 0; i < len; i++) {

        // Multiple of 16 means new line (with line offset).
        if ((i % 16) == 0) {
            // Just don't print ASCII for the zeroth line.
            if (i != 0) {
                sprintf(str, "  %s\n", buff);
                fputs(str, stdout);
            }
            // Output the offset.
            sprintf(str, "  %p ", i + addr);
            fputs(str, stdout);
        }

        // Output the hex code for the specific character.
        sprintf(str, " %02x", pc[i]);
        fputs(str, stdout);


        // And store a printable ASCII character for later.
        if ((pc[i] < 0x20) || (pc[i] > 0x7e)) {
            buff[i % 16] = '.';
        }
        else {
            buff[i % 16] = pc[i];
        }

        buff[(i % 16) + 1] = '\0';
    }

    // Pad out last line if not exactly 16 characters.
    while ((i % 16) != 0) {
        sprintf(str, "   ");
        fputs(str, stdout);
        i++;
    }

    // And print the final ASCII bit.
    sprintf(str, "  %s\n", buff);
    fputs(str, stdout);
}

int main(int argc, char* argv[]) {
    void* heap_bottom = sbrk(0);
    sbrk(1);
    void* heap_top = sbrk(0);
    *(char*)heap_top = 'a';
    char str[16];

    hexDump(heap_top - 1, 1024);

    return 0;
}