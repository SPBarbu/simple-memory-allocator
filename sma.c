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
#define MIN_EXCESS_SIZE (1024 - INUSE_OVERHEAD) //Minimum excess size for split. 
#define PBRK_CHUNK_ALLOCATION (16 * 1024)//size of chunk pbrk allocates

typedef enum //	Policy type definition
{
	WORST,
	NEXT
} Policy;

char* sma_malloc_error;
void* freeListHead = NULL;			  //	The pointer to the HEAD of the doubly linked free memory list
void* last_allocated_block = 0; //memory that was last allocated to the user

unsigned long total_bytes_allocated_data = 0;
unsigned long total_bytes_allocated_overhead = 0;
unsigned long total_bytes_free_includes_overhead = 0;

Policy currentPolicy = WORST;		  //	Current Policy
int reallocating = 0;	//indicator for memory cleaning when realloc called
void* heap_start = 0;
int requesting = 0;//indicator for memory cleaning when pbrk expending

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
		last_allocated_block = pMemory;
	}
	// Validates memory allocation
	if (pMemory < 0 || pMemory == NULL) {
		sma_malloc_error = "Error: Memory allocation failed!";
		return NULL;
	}

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
		if (ptr == last_allocated_block)last_allocated_block = 0;//resets if freeing the last allocated memory
		//	Adds the block to the free memory list
		//overhead for free list is bigger so need to change properties
		print_block(ptr);
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
		if (currentPolicy == 1)last_allocated_block = 0;//reset where to start looking when changing policy
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
	char str[100];

	update_stats();

	sprintf(str, "Total number of bytes of data allocated not including overhead: %lu", total_bytes_allocated_data);
	puts(str);
	sprintf(str, "Total number of bytes of data allocated including overhead: %lu", total_bytes_allocated_data + total_bytes_allocated_overhead);
	puts(str);
	sprintf(str, "Total number of bytes of free space: %lu", total_bytes_free_includes_overhead);
	puts(str);
	sprintf(str, "Size of the largest contigious free space (in bytes): %d", largestFreeBlock);
	puts(str);
	sprintf(str, "Size of the block at the top of the heap, if any: %d", SIZE(get_free_block_top_heap()));
	puts(str);
	sprintf(str, "Number of free holes on the heap: %d", clean_memory());
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
	reallocating = 1;
	sma_free(ptr);
	ptr = sma_malloc(size);
	reallocating = 0;
	if (ptr > 0) {//check if successfully malloced
		for (int i = 0; i < sizeof(data); i++) {
			*(char*)(ptr + i) = data[i];//copy data
		}
	}
	return ptr;


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
	if (!heap_start) heap_start = sbrk(0);
	void* free_block_top_heap = get_free_block_top_heap();
	int length_free_block_top_head = (free_block_top_heap ? SIZE(free_block_top_heap) + FREE_OVERHEAD : 0);//if there is a free block at the top of the heap, get its length
	int for_request_size = size + (FREE_OVERHEAD - INUSE_OVERHEAD);
	int number_chunks_to_fill_request = (int)(ceil((double)for_request_size / PBRK_CHUNK_ALLOCATION));
	int length_chunks_to_fill_request = number_chunks_to_fill_request * PBRK_CHUNK_ALLOCATION;
	int extra_length_need_to_fill_request = length_chunks_to_fill_request - length_free_block_top_head;

	void* new_block_insert_at;
	int excess_length;

	if (free_block_top_heap) {
		sbrk(extra_length_need_to_fill_request);
		void* excess_block_start = free_block_top_heap + SIZE(free_block_top_heap) + BLOCK_TAG_SIZE + BLOCK_TAG_SIZE + FREE_BLOCK_HEADER_SIZE;
		int excess_block_size = extra_length_need_to_fill_request - FREE_OVERHEAD;
		requesting = 1;
		add_block_freeList(excess_block_start, excess_block_size);
		requesting = 0;

		new_block_insert_at = free_block_top_heap - (FREE_OVERHEAD - INUSE_OVERHEAD);
		excess_length = SIZE(free_block_top_heap) /* has new size b/c it merged*/ + FREE_OVERHEAD - for_request_size - INUSE_OVERHEAD;
		allocate_block(new_block_insert_at, for_request_size, excess_length, 1);

	}
	else {
		new_block_insert_at = sbrk(extra_length_need_to_fill_request) + BLOCK_TAG_SIZE + INUSE_BLOCK_HEADER_SIZE;
		excess_length = extra_length_need_to_fill_request - for_request_size - INUSE_OVERHEAD;
		allocate_block(new_block_insert_at, for_request_size, excess_length, 0);

	}

	// return newBlock;
	return new_block_insert_at;
}

void* get_free_block_top_heap() {
	void* current_block = freeListHead;
	//iterate through all the blocks in the free list
	while (current_block) {
		if (!NEXT(current_block))break;//dont increment once found
		current_block = NEXT(current_block);
	}
	if (current_block && (current_block + SIZE(current_block) + BLOCK_TAG_SIZE) == sbrk(0)) {//check theres no INUSE memory after that free block
		return current_block;
	}
	return 0;
}

/*
 *	Funcation Name: allocate_freeList
 *	Input type:		int
 * 	Output type:	void*
 * 	Description:	Allocates memory from the free memory list
 */
void* allocate_freeList(int size) {
	void* pMemory = NULL;
	size += (FREE_OVERHEAD - INUSE_OVERHEAD);//insures it can be turn into a FREE block after it's freed

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
	int excessLength = 0;
	void* current_block = freeListHead; //start iteration by the head
	void* new_block_insert_at = 0;

	while (current_block) {//check if theres a next block
		if (SIZE(current_block) + FREE_OVERHEAD >= size) {//check if big enough
			if (!worstBlock || SIZE(current_block) + FREE_OVERHEAD > SIZE(worstBlock) + FREE_OVERHEAD) {//valid if first big enough or bigger than the previous worst
				worstBlock = current_block;
			}

		}
		current_block = NEXT(current_block);
	}

	//	Checks if appropriate block is found.
	if (worstBlock) {
		new_block_insert_at = worstBlock - (FREE_OVERHEAD - INUSE_OVERHEAD);
		//assumes allocate_freeList already took care of adding space for the free block header so size of new block is size
		excessLength = SIZE(worstBlock) + FREE_OVERHEAD - size - INUSE_OVERHEAD;

		//	Allocates the Memory Block
		allocate_block(new_block_insert_at, size, excessLength, 1);
	}
	else {
		//	Assigns invalid address if appropriate block not found in free list
		new_block_insert_at = (void*)-2;
	}

	return new_block_insert_at;
}

/*
 *	Funcation Name: allocate_next_fit
 *	Input type:		int
 * 	Output type:	void*
 * 	Description:	Allocates memory using Next Fit from the free memory list
 */
void* allocate_next_fit(int size) {
	void* new_block_insert_at = 0;
	int excess_length = 0;
	void* found_free_spot = 0;
	void* current_block = 0;
	void* next_free_block = 0;

	if (!last_allocated_block) {//check if last allocated block is empty
		next_free_block = freeListHead;
	}
	else {
		current_block = last_allocated_block;
		next_free_block = find_next_free_block(current_block);
	}


	if (next_free_block == 0) {
		new_block_insert_at = 0;
	}
	else {//if found a free memory block, so search through list
		current_block = next_free_block;
		if (current_block >= last_allocated_block) {
			while (current_block) {//search up to the end of the free list
				if (SIZE(current_block) + FREE_OVERHEAD - INUSE_OVERHEAD >= size) {
					found_free_spot = current_block;
					break;
				}
				current_block = NEXT(current_block);
			}
			if (freeListHead <= last_allocated_block && !found_free_spot) {
				current_block = freeListHead;
				while (current_block) {
					if (SIZE(current_block) + FREE_OVERHEAD > size + INUSE_BLOCK_HEADER_SIZE) {
						found_free_spot = current_block;
						break;
					}
					current_block = NEXT(current_block);
				}
			}
		}
		else {
			if (freeListHead <= last_allocated_block) {
				current_block = freeListHead;
				while (current_block) {
					if (SIZE(current_block) + FREE_OVERHEAD > size + INUSE_BLOCK_HEADER_SIZE) {
						found_free_spot = current_block;
						break;
					}
					current_block = NEXT(current_block);
				}
			}
		}
	}



	//	Checks if appropriate found is found.
	if (found_free_spot) {
		new_block_insert_at = found_free_spot - (FREE_OVERHEAD - INUSE_OVERHEAD);
		excess_length = SIZE(found_free_spot) + FREE_OVERHEAD - size - INUSE_OVERHEAD;
		//	Allocates the Memory Block
		allocate_block(new_block_insert_at, size, excess_length, 1);
	}
	else {
		//	Assigns invalid address if appropriate block not found in free list
		new_block_insert_at = (void*)-2;
	}

	return new_block_insert_at;
}

void* find_next_free_block(void* block) {
	void* free_block_found = 0;
	int reached_pbrk = 0;
	void* current_block = block;
	while (current_block) {
		if (current_block + SIZE(current_block) + BLOCK_TAG_SIZE == sbrk(0)) {//check havent reached top of the heap
			reached_pbrk = 1;
			break;
		}
		else {
			if (*(char*)(current_block + SIZE(current_block) + BLOCK_TAG_SIZE) == 2) {//0 indicates free block so found it
				free_block_found = current_block + SIZE(current_block) + BLOCK_TAG_SIZE + BLOCK_TAG_SIZE + FREE_BLOCK_HEADER_SIZE;
				break;
			}
			else {//this block isnt free, so continue
				current_block = current_block + SIZE(current_block) + BLOCK_TAG_SIZE + BLOCK_TAG_SIZE + INUSE_BLOCK_HEADER_SIZE;
			}
		}
	}
	if (reached_pbrk) {
		if (freeListHead < last_allocated_block) {//theres a free block before the last allocated block, return that
			free_block_found = freeListHead;
		}
		else {
			current_block = heap_start + BLOCK_TAG_SIZE + INUSE_BLOCK_HEADER_SIZE;
			while (current_block) {// continue searching up to the last allocated size
				if (current_block >= last_allocated_block) {//stop searching once searched up to start of the search
					break;
				}
				else {
					if (*(char*)(current_block + SIZE(current_block) + BLOCK_TAG_SIZE) == 2) {//0 indicates free block so found it
						free_block_found = current_block + SIZE(current_block) + BLOCK_TAG_SIZE + BLOCK_TAG_SIZE + FREE_BLOCK_HEADER_SIZE;
						break;
					}
					else {//this block isnt free, so continue
						current_block = current_block + SIZE(current_block) + BLOCK_TAG_SIZE + BLOCK_TAG_SIZE + INUSE_BLOCK_HEADER_SIZE;
					}
				}
			}
		}
	}
	return free_block_found;
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
	void* old_free_position = newBlock + (FREE_OVERHEAD - INUSE_OVERHEAD);

	// 	Checks if excess free size is big enough to be added to the free memory list
	if (excessLength > MIN_EXCESS_SIZE) {

		excessFreeBlock = newBlock + size + BLOCK_TAG_SIZE + BLOCK_TAG_SIZE + FREE_BLOCK_HEADER_SIZE; // points to data of free block
		excessSize = excessLength - FREE_OVERHEAD;//excessSize is length of whole excess block

		//	Checks if the new block was allocated from the free memory list
		if (fromFreeList) {
			//	Removes new block and adds the excess free block to the free list
			replace_block_freeList(old_free_position, excessFreeBlock, excessSize);
			write_block(newBlock, 1, 0, 0, size);//write the inuse block to memory
		}
		else {
			write_block(newBlock, 1, 0, 0, size);//write the inuse block to memory
			//	Adds excess free block to the free list
			add_block_freeList(excessFreeBlock, excessSize);
		}
	}
	else {
		//	Checks if the new block was allocated from the free memory list
		if (fromFreeList) {
			//	Removes the new block from the free list
			remove_block_freeList(old_free_position);
		}
		write_block(newBlock, 1, 0, 0, size + excessLength);//write the inuse block to memory with excess size to it
	}
}

/*
 *	Funcation Name: replace_block_freeList
 *	Input type:		void*, void*
 * 	Output type:	void
 * 	Description:	Updates references of neighbours to oldBlock to newBlock and rewrite to new position
 */
void replace_block_freeList(void* oldBlock, void* newBlock, int size) {

	void* previous_block = PREVIOUS(oldBlock);
	void* next_block = NEXT(oldBlock);

	if (previous_block) {
		NEXT(previous_block) = newBlock;//update self reference in previous
	}
	if (next_block) {
		PREVIOUS(next_block) = newBlock;//update self reference in next
	}
	if (freeListHead == oldBlock) {
		freeListHead = newBlock;
	}
	write_block(newBlock, 0, previous_block, next_block, size);

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
			while (current_pos < position_to_add_at) {
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

	merge(block, NEXT(block));
	merge(PREVIOUS(block), block);

	//cant reuse block variable after merging
	clean_memory();

}

/**
 * Reduces the size of the free block at the top of the heap if too large
 * For debugging purposes and testing merging ajdacent free blocks, returns the number of free holes on the heap.
 *
 */
int clean_memory() {
	/* Clean memory if top is free and larger than MAX_TOP_FREE */
	void* current_block = freeListHead;
	int number_of_blocks = 0;//just for debugging purposes
	if (!requesting && !reallocating) {//to avoid cleaning memory when some chunk of it is required at some other place
		while (current_block) {
			if (!NEXT(current_block)) {//check if current block is the last block
				if ((SIZE(current_block) + FREE_OVERHEAD) >= MAX_TOP_FREE) {//check if too big
					if (current_block == freeListHead) {
						freeListHead = 0;
					}
					else {
						NEXT(PREVIOUS(current_block)) = 0; //make previous block the last block
					}
					sbrk(-((int)((SIZE(current_block) + FREE_OVERHEAD) / 2)));
					add_block_freeList(current_block, SIZE(current_block) - ((int)((SIZE(current_block) + FREE_OVERHEAD) / 2)));
				}
			}
			current_block = NEXT(current_block);
			number_of_blocks++;
		}
	}
	return number_of_blocks;
}

/*
 *	Funcation Name: remove_block_freeList
 *	Input type:		void*
 * 	Output type:	void
 * 	Description:	Removes a memory block from the the free memory list
 */
void remove_block_freeList(void* block) {
	if (block && NEXT(block)) {
		PREVIOUS(NEXT(block)) = PREVIOUS(block);// update our next's previous to our previous
		if (block == freeListHead) {
			freeListHead = NEXT(block);
		}
	}
	if (block && PREVIOUS(block)) {
		NEXT(PREVIOUS(block)) = NEXT(block);//update our previous' next to our next
	}
	if (block == freeListHead) {
		freeListHead = 0;
	}
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
	if (TYPE(bottom_block) != 2 || TYPE(top_block) != 2) return;//if any isn't a FREE block
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
		// memset(block, 0, SIZE(block));
		print_block(block);
	}
	else if (type == 0) {//FREE block
		*(char*)(block - FREE_BLOCK_HEADER_SIZE - BLOCK_TAG_SIZE) = 2;//head tag
		*(int*)(block - sizeof(int)) = size;//length register
		*(void**)(block - sizeof(int) - sizeof(void*)) = next;//next register//TOTEST
		*(void**)(block - sizeof(int) - sizeof(void*) - sizeof(void*)) = previous;//previous register//TOTEST
		*(char*)(block + size) = 2;//foot tag
		// memset(block, 0, SIZE(block));
		print_block(block);
	}

}

void print_heap() {
	void* heap = sbrk(0);
	hex_dump(heap_start, (int)(heap - heap_start));
}

void print_block(void* block) {
	return;

	if (TYPE(block) == 1) {//INUSE block
		if (SIZE(block) < 256) {
			puts("f");
			hex_dump(block - INUSE_BLOCK_HEADER_SIZE - BLOCK_TAG_SIZE, SIZE(block) + INUSE_OVERHEAD);
		}
		else {
			puts("t");
			hex_dump(block - INUSE_BLOCK_HEADER_SIZE - BLOCK_TAG_SIZE, INUSE_BLOCK_HEADER_SIZE);
		}
	}
	else if (TYPE(block) == 2) {//FREE block
		if (SIZE(block) < 256) {
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
 * Dumps memory, for debugging purposes
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
/*
 * Traverse every single block, INUSE and FREE to compute the stats
 *
 */
void update_stats() {
	void* current_block;
	total_bytes_allocated_data = 0;
	total_bytes_allocated_overhead = 0;
	total_bytes_free_includes_overhead = 0;

	if (heap_start) {//make sure dont call get_stats before allocating something
		if (*(char*)heap_start == 1) {//first block is inuse
			current_block = heap_start + BLOCK_TAG_SIZE + INUSE_BLOCK_HEADER_SIZE;
		}
		else {//first block is free
			current_block = heap_start + BLOCK_TAG_SIZE + FREE_BLOCK_HEADER_SIZE;
		}
	}
	else {
		return;
	}


	while (current_block) {
		/*compute stats for current block*/
		if (TYPE(current_block) == 1) {//INUSE block
			total_bytes_allocated_data += SIZE(current_block);
			total_bytes_allocated_overhead += INUSE_OVERHEAD;
		}
		else {
			total_bytes_free_includes_overhead += SIZE(current_block) + FREE_OVERHEAD;
		}

		/*got to the next block*/
		if (current_block + SIZE(current_block) + BLOCK_TAG_SIZE == sbrk(0)) {//we've computed all the blocks
			break;
		}
		else {
			if (*(char*)(current_block + SIZE(current_block) + BLOCK_TAG_SIZE) == 1) { // next block is an INUSE
				current_block += SIZE(current_block) + BLOCK_TAG_SIZE + BLOCK_TAG_SIZE + INUSE_BLOCK_HEADER_SIZE;
			}
			else {//next block is a free
				current_block += SIZE(current_block) + BLOCK_TAG_SIZE + BLOCK_TAG_SIZE + FREE_BLOCK_HEADER_SIZE;
			}
		}

	}
}