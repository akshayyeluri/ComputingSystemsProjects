/*! \file
 * Implementation of a simple memory allocator.  The allocator manages a small
 * pool of memory, provides memory chunks on request, and reintegrates freed
 * memory back into the pool.
 *
 * Adapted from Andre DeHon's CS24 2004, 2006 material.
 * Copyright (C) California Institute of Technology, 2004-2010.
 * All rights reserved.
 */


/*! 
 * README FIRST
 * ------------------------------------------------------------ 
 * Important details about representation of memory blocks, the data structures
 * used in implementing this allocator, and the meaning of various commonly
 * used variables 
 * ------------------------------------------------------------ 
 * Memory block representation:
 *      Free blocks: The metadata for free blocks includes a node struct
 *      (see myalloc.h), which is the "header", as well as a simple int
 *      which is the "footer". The "header" node struct has 3 fields,
 *      which are:
 *          a) space: a positive integer that is the amount of usable bytes
 *              in the block (the payload size), and is also the distance from
 *              the int tag in the header to the footer. *NOTE* -- this
 *              is not the distance from the end of the header struct to
 *              the footer, it is actually the distance from the int tag
 *              in the header (offset of 4 bytes from header start) to footer.
 *          b) prev: This is a pointer to another header struct (a node *).
 *              The allocator has an explicit free list, implemented as a 
 *              doubly linked list. This field links each free block to the
 *              previous free block. Note that the first block in the free list
 *              would have NULL for this field
 *          c) next: Again, pointer to another header to implement the free
 *              list. Note that the last block in the free list would have NULL.
 *      The "footer", which is just an int, has the same value as the space 
 *      field of the header struct. Thus, there is an int tag on both ends
 *      of free blocks giving the amount of bytes between the two int tags.
 * 
 *      Allocated blocks: The metadata for allocated blocks is more limited
 *      -- there is just an int tag on both end, no header struct. This tag
 *      is very similar to the tags in free blocks, except it is negative
 *      , to denote the memory is in use. Thus, it is the negation of the space
 *      field in the header for the freed version of the same block.      
 *  
 * Commonly used variables:
 *      freeList -- a global node * (static memory) that points to the header
 *          for the first free block in the explicit free list.
 *      dataptr -- an unsigned char * that is always used to be the exact 
 *          address of the start of a block
 *      headptr -- a node * that has the same value as dataptr, 
 *          (meaning it points to the start of a block), but can be used to
 *          do linked list operations/look at fields of the header for a free
 *          block.
 *      footptr -- an int * that points to the footer of a block (the int tag)
 *      space -- an int that is always the size of the payload of a block.
 *          In other words, it is the value saved in headptr->space, as well
 *          as footptr, for a free block, and it is the negation of the value
 *          in (int *) dataptr, as well as footptr, for an allocated block. 
 *          Thus, to find a footer address from a dataptr, one can say
 *          dataptr + sizeof(int) + space, and to find the next block's address
 *          (a new dataptr), one can say dataptr + space + 2 * sizeof(int)
 *
 * Implementation features:
 *      -- explicit free list
 *      -- constant time deallocation
 *      -- best fit instead of next-fit or first-fit
 *      -- realloc function, fun stuff XD, made it on a whim.
 *
 * Things minimizing fragmentation:
 *      -- best fit ensures that smallest block that can accomodate a request
 *         is used
 *      -- both forward and reverse coalescing means contiguous freed blocks
 *         are combined
 * 
 *
 * ------------------------------------------------------------ 
 * ------------------------------------------------------------ 
 */


#include <stdio.h>
#include <stdlib.h>
#include <assert.h>

#include "myalloc.h"
#define MAX(X, Y) (((X) > (Y)) ? (X) : (Y)) /* used in myalloc */

/*!
 * These variables are used to specify the size and address of the memory pool
 * that the simple allocator works against.  The memory pool is allocated within
 * init_myalloc(), and then myalloc() and free() work against this pool of
 * memory that mem points to.
 */
int MEMORY_SIZE;
unsigned char *mem;
node *freeList; /* Pointer to start of explicit free list */



/* ------------------------------------------------------------------- 
 * ------------------------------------------------------------------- 
 * Allocator functions (larger functions that handle high level
 * allocator operations)
 * ------------------------------------------------------------------- 
 * ------------------------------------------------------------------- 
 */


/*!
 * This function initializes both the allocator state, and the memory pool.  It
 * must be called before myalloc() or myfree() will work at all.
 */
void init_myalloc() 
{
    /*
     * Allocate the entire memory pool, from which our simple allocator will
     * serve allocation requests.
     */
    mem = (unsigned char *) malloc(MEMORY_SIZE);
    if (mem == 0) 
    {
        fprintf(stderr, "init_myalloc: could not get %d bytes from the" \
                                                     " system\n", MEMORY_SIZE);
        abort();
    }

    freeList = NULL; /* No blocks in free list */
    
    /*
     * entire memory is one giant block, whose header freeList points to.
     */
    node *headptr = (node *) mem;
    int space = MEMORY_SIZE - 2 * sizeof(int); /* subtract int tags on end */
    headptr->space = space;
    addNode(headptr);
    int *footptr = (int *) ((unsigned char *) (headptr) + sizeof(int) + space);
    *footptr = space;
}


/*!
 * Attempt to allocate a chunk of memory of "size" bytes.  Return NULL if
 * allocation fails. See findHead for time complexity analysis.
 */
unsigned char *myalloc(int size) 
{
    /*
     * find a suitable block for the allocation request, and if not found, 
     * return NULL
     */
    node *headptr = findHead(size); 
    if (headptr == NULL)
    {
        fprintf(stderr, "myalloc: cannot service request of size %d\n", size);
        return NULL;
    }
    int space = headptr->space;

    /*
     * have to allocate atleast enough memory to fit the whole header and 
     * footer when the block is freed. Thus, cannot make block less than
     * sizeof(node) + sizeof(int) size, meaning space must be at least
     * sizeof(node) + sizeof(int) - 2 * sizeof(int) size 
     */
    size = MAX(size, sizeof(node) - sizeof(int)); 
    
    /* 
     * Either swap the old block in the free list for a smaller split-off,
     * if the old block is big enough to split, or remove the old block 
     * altogether.
     */
    if (space > size + sizeof(int) + sizeof(node))
    {
        node *newHeadptr = splitBlock(headptr, size);
        addNode(newHeadptr);
    }
    removeNode(headptr);
    
    /*
     * Now that a block has been found, split, and the free list is up to date,
     * have to mark the found block (which has just been removed from free list)
     * as allocated, and return a pointer to the address of the payload (offset
     * sizeof(int) from beginning of block)
     */
    space = headptr->space;
    unsigned char *resultptr = (unsigned char *) (headptr) + sizeof(int);
    int *footptr = (int *) (resultptr + space);
    headptr->space = -space;
    *footptr = -space;
    assert(checkMem() == MEMORY_SIZE); 
    return resultptr;
}


/*!
 * Free a previously allocated pointer.  oldptr should be an address returned by
 * myalloc().
 *
 * Time complexity of deallocation/block coalescing: constant time
 * ------------------------------------------------------------ 
 * Operations all involve primarily pointer arithmetic, and none involve
 * iterating over each block. To simply update int tags to be positive requires
 * trivial pointer arithmetic, and the addNode function is constant time since 
 * the node is added to the beginning of free list, so no iteration through
 * the free list is required. The coalesce backward logic involves
 * just using the footer of the previous block to find the block head, so
 * no iteration through blocks is required, while the coalesce forward
 * logic is also pointer arithmetic. The lack of iteration through the
 * list means that both versions of coalescing are constant time, since
 * the coalesce function itself is constant time (also just pointer arithmetic).
 * Hence, myfree has a fixed number of stages, all that occur in constant time, 
 * and so, is O(1) with respect to number of blocks in the memory pool overall. 
 */
void myfree(unsigned char *oldptr) 
{
    if (isValid(oldptr) == 0)
    {
        fprintf(stderr, "Cannot free invalid address %p\n", (void *) oldptr);
        abort();
    }
    
    /* Some basic values and addresses */
    unsigned char *dataptr = oldptr - sizeof(int);
    node *headptr = (node *) dataptr;
    int space = -headptr->space;
    int *footptr = (int *) (oldptr + space);
    
    /*
     * make int tags positive to show block is free, and add new free block
     * to free list
     */
    headptr->space = space;
    *footptr = space;
    addNode(headptr);

    /* Coealesce backward logic. */
    if (dataptr != mem) /* make sure it is not first block */
    {
        int *prevFootptr = (int *) (dataptr) - 1;
        int prevSpace = *prevFootptr;
        if (prevSpace > 0) /* if the previous block is also free, coalesce */
        {
            node *prevHeadptr = (node *) (dataptr - prevSpace 
                                                  - 2 * sizeof(int));
            coalesce(prevHeadptr, headptr);
            /*
             * reset the variables to refer to new, coalesced block before
             * checking for forward coalescing
             */
            headptr = prevHeadptr;
            dataptr = (unsigned char *) headptr;
            space = headptr->space;
        }
    }

    /* Coalesce forward logic */
    unsigned char *endptr = dataptr + space + 2 * sizeof(int);
    if (endptr != mem + MEMORY_SIZE) /* if block is not the last block */
    {
        node *nextHeadptr = (node *) endptr;
        if (nextHeadptr->space > 0) /* if the next block is also free */
        {
            coalesce(headptr, nextHeadptr);
        }
    }
    assert(checkMem() == MEMORY_SIZE); 
}

     
/*!
 * This is a cool function similar to realloc that, given a pointer previously
 * returned by myalloc, as well as a new size, will try to shift all the
 * data in the old location to a new location of the specified new size.
 * If it works, a pointer to the new payload will be returned. If not, 
 * the old data will remain unaffected, and NULL will be returned.
 */
unsigned char *myrealloc(unsigned char *oldptr, int size)
{
    /*
     * Save some addresses and values from the old location.
     */
    unsigned char *oldDataptr = oldptr - sizeof(int);
    node *oldHeadptr = (node *) oldDataptr;
    int oldSpace = -oldHeadptr->space;
    int *oldFootptr = (int *) (oldptr + oldSpace);
    unsigned char *endptr = (unsigned char *) (oldFootptr + 1);
    
    /*
     * Only the data within the size of the node struct is modified by
     * free'ing (have to put in new addresses), so here we abuse notation
     * to save the data stored in what will become the next and prev fields
     * when oldptr is freed. The rest of the data in the oldptr location is
     * untouched.
     */
    node *tempA = oldHeadptr->next;
    node *tempB = oldHeadptr->prev; 

    /*
     * These are pointers to the headers for the previous and next blocks
     * in memory. They will remain none unless there is in fact a free block
     * contiguous with the old data.
     */
    node *prevHeadptr = NULL;
    node *nextHeadptr = NULL;
    int prevSpace = 0;

    /*
     * This block updates prevHeadptr to point to the previous block iff
     * the previous block is free. Thus, after this block, if prevHeadptr
     * is not NULL, then myfree will coallesce the old block with this 
     * previous block.
     */
    if (oldDataptr != mem)
    {
        int *prevFootptr = (int *) (oldDataptr) - 1;
        prevSpace = *prevFootptr;
        if (prevSpace > 0)
        {
            prevHeadptr = (node *) (oldDataptr - prevSpace - 2 * sizeof(int));
        }
    } 

    /*
     * Same thing as previous block, after this block, if nextHeadptr
     * is not NULL, then the old block will coalesce with the block
     * after it.
     */
    if (endptr != mem + MEMORY_SIZE)
    {
        int nextSpace = *((int *) endptr);
        if (nextSpace > 0)
        {
            nextHeadptr = (node *) endptr;
        }
    }

    /* We free the old block (all important/modifiable data has been saved) */
    myfree(oldptr);
    /* Then, we find the new best free block for our purpse */
    unsigned char *newptr = myalloc(size);
    
    /* 
     * This large block handles the case where reallocating is not possible.
     * It will uncoalesce any coalesced blocks, update the freeList to be
     * in the state before myfree was called, and put back all old data
     * before returning NULL.
     */
    if (newptr == NULL)
    {
        /* forward and back coalescing */
        if (prevHeadptr != NULL && nextHeadptr != NULL) 
        {
            splitBlock(prevHeadptr, prevSpace);
            splitBlock(oldHeadptr, oldSpace);
            addNode(nextHeadptr);
        }
        else if (prevHeadptr != NULL) /* just back coalescing */
        {
            splitBlock(prevHeadptr, prevSpace);
        }
        else if (nextHeadptr != NULL) /* just forward */
        {
            splitBlock(oldHeadptr, oldSpace);
            addNode(nextHeadptr);
            removeNode(oldHeadptr);
        }
        else /* no coalescing */
        {
            removeNode(oldHeadptr);
        }
        
        /* Restore all data, make int tags negative again */
        oldHeadptr->space = -oldSpace;
        oldHeadptr->next = tempA;
        oldHeadptr->prev = tempB;
        *oldFootptr = -oldSpace;
        return NULL;
    }

    /* Some addresses/values from the new place */
    unsigned char *newDataptr = newptr - sizeof(int);
    node *newHeadptr = (node *) newDataptr;
    int nSpace = -newHeadptr->space;

    /* 
     * This copies the initial data from the old location to the new.
     * In particular, the "padding" after the int tag in the header
     * struct of the old location is not modified in the freeing process,
     * so this can be directly copied, while data from other areas of the
     * struct is modified, hence why we saved it as tempA and temp B 
     */
    *((int *) newptr) = *((int *) oldptr);
    newHeadptr->next = tempA;
    newHeadptr->prev = tempB;
    
    /* 
     * This loop goes through the old location, copies values into the
     * new location one byte at a time. i is the offset from the
     * start of the payload (so since some initial data has already been
     * copied, i is more than 0 to begin with.
     */
    for (int i = sizeof(node) - sizeof(int); i < nSpace && i < size; i++)
    {
        *(newptr + i) = *(oldptr + i);
    }
    return newptr; 
}


/*!
 * Clean up the allocator state.
 * All this really has to do is free the user memory pool. This function mostly
 * ensures that the test program doesn't leak memory, so it's easy to check
 * if the allocator does.
 */
void close_myalloc() 
{
    free(mem);
}



/* ------------------------------------------------------------------- 
 * ------------------------------------------------------------------- 
 * Helper functions (functions that help perform allocator operations
 * or check things like sanity and validity)
 * ------------------------------------------------------------------- 
 * ------------------------------------------------------------------- 
 */


/*!
 * Sanity check function, sums up total free and allocated memory. Compare the
 * return to MEMORY_SIZE to make sure there are no logic errors. Will infinite
 * loop if the last block does not end at the end of the memory pool.
 */
int checkMem()
{
    int freeMem = 0;        /* counts free memory */
    int allocMem = 0;       /* counts allocated memory */
    unsigned char *dataptr; /* pointer to start of every block */
    int blockSize;

    /* 
     * iterate through all blocks, compute block size, and add block size
     * to appropriate counter variable
     */
    for (dataptr = mem; dataptr != mem + MEMORY_SIZE; dataptr += blockSize)
    {
        node *headptr = (node *) dataptr;
        int space = headptr->space;
        blockSize = abs(space) + 2 * sizeof(int);
        if (space < 0)
        {
            allocMem += blockSize;
        }
        else
        {
            freeMem += blockSize;
        }
    }
    return allocMem + freeMem;
}


/*!
 * Check to see if a given address to myfree is valid. Will return 0 if invalid
 * , but does not guarantee validity. This will never reject a valid address,
 * , but it might accept an invalid address. 
 */
int isValid(unsigned char *oldptr)
{
    /* Ensure that oldptr is within acceptable addresses of the memory pool */
    if (mem + sizeof(int) > oldptr || mem + MEMORY_SIZE - sizeof(int) < oldptr)
    {
        return 0;
    }
    int space = - *((int *) (oldptr) - 1);

    /*
     * Ensure that block is not already free, and oldptr's specified block is
     * fully within memory pool.
     */
    if (space < 0 || oldptr + space > mem + MEMORY_SIZE - sizeof(int))
    {
        return 0;
    }

    /*
     * Partially ensure that oldptr did point to a block, not a random location
     * in memory pool. footptr should have same value for space as original in
     * header; however, this is a necessary, not sufficient condition.
     */
    if (*((int *) (oldptr + space)) != -space)
    {
        return 0;
    }
    return 1;
}


/*!
 * Helper function that will take a free block and break it off into two
 * smaller free blocks, the first having a payload as large as the size 
 * argument, the second being the rest of the original block. Returns a 
 * node pointer to the newly made block.
 */
node *splitBlock(node *headptr, int size)
{
    unsigned char *dataptr = (unsigned char *) headptr;
    int space = headptr->space;

    /* establish all the header and footer pointers */
    int *footptr = (int *) (dataptr + sizeof(int) + space);
    int *newFootptr = (int *) (dataptr + sizeof(int) + size);
    node *newHeadptr = (node *) (newFootptr + 1);

    /* 
     * The space in second block is the remainder of space minus the size
     * of the first block's footer and the second block's int tag in the
     * header.
     */
    int newSpace = space - size - 2 * sizeof(int);  

    /* update the int tags for both blocks */
    headptr->space = size;
    *newFootptr = size;
    newHeadptr->space = newSpace;
    *footptr = newSpace;
    return newHeadptr;
}


/*!
 * Helper function that will scan through free list and find a suitable block
 * to be allocated for size amount of bytes. Uses best-fit to find a block
 * (see function comments for reason this is best-fit). This strategy will be
 * good for smaller amounts of blocks, as it ensures better memory utilization
 * than first fit or next fit, but is bad for situations where there are a
 * large number of blocks, as it takes >= as much time as first/next fit,
 * and as all 3 strategies scale linearly, it takes significantly longer
 * for large amounts of blocks.
 *
 * Time complexity of allocation: linear time
 * ------------------------------------------------------------ 
 * Outside of this function, operations all are primarily pointer arithmetic.
 * splitBlock, addNode, and removeNode all just change values in structs,
 * manipulate variables, and use pointer arithmetic, so all are constant time.
 * The high level myalloc function also has only a constant amount of steps.
 * This function, though, involves iterating through the entire free list,
 * which is O(n) for n = number of blocks in the memory pool. Thus, the entire
 * operation is linear in the number of blocks there are in the memory pool.
 */
node *findHead(int size)
{
    node *resultptr = NULL; 
    int lowest; /* keep track of smallest block size accomodating request */

    /* iterate through all free blocks */
    for (node *headptr = freeList; headptr != NULL; headptr = headptr->next)
    {
        int space = headptr->space;
        if (space == size) /* Perfect fit! */
        {
            resultptr = headptr;
            break;
        }
        else if (space > size)
        {
            /* 
             * if it's the first block that accomodates the request, or it
             * is a working block smaller than the current best option
             */
            if (resultptr == NULL || space < lowest)
            {
                lowest = space;
                resultptr = headptr;
            }
        }
    }
    return resultptr;
}



/* ------------------------------------------------------------------- 
 * ------------------------------------------------------------------- 
 * Node functions (functions that primarily alter the doubly linked
 * list implementation of the free list)
 * ------------------------------------------------------------------- 
 * ------------------------------------------------------------------- 
 */


/*!
 * Will take a node out of the free list and repair the links in the list,
 * useful in both allocating and coalescing free blocks.
 */
void removeNode(node *badNode)
{
    node *prevNode = badNode->prev;
    node *nextNode = badNode->next;
    if (prevNode == NULL)
    {
        freeList = nextNode; /* One would never remove the first/only node */
    }
    else
    {
        prevNode->next = nextNode;
    }
    if (nextNode != NULL)
    {
        nextNode->prev = prevNode;
    }
}


/*!
 * Adds a new node to the beginning of the free list, which is thus constant
 * time.
 */
void addNode(node *newNode)
{
    node *oldFirstNode = freeList;
    newNode->next = oldFirstNode;
    newNode->prev = NULL;
    freeList = newNode;
    if (oldFirstNode != NULL)
    {
        oldFirstNode->prev = newNode;
    }
}


/*!
 * This function, given pointers to two headers for free blocks, will combine
 * the blocks and update the free list to have one bigger free block
 */
void coalesce(node *headptrA, node *headptrB)
{
    /*
     * Make a single header and footer for the aggregate block with the new
     * space
     */
    int newSpace = headptrA->space + headptrB->space + 2 * sizeof(int);
    headptrA->space = newSpace;
    int *footptr = (int *) ((unsigned char *) (headptrA) + newSpace 
                                                         + sizeof(int));
    *footptr = newSpace;
    removeNode(headptrB); /* update the free list */
}

    
    
    
    
    
    
    
    
