/*! \file
 * The definitions in this file allow the memory-allocator tester to record a
 * sequence of allocations and deallocations, so that they can be replayed to
 * the allocator and the responses analyzed.
 *
 * Adapted from Andre DeHon's CS24 2004, 2006 material.
 * Copyright (C) California Institute of Technology, 2004-2009.
 * All rights reserved.
 */

#include <stdio.h>
#include <stdlib.h>
#include "sequence.h"

SEQLIST *seq_add_front(int size, unsigned char *ref_block, SEQLIST *next) {
  SEQLIST *result = (SEQLIST *) malloc(sizeof(SEQLIST));

  if (result == (SEQLIST *) 0) {
    fprintf(stderr, "real memory exhausted.\n");
    abort();
  }

  result->alloc = 1;
  result->freed = 0;
  result->size = size;
  result->ref_block = ref_block;
  result->myalloc_block = (unsigned char *) 0;
  result->tofree = (SEQLIST *) 0;
  result->next = next;
  return result;
}

SEQLIST *seq_set_next_allocate(int size, unsigned char *ref_block,
                               SEQLIST *prev) {

  SEQLIST *result = (SEQLIST *) malloc(sizeof(SEQLIST));

  if (result == (SEQLIST *) 0) {
    fprintf(stderr, "real memory exhausted.\n");
    abort();
  }

  result->alloc = 1;
  result->freed = 0;
  result->size = size;
  result->ref_block = ref_block;
  result->myalloc_block = (unsigned char *) 0;
  result->tofree = (SEQLIST *) 0;
  result->next = (SEQLIST *) 0;
  prev->next = result;

  return result;
}


SEQLIST *seq_set_next_free(SEQLIST *tofree, SEQLIST *prev) {

  SEQLIST *result = (SEQLIST *) malloc(sizeof(SEQLIST));

  if (result == (SEQLIST *) 0) {
    fprintf(stderr, "real memory exhausted.\n");
    abort();
  }

  result->alloc = 0;
  result->freed = 0;
  result->size = 0;
  result->ref_block = (unsigned char *) 0;
  result->myalloc_block = (unsigned char *) 0;
  result->tofree = tofree;
  result->next = (SEQLIST *) 0;
  prev->next = result;

  return result;
}

int seq_alloc(SEQLIST *seq) {
  return seq->alloc;
}

int seq_freed(SEQLIST *seq) {
  return seq->freed;
}

void seq_free(SEQLIST *seq) {
  seq->freed = 1;
}

void seq_set_myalloc_block(SEQLIST *seq, unsigned char *myalloc_block) {
  seq->myalloc_block = myalloc_block;
}

int seq_size(SEQLIST *seq) {
  return seq->size;
}

unsigned char * seq_ref_block(SEQLIST *seq) {
  return seq->ref_block;
}

unsigned char * seq_myalloc_block(SEQLIST *seq) {
  return seq->myalloc_block;
}

SEQLIST * seq_next(SEQLIST *seq) {
  return seq->next;
}

SEQLIST * seq_tofree(SEQLIST *seq) {
  return seq->tofree;
}

int seq_null(SEQLIST *seq) {
  return (seq == (SEQLIST *) 0);
}

SEQLIST * find_nth_allocated_block(SEQLIST *seq, int n) {
  int cnt = 0;
  SEQLIST *sptr;

  for (sptr = seq; !seq_null(sptr); sptr = seq_next(sptr)) {
    if (seq_alloc(sptr) && !seq_freed(sptr)) {
      cnt++;
      if (cnt == n)
        return sptr;
    }
  }

  fprintf(stderr, "find_nth_allocated_block found only %d blocks, "
          "but asked for %dth block\n", cnt, n);
  seq_print(seq);
  abort();
}

void seq_print(SEQLIST *seq) {
  int cnt = 0;
  SEQLIST *sptr;

  for (sptr = seq; !seq_null(sptr); sptr = seq_next(sptr)) {
    cnt++;
    printf("\t");
    if (seq_alloc(sptr)) {
      printf("ALLOC");

      if (seq_freed(sptr))
        printf(" FREED ");
      else
        printf(" LIVE  ");

      printf("%d r=%p m=%p ", seq_size(sptr), seq_ref_block(sptr),
             seq_myalloc_block(sptr));
    }
    else {    // dealloc
      printf("FREE  ");
      printf("r to free %p", seq_ref_block(seq_tofree(sptr)));
    }

    printf("\n");
  }

  printf("Length=%d\n", cnt);
}

/* Free all of the real memory resources used by the sequence.
 * This does NOT clean up the resources held in the myalloc pool. */
void seq_cleanup(SEQLIST *seq) {
  SEQLIST *current = seq;
  SEQLIST *next;
  while (current != NULL) {
    next = current->next;
    free(current->ref_block);
    free(current);
    current = next;
  }
}
