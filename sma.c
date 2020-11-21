/*
 * =====================================================================================
 *
 *	Filename:  		sma.c
 *
 *  Description:	Base code for Assignment 3 for ECSE-427 / COMP-310
 *
 *  Version:  		1.0
 *  Created:  		6/11/2020 9:30:00 AM
 *  Revised:  		-
 *  Compiler:  		gcc
 *
 *  Author:  		Mohammad Mushfiqur Rahman
 *
 *  Instructions:   Please address all the "TODO"s in the code below and modify
 * 					them accordingly. Feel free to modify the "PRIVATE" functions.
 * 					Don't modify the "PUBLIC" functions (except the TODO part), unless
 * 					you find a bug! Refer to the Assignment Handout for further info.
 * =====================================================================================
 */

 /* Includes */
#include "sma.h" // Please add any libraries you plan to use inside this file

/* Function macros */
#define PREVIOUS(BLOCK) (*(void**)(BLOCK-sizeof(int)-sizeof(void*)-sizeof(void*)))
#define NEXT(BLOCK) (*(void**)(BLOCK-sizeof(int)-sizeof(void*)))
#define SIZE(BLOCK) (*(int*)(BLOCK-sizeof(int)))
#define TYPE(BLOCK) (*(char*)(BLOCK+SIZE(BLOCK)))

/* Definitions*/
#define MAX_TOP_FREE (128 * 1024) // Max top free block size = 128 Kbytes

#define FREE_BLOCK_HEADER_SIZE (2*sizeof(void*) + sizeof(int)) // Size of the Header in a free memory block
#define INUSE_BLOCK_HEADER_SIZE (sizeof(int)) // Size of the Header in a used memory block
#define BLOCK_TAG_SIZE (sizeof(char)) // Size of a INUSE/FREE tag. Expect to use 2 at the start and end of a block
#define INUSE_OVERHEAD (INUSE_BLOCK_HEADER_SIZE + 2 * BLOCK_TAG_SIZE) //Memory overhead for INUSE block
#define FREE_OVERHEAD (FREE_BLOCK_HEADER_SIZE + 2 * BLOCK_TAG_SIZE)//Memory overhead for FREE block
#define MIN_EXCESS_SIZE (INUSE_OVERHEAD + 32) //Minimum excess size for split. 
#define PBRK_CHUNK_ALLOCATION (128)//size of chunk pbrk allocates

typedef enum //	Policy type definition
{
	WORST,
	NEXT
} Policy;

char* sma_malloc_error;
void* freeListHead = NULL;			  //	The pointer to the HEAD of the doubly linked free memory list
void* startNextSearch = NULL; //Block on which to start next-fit algorithm
unsigned long totalAllocatedSize = 0; //	Total Allocated memory in Bytes
unsigned long totalFreeSize = 0;	  //	Total Free memory in Bytes in the free memory list
Policy currentPolicy = WORST;		  //	Current Policy
void* reallocating = 0;	//indicates to allocate_free_list to start searching where previous ptr was allocated
void* heap_start = 0;

/*
 * =====================================================================================
 *	Public Functions for SMA
 * =====================================================================================
 */

void test() {

	//test worst fit
	heap_start = sbrk(0);
	void* b0 = sbrk(128) + 21;
	add_block_freeList(b0, 128 - FREE_OVERHEAD);
	void* b1 = allocate_worst_fit(1);


	/*// test clean memory
	void* block1 = sbrk(128) + 21;
	add_block_freeList(block1, 128 - FREE_OVERHEAD);
	sbrk(128);
	void* block2 = block1 + SIZE(block1) + BLOCK_TAG_SIZE + BLOCK_TAG_SIZE + FREE_BLOCK_HEADER_SIZE;
	add_block_freeList(block2, 128 - FREE_OVERHEAD);
	*/

	// test add block free list
	// void* block = sbrk(1024) + 21;
	// add_block_freeList(block, 1);
	// block += 23;
	// block += 23;
	// add_block_freeList(block, 1);
	// block -= 23;
	// add_block_freeList(block, 1);
}

/*
 *	Funcation Name: sma_malloc
 *	Input type:		int
 * 	Output type:	void*
 * 	Description:	Allocates a memory block of input size from the heap, and returns a
 * 					pointer pointing to it. Returns NULL if failed and sets a global error.
 */
void* sma_malloc(int size) {
	void* pMemory = NULL;

	// Checks if the free list is empty
	if (freeListHead == NULL) {
		// Allocate memory by increasing the Program Break
		pMemory = allocate_pBrk(size);
	}
	// If free list is not empty
	else {
		// Allocate memory from the free memory list
		pMemory = allocate_freeList(size);

		// If a valid memory could NOT be allocated from the free memory list
		if (pMemory == (void*)-2) {
			// Allocate memory by increasing the Program Break
			pMemory = allocate_pBrk(size);
		}
	}//TODO support if top of free list is free but not big enough to fit size
	// Validates memory allocation
	if (pMemory < 0 || pMemory == NULL) {
		sma_malloc_error = "Error: Memory allocation failed!";
		return NULL;
	}

	// Updates SMA Info
	totalAllocatedSize += size;

	return pMemory;
}

/*
 *	Funcation Name: sma_free
 *	Input type:		void*
 * 	Output type:	void
 * 	Description:	Deallocates the memory block pointed by the input pointer
 */
void sma_free(void* ptr) {
	//	Checks if the ptr is NULL
	if (ptr == NULL) {
		puts("Error: Attempting to free NULL!");
	}
	//	Checks if the ptr is beyond Program Break
	else if (ptr > sbrk(0)) {
		puts("Error: Attempting to free unallocated space!");
	}
	else {
		//	Adds the block to the free memory list
		//overhead for free list is bigger so need to change properties
		add_block_freeList(ptr + (FREE_BLOCK_HEADER_SIZE - INUSE_BLOCK_HEADER_SIZE), SIZE(ptr) - (FREE_BLOCK_HEADER_SIZE - INUSE_BLOCK_HEADER_SIZE));
	}
}

/*
 *	Funcation Name: sma_mallopt
 *	Input type:		int
 * 	Output type:	void
 * 	Description:	Specifies the memory allocation policy
 */
void sma_mallopt(int policy) {
	// Assigns the appropriate Policy
	if (policy == 1) {
		currentPolicy = WORST;
	}
	else if (policy == 2) {
		currentPolicy = NEXT;
	}
}

/*
 *	Funcation Name: sma_mallinfo
 *	Input type:		void
 * 	Output type:	void
 * 	Description:	Prints statistics about current memory allocation by SMA.
 */
void sma_mallinfo() {
	//	Finds the largest Contiguous Free Space (should be the largest free block)
	int largestFreeBlock = get_largest_freeBlock();
	char str[60];

	//	Prints the SMA Stats
	sprintf(str, "Total number of bytes allocated for data: %lu", totalAllocatedSize);
	puts(str);
	//TODO
	// sprintf(str, "Total number of bytes allocated including overhead: %lu", totalAllocatedSize);
	// puts(str);
	sprintf(str, "Total free space: %lu", totalFreeSize);
	puts(str);
	sprintf(str, "Size of largest contigious free space (in bytes): %d", largestFreeBlock);
	puts(str);
}

/*
 *	Funcation Name: sma_realloc
 *	Input type:		void*, int
 * 	Output type:	void*
 * 	Description:	Reallocates memory pointed to by the input pointer by resizing the
 * 					memory block according to the input size.
 */
void* sma_realloc(void* ptr, int size) {
	//	Checks if the ptr is NULL
	if (ptr == NULL) {
		puts("Error: Attempting to reallocate NULL!");
		return 0;
	}
	//	Checks if the ptr is beyond Program Break
	else if (ptr > sbrk(0)) {
		puts("Error: Attempting to reallocate unallocated space!");
		return 0;
	}

	//save the data
	char data[SIZE(ptr)];
	for (int i = 0; i < SIZE(ptr); i++) {
		data[i] = (*(char*)(ptr + i));
	}

	//replace the data in memory where theres enough size
	reallocating = ptr;
	sma_free(ptr);
	ptr = sma_malloc(size);
	reallocating = 0;
	if (ptr > 0) {//check if successfully malloced
		for (int i = 0; i < sizeof(data); i++) {
			*(char*)(ptr + i) = data[i];//copy data
		}
	}
	return ptr;




	// TODO: 	Should be similar to sma_malloc, except you need to check if the pointer address
	//			had been previously allocated.
	// Hint:	Check if you need to expand or contract the memory. If new size is smaller, then
	//			chop off the current allocated memory and add to the free list. If new size is bigger
	//			then check if there is sufficient adjacent free space to expand, otherwise find a new block
	//			like sma_malloc.
	//			Should not accept a NULL pointer, and the size should be greater than 0.
}

/*
 * =====================================================================================
 *	Private Functions for SMA
 * =====================================================================================
 */

 /*
  *	Funcation Name: allocate_pBrk
  *	Input type:		int
  * 	Output type:	void*
  * 	Description:	Allocates memory by increasing the Program Break
  */
void* allocate_pBrk(int size) {
	void* newBlock = NULL;
	int excessLength;

	//calculate number of chunks to request
	int minimum_INUSE_size = size + FREE_OVERHEAD;
	int number_chunks_to_allocate = (int)ceil((double)minimum_INUSE_size / PBRK_CHUNK_ALLOCATION);
	int allocate_size = number_chunks_to_allocate * PBRK_CHUNK_ALLOCATION;

	newBlock = sbrk(allocate_size); //get previous pbrk location and request chunks of memory
	if (heap_start = 0)
		heap_start = newBlock;
	newBlock += BLOCK_TAG_SIZE + INUSE_BLOCK_HEADER_SIZE;//points to begining of data

	int newBlock_size = size + FREE_OVERHEAD - INUSE_OVERHEAD; // adding free overhead rather than just inuse overhead so can convert to free when needed b/c free overhead is move than inuse overhead

	excessLength = allocate_size - (newBlock_size + INUSE_OVERHEAD);

	//	Allocates the Memory Block
	allocate_block(newBlock, newBlock_size, excessLength, 0);

	return newBlock;
}

/*
 *	Funcation Name: allocate_freeList
 *	Input type:		int
 * 	Output type:	void*
 * 	Description:	Allocates memory from the free memory list
 */
void* allocate_freeList(int size) {
	void* pMemory = NULL;

	if (currentPolicy == WORST) {
		// Allocates memory using Worst Fit Policy
		pMemory = allocate_worst_fit(size);
	}
	else if (currentPolicy == NEXT) {
		// Allocates memory using Next Fit Policy
		pMemory = allocate_next_fit(size);
	}
	else {
		pMemory = NULL;
	}

	return pMemory;
}

/*
 *	Funcation Name: allocate_worst_fit
 *	Input type:		int
 * 	Output type:	void*
 * 	Description:	Allocates memory using Worst Fit from the free memory list
 */
void* allocate_worst_fit(int size) {
	void* worstBlock = NULL;
	int excessSize;
	int blockFound = 0;

	//	TODO: 	Allocate memory by using Worst Fit Policy
	//	Hint:	Start off with the freeListHead and iterate through the entire list to 
	//			get the largest block

	//	Checks if appropriate block is found.
	if (blockFound) {
		//	Allocates the Memory Block
		allocate_block(worstBlock, size, excessSize, 1);
	}
	else {
		//	Assigns invalid address if appropriate block not found in free list
		worstBlock = (void*)-2;
	}

	return worstBlock;
}

/*
 *	Funcation Name: allocate_next_fit
 *	Input type:		int
 * 	Output type:	void*
 * 	Description:	Allocates memory using Next Fit from the free memory list
 */
void* allocate_next_fit(int size) {
	void* nextBlock = NULL;
	int excessSize;
	int blockFound = 0;

	//	TODO: 	Allocate memory by using Next Fit Policy
	//	Hint:	You should use a global pointer to keep track of your last allocated memory address, and 
	//			allocate free blocks that come after that address (i.e. on top of it). Once you reach 
	//			Program Break, you start from the beginning of your heap, as in with the free block with
	//			the smallest address)

	//	Checks if appropriate found is found.
	if (blockFound) {
		//	Allocates the Memory Block
		allocate_block(nextBlock, size, excessSize, 1);
	}
	else {
		//	Assigns invalid address if appropriate block not found in free list
		nextBlock = (void*)-2;
	}

	return nextBlock;
}

/*
 *	Funcation Name: allocate_block
 *	Input type:		void*, int, int, int
 * 	Output type:	void
 * 	Description:	Performs routine operations for allocating a memory block
 */
void allocate_block(void* newBlock, int size, int excessLength, int fromFreeList) {
	void* excessFreeBlock; //	pointer for any excess free block
	int excessSize;

	// 	Checks if excess free size is big enough to be added to the free memory list
	if (excessLength > MIN_EXCESS_SIZE) {

		write_block(newBlock, 1, 0, 0, size);//write the inuse block to memory
		//	TODO: Create a free block using the excess memory size, then assign it to the Excess Free Block
		excessFreeBlock = newBlock + size + BLOCK_TAG_SIZE + BLOCK_TAG_SIZE + FREE_BLOCK_HEADER_SIZE; // points to data of free block
		excessSize = excessLength - FREE_OVERHEAD;//excessSize is length of whole excess block

		//	Checks if the new block was allocated from the free memory list
		if (fromFreeList) {
			//	Removes new block and adds the excess free block to the free list
			replace_block_freeList(newBlock, excessFreeBlock);
		}
		else {
			//	Adds excess free block to the free list
			add_block_freeList(excessFreeBlock, excessSize);
		}
	}
	else {
		write_block(newBlock, 1, 0, 0, size + excessLength);//write the inuse block to memory with excess size to it
		//	TODO: Add excessSize to size and assign it to the new Block

		//	Checks if the new block was allocated from the free memory list
		if (fromFreeList) {
			//	Removes the new block from the free list
			remove_block_freeList(newBlock);
		}
	}
}

/*
 *	Funcation Name: replace_block_freeList
 *	Input type:		void*, void*
 * 	Output type:	void
 * 	Description:	Replaces old block with the new block in the free list
 */
void replace_block_freeList(void* oldBlock, void* newBlock) {
	//	TODO: Replace the old block with the new block

	//	Updates SMA info
	totalAllocatedSize += (get_blockSize(oldBlock) - get_blockSize(newBlock));
	totalFreeSize += (get_blockSize(newBlock) - get_blockSize(oldBlock));
}

/*
 *	Funcation Name: add_block_freeList
 *	Input type:		void*
 * 	Output type:	void
 * 	Description:	Adds a memory block to the the free memory list
 */
void add_block_freeList(void* block, int size) {
	void* previous_block;
	void* next_block;
	int position_to_add_at;

	if (!freeListHead) {//empty free list
		write_block(block, 0, 0, 0, size);
		freeListHead = block;
	}
	else {
		position_to_add_at = find_position_in_free_list(block);
		if (position_to_add_at == 0) {//self added to head
			//replace the free list head by self

			next_block = freeListHead;//list head will become self's next
			PREVIOUS(freeListHead) = block;//self becomes previous of list head
			freeListHead = block; //self becomes list head

			write_block(block, 0, 0, next_block, size);
		}
		else {//self added after head
			int current_pos = 0;
			void* current_block = freeListHead;

			//find block that will become self's previous
			while (current_pos <= position_to_add_at) {
				if (current_pos == position_to_add_at - 1) {
					previous_block = current_block;//get self's previous block
					next_block = NEXT(current_block);//get self's next block
					NEXT(current_block) = block;//update previous block's next as self
					if (next_block != 0) {//if current_block is not the tail
						PREVIOUS(next_block) = block;//update next block's previous as self
					}
					write_block(block, 0, previous_block, next_block, size);
				}

				current_block = NEXT(current_block);
				current_pos++;
			}
		}
	}

	merge(PREVIOUS(block), block);
	merge(block, NEXT(block));


	/* Clean memory if top is free and larger than MAX_TOP_FREE */
	//if(!reallocating) { // dont clean memory if reallocating
	void* current_block = block;
	if (!NEXT(current_block)) {//check if current block is the last block
		if ((SIZE(current_block) + FREE_OVERHEAD) >= MAX_TOP_FREE) {//check if too big
			if (current_block == freeListHead) {
				freeListHead = 0;
			}
			else {
				NEXT(PREVIOUS(current_block)) = 0; //make previous block the last block
			}
			sbrk(-((int)((SIZE(current_block) + FREE_OVERHEAD) / 2)));//reduce free memory in half
			add_block_freeList(current_block, ((int)(MAX_TOP_FREE / 2) - FREE_OVERHEAD));//rewrite and add to the free list the block but with half size
		}
	}

	//	Updates SMA info
	totalAllocatedSize -= get_blockSize(block);
	totalFreeSize += get_blockSize(block);
}

/*
 *	Funcation Name: remove_block_freeList
 *	Input type:		void*
 * 	Output type:	void
 * 	Description:	Removes a memory block from the the free memory list
 */
void remove_block_freeList(void* block) {
	//	TODO: 	Remove the block from the free list
	//	Hint: 	You need to update the pointers in the free blocks before and after this block.
	//			You also need to remove any TAG in the free block.

	//	Updates SMA info
	totalAllocatedSize += get_blockSize(block);
	totalFreeSize -= get_blockSize(block);
}

/*
 *	Funcation Name: get_blockSize
 *	Input type:		void*
 * 	Output type:	int
 * 	Description:	Extracts the Block Size
 */
int get_blockSize(void* ptr) {
	return SIZE(ptr);
}

/*
 *	Funcation Name: get_largest_freeBlock
 *	Input type:		void
 * 	Output type:	int
 * 	Description:	Extracts the largest Block Size
 */
int get_largest_freeBlock() {
	int largestBlockSize = 0;
	void* current_block = freeListHead;

	while (current_block != 0) {
		if (SIZE(current_block) > largestBlockSize) {
			largestBlockSize = SIZE(current_block);//update if larger than current largest
		}
		current_block = NEXT(current_block);
	}
	return largestBlockSize;
}

/**
 * If the two blocks can be merged, they are merged into the bottom block
 */
void merge(void* bottom_block, void* top_block) {
	if (!bottom_block || !top_block) return;//if any is empty, merged
	if (TYPE(bottom_block) != 0 || TYPE(top_block) != 0) return;//if any isn't a FREE block
	if ((bottom_block + SIZE(bottom_block) + BLOCK_TAG_SIZE) != (top_block - FREE_BLOCK_HEADER_SIZE - BLOCK_TAG_SIZE))return; //if not contiguous memory

	void* previous_block = PREVIOUS(bottom_block);
	void* next_block = NEXT(top_block);
	int size = SIZE(bottom_block) + SIZE(top_block) + FREE_OVERHEAD;

	if (next_block) {//check if top_block is not last
		PREVIOUS(next_block) = bottom_block;
	}
	if (previous_block) {//check if bottom_block is not first
		NEXT(previous_block) = bottom_block;
	}
	write_block(bottom_block, 0, previous_block, next_block, size);

}

/*
 * Write the block informations in the header & tag.
 * For INUSE block, type = 1.
 * For FREE block, type = 0.
 * Only specify previous and next for FREE block.
 */
void write_block(void* block, int type, void* previous, void* next, int size) {
	if (type == 1) {//INUSE block
		*(char*)(block - INUSE_BLOCK_HEADER_SIZE - BLOCK_TAG_SIZE) = 1;//head tag
		*(int*)(block - sizeof(int)) = size;//size register
		*(char*)(block + size) = 1;//foot tag
		print_block(block);
	}
	else if (type == 0) {//FREE block
		*(char*)(block - FREE_BLOCK_HEADER_SIZE - BLOCK_TAG_SIZE) = 0;//head tag
		*(int*)(block - sizeof(int)) = size;//length register
		*(void**)(block - sizeof(int) - sizeof(void*)) = next;//next register//TOTEST
		*(void**)(block - sizeof(int) - sizeof(void*) - sizeof(void*)) = previous;//previous register//TOTEST
		*(char*)(block + size) = 0;//foot tag
		print_block(block);
	}
	memset(block, 0, SIZE(block));
}

void print_heap() {
	void* heap = sbrk(0);
	hex_dump(heap_start, (int)(heap - heap_start));
}

void print_block(void* block) {
	// return;

	if (TYPE(block) == 1) {//INUSE block
		if (SIZE(block) < 64) {
			puts("f");
			hex_dump(block - INUSE_BLOCK_HEADER_SIZE - BLOCK_TAG_SIZE, SIZE(block) + INUSE_OVERHEAD);
		}
		else {
			puts("t");
			hex_dump(block - INUSE_BLOCK_HEADER_SIZE - BLOCK_TAG_SIZE, INUSE_BLOCK_HEADER_SIZE);
		}
	}
	else if (TYPE(block) == 0) {//FREE block
		if (SIZE(block) < 64) {
			puts("f");
			hex_dump(block - FREE_BLOCK_HEADER_SIZE - BLOCK_TAG_SIZE, SIZE(block) + FREE_OVERHEAD);
		}
		else {
			puts("t");
			hex_dump(block - FREE_BLOCK_HEADER_SIZE - BLOCK_TAG_SIZE, FREE_BLOCK_HEADER_SIZE);
		}
	}
}

/*
 * Returns at which position the block should be inserted to preserve block ordering in the free list.
 */
int find_position_in_free_list(void* block) {
	int position = 0;
	void* current_block = freeListHead;
	//iterate through all the free list
	while (current_block != NULL) {
		if (current_block > block) {//when we pass in size, it mean we should be inserted there
			return position;
		}

		current_block = *(void**)(current_block - sizeof(int) - sizeof(void**));

		position++;
	}
	return position;
}

/**
 * Modified from https://gist.github.com/domnikl/af00cc154e3da1c5d965
 */
void hex_dump(void* addr, int len) {
	int i;
	unsigned char buff[17];
	unsigned char* pc = (unsigned char*)addr;
	char str[20];

	fputs("---\n", stdout);//indicate start

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