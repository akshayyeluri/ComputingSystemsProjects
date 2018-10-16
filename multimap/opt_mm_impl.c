#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "multimap.h"


/*============================================================================
README:
    Just some important notes about data structure implementations and common
    coding motifs in these implementations:
        1) Multimap representation --- To improve locality of access, the
            multimap is implemented as a b-tree with a max of MAX_KEYS keys
            per node. It is a general b-tree, where by changing the #define
            for MAX_KEYS, one can change the b-tree (making this value 2
            makes a 2-3 b tree, while making this value 1 or less just breaks
            everything, so plz don't break my tree???)
        2) More on structs --- To implement this tree, there are many levels
            of wrappers. Here they are, in order of highest to lowest level.

            Struct          variable Name       Stuff it holds
            ------          -------------       --------------
        a)  multimap        mm                  pointer to root of a tree
        b)  mm_node         node, parent        whether or not it's a leaf
                            child, root         how many key_nodes are in it
                                                an array of its key nodes
                                                an array of pointers to kids
        c)  key_node        kNode               the key, the number of vals
                                                associated with the key,
                                                pointer to the array of vals
        d)  multimap_value  value               literally an int
            
            For a visual representation, a tree might look like:
                    mm->root
                    |  
                    --------------------------------------------       
                    | isLeaf=0   | kNodes[0]:   | kNodes[1]:   |
                    | nKeys=2    |    key=2     |    key=5     |
                    |            |    nVals=2   |    nVals=20  |
                    |            |    values=   |    values=   |
                    |            |      0x60323 |      0x60ec3 |
                    |------------|--------------|--------------|
                    | kids[0]    | kids[1]      | kids[2]      |
                    --------------------------------------------
                                 |              |              |
 --------------------------------------------   |              |
 | isLeaf=1   | kNodes[0]:   | kNodes[1]:   |   |              |
 | nKeys=1    |    key=1     |    key;      |   |              |
 |            |    nVals=2   |    nVals;    |   |              |
 |            |    values=   |    values=   |   |              |
 |            |      0x60323 |      NULL    |   |              |
 |------------|--------------|--------------|   |              |
 | NULL       | NULL         | NULL         |   |              |
 --------------------------------------------   |              |
                                                |              |
                                                |              |
              --------------------------------------------     |       
              | isLeaf=1   | kNodes[0]:   | kNodes[1]:   |     |
              | nKeys=2    |    key=3     |    key=4     |     |
              |            |    nVals=3   |    nVals=10  |     |
              |            |    values=   |    values=   |     |
              |            |      0x62323 |      0x68ec3 |     |
              |------------|--------------|--------------|     |
              | NULL       | NULL         | NULL         |     |
              --------------------------------------------     |
                                                               |
                                                               |
                                --------------------------------------------       
                                | isLeaf=1   | kNodes[0]:   | kNodes[1]:   |
                                | nKeys=2    |    key=6     |    key=7     |
                                |            |    nVals=4   |    nVals=8   |
                                |            |    values=   |    values=   |
                                |            |      0x52323 |      0x58ec3 |
                                |------------|--------------|--------------|
                                | NULL       | NULL         | NULL         |
                                --------------------------------------------

            Here, you see a simple 2-3 tree that addresses some important
            points. In general, one can view the key_nodes as being within
            the node, and the pointers to children as being along the
            divider between two key_nodes. This makes it obvious why there
            can be one more pointer to a kid than key_node.
            Also observe that the keys are in ascending order from left to
            right, although they are vertically separate. This idea is crucial
            in traversal.
        3)  Another important point is how insertion works. In general,
            an insertion operation would begin at a leaf node, and if after
            insertion, the nKeys exceeds the max, the node would split, and
            push a key_node to the parent node. If this parent node now also had
            too many key_nodes, then another split would happen, and
            would recursively travel up the tree, perhaps making a new root
            and extending the depth if necessary. 
            Since we don't have room for more than MAX_KEYS key nodes, instead
            we proactively split nodes, using the nifty splitNodes function.
            Essentially, as we travel down the tree, if the next node we 
            are to visit ever is full, before visiting it, we split it, and 
            add a new key to its parent (who has room for this key because we
            would have already split it one step before on our way down). We
            then cannot simply travel to the original child, because it's new
            brother might actually be the node we want to go to, so we 
            reinvestigate from the parent. 
            This proactive splitting ensures that leaf nodes will have room if
            a key_node needs to be inserted. Thus, to insert
            into a leaf node, one simply pushes the key nodes greater than
            the new insert one place to the right (there is guaranteed to be
            room for this), and adds the new key_node.
 *============================================================================*/


/*============================================================================
 * TYPES
 *
 *   These types are defined in the implementation file so that they can
 *   be kept hidden to code outside this source file.  This is not for any
 *   security reason, but rather just so we can enforce that our testing
 *   programs are generic and don't have any access to implementation details.
 *============================================================================*/

#define MAX_KEYS (500) /* How many key_nodes fit in a node, 50 is arbitrary */
#define LINE_SIZE (64) /* the size of a cache line in bytes */

typedef int multimap_value; /* just for readability */

typedef struct key_node /* see README */
{
    int key;
    int nVals; 
    multimap_value *values;
} key_node;

typedef struct mm_node  /* see README */
{
    int isLeaf;     /* is this a leaf */
    int nKeys;    /*  many keys does this node contain? */
    key_node kNodes[MAX_KEYS];
    struct mm_node *kids[MAX_KEYS + 1];  /* kids[i] has keys < kNodes[i].key */
} mm_node;

/* The entry-point of the multimap data structure. */
struct multimap 
{
    mm_node *root;
};



/*============================================================================
 * HELPER FUNCTION DECLARATIONS
 *
 *   Declarations of helper functions that are local to this module.  Again,
 *   these are not visible outside of this module.
 *============================================================================*/

/* allocate a single mm_node */
mm_node * alloc_node();

/* find the index of the first kNode with key > the argument key */
int searchInNode(mm_node *node, int key);

/* 
 * given a parent and the pos (which allows finding the child), will split
 * child into two separate nodes, and patch up parent to point to both this
 * child and the newly created splitoff of child
 */
void splitNode(mm_node *parent, int pos);

/*
 * will recursively search through a tree, splitting nodes as it goes along
 * if an insert is requested, and will either find the kNode searched for,
 * or insert this new kNode in the appropriate place
 */
key_node * searchAndInsert(mm_node *node, int key, int create_if_not_found);

/* does the same thing as find_mm_node in mm_impl.c */
key_node * find_node(multimap *mm, int key, int create_if_not_found);

/* free's an entire subtree starting at node */
void free_multimap_node(mm_node *node);

/* 
 * This is a helper function for mm_traverse_helper that traverses just the
 * values for a single key node.  
 */
void kNode_traverse(key_node *kNodePtr, void (*f)(int key, int value));



/*============================================================================
 * FUNCTION IMPLEMENTATIONS
 *============================================================================*/

/* 
 * Allocates a multimap node, and zeros out its contents so that we know what
 * the initial value of everything will be. Also, explicitly sets nKeys to be 0.
 */
mm_node * alloc_node()
{    
    mm_node *node = (mm_node *) malloc(sizeof(mm_node));
    bzero(node, sizeof(mm_node));
    node->nKeys = 0;
    return node;
}


/* 
 * Search within a node to find the index of the first key_node with a key
 * greater than the key passed as an argument. If all the keys in a node are
 * less than the query key, will return the node's nKeys. This is useful
 * for figuring out which subtree to look through in a search or insert.
 */
int searchInNode(mm_node *node, int key)
{
    for (int i = 0; i < node->nKeys; i++)
    {
        if (key <= node->kNodes[i].key)
        {
            return i;
        }
    }
    return node->nKeys;
}


/*
 * The "key" (haha, see what I did there) to the insert operation. Given a
 * parent node, and the position of the child subtree (an index from 0 to
 * nKeys), will take the middle keyNode of the child, move it to parent,
 * and split the child node with the second half of its kNodes being placed
 * in the new splitoff. Will also add a pointer to the splitoff to parent.
 *
 * Visually (* is empty/NULL, numbers are keys, a line is a level of tree)
 * 
 * original tree:
 *         4      6      *                 -- parent
 *       /     |     |     \
 *     0 1 2   5     *      *              -- children
 *   
 * to add key 3 to this tree, first call splitNode(parent, 0):
 *     step 1, shift things in the parent:
 *        result:
 *           *       4       6                -- parent
 *          /    |       |     \
 *        0 1 2  *       5      *             -- children
 *
 *     step 2, update the keys and kids of the parent: 
 *        result:
 *           1       4       6                -- parent
 *          /    |       |     \
 *        0 1 2 (*)      5      *             -- children, (*) is new, empty kid
 *     step 3, update the newly created "younger" node:
 *        result:
 *           1       4       6                -- parent
 *          /    |       |     \
 *        0 1 2  2       5      *             -- children
 *     step 4, update the "elder" node (0 out useless stuff):
 *        result:
 *           1       4       6                -- parent
 *          /    |       |     \
 *        0      2       5      *             -- children
 * Now, we can add 3:
 * final:
 *        1       4       6                -- parent
 *       /    |       |     \
 *     0     2 3      5      *             -- children
 */
void splitNode(mm_node *parent, int pos)
{
    mm_node *elder = parent->kids[pos]; /* child to be split */
    mm_node *younger = alloc_node(); /* child made from split */
    
    /* 
     * Here, we shift the kids pointers and key nodes in the parent down 1,
     * from the position that the new key (from elder) will be added.
     * Note that due to proactive splitting, pos will always be
     * < MAX_KEYS - 1, and since kNodes has length MAX_KEYS, while kids
     * has length MAX_KEYS + 1, there is never an invalid memory access, and
     * there is always room for this shift down by 1 operation.
     * 
     * Step 1 in the little visual aid above.
     */
    memmove(&parent->kNodes[pos + 1], &parent->kNodes[pos], 
                                    sizeof(key_node) * (parent->nKeys - pos));
    memmove(&parent->kids[pos + 2], &parent->kids[pos + 1], 
                                    sizeof(mm_node *) * (parent->nKeys - pos));

    /*
     * Find where to break off elder, move the kNode there to parent, and
     * add a pointer to the new child (younger) to the parent after the
     * new kNode.
     *
     * Step 2
     */
    int mid = elder->nKeys / 2;
    parent->kNodes[pos] = elder->kNodes[mid];
    parent->kids[pos + 1] = younger;
    parent->nKeys++;
    assert(!(parent->nKeys > MAX_KEYS));
    
    /* 
     * Move the appropriate key nodes to the younger node, update its other
     * fields.
     * 
     * Step 3
     */
    younger->nKeys = elder->nKeys - (mid + 1);
    memmove(younger->kNodes, &elder->kNodes[mid + 1], 
                                    sizeof(key_node) * (younger->nKeys));
    younger->isLeaf = elder->isLeaf;
    if (!(younger->isLeaf))
    {
        memmove(younger->kids, &elder->kids[mid + 1], 
                                    sizeof(mm_node *) * (younger->nKeys + 1));
    }

    /*
     * Update the elder's fields, zeroing out the things moved to younger to
     * avoid confusion later on
     *
     * Step 4
     */
    elder->nKeys = mid;
    bzero(&elder->kNodes[mid], sizeof(key_node) * (younger->nKeys + 1));
    if (!(elder->isLeaf))
    {
        bzero(&elder->kids[mid + 1], sizeof(mm_node *) * (younger->nKeys + 1));
    }
}


key_node * searchAndInsert(mm_node *node, int key, int create_if_not_found)
{
    /* look for smallest position that key fits below */
    int pos = searchInNode(node, key);

    if (pos < node->nKeys && node->kNodes[pos].key == key) 
    {
        return &node->kNodes[pos];
    } 
    if (node->isLeaf)
    {
        if (create_if_not_found)
        {
            /* should have space cuz proactive splitting */
            memmove(&node->kNodes[pos + 1], &node->kNodes[pos], 
                                    sizeof(key_node) * (node->nKeys - pos));
            bzero(&node->kNodes[pos], sizeof(key_node));
            node->kNodes[pos].key = key;
            node->nKeys++;
            assert(!(node->nKeys > MAX_KEYS));
            return &node->kNodes[pos];
        }
        return NULL;
    }
    mm_node * nextNode = node->kids[pos];
    if (create_if_not_found)
    {
        if (nextNode->nKeys == MAX_KEYS)
        {
            splitNode(node, pos);
            nextNode = node; /* Must re-examine since tree modified */
        }
    }
    return searchAndInsert(nextNode, key, create_if_not_found);
}


/* This helper function searches for the key node that contains the
 * specified key.  If such a node doesn't exist, the function can initialize
 * a new key node and add this into a multimap node, initializing that
 * too if necessary; the function might also simply return NULL.
 * Most of the searching and inserting legwork is done in searchAndInsert,
 * this function primarily handles edge cases involving the root, before
 * calling the helper.
 */
key_node * find_node (multimap *mm, int key, int create_if_not_found)
{
    mm_node *node;

    /* edge case where tree does not exist */
    if (mm->root == NULL)
    {
        if (create_if_not_found)
        {
            mm->root = alloc_node();
            node = mm->root;
            node->isLeaf = 1;
            node->kNodes[0].key = key;
            node->nKeys++;
            assert(!(node->nKeys > MAX_KEYS));
        }
        return &mm->root->kNodes[0];
    }
    
    node = mm->root;

    /* 
     * edge case where the root is a full node, and a key might
     * potentially be inserted. In keeping with the proactive splitting
     * strategy, generate a new root and extend the tree height. In fact,
     * this is the only way to extend the tree depth
     */
    if (node->nKeys == MAX_KEYS)
    {
        if (create_if_not_found)
        {
            mm->root = alloc_node();
            mm->root->isLeaf = 0;
            mm->root->kids[0] = node;
            splitNode(mm->root, 0);
            node = mm->root; /* re-examine from root since tree was changed */
        }
    }
    return searchAndInsert(node, key, create_if_not_found);
}


/*
 * Free a subtree of a multimap starting at the node "node". Essentially moves
 * in the same way as the traversal (same idea of visit 0th kid, 0th kNode,
 * then 1st kid, 1st kNode, and so on).
 */
void free_multimap_node(mm_node *node)
{
    for (int i = 0; i < node->nKeys; i++) 
    {
        if (!(node->isLeaf)) 
        {
            free_multimap_node(node->kids[i]);
        }
        free(node->kNodes[i].values);
    }

    /* Again, one more subtree at the far right of a node after all values */
    if (!(node->isLeaf)) 
    {
        free_multimap_node(node->kids[node->nKeys]);
    }
    free(node);
}


/* Initialize a multimap data structure. */                                     
multimap * init_multimap() 
{                                                    
    multimap *mm = malloc(sizeof(multimap));
    mm->root = NULL;
    return mm;
}


/* Frees the contents of a whole multimap (not the multimap itself though) */
void clear_multimap(multimap *mm)
{
    assert(mm != NULL);
    if (mm->root != NULL)
    {
        free_multimap_node(mm->root);
    }
    mm->root = NULL;
}


/* Adds the specified (key, value) pair to the multimap. */
void mm_add_value(multimap *mm, int key, int value) 
{
    assert(mm != NULL);

    /* Look up the key node with the specified key.  Create if not found. */
    key_node *kNodePtr = find_node(mm, key, /* create */ 1);
 
    assert(kNodePtr != NULL); 
    assert(kNodePtr->key == key);

    
    /* 
     * figure out how much space is left in the values array for this key node
     * and realloc to extend the array if more space is needed. This works
     * on the premise that space for values array is always an integer multiple
     * of the cache line size (to help with caching), so spaceAlloced is first
     * integer multiple of line size more than spaceTaken. (Could also do this
     * with the ceiling function, but this is almost as simple)
     */
    int spaceTaken = kNodePtr->nVals * sizeof(multimap_value);
    int spaceAlloced = 0;
    while (spaceAlloced < spaceTaken)
    {
        spaceAlloced += LINE_SIZE;
    }
    if (spaceAlloced - spaceTaken < sizeof(multimap_value))
    {
        kNodePtr->values = (multimap_value *) realloc(kNodePtr->values, 
                                                      spaceAlloced + LINE_SIZE);
    }

    /* Add the new value to the key node. */
    kNodePtr->values[kNodePtr->nVals] = value;
    kNodePtr->nVals++;
}


/* 
 * Returns nonzero if the multimap contains the specified key-value, zero
 * otherwise.
 */
int mm_contains_key(multimap *mm, int key) {
    return find_node(mm, key, /* create */ 0) != NULL;
}


/* 
 * Returns nonzero if the multimap contains the specified (key, value) pair,
 * zero otherwise.
 */
int mm_contains_pair(multimap *mm, int key, int value) 
{
    /* Is the right key_node even there? */
    key_node *kNodePtr = find_node(mm, key, /* create */ 0);
    if (kNodePtr == NULL)
    {
        return 0;
    }

    /* if it is, is the right value in that key node? */
    multimap_value *curr = kNodePtr->values;
    for (int i = 0; i < kNodePtr->nVals; i++)
    {
        if (*curr == value)
        {
            return 1;
        }
        curr++;
    }
    return 0;
}


/* This is a helper function for mm_traverse_helper that traverses just the
 * values for a single key node. Essentially, the way traversal works is
 * every key node is visited in order, and within each key node, every
 * value is visited. 
 * A clearer way of explaining this:
 * 
 * for kNode in tree:
 *      for value in kNode:
 *          do f(kNode's key, value)
 *
 * This helper function handles the inner level of this loop.
 */
void kNode_traverse(key_node *kNodePtr, void (*f)(int key, int value))
{
    multimap_value *curr = kNodePtr->values;
    for (int i = 0; i < kNodePtr->nVals; i++)
    {
        f(kNodePtr->key, *curr);
        curr++;
    }
}
    
 
/* 
 * This helper function is used by mm_traverse() to traverse every pair within
 * the multimap.
 */
void mm_traverse_helper(mm_node *node, void (*f)(int key, int value)) 
{
    /* 
     * To traverse, go from left to right looking first at leftmost subtree,
     * then the key node right after the subtree, then next subtee, then next
     * key node, etc.     
     */
    for (int i = 0; i < node->nKeys; i++)
    {
        if (!(node->isLeaf))
        {
            mm_traverse_helper(node->kids[i], f);
        }
        kNode_traverse(&node->kNodes[i], f);
    }

    /* At the end, one subtree after all key nodes, so look at that too */
    if (!(node->isLeaf))
    {
        mm_traverse_helper(node->kids[node->nKeys], f);
    }
}


/* 
 * Performs an in-order traversal of the multimap, passing each (key, value)
 * pair to the specified function.
 */
void mm_traverse(multimap *mm, void (*f)(int key, int value)) 
{
    mm_traverse_helper(mm->root, f);
}
    

    

