/*! \file
 * The declarations in this file allow the memory-allocator tester to record a
 * sequence of allocations and deallocations, so that they can be replayed to
 * the allocator and the responses analyzed.
 *
 * Adapted from Andre DeHon's CS24 2004, 2006 material.
 * Copyright (C) California Institute of Technology, 2004-2009.
 * All rights reserved.
 */

typedef struct sequence_struct {
  int alloc; // is this block an allocate
             // 1=allocate; 0=free
  int freed; // has this block been freed
  int size; // in bytes
  unsigned char *ref_block; // ref. block for checking data
  unsigned char *myalloc_block; // pointer to block from myalloc
  struct sequence_struct *tofree; // for a free, the sequence_struct
                                  // whose allocation should be freed
  struct sequence_struct *next;  // next pointer
} SEQLIST;

// add to front, always an allocate
SEQLIST *seq_add_front(int size, unsigned char *ref_block, SEQLIST *next);
// add to tail ... allocate and free version
SEQLIST *seq_set_next_allocate(int size, unsigned char *ref_block, SEQLIST *prev);
SEQLIST *seq_set_next_free(SEQLIST *tofree, SEQLIST *prev);
// accessors
int seq_alloc(SEQLIST *seq);
int seq_freed(SEQLIST *seq);
int seq_size(SEQLIST *seq);
unsigned char *seq_ref_block(SEQLIST *seq);
unsigned char *seq_myalloc_block(SEQLIST *seq);
SEQLIST  *seq_next(SEQLIST *seq);
SEQLIST  *seq_tofree(SEQLIST *seq);
// predicate
int seq_null(SEQLIST *seq);
// mutators
void seq_set_myalloc_block(SEQLIST *seq,unsigned char *myalloc_block);
void seq_free(SEQLIST *seq);
// utilities
SEQLIST *find_nth_allocated_block(SEQLIST *seq,int n);
void seq_print(SEQLIST *seq);
void seq_cleanup(SEQLIST *seq);

