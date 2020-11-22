For the grader: 

For the NEXT-FIT allocation, I assume that changing the memory allocation policy resets the last allocated memory to the start of the heap. This is because in the test file, before test4, the last allocated memory was cp2 at c2[27] so the next-fit algorithm should have allocated at c2[29] where there were 3*16kB free, yet test4 expects cp3 to be allocated at c2[8]. Not resetting the last allocated memory after the policy has been changed could also lead to having the last allocated memory pointer in the middle of some other allocated block's memory. So, if the last allocated memory is freed before a new memory is allocated, the next-fit also resets from the base of the heap.

Regarding the runtime errors of misaligned memory accessing, Professor Maheswaran told me by email it should be ok.

------------
|type|size|relative position
FREE block:
|char tag = '0'|1|length-1
|data|X|21
|int length|4|17
|void* next|8|9
|void* previous|8|1
|char tag = '0'|1|0

INUSE block:
|char tag = '1'|1|length-1
|data|X|5
|int length|4|1
|char tag = '1'|1|0

A block's "start" ie what previous & next point to is the data's first memory location.

~~The linked list for the free blocks maintains the blocks by increasing memory position. IE, the first block is the lowest on the heap and the last, the highest on the heap.
Made this decision of preserving free blocks in order after implementing the tags in the blocks. The tags are useless if the ordering can be preserved. Possibly remove so it doesn't cause misalignment anymore, but not necessary, just 2 chars more in overhead.~~ The tags are useful for moving around the memory to get statistics or finding the next free block when the current block is allocated and surrounded by other allocated blocks. With tags, the library can get from any block to any other.

To minimize external fragmentation, a contiguous block is split only if the unused block is at least 1024 bytes + FREE_BLOCK_HEADER_SIZE + 2 * BLOCK_TAG_SIZE in length, ie it can fit one 1024 bytes block + the overhead. If the excess is less than that, there is internal fragmentation.

Undefined behavior when calling sma_free and sma_realloc on a modified pointer than the one returned by sma_malloc or sma_realloc.