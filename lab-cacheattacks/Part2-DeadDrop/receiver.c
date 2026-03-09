
#include"util.h"
// mman library to be used for hugepage allocations (e.g. mmap or posix_memalign only)
#include <sys/mman.h>
#define BUFF_SIZE 1048576
#define THRESHOLD 250  // Median latency based on Part 1


int main(int argc, char **argv)
{
	// Put your covert channel setup code here
	// Allocate a buffer using huge page
	// See the handout for details about hugepage management
	void *buf= mmap(NULL, BUFF_SIZE, PROT_READ | PROT_WRITE, MAP_POPULATE | MAP_ANONYMOUS | MAP_PRIVATE | MAP_HUGETLB, -1, 0);
	
	if (buf == (void*) - 1) {
		perror("mmap() error\n");
		exit(EXIT_FAILURE);
	}
	// The first access to a page triggers overhead associated with
	// page allocation, TLB insertion, etc.
	// Thus, we use a dummy write here to trigger page allocation
	// so later access will not suffer from such overhead.
	*((char *)buf) = 1; // dummy write to trigger page allocation


	void *sets[256]; // 256 cache sets we'll check
	for (int i = 0; i < 256; i++) {
		sets[i] = (char*) buf + (i*4096);
	}

	

	int last_idx = -1;
	int consecutive_hits = 0;

	printf("Please press enter.\n");

	char text_buf[2];
	fgets(text_buf, sizeof(text_buf), stdin);

	printf("Receiver now listening.\n");

	bool listening = true;
	while (listening) {
		
		// Put your covert channel code here
		uint32_t largest_latency = 0;
		int slowest_idx = 0;

		// Prime the sets
		for (int set = 0; set < 256; set++) {
			for (int way = 0; way < 16; way++) {
				*((char*)sets[set]) = 1;
			}
		}

		for (int i = 0; i < 256; i++) {
			uint32_t latency = measure_one_block_access_time((ADDR_PTR) sets[i]);
			if (latency > largest_latency) {
				largest_latency = latency;
				slowest_idx = i;
			}
		}
		if (largest_latency > THRESHOLD) {
			if (slowest_idx == last_idx) { // The same cache set has the most latency
				consecutive_hits++;
			}
			else {
				consecutive_hits = 1;
				last_idx = slowest_idx;
			}
			if (consecutive_hits > 2) {
				printf("%d\n", slowest_idx);
				listening = false;
			}
		}
			
	}
	printf("Receiver finished.\n");

	return 0;
}


