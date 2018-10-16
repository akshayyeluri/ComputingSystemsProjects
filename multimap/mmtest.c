#include <stdio.h>
#include <stdlib.h>

#include "multimap.h"


int failures = 0;


int test_values[] = {
    1, 10,  /* key, value */
    2, 20,
    3, 30,
    2, 15,
    1, 40,
    4, 50,
    -1
};


int probe_values[] = {
    1, 40, 1,  /* key, value, answer */
    2, 20, 1,
    2, 40, 0,
    4, 50, 1,
    4, 30, 0,
    1, 10, 1,
    2, 15, 1,
    3, 20, 0,
    3, 30, 1,
    -1
};


int probe_keys[] = {
    2, 1,  /* key, answer */
    5, 0,
    1, 1,
    0, 0,
    30, 0,
    20, 0,
    4, 1,
    -1
};


int prev_key;

void check_order(int key, int value) {
    printf(" * (%d, %d)", key, value);

    if (prev_key != -1 && key < prev_key) {
        printf(" - OUT OF ORDER!");
        failures++;
    }
    prev_key = key;

    printf("\n");
}



int main() {
    multimap *mm;
    int i;

    failures = 0;

    /* Initialize the multimap data structure. */
    mm = init_multimap();

    printf("Adding test values to multimap.\n");
    for (i = 0; test_values[i] != -1; i += 2) {
        printf(" * (%d, %d)\n", test_values[i], test_values[i + 1]);
        mm_add_value(mm, test_values[i], test_values[i + 1]);
    }

    printf("\nProbing multimap for pairs.\n");
    for (i = 0; probe_values[i] != -1; i += 3) {
        int answer = probe_values[i + 2];
        int probe = mm_contains_pair(mm, probe_values[i], probe_values[i + 1]);

        printf(" * (%d, %d) should%s be present:  %s",
            probe_values[i], probe_values[i + 1], answer ? "" : " NOT",
            answer ? "    " : "");

        if ((probe && answer) || (!probe && !answer)) {
            printf("PASS");
        }
        else {
            printf("FAIL");
            failures++;
        }

        printf("\n");
    }

    printf("\nProbing multimap for keys.\n");
    for (i = 0; probe_keys[i] != -1; i += 2) {
        int answer = probe_keys[i + 1];
        int probe = mm_contains_key(mm, probe_keys[i]);

        printf(" * key %2d should%s be present:  %s",
            probe_keys[i], answer ? "" : " NOT", answer ? "    " : "");

        if ((probe && answer) || (!probe && !answer)) {
            printf("PASS");
        }
        else {
            printf("FAIL");
            failures++;
        }

        printf("\n");
    }

    printf("\nChecking traversal order.\n");
    prev_key = -1;
    mm_traverse(mm, check_order);

    printf("\nTesting finished, freeing multimap.\n");
    clear_multimap(mm);
    free(mm);

    printf("\nFinal results:  %d failures\n", failures);

    return 0;
}

