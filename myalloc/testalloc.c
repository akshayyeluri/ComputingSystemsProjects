/*! \file
 * This is a relatively sophisticated memory-allocator tester to record a
 * sequence of allocations and deallocations, so that they can be replayed to
 * the allocator and the responses analyzed.
 *
 * Adapted from Andre DeHon's CS24 2004, 2006 material.
 * Copyright (C) California Institute of Technology, 2004-2009.
 * All rights reserved.
 */

#include <stdio.h>
#include <stdlib.h>
#include <getopt.h>

#include "errno.h"
#include "myalloc.h"
#include "sequence.h"

#define VERBOSE 0

#define DEFAULT_MAX_ALLOCATION 16000
#define DEFAULT_RANDOM_SEED 1

// some random numbers...
int random_int(int max) {
  int rnd;
  int result;
  rnd = rand();
  result = 1 + (int) ((long long) max * (long long) rnd / ((long long) RAND_MAX + 1.0));

  return result;
}


int random_block_size(int max_value) {

  // blah, almost certainly not a good model of
  //  typical allocations, but workable for a crude test
  return random_int(max_value / 4);

}

int random_byte() {
  return random_int(256) - 1;
}

// fill in block p2 of length len with data from p1
void fill_data(unsigned char *p1, unsigned char *p2, int len) {
  int i;
  unsigned char *ptr1, *ptr2;
  ptr1 = p1;
  ptr2 = p2;

  if (VERBOSE) {   // very verbose (for debugging)
    printf("now filling %p from %p to length %d\n", p2, p1, len);
  }

  for (i = 0; i < len; i++) {
    *ptr2 = *ptr1;
    if (VERBOSE) {    // very verbose
      printf("now writing %p<-%u (from %p)\n", ptr2, *ptr2, ptr1);
    }
    ptr1++;
    ptr2++;
  }
}


// try applying sequence
int try_sequence(SEQLIST *test_sequence, int mem_size) {
  SEQLIST *sptr;
  unsigned char *mblock;

  // reset the memory allocator being tested
  MEMORY_SIZE = mem_size;
  init_myalloc();

  for (sptr = test_sequence; !seq_null(sptr); sptr = seq_next(sptr)) {
    if (seq_alloc(sptr)) {     // allocate a block
      mblock = myalloc(seq_size(sptr));
      if (mblock == 0) {
        return 0; // failed -- return indication
      }
      else {
        // keep track of address allocated (for later frees)
        seq_set_myalloc_block(sptr, mblock);
        // put data in the block
        //  (so we can test that it holds data w/out corruption)
        fill_data(seq_ref_block(sptr), mblock, seq_size(sptr));
      }
    }
    else {    // dealloc
      myfree(seq_myalloc_block(seq_tofree(sptr)));
    }
  }

  return 1; // succeeded in allocating entire sequence
}


// search over memory sizes between low and high
//  report smallest size that can accommodate the sequence
int binary_search_required_memory(SEQLIST *test_sequence, int low, int high) {
  // invariant: low not achievable, high is achievable

  int mid;

  if (low + 1 == high) {     // nothing in between, we've found the smallest
    return high;
  }
  else {
    mid = (low + high + 1) / 2;
    if (try_sequence(test_sequence, mid)) {
      close_myalloc();
      if (VERBOSE)
        printf("\tSucceeded for %d\n", mid);

      return(binary_search_required_memory(test_sequence, low, mid));
    }
    else {
      close_myalloc();
      if (VERBOSE)
        printf("\tFailed for %d\n", mid);

      return(binary_search_required_memory(test_sequence, mid, high));
    }
  }
}


// allocate (from normal malloc) a block of size blocks
//  and put data into it
// This supports data integrity tests.
unsigned char *allocate_and_fill(int size) {
  unsigned char *result;
  unsigned char *ptr;

  result = (unsigned char *) malloc(size);
  if (result == (unsigned char *) 0) {
    fprintf(stderr,"real memory system out of memory.\n");
    abort();
  }

  if (VERBOSE) {
      printf("ref fill for %p\n", result);
  }

  for (ptr = result; ptr < result + size; ptr++) {
    *ptr = random_byte();
    if (VERBOSE) {     // very
      printf("\tputting %u in %p\n", *ptr, ptr);
    }
  }

  return result;
}


// check if p1 and p2 contain the same data up to length len
// This is used to check buffer has not been corrupted
int same_data(unsigned char *p1, unsigned char *p2, int len) {
  int i;
  int result;
  unsigned char *ptr1, *ptr2;

  ptr1 = p1;
  ptr2 = p2;

  result = 1;

  for (i = 0; i < len; i++) {
    if (*ptr1 != *ptr2) {
      if (VERBOSE) {
        printf("error at %p (%d/%d): got %u expect %u (from %p base %p)\n",
               ptr2, i, len, *ptr2, *ptr1, ptr1, p1);
      }
      result = 0;
    }
    ptr1++;
    ptr2++;
  }

  return result;
}


// check all still allocated blocks in a test sequence
//  contain the data originally placed into them
//  i.e. have not been corrupted
int check_data(SEQLIST *test_sequence) {
  int result;
  SEQLIST *current;

  result = 0; // stays zero if no errors

  for (current = test_sequence; !seq_null(current); current = seq_next(current)) {
    // only check if an allocate which has not been freed
    if (seq_alloc(current) && !seq_freed(current)) {
      if (!same_data(seq_ref_block(current), seq_myalloc_block(current),
                     seq_size(current))) {
        if (VERBOSE) {
          printf("Mismatch in sequence starting at:\n");
          seq_print(current);
        }

        // returning a 1 means it failed
        result = 1;
      }
    }
  }

  // After the data is checked, we don't need the myalloc pool that the
  // sequence was stored in - we can close it.
  close_myalloc();

  return result;
}


// create a test sequence which never uses more than max_used_memory
//   and allocates a total of max_used_memory*allocation_factor
SEQLIST *generate_sequence(int max_used_memory, int allocation_factor) {
  int used_memory = 0;
  int total_allocated = 0;
  int next_block_size = 0;
  int allocated_blocks = 0;
  int actual_max_used_memory = 0;

  SEQLIST *test_sequence = NULL;
  SEQLIST *tail_sequence = NULL;

  unsigned char *new_block_ref;

  while (total_allocated < allocation_factor * max_used_memory) {
    next_block_size = random_block_size(max_used_memory);

    // first see if we need to free anything in order to
    //  accommodate the new allocation
    while (used_memory + next_block_size > max_used_memory) {
      // randomly pick a block to free
      SEQLIST *tofree =
        find_nth_allocated_block(test_sequence, random_int(allocated_blocks));

      // add the free
      tail_sequence = seq_set_next_free(tofree, tail_sequence);

      // reclaim the memory
      used_memory -= seq_size(tofree);
      allocated_blocks--;

      // mark the old block as something that has been freed
      seq_free(tofree);
    }

    // allocate a reference buffer for the new block
    new_block_ref = allocate_and_fill(next_block_size);

    // now allocate that block
    if (seq_null(test_sequence)) {
      // special case for first allocation
      test_sequence = seq_add_front(next_block_size, new_block_ref, (SEQLIST *) 0);
      tail_sequence = test_sequence;
    }
    else {
      // typical case we add at the end
      tail_sequence =
        seq_set_next_allocate(next_block_size, new_block_ref, tail_sequence);
    }

    // debug
    //seq_print(tail_sequence); // just prints the new one

    total_allocated += next_block_size;
    used_memory += next_block_size;

    if (used_memory > actual_max_used_memory)
      actual_max_used_memory = used_memory;

    allocated_blocks++;
  }

  // just so can manually see this is doing something sensible
  printf("Actual maximum memory usage %d (%f)\n", actual_max_used_memory,
         ((double) actual_max_used_memory / (double) max_used_memory));

  return test_sequence;
}

// A basic test that the memory allocator coalesces free blocks.
void coalesce_test() {
  unsigned char * a;
  unsigned char * b;
  unsigned char * c;
  int failure = 0;

  printf("Performing a basic test of forward and backward coalescing.\n");

  MEMORY_SIZE = 1024;
  init_myalloc();

  // Forward coalescing test
  a = myalloc(400);
  if (a == NULL) {
    printf("Couldn't allocate one block of size 400 in a 1024 byte pool.\n");
    failure = 1;
    goto done;
  }
  b = myalloc(400);
  if (b == NULL) {
    printf("Couldn't allocate second block of size 400 in 1024 byte pool.\n");
    failure = 1;
    goto done;
  }
  myfree(b);
  myfree(a);
  c = myalloc(800);
  if (c == NULL) {
    printf("Failed to coalesce forward.\n");
    failure = 1;
    goto done;
  }
  myfree(c);

  // Backward coalescing test
  a = myalloc(400);
  if (a == NULL) {
    printf("Couldn't allocate one block of size 400 in a 1024 byte pool.\n");
    failure = 1;
    goto done;
  }
  b = myalloc(400);
  if (b == NULL) {
    printf("Couldn't allocate second block of size 400 in 1024 byte pool.\n");
    failure = 1;
    goto done;
  }
  myfree(a);
  myfree(b);
  c = myalloc(800);
  if (c == NULL) {
    printf("Failed to coalesce backward.\n");
    failure = 1;
    goto done;
  }
  myfree(c);


done:
  if (!failure) {
    printf("Passed coalescing test.\n");
  }
  close_myalloc();

}

// Allocates chunks of the given size until unable to anymore, then
// deallocates all of them - returns the number of chunks allocated.
int uniform_chunks(int chunk_size, int memory_size) {
  unsigned char ** pointers =
    malloc(sizeof(unsigned char *) * memory_size / chunk_size);
  unsigned char * p;
  int chunks = -1;
  do {
    chunks++;
    p = myalloc(chunk_size);
    pointers[chunks] = p;
  } while (p != NULL);

  for (int i = 0; i < chunks; i++) {
    myfree(pointers[i]);
  }

  free(pointers);
  return chunks;
}

// Basic correctness test -
// allocates a bunch of uniformly-sized chunks, deallocates all of them, and
// repeats a few times, making sure it works every time - no leaks in the
// user pool. This actually won't give much more information than the other
// test, which is more rigorous.
int uniform_chunk_test() {
  int chunk_size = 256;
  int repetitions = 10;
  int chunks = 0;
  int failure = 0;

  MEMORY_SIZE = 16000;

  printf("Performing the basic uniform chunks test.\n");

  init_myalloc();

  chunks = uniform_chunks(chunk_size, MEMORY_SIZE);

  printf("Allocated %d uniform chunks on a first pass.\n"
          "Theoretical maximum: %d\n", chunks, MEMORY_SIZE / chunk_size);
  if (chunks < MEMORY_SIZE / (chunk_size + 64)) {
    printf("not enough uniform chunks could be allocated.\n"
            "Too much overhead in memory allocator.\n");
    failure = 1;
  }
  for (int i = 1; i < repetitions; i++) {
    if (uniform_chunks(chunk_size, MEMORY_SIZE) != chunks) {
      printf("Could not allocate %d chunks on round %d.\n"
              "Uniform chunk test FAILED.\n", chunks, i);
      failure = 1;
      break;
    }
  }

  if (!failure)
    printf("Passed uniform chunks test.\n");

  close_myalloc();
  return 0;

}


/* This test runs a series of random allocations and deallocations,
 * to see how much overhead is required by the allocator in question
 * for a certain number of bytes to be allocated.  During the test,
 * the allocated regions are verified to not overlap with each other,
 * and so forth.
 */
void utilization_test(int max_allocation) {
  int max_used_memory;
  int allocation_factor;
  int memory_required;

  SEQLIST *test_sequence;

  max_used_memory = max_allocation;
  allocation_factor = 11;

  printf("running with MAX_USED_MEMORY=%d and ALLOCATION_FACTOR=%d\n",
    max_used_memory, allocation_factor);

  test_sequence = generate_sequence(max_used_memory, allocation_factor);
  if (VERBOSE)
    seq_print(test_sequence);

  // check that allocation can actually do something.
  // This becomes upper bound on binary search.
  if (try_sequence(test_sequence, max_used_memory * allocation_factor * 2)) {

    // That call to try_sequence allocated a memory pool for myalloc, which
    // is no longer in use.
    close_myalloc();

    // binary search for smallest MEMORY_SIZE which can accommodate
    memory_required = binary_search_required_memory(test_sequence,
      max_used_memory - 1, max_used_memory * allocation_factor * 2);

    // run it one more time at the identified size.
    // this makes sure that the data is set from a successful run.
    if (try_sequence(test_sequence, memory_required)) {
      // check if data contents are intact
      if (check_data(test_sequence)) {
        printf("Data integrity FAIL.\n");
      }
      else {
        printf("Data integrity PASS.\n");
      }

      // print statistics
      printf("Memory utilization: (%d/%d)=%f\n", max_used_memory, memory_required,
             ((double) max_used_memory / (double) memory_required));
    }
    else {
      printf("Consistency problem: binary_search_required_memory "
             "returned %d, but final test failed\n", memory_required);
    }
  }
  else {
    printf("Requires more memory than the no-free case.\n");
  }
  seq_cleanup(test_sequence);
}


void usage(char *program) {
  printf("usage: %s [-s seed] [-m max_allocation]\n", program);
  printf("\tRuns the myalloc tester.\n\n");
  printf("\t-s seed sets the tester to use a specific random seed\n\n");
  printf("\t-m max_allocation sets the maximum number of bytes that the\n");
  printf("\ttester should try to allocate during utilization tests\n\n");
}


/* The main program, which calls the various tests to exercise the allocator. */
int main(int argc, char *argv[]) {
  unsigned int seed = DEFAULT_RANDOM_SEED;
  int max_allocation = DEFAULT_MAX_ALLOCATION;
  int c;

  while ((c = getopt(argc, argv, "s:m:")) != -1) {
    switch (c) {
      case 's':    /* Random seed */
        seed = atoi(optarg);
        break;

      case 'p':
        max_allocation = atoi(optarg);
        if (max_allocation < 0)
          printf("ERROR:  Max allocation must be nonnegative.\n");
          usage(argv[0]);
          return 1;
        break;

      case 'h':
        usage(argv[0]);
        return 1;
    }
  }

  if (seed != DEFAULT_RANDOM_SEED)
    printf("Using seed:  %u\n\n", seed);

  srand(seed);

  // Do the basic test of coalescing behavior
  coalesce_test();
  printf("\n");

  // Do the basic test with repeated allocation of many uniform chunks
  uniform_chunk_test();
  printf("\n");

  // Do the memory utilization test to see how efficient the allocator is
  utilization_test(max_allocation);

  return 0;
}


