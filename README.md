For the grader: 

My code includes the math.h library so the -lm option should be used for compiling.

All parts are expected to work well. The NEXT-FIT allocation wasn't thoroughly tested, so it might produce segmentation faults where I didn't test.

For the NEXT-FIT allocation, I assume that changing the memory allocation policy resets the last allocated memory to the start of the heap. This is because in the test file, before test4, the last allocated memory was cp2 at c2[27] so the next-fit algorithm should have allocated at c2[29] where there were 3*16kB free, yet test4 expects cp3 to be allocated at c2[8]. Not resetting the last allocated memory after the policy has been changed could also lead to having the last allocated memory pointer in the middle of some other allocated block's memory. So, if the last allocated memory is freed, the next-fit also resets from the base of the heap.

Regarding the runtime errors of misaligned memory accessing, Professor Maheswaran told me by email it should be ok.

Output for testing my program with the given tests (a3_test.c):
Test 1: Excess Memory Allocation...
                                 PASSED

Test 2: Program break expansion test...
                                 PASSED

Test 3: Check for Worst Fit algorithm...
                                 PASSED

Test 4: Check for Next Fit algorithm...
                                 PASSED

Test 5: Check for Reallocation with Next Fit...
                                 PASSED

Test 6: Print SMA Statistics...
===============================
Total number of bytes of data allocated not including overhead: 26000402
Total number of bytes of data allocated including overhead: 26000900
Total number of bytes of free space: 140354
Size of the largest contiguous free space (in bytes): 49196
Size of the block at the top of the heap, if any: 16428
Number of free holes on the heap: 5

--------------------------------------------------------------------------------------------
Personnal design decisions, I don't know if relevant:

A block's "start" ie what previous & next point to is the data's first memory location.

~~The linked list for the free blocks maintains the blocks by increasing memory position. IE, the first block is the lowest on the heap and the last, the highest on the heap.
Made this decision of preserving free blocks in order after implementing the tags in the blocks. The tags are useless if the ordering can be preserved. Possibly remove so it doesn't cause misalignment anymore, but not necessary, just 2 chars more in overhead.~~ The tags are useful for moving around the memory to get statistics or finding the next free block when the current block is allocated and surrounded by other allocated blocks. With tags, the library can get from any block to any other.

To minimize external fragmentation, a contiguous block is split only if the unused block is at least 1024 bytes + FREE_BLOCK_HEADER_SIZE + 2 * BLOCK_TAG_SIZE in length, ie it can fit one 1024 bytes block + the overhead. If the excess is less than that, there is internal fragmentation.

Undefined behavior when calling sma_free and sma_realloc on a modified pointer than the one returned by sma_malloc or sma_realloc.