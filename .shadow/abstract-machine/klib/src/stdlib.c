#include <am.h>
#include <klib.h>
#include <klib-macros.h>

#if !defined(__ISA_NATIVE__) || defined(__NATIVE_USE_KLIB__)
static unsigned long int next = 1;

int rand(void) {
  // RAND_MAX assumed to be 32767
  next = next * 1103515245 + 12345;
  return (unsigned int)(next/65536) % 32768;
}

void srand(unsigned int seed) {
  next = seed;
}

int abs(int x) {
  return (x < 0 ? -x : x);
}

int atoi(const char* nptr) {
  int x = 0;
  while (*nptr == ' ') { nptr ++; }
  while (*nptr >= '0' && *nptr <= '9') {
    x = x * 10 + *nptr - '0';
    nptr ++;
  }
  return x;
}

void* stack_ptr() {
  void* sp;
  asm volatile("mov %%rsp, %0" : "=r"(sp));
  return sp;
}

void *malloc(size_t size) {
  // On native, malloc() will be called during initialization of C runtime.
  // Therefore do not call panic() here, else it will yield a dead recursion:
  //   panic() -> putchar() -> (glibc) -> malloc() -> panic()
#if !(defined(__ISA_NATIVE__) && defined(__NATIVE_USE_KLIB__))
  extern char end;
  static char *heap_end = &end;
  char *prev_heap_end;
  char *brk;

  if (size == 0) {
    return NULL;
  }

  prev_heap_end = heap_end;
  brk = prev_heap_end + size;

  if (brk > (char *)stack_ptr()) {
    panic("out of memory");
  }

  heap_end = brk;
  return prev_heap_end;

#endif
  return NULL;
}

void free(void *ptr) {
}

#endif
