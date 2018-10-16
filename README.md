# Memory-Allocator
A homemade memory allocator. Given a region of memory to work with, the allocator can essentially replace malloc, allocating and keeping track of memory segments. This is fairly high performance, and has various optimizations to utilize the maximum amount of the memory region, including forward and backward coalescing, a best-fit allocation strategy, and a doubly linked list to keep track of free memory regions.

To use the memory allocator, just set the global variable MEMORY_SIZE to the desired memory pool size, and call init_myalloc(). From that point on, the allocator can be used like malloc (p = myalloc(nBytes) will make p a pointer to a memory region of sign nBytes. myfree(p) will free this region, and one can call myrealloc(p, newN) to realloc). Finally, call close_myalloc() to terminate. An example of this usage is shown in simpletest.c.

To see more detailed performance information, you can run testmyalloc, which will run in depth tests of the allocator, and calculate memory usage statistics.

