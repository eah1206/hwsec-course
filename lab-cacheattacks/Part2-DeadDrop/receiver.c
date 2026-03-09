
#include"util.h"
// mman library to be used for hugepage allocations (e.g. mmap or posix_memalign only)
#include <sys/mman.h>
#define BUFF_SIZE 1048576
#define THRESHOLD 200  // Median latency based on Part 1


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


	void *sets[8]; // 8 cache sets for 8 bits
	for (int i = 0; i < 8; i++) {
		sets[i] = (char*) buf + (i*4096);
	}
	int last_value = -1;
	int consecutive_hits = 0;

	printf("Please press enter.\n");

	char text_buf[2];
	fgets(text_buf, sizeof(text_buf), stdin);

	printf("Receiver now listening.\n");

	bool listening = true;
	while (listening) {
		

		// Put your covert channel code here
		int bits[8] = {0};
		for (int i = 0; i < 8; i++) {
			uint32_t latency = measure_one_block_access_time((ADDR_PTR) sets[i]);
			if (latency > THRESHOLD) {
				bits[i] = 1;
			}
		}
		int value = 0;
		for (int i = 0; i < 8; i++) {
			if (bits[i]) value |= (1 << i);
		}
		if (value == last_value) {
			consecutive_hits++;
		} else {
			consecutive_hits = 1;
			last_value = value;
		}
		if (consecutive_hits > 2) {
			printf("%d\n", value);
			listening = false;
		}
	}
	printf("Receiver finished.\n");

	return 0;
}


