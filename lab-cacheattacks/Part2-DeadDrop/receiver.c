#include "util.h"
#include <sys/mman.h>

// Size of hugepage buffer (2MB)
#define BUFF_SIZE        (1 << 21)

// Cache line size
#define CACHE_LINE       64

// Number of L2 cache sets
#define L2_NUM_SETS      1024

// Distance between addresses mapping to the same set
#define SET_SPAN         (L2_NUM_SETS * CACHE_LINE)

// We only expect values 0–255, so receiver only checks these sets
#define CANDIDATE_SETS   256

// Number of addresses used per set when probing
#define PROBE_LINES      2

// Number of times we prime the cache before measuring
#define PRIME_REPS       2

// Minimum difference between best and second-best score
// required to consider the result valid
#define DETECT_GAP       35

// Ignore weak signals below this threshold
#define ABS_MIN_SCORE    70

// Require the same result multiple rounds before printing
#define STABLE_ROUNDS    2

// Force a memory access (same as in sender)
static inline void touch_addr(volatile uint8_t *p) {
    asm volatile("" ::: "memory");
    (void)*p;
}

// Compute address mapping to a specific L2 cache set
static volatile uint8_t *addr_for_set(volatile uint8_t *base, int set_idx, int line_idx) {
    return base + (size_t)set_idx * CACHE_LINE + (size_t)line_idx * SET_SPAN;
}

// Prime the cache by accessing all candidate sets.
// This loads our lines into the cache before probing.
static void prime_all(volatile uint8_t *base) {
    for (int rep = 0; rep < PRIME_REPS; rep++) {
        for (int set = 0; set < CANDIDATE_SETS; set++) {
            for (int line = 0; line < PROBE_LINES; line++) {
                touch_addr(addr_for_set(base, set, line));
            }
        }
    }
}

// Measure how slow accesses to a given set are.
// A higher latency suggests that the sender evicted our lines.
static int score_set(volatile uint8_t *base, int set_idx) {
    int total = 0;

    // Access the probe lines and sum the access times
    for (int i = PROBE_LINES - 1; i >= 0; i--) {
        total += (int)measure_one_block_access_time((ADDR_PTR)addr_for_set(base, set_idx, i));
    }

    return total;
}

int main(int argc, char **argv)
{
    // Allocate hugepage buffer
    void *buf = mmap(NULL, BUFF_SIZE,
                     PROT_READ | PROT_WRITE,
                     MAP_POPULATE | MAP_ANONYMOUS | MAP_PRIVATE | MAP_HUGETLB,
                     -1, 0);

    if (buf == (void *)-1) {
        perror("mmap() error");
        exit(EXIT_FAILURE);
    }

    volatile uint8_t *base = (volatile uint8_t *)buf;

    // Force physical allocation of the hugepage
    for (size_t off = 0; off < BUFF_SIZE; off += 4096) {
        base[off] = 1;
    }

    printf("Please press enter.\n");

    char text_buf[2];
    fgets(text_buf, sizeof(text_buf), stdin);

    printf("Receiver now listening.\n");

    // Track candidate stability between rounds
    int last_candidate = -1;
    int stable_count = 0;
    bool locked = false;

    while (true) {

        int best_set = -1;
        int best_score = -1;
        int second_score = -1;

        // Step 1: Prime all sets into the cache
        prime_all(base);

        // Step 2: Probe all candidate sets and find the slowest one
        for (int set = 0; set < CANDIDATE_SETS; set++) {

            int s = score_set(base, set);

            if (s > best_score) {
                second_score = best_score;
                best_score = s;
                best_set = set;
            }
            else if (s > second_score) {
                second_score = s;
            }
        }

        // Determine whether the signal is strong enough
        bool active = (best_score >= ABS_MIN_SCORE) &&
                      ((best_score - second_score) >= DETECT_GAP);

        // If not strong, reset detection state
        if (!active) {
            last_candidate = -1;
            stable_count = 0;
            locked = false;
            continue;
        }

        // Track how many consecutive rounds the same set wins
        if (best_set == last_candidate) {
            stable_count++;
        } else {
            last_candidate = best_set;
            stable_count = 1;
        }

        // Only print when the same result appears multiple rounds
        if (!locked && stable_count >= STABLE_ROUNDS) {
            printf("%d\n", best_set);
            //fflush(stdout);
            locked = true;
        }
    }

    printf("Receiver finished.\n");
    return 0;
}