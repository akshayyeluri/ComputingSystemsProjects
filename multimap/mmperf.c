#include <assert.h>
#include <stdio.h>
#include <stdlib.h>

#include "multimap.h"
#include "realtime.h"

/* Set this to 1 to turn on extremely verbose output. */
#define VERBOSE 0

/* Various populate-mode values used by populate_multimap() to generate keys. */
#define MODE_RAND 0
#define MODE_INCR 1
#define MODE_DECR 2

/* This value can be increased or decreased baased on how fast or slow the
 * performance tests run on your machine.
 */
#define SCALE 5

/* If the last tests are really slow, they can be temporarily excluded by
 * setting this value to 1.  Make sure to set this to 0 before testing for
 * the assignment.
 */
#define EXCLUDE_SLOW_TESTS 0


/* Populate the multimap with a specific number of key/value pairs.  The keys
 * can be generated in one of three ways, either randomly, incrementing, or
 * decrementing.
 */
void populate_multimap(multimap *mm, int num_pairs, int keygen_mode,
                       int max_key, int max_val) {
    int i, key, value;

    assert(mm != NULL);
    assert(num_pairs > 0);
    assert(keygen_mode >= 0 && keygen_mode <= 2);
    assert(max_key > 0);
    assert(max_val > 0);

    printf("Adding %d randomly generated pairs to multimap.\n", num_pairs);
    printf("Keys in range [0, %d), values in range [0, %d).\n",
           max_key, max_val);

    /* Add a bunch of (key, value) pairs to the multimap. */
    for (i = 0; i < num_pairs; i++) {
        if (keygen_mode == MODE_RAND) {
            key = rand() % max_key;
        }
        else if (keygen_mode == MODE_INCR) {
            /* Start keys at 0, and increment the key for each insertion.
             * Wrap around to 0 once we pass max_key.
             */
            if (i == 0)
                key = 0;
            else
                key = (key + 1) % (max_key + 1);
        }
        else {
            assert(keygen_mode == MODE_DECR);

            /* Start keys at max_key, and decrement the key for each insertion.
             * Wrap around to max_key once we hit 0.
             */
            if (i == 0)
                key = max_key;
            else
                key = (key + max_key) % (max_key + 1);
        }

        /* Always generate a random value to add. */
        value = rand() % max_val;

#if VERBOSE
        printf("Adding:  (%d, %d)\n", key, value);
#endif
        mm_add_value(mm, key, value);
    }
}


/* Performs a fixed number of probes against the multimap, choosing random keys
 * and values to use in probing.  The function returns how many probes were
 * found in the map.
 */
int probe_multimap(multimap *mm, int num_probes, int max_key, int max_val) {
    int i, key, value, in_map, total;

    assert(mm != NULL);
    assert(num_probes > 0);
    assert(max_key > 0);
    assert(max_val > 0);

    printf("Probing multimap %d times with randomly generated test-pairs.\n",
           num_probes);
    printf("Keys in range [0, %d), values in range [0, %d).\n",
           max_key, max_val);

    /* Probe the multimap with a bunch of (key, value) pairs. */
    for (i = 0, total = 0; i < num_probes; i++) {
        key = rand() % max_key;
        value = rand() % max_val;

        in_map = mm_contains_pair(mm, key, value);
        if (in_map)
            total++;

#if VERBOSE
        printf("Probing:  (%d, %d) is%s in the multimap.\n", key, value,
               in_map ? "" : " NOT");
#endif
    }

    return total;
}


/* Performs a single performance test against the multimap:
 *   1)  Generates key/value pairs to add to the map, using either incrementing,
 *       decrementing, or random key generation, and the specified maximum key
 *       and value parameters.
 *
 *   2)  Performs the specified number of probes, measuring the total wall-clock
 *       time that is required to perform the test.  This is not a particularly
 *       accurate way to measure the performance, but it should work well enough.
 */
void test_multimap_perf(int num_pairs, int num_probes, int keygen_mode,
                        int max_key, int max_val) {
    multimap *mm;
    struct timespec ts;
    int total_hits;
    long long int start_us, end_us;
    double total_seconds, us_per_probe;
    const char *mode_str[] = { "random", "incrementing", "decrementing" };

    assert(keygen_mode >= 0 && keygen_mode <= 2);

    printf("Testing multimap performance:  %d pairs, %d probes, %s keys.\n",
           num_pairs, num_probes, mode_str[keygen_mode]);

    /* Initialize the multimap data structure. */
    mm = init_multimap();

    populate_multimap(mm, num_pairs, keygen_mode, max_key, max_val);

    clock_get_realtime(&ts);
    start_us = (ts.tv_sec * 1000000) + (ts.tv_nsec / 1000);

    total_hits = probe_multimap(mm, num_probes, max_key, max_val);

    clock_get_realtime(&ts);
    end_us = (ts.tv_sec * 1000000) + (ts.tv_nsec / 1000);

    printf("%d out of %d test-pairs were in the map (%.1f%%)\n",
           total_hits, num_probes,
           (double) total_hits * 100.0 / (double) num_probes);

    total_seconds = (double) (end_us - start_us) / 1000000.0;
    us_per_probe = (double) (end_us - start_us) / (double) num_probes;
    printf("Total wall-clock time:  %.2f seconds\t\t\u03BCs per probe:"
           "  %.3f \u03BCs\n\n", total_seconds, us_per_probe);

    /* Free it!  We're done. */
    clear_multimap(mm);
}


int main(int argc, char **argv) {
    srand((argc == 2) ? atoi(argv[1]) : 11);

    printf("This program measures multimap read performance by doing the"
           " following, for\n");
    printf("various kinds of usage patterns:\n\n");
    printf(" * First, a large number of randomly generated (key, value)"
           " pairs are inserted\n");
    printf("   into an empty multimap.\n\n");
    printf(" * Next, more (key, value) pairs are randomly generated,"
           " and the multimap is\n");
    printf("   probed to see if each pair is in the map.  The amount of"
           " wall-clock time\n");
    printf("   taken for this step is measured and used to"
           " estimate the time-per-probe\n");
    printf("   of the map.\n\n");
    printf(" * Finally, the program prints out how many of the generated"
           " (key, value) pairs\n");
    printf("   were in the map.  This number should not change regardless of"
           " optimizations\n");
    printf("   to the data structure, because the same random seed is always"
           " used at the\n");
    printf("   start of the program.\n\n");

    /* Arguments:  num_pairs, num_probes, keygen_mode, max_key, max_value */

    test_multimap_perf(300000, SCALE * 100000, MODE_RAND, 50, 1000);
    test_multimap_perf(300000, SCALE * 100000, MODE_INCR, 50, 1000);
    test_multimap_perf(300000, SCALE * 100000, MODE_DECR, 50, 1000);

    test_multimap_perf(15000000, SCALE * 100000, MODE_RAND, 100000, 50);

#if EXCLUDE_SLOW_TESTS == 0
    test_multimap_perf(100000, SCALE * 5000, MODE_INCR, 100000, 50);
    test_multimap_perf(100000, SCALE * 5000, MODE_DECR, 100000, 50);
#endif

    return 0;
}

