#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>

#include "multimap.h"

/*============================================================================
README:
    Just some important notes about data structure implementations and common
    coding motifs in these implementations:
        1) Multimap representation --- To improve locality of access, the
            array implementation of a binary tree is used. What this means
            is that the multimap is actually an array of multimap nodes,
            where the children of node i are the (2i + 1) node and
            (2i + 2) node (left and right children respectively). Thus,
            the amount of space taken up by a multimap, assuming it's a
            tree of depth d is 2^d - 1 multimap nodes (here, d is 1-indexed).
            For example, if you have a tree (number is value there, * means
            nothing present):
                                       0
                                   1       2
                                 *   3   4   5
                                * * 6 * * * * 7  
            d = 4, and the array would have 15 elements, and indices 0-14.
            3 would be found at index 4 in the array, and its children
            would be indices 9 (value 6) and 10 (*, no value). 
        2) Common motifs ---
            a) A common pattern to get a pointer to the i'th node of a multimap
            pointed to by mm is (mm->root + i). Additionally,
            this ith node has a valid field, so to check if node i is valid,
            one can use (mm->root + i)-> valid. To make sure there is no
            invalid memory accessed, the multimap struct has a field maxI,
            which is the max index allowed, so the full pattern to check if
            the node at index nodeI is invalid is:
                if (nodeI > mm->maxI || (mm->root + nodeI)->valid == 0) {
                    // node is invalid
                }
            b) Recursive traversals: this is found in both find_mm_node and
               traverse_helper. Essentially, the way these functions work is
               that they take a nodeI (a node index) as an argument, and
               recursively traverse just the subtree starting from that index
               . Thus, to search whole tree, you'd call with a 0. The
               base case for the recursion is what's mentioned in a), when
               you hit an invalid node. Thus, until an invalid node is hit
               (or until the right node is found in the case of find_mm_node),
               these functions recursively search the two subtrees whose roots
               are the left and right children of the original node.
 *============================================================================*/

    
/*============================================================================
 * TYPES
 *
 *   These types are defined in the implementation file so that they can
 *   be kept hidden to code outside this source file.  This is not for any
 *   security reason, but rather just so we can enforce that our testing
 *   programs are generic and don't have any access to implementation details.
 *============================================================================*/

/* typedef int multimap_value; */
 /* Represents a value that is associated with a given key in the multimap. */  
typedef struct multimap_value {                                                 
    int value;                                                                  
    struct multimap_value *next;                                                
} multimap_value;           

/* Represents a key and its associated values in the multimap,
 * which is a large array of these nodes. Each node also stores its
 * index in the multimap array, which allows for convenient access to left
 * and right children. For example, if a node is index i in the multimap,
 * then it's left child is index 2i + 1, and its right child is index 2i + 2
 * in the array. Also, each node stores whether or not it is valid.
 */
typedef struct multimap_node {
    int valid;

    /* The key-value that this multimap node represents. */
    int key;

    /* A linked list of the values associated with this key in the multimap. */
    multimap_value *values;

    multimap_value *values_tail;

} multimap_node;


/* The entry-point of the multimap data structure. */
struct multimap {
    int maxI;
    multimap_node *root;
};


/*============================================================================
 * HELPER FUNCTION DECLARATIONS
 *
 *   Declarations of helper functions that are local to this module.  Again,
 *   these are not visible outside of this module.
 *============================================================================*/

multimap_node * alloc_mm_node();

multimap_node * find_mm_node(multimap *mm, int key, int nodeI,
                             int create_if_not_found);

void free_multimap_values(multimap_value *values);

int findMaxI(unsigned int index);


/*============================================================================
 * FUNCTION IMPLEMENTATIONS
 *============================================================================*/

/* This function is useful when reallocating the multimap array. Given
 * an index, this function calculates what the max index would be of the
 * smallest multimap that could fit the index passed in. For example, if one
 * passed in 5, this function would return 6, since a multimap with 7 spots
 * would be the smallest array that has an index 5, because multimap arrays
 * have to be able to hold complete trees, so they cannot be arbitrary sizes.
 */
int findMaxI(unsigned int index)
{
    /* To find tree depth, one must do log2(index + 1), and take the ceiling.
     * This is because the index is 1 less than the minimum number of
     * elements the tree must hold, the log 2 is for the binary nature of the
     * tree, and the ceiling is because depth is an integer. 
     */
    int depth = 0; 
    index++;
    while (index != 0) /* This implements the ceil(log2(index)) */
    {
        index = index >> 1;
        depth++;
    }

    /* Once depth has been found, the max index, as explained above, is 1 less
     * than the max number of elements the tree can hold, which is 2^d - 1. 
     * Thus, return 2^d - 2.
     */
    return (1 << depth) - 2;
}


/* Allocates a multimap node. This entails setting the node's valid flag to be
 * 1 in the case where the node is already inside the multimap. However,
 * if the nodeI is beyond maxI (the node is not in the multimap), this
 * function also extends the multimap and updates the fields in the struct.
 */
multimap_node * alloc_mm_node(multimap *mm, int nodeI) {
    /* if multimap needs extending */
    if (mm->maxI < nodeI)
    {
        /* figure out new size, and reallocate */ 
        int newMaxI = findMaxI(nodeI); 
        mm->root = (multimap_node *) realloc(mm->root, 
                                        (newMaxI + 1) * sizeof(multimap_node));

        /* Make sure all the new nodes are clearly invalid */
        for (int i = mm->maxI + 1; i <= newMaxI; i++)
        {
            (mm->root + i)->valid = 0;
        }
        mm->maxI = newMaxI; /* update size */
    }
    multimap_node *node = mm->root + nodeI;
    node->valid = 1;
    return node;
}

    
/* This helper function searches for the multimap node that contains the
 * specified key.  If such a node doesn't exist, the function can initialize
 * a new node and add this into the structure, or it will simply return NULL.
 * The function actually recursively searches a subtree starting at nodeI, so
 * to search the whole tree, call with nodeI = 0.  
 */
multimap_node * find_mm_node(multimap *mm, int key, int nodeI,
                                           int create_if_not_found) 
{                         
    multimap_node *node;
    /* if an invalid node, either make a valid node, or return NULL */      
    if (nodeI > mm->maxI || (mm->root + nodeI)->valid == 0)                 
    {                                                                       
        if (nodeI == 0 || create_if_not_found)                                            
        {                                                                   
            node = alloc_mm_node(mm, nodeI);                                
            node->key = key;                                                
            return node;                                                    
        }                                                                   
        return NULL;                                                        
    }                                                                       
                                                                            
    node = mm->root + nodeI;                                                
    if (node->key == key) /* perfect match! */                              
    {                                                                       
        return node;                                                        
    }                                                                       
    /* decide which subtree to search (what the value of childI should be).
     * if node->key < key, want to search right child branch, so let
     * childI be 2 * nodeI + 2
     */
    int childI = 2 * nodeI + 1 + (node->key < key);
    return find_mm_node(mm, key, childI, create_if_not_found);
}


/* This helper function frees all values in a multimap node's value-list. */
void free_multimap_values(multimap_value *values) {
    while (values != NULL) {
        multimap_value *next = values->next;
#ifdef DEBUG_ZERO
        /* Clear out what we are about to free, to expose issues quickly. */
        bzero(values, sizeof(multimap_value));
#endif
        free(values);
        values = next;
    }
}


/* Initialize a multimap data structure. */
multimap * init_multimap() {
    multimap *mm = (multimap *) malloc(sizeof(multimap));
    mm->maxI = -1; /* since even 0 is currently an invalid index */
    mm->root = NULL;
    return mm;
}


/* Release all dynamically allocated memory associated with the multimap
 * data structure.
 */
void clear_multimap(multimap *mm) {
    assert(mm != NULL);
    multimap_node *node = mm->root;
    /* Here, can just traverse array linearly, freeing values associated with
     * each node, if the node is valid. Do not need to do an ordered traversal
     */
    for (int i = 0; i <= mm->maxI; i++)
    {
        if (node->valid == 1) 
        {
            free_multimap_values(node->values);
        }
        node++;
    }
    free(mm->root); /* Since multimap is an array, can free as one block */
    mm->root = NULL;
}


/* Adds the specified (key, value) pair to the multimap. */
void mm_add_value(multimap *mm, int key, int value) {
    multimap_node *node;
    multimap_value *new_value;

    assert(mm != NULL);

    /* Look up the node with the specified key.  Create if not found. */
    node = find_mm_node(mm, key, /* from root */ 0, /* create */ 1);

    assert(node != NULL);
    assert(node->key == key);

    /* Add the new value to the multimap node. */

    new_value = malloc(sizeof(multimap_value));
    new_value->value = value;
    new_value->next = NULL;

    if (node->values_tail != NULL)
        node->values_tail->next = new_value;
    else
        node->values = new_value;

    node->values_tail = new_value;
}


/* Returns nonzero if the multimap contains the specified key-value, zero
 * otherwise.
 */
int mm_contains_key(multimap *mm, int key) {
    return find_mm_node(mm, key, /* from root */ 0, /* no create */ 0) != NULL;
}


/* Returns nonzero if the multimap contains the specified (key, value) pair,
 * zero otherwise.
 */
int mm_contains_pair(multimap *mm, int key, int value) {
    multimap_node *node;
    multimap_value *curr;

    node = find_mm_node(mm, key, /* from root */ 0, /* no create */ 0);
    if (node == NULL)
        return 0;

    curr = node->values;
    while (curr != NULL) {
        if (curr->value == value)
            return 1;

        curr = curr->next;
    }

    return 0;
}


/* This helper function is used by mm_traverse() to traverse the multimap. 
 * It recursively traverses the subtree starting at index nodeI (so to traverse
 * the whole multimap, call with nodeI = 0)
 */
void mm_traverse_helper(multimap *mm, int nodeI, void (*f)(int key, int value)) 
{
    multimap_value *curr;

    /* If you hit an invalid node, have finished traversing a tree branch */
    if (nodeI > mm->maxI || (mm->root + nodeI)->valid == 0)
        return;

    multimap_node *node = mm->root + nodeI;

    /* traverse the left child branch of the tree */
    mm_traverse_helper(mm, 2 * nodeI + 1, f);

    /* go through the values at this node of the tree */
    curr = node->values;
    while (curr != NULL) {
        f(node->key, curr->value);
        curr = curr->next;
    }

    /* Finally traverse the right child branch */
    mm_traverse_helper(mm, 2 * nodeI + 2, f);
}


/* Performs an in-order traversal of the multimap, passing each (key, value)
 * pair to the specified function.
 */
void mm_traverse(multimap *mm, void (*f)(int key, int value)) {
    mm_traverse_helper(mm, /* from root */ 0, f);
}

