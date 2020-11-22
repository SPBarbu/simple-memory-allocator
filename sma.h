/*
 * =====================================================================================
 *
 *  Filename:  		sma.h
 *
 *  Description:	Header file for SMA.
 *
 *  Version:  		1.0
 *  Created:  		3/11/2020 9:30:00 AM
 *  Revised:  		-
 *  Compiler:  		gcc
 *
 *  Author:  		Mohammad Mushfiqur Rahman
 *
 *  Instructions:   Please address all the "TODO"s in the code below and modify them
 *                  accordingly. Refer to the Assignment Handout for further info.
 * =====================================================================================
 */

 /* Includes */
#include <string.h>
#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>
#include <math.h> //For ceiling function. Make sure to use -lm compiler option

//  Policies definition
#define WORST_FIT	1
#define NEXT_FIT	2

extern char* sma_malloc_error;

//  Public Functions declaration
void* sma_malloc(int size);
void sma_free(void* ptr);
void sma_mallopt(int policy);
void sma_mallinfo();
void* sma_realloc(void* ptr, int size);

//  Private Functions declaration
static void* allocate_pBrk(int size);
static void* allocate_freeList(int size);
static void* allocate_worst_fit(int size);
static void* allocate_next_fit(int size);
static void allocate_block(void* newBlock, int size, int excessSize, int fromFreeList);
static void replace_block_freeList(void* oldBlock, void* newBlock, int size);
static void add_block_freeList(void* block, int size);
static void remove_block_freeList(void* block);
static int get_blockSize(void* ptr);
static int get_largest_freeBlock();
static void write_block(void* block, int type, void* previous, void* next, int size);
static void hex_dump(void* addr, int len);
static void print_block(void* block);
static void merge(void* bottom_block, void* top_block);
static void print_heap();
static int find_position_in_free_list(void* block);
static void* get_free_block_top_heap();
static int clean_memory();
static void* find_next_free_block(void* block);
static void update_stats();