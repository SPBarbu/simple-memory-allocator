For the grader: Regarding the runtime errors of misaligned memory accessing; Professor Maheswaran confirmed with me that it should not be penalized.

FREE block:
|char tag = '0'|1|length-1
|data|X|21
|int length|4|17
|void* next|8|9
|void* previous|8|1
|char tag = '0'|1|0

INUSE block
|char tag = '1'|1|length-1
|data|X|5
|int length|4|1
|char tag = '1'|1|0

A block's "start" ie what previous & next point to is the data's first memory location.

The linked list for the free blocks maintains the blocks by increasing memory position. IE, the first block is the lowest on the heap and the last, the highest on the heap.
Made this decision of preserving free blocks in order after implementing the tags in the blocks. The tags are useless if the ordering can be preserved. Possibly remove so it doesn't cause misalignment anymore, but not critical.


To minimize external fragmentation, a contiguous block is split only if the unused block is at least 1024 bytes + FREE_BLOCK_HEADER_SIZE + 2 * BLOCK_TAG_SIZE in length, ie it can fit one 1024 bytes block + the overhead.

Undefined behavior when calling sma_free on a modified pointer than the one returned by sma_malloc or sma_realloc.