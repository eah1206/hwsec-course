#include "util.h"
#include <sys/mman.h>

// Size of our hugepage-backed buffer.
// 1 << 21 = 2MB. Hugepages are used so the lower address bits
// match physical memory bits, letting us control cache set mapping.
#define BUFF_SIZE       (1 << 21)

// Cache line size for this CPU (64 bytes).
// Each cache set index advances every 64 bytes.
#define CACHE_LINE      64

// From Part 1 of the lab: the L2 cache has 1024 sets.
#define L2_NUM_SETS     1024

// Distance between addresses that map to the SAME cache set.
// If we jump by (sets * line_size) we change the tag bits but
// keep the set index the same.
#define SET_SPAN        (L2_NUM_SETS * CACHE_LINE)

// Number of different congruent addresses used to hammer a set.
// More addresses increases eviction pressure on that set.
#define EVICT_LINES     32

// How long we hammer the chosen set (in CPU cycles).
#define BURST_CYCLES    50000000ULL

// Quiet period after sending to reduce duplicate detections.
#define QUIET_CYCLES    15000000ULL

// Read the CPU timestamp counter using the RDTSCP instruction.
// This gives a very precise cycle counter for timing.
static inline uint64_t rdtscp64(void) {
    uint32_t lo, hi;

    // rdtscp returns the lower 32 bits in eax and upper in edx
    asm volatile("rdtscp" : "=a"(lo), "=d"(hi) :: "rcx");

    // lfence ensures instructions don't reorder around this point
    asm volatile("lfence" ::: "memory");

    // combine high and low parts into one 64-bit timestamp
    return ((uint64_t)hi << 32) | lo;
}

// Busy-wait loop until the timestamp counter reaches "target".
// Used to create controlled idle periods.
static inline void wait_until(uint64_t target) {
    while (rdtscp64() < target) {
        asm volatile("pause");   // hint to CPU that we are spinning
    }
}

// Force a memory access to happen.
// The asm memory barrier prevents the compiler from optimizing it away.
static inline void touch_addr(volatile uint8_t *p) {
    asm volatile("" ::: "memory");
    (void)*p;   // read the address to bring the cache line into cache
}

// Calculate an address that maps to a particular L2 cache set.
//
// base      = start of hugepage buffer
// set_idx   = which cache set we want
// way_idx   = which congruent line for that set
//
// The formula manipulates address bits so the set index bits stay constant.
static volatile uint8_t *addr_for_set(volatile uint8_t *base, int set_idx, int way_idx) {
    return base + (size_t)set_idx * CACHE_LINE + (size_t)way_idx * SET_SPAN;
}

// Continuously access addresses that map to the chosen cache set.
// This evicts other cache lines in that set, creating contention
// detectable by the receiver.
static void hammer_set(volatile uint8_t *base, int set_idx, uint64_t end_tsc) {

    // Continue hammering until we reach the desired timestamp
    while (rdtscp64() < end_tsc) {

        // Access multiple congruent addresses for that set
        for (int i = 0; i < EVICT_LINES; i++) {

            // Slightly shuffle the order to avoid predictable patterns
            int j = (i * 5 + 3) % EVICT_LINES;

            // Access the address mapping to this cache set
            touch_addr(addr_for_set(base, set_idx, j));
        }
    }
}

int main(int argc, char **argv)
{
    // Allocate a 2MB hugepage buffer.
    // MAP_HUGETLB ensures the allocation is a hugepage.
    void *buf = mmap(NULL, BUFF_SIZE,
                     PROT_READ | PROT_WRITE,
                     MAP_POPULATE | MAP_ANONYMOUS | MAP_PRIVATE | MAP_HUGETLB,
                     -1, 0);

    // Check for allocation failure
    if (buf == (void *)-1) {
        perror("mmap() error");
        exit(EXIT_FAILURE);
    }

    volatile uint8_t *base = (volatile uint8_t *)buf;

    // Touch one byte per 4KB page so the OS actually allocates
    // the physical memory immediately rather than lazily later.
    for (size_t off = 0; off < BUFF_SIZE; off += 4096) {
        base[off] = 1;
    }

    printf("Please type a message.\n");

    bool sending = true;
    while (sending) {

        char text_buf[128];

        // Read a line of input from the user
        if (!fgets(text_buf, sizeof(text_buf), stdin)) {
            break;
        }

        // Convert the string to an integer
        int value = string_to_int(text_buf);

        // Only allow values 0–255 (one byte)
        if (value < 0 || value > 255) {
            printf("Please enter an integer from 0 to 255.\n");
            continue;
        }

        // Wait a small amount of time before sending
        uint64_t start = rdtscp64() + 5000000ULL;

        // End time for the hammering burst
        uint64_t end   = start + BURST_CYCLES;

        // Hammer the cache set equal to the value we want to send
        hammer_set(base, value, end);

        // Remain idle briefly to reduce repeated detections
        wait_until(end + QUIET_CYCLES);

        printf("Sent %d\n", value);
    }

    printf("Sender finished.\n");
    return 0;
}