
#include"util.h"
// mman library to be used for hugepage allocations (e.g. mmap or posix_memalign only)
#include <sys/mman.h>

// TODO: define your own buffer size
//#define BUFF_SIZE (1<<21)
#define BUFF_SIZE 1048576

int main(int argc, char **argv)
{
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


  // TODO:
  // Put your covert channel setup code here
  int gap = 4096; // By jumping 4096 bytes in memory, you end up at the same physical address on the same page.


  printf("Please type a message.\n");

  bool sending = true;
  while (sending) {
      char text_buf[128];
      fgets(text_buf, sizeof(text_buf), stdin);
      int message = atoi(text_buf);

      // TODO:
      // Put your covert channel code here
      for (int i = 0; i < 8; i++) {
        if ((message >> i ) & 1) {
          for (int ways = 0; ways < 16; ways ++) {
            char *addr = (char *)buf + (i * gap) + (ways * 64);
            *addr = 1;
          }
        }
      }

  printf("Sender finished.\n");
  return 0;
}


