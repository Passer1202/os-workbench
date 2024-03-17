
#include <stdio.h>
#include <assert.h>
#include <unistd.h>
#include <getopt.h>

const struct option opt_table[]={
  {"numeric-sort",no_argument,NULL,'n'},
  {"show-pids",no_argument,NULL,'p'},
  {"version",no_argument,NULL,'V'},
  {0,0,NULL,0},
};

int nf;
int pf;
int vf;

int main(int argc, char *argv[]) {
  //for (int i = 0; i < argc; i++) {
  //  assert(argv[i]);
  //  printf("argv[%d] = %s\n", i, argv[i]);
  //}
  //assert(!argv[argc]);
  int opt;
  while((opt=getopt_long(argc,argv,"-npV",opt_table,NULL))!=-1){
    switch (opt)
    {
    case 'n':
      nf=1;
      break;
    case 'p':
      pf=1;
      break;
    case 'V':
      vf=1;
      break;
    case 0:
      break;
    case '?':
      printf("pstree: invalid option -- '%c'\n",optopt);
      break;
    default:
      printf("pstree: invalid option -- '%c'\n",optopt);
    }
  }

  printf("nf=%d, pf=%d, vf=%d\n", nf, pf, vf);

  return 0;
}
