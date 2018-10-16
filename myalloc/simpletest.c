/*! \file
 * This file contains code for performing simple tests of the memory allocator.
 * It can be edited by students to perform simple sequences of allocations and
 * deallocations to make sure the allocator works properly.
 */

#include <stdio.h>
#include <stdlib.h>

#include "myalloc.h"


/* Try to allocate a block of memory, and fill its entire contents with
 * the specified fill character.
 */
unsigned char * allocate(int size, unsigned char fill) {
    unsigned char *block = myalloc(size);
    if (block != NULL) {
        int i;

        printf("Allocated block of size %d bytes.\n", size);
        for (i = 0; i < size; i++)
            block[i] = fill;
    }
    else {
        printf("Couldn't allocate block of size %d bytes.\n", size);
    }
    return block;
}


int main(int argc, char *argv[]) {

    /* Specify the memory pool size, then initialize the allocator. */
    MEMORY_SIZE = 40000;
    init_myalloc();

    /* Perform simple allocations and deallocations. */
    /* Change the below code as you see fit, to test various scenarios. */

#if 0
    unsigned char *a = allocate(16, 'A');
    unsigned char *b = allocate(16, 'B');
    unsigned char *c = allocate(16, 'C');
    myfree(a);
    myfree(b);
    myfree(c);

    for (int i = 0; i < 30; i++)
    {
        a = allocate(10, i);
    }
#endif

    unsigned char *a = allocate(100, 'A');
    /*myrealloc(a, 40000);*/

    unsigned char *b = allocate(200, 'B');
    unsigned char *c = allocate(300, 'C');
    myfree(a);
    /*myrealloc(b, 40000);*/

    allocate(100, 'A');
    /* myrealloc(b, 40000); */

    myfree(a); 
    myfree(c);
    /* myrealloc(b, 40000); */

    b = myrealloc(b, 400);

    myfree(b);
    close_myalloc();

    return 0;
}


