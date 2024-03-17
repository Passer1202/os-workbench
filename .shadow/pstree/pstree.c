#include <stdio.h>
#include <assert.h>
#include <unistd.h>

const struct option opt_table[]={
  {"numeric-sort",no_argument,NULL,'n'},
  {"show-pids",no_argument,NULL,'p'},
  {"version",no_argument,NULL,'V'},
  {0,0,NULL,0},
};


int main(int argc, char *argv[]) {
  for (int i = 0; i < argc; i++) {
    assert(argv[i]);
    printf("argv[%d] = %s\n", i, argv[i]);
  }
  assert(!argv[argc]);
  return 0;
}
