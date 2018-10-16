/*! \file
 * Declarations for a simple memory allocator.  The allocator manages a small
 * pool of memory, provides memory chunks on request, and reintegrates freed
 * memory back into the pool.
 *
 * Adapted from Andre DeHon's CS24 2004, 2006 material.
 * Copyright (C) California Institute of Technology, 2004-2009.
 * All rights reserved.
 */


/*! Specifies the size of the memory pool the allocator has to work with. */
extern int MEMORY_SIZE;
extern int counter;


/* Struct for doubly linked list that explicit free list is implemented as */
typedef struct node
{
    int space;
    struct node *next;
    struct node *prev;
} node;


/* ------------------------------------------------------------------- 
 * Allocator functions
 * ------------------------------------------------------------------- 
 */

/* Initializes allocator state, and memory pool state too. */
void init_myalloc();


/* Attempt to allocate a chunk of memory of "size" bytes. */
unsigned char * myalloc(int size);


/* Free a previously allocated pointer. */
void myfree(unsigned char *oldptr);


/* 
 * Reallocate data to a block of a different size, returns NULL if doesn't work
 * and leaves data unchanged. If it works, returns a pointer to the new
 * location of the data
 */
unsigned char *myrealloc(unsigned char *oldptr, int size);


/* Clean up the allocator and memory pool state. */
void close_myalloc();


/* ------------------------------------------------------------------- 
 * Helper functions
 * ------------------------------------------------------------------- 
 */

/* 
 * Sanity check -- Return the sum of allocated and free memory (infinite loop if
 * last block doesn't end where memory pool ends)
 */
int checkMem();


/*
 * Validity check for an address to myfree. Will return 0 for many invalid
 * addresses, will return 1 for (most likely) valid addresses
 */
int isValid(unsigned char *oldptr);


/*
 * Scans through the free list to find a suitable block using best-fit
 */
node *findHead(int size);


/*
 * Given a block address and a size to cut the block into,
 * will cut the block, set the header and footer tags, and
 * return a node pointer to the new block made
 */
node *splitBlock(node *headptr, int size);


/* ------------------------------------------------------------------- 
 * Node functions
 * ------------------------------------------------------------------- 
 */

/*
 * Removes a node in the free list (links the nodes before and
 * after together)
 */
void removeNode(node *badNode);


/* Adds a node to the beginning of the free list */
void addNode(node *newNode);


/* Coalesces two nodes and update free list */
void coalesce(node *headptrA, node *headptrB);

