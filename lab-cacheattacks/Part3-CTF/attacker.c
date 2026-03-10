#include "util.h"
// mman library to be used for hugepage allocations (e.g. mmap or posix_memalign only)
#include <sys/mman.h>
#define BUFF_SIZE (1<<21)

int main(int argc, char const *argv[]) {
    int flag = -1;

    // Put your capture-the-flag code here

    void *buf = mmap(NULL, BUFF_SIZE,
                     PROT_READ | PROT_WRITE,
                     MAP_POPULATE | MAP_ANONYMOUS | MAP_PRIVATE | MAP_HUGETLB,
                     -1, 0);

    // Check for allocation failure
    if (buf == (void *)-1) {
        perror("mmap() error");
        exit(EXIT_FAILURE);
    }

    // Prime all cache sets in the buffer

    // In a loop, probe all cache sets, checking for high latency hits

    // Yeah

    printf("Flag: %d\n", flag);
    return 0;
}
