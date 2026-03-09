
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
  void *sets[8];
  for (int i = 0; i < 8; i++) {
    sets[i] = (char*) buf + (i*4096);
  }

  printf("Please type a message.\n");

  bool sending = true;
  while (sending) {
      char text_buf[128];
      fgets(text_buf, sizeof(text_buf), stdin);

      // TODO:
      // Put your covert channel code here
      int value = string_to_int(text_buf);
      for (int i = 0; i < 8; i++) {
          if (value & (1 << i)) {
              for (int j = 0; j < 1500; j++) {
                  *((char*)sets[i]) = 1;
              }
          }
      }
  }

  printf("Sender finished.\n");
  return 0;
}


