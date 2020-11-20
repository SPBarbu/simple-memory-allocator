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

/* Definitions*/
#define MAX_TOP_FREE (128 * 1024) // Max top free block size = 128 Kbytes

#define FREE_BLOCK_HEADER_SIZE 2 * sizeof(char *) + sizeof(int) // Size of the Header in a free memory block
#define INUSE_BLOCK_HEADER_SIZE sizeof(int) // Size of the Header in a used memory block
#define BLOCK_TAG_SIZE sizeof(char) // Size of a INUSE/FREE tag. Expect to use 2 at the start and end of a block
#define INUSE_OVERHEAD (INUSE_BLOCK_HEADER_SIZE + 2 * BLOCK_TAG_SIZE) //Memory overhead for INUSE block
#define FREE_OVERHEAD (FREE_BLOCK_HEADER_SIZE + 2 * BLOCK_TAG_SIZE)//Memory overhead for FREE block
#define MIN_EXCESS_SIZE (INUSE_OVERHEAD + 1024) //Minimum excess size for split. 
#define PBRK_CHUNK_ALLOCATION (64 * 1024)//size of chunk pbrk allocates

typedef enum //	Policy type definition
{
	WORST,
	NEXT
} Policy;

char* sma_malloc_error;
void* freeListHead = NULL;			  //	The pointer to the HEAD of the doubly linked free memory list
void* freeListTail = NULL;			  //	The pointer to the TAIL of the doubly linked free memory list
unsigned long totalAllocatedSize = 0; //	Total Allocated memory in Bytes
unsigned long totalFreeSize = 0;	  //	Total Free memory in Bytes in the free memory list
Policy currentPolicy = WORST;		  //	Current Policy

/*
 * =====================================================================================
 *	Public Functions for SMA
 * =====================================================================================
 */

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
		add_block_freeList(ptr);
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
	sprintf(str, "Total number of bytes allocated: %lu", totalAllocatedSize);
	puts(str);
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
	int excessSize;

	int minimum_INUSE_size = size + INUSE_OVERHEAD;
	int number_chunks_to_allocate = (int)ceil((double)minimum_INUSE_size / PBRK_CHUNK_ALLOCATION);
	int allocate_size = number_chunks_to_allocate * PBRK_CHUNK_ALLOCATION;

	newBlock = sbrk(allocate_size); //get previous pbrk location and allocation chunks of memory
	newBlock += BLOCK_TAG_SIZE + INUSE_BLOCK_HEADER_SIZE;//points to begining of data

	excessSize = allocate_size - (size + INUSE_OVERHEAD);

	//	Allocates the Memory Block
	allocate_block(newBlock, size, excessSize, 0);

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
void allocate_block(void* newBlock, int size, int excessSize, int fromFreeList) {
	void* excessFreeBlock; //	pointer for any excess free block

	// 	Checks if excess free size is big enough to be added to the free memory list
	if (excessSize > MIN_EXCESS_SIZE) {
		write_block(newBlock, 1, (void*)0, (void*)0, size);//write the inuse block to memory
		//	TODO: Create a free block using the excess memory size, then assign it to the Excess Free Block
		excessFreeBlock = newBlock + size + BLOCK_TAG_SIZE + BLOCK_TAG_SIZE + FREE_OVERHEAD; // points to data of free block
		int position = find_position_in_free_list(excessFreeBlock, size + FREE_OVERHEAD);

		//	Checks if the new block was allocated from the free memory list
		if (fromFreeList) {
			//	Removes new block and adds the excess free block to the free list
			replace_block_freeList(newBlock, excessFreeBlock);
		}
		else {
			//	Adds excess free block to the free list
			add_block_freeList(excessFreeBlock);
		}
	}
	else {
		write_block(newBlock, 1, (void*)0, (void*)0, size + excessSize);//write the inuse block to memory with excess size to it
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
void add_block_freeList(void* block) {
	//	TODO: 	Add the block to the free list
	//	Hint: 	You could add the free block at the end of the list, but need to check if there
	//			exits a list. You need to add the TAG to the list.
	//			Also, you would need to check if merging with the "adjacent" blocks is possible or not.
	//			Merging would be tideous. Check adjacent blocks, then also check if the merged
	//			block is at the top and is bigger than the largest free block allowed (128kB).

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
	int* pSize;

	//	Points to the address where the Length of the block is stored
	pSize = (int*)ptr;
	pSize--;

	//	Returns the deferenced size
	return *(int*)pSize;
}

/*
 *	Funcation Name: get_largest_freeBlock
 *	Input type:		void
 * 	Output type:	int
 * 	Description:	Extracts the largest Block Size
 */
int get_largest_freeBlock() {
	int largestBlockSize = 0;

	//	TODO: Iterate through the Free Block List to find the largest free block and return its size

	return largestBlockSize;
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
		*(int*)(block - sizeof(int)) = size + INUSE_OVERHEAD;//length register
		*(char*)(block + size) = 1;//foot tag
	}
	else if (type == 0) {//FREE block
		*(char*)(block - INUSE_BLOCK_HEADER_SIZE - BLOCK_TAG_SIZE) = 1;//head tag
		*(int*)(block - sizeof(int)) = size + INUSE_OVERHEAD;//length register
		*(void**)(block - sizeof(int) - sizeof(void*)) = next;//next register//TOTEST
		*(void**)(block - sizeof(int) - sizeof(void*) - sizeof(void*)) = previous;//previous register//TOTEST
		*(char*)(block + size) = 1;//foot tag
	}

}

int find_position_in_free_list(void* block, int length) {
	int position = 0;
	void* current_block = freeListHead;
	while (current_block != NULL) {
		if (current_block > block) {
			return position;
		}

		current_block = (void*)(*(((char*)current_block) - (sizeof(int) + sizeof(void*))));//TOTEST

		position++;
	}
	return position;
}

/**
 * Modified from https://gist.github.com/domnikl/af00cc154e3da1c5d965
 */
void hexDump(void* addr, int len) {
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