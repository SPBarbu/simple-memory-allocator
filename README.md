FREE block:
|short tag = '0'
|data
|int length
|void* next
|void* previous
|short tag = '0'

INUSE block
|short tag = '1'
|data
|int length
|short tag = '1'

A block's "start" ie what previous & next point to is the data's first memory location.

The linked list for the free blocks maintains the blocks by increasing memory position. IE, the first block is the lowest on the heap and the last, the highest on the heap.

To minimize external fragmentation, a contiguous block is split only if the unused block is at least 1024 bytes + FREE_BLOCK_HEADER_SIZE + 2 * BLOCK_TAG_SIZE in length, ie it can fit one 1024 bytes block + the overhead.