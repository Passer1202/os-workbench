
#include <stdio.h>
#include <assert.h>
#include <unistd.h>
#include <getopt.h>
int nf;
int pf;
int vf;


struct option opt_table[] = {{"show-pids", no_argument, &pf, 1},
                                  {"numeric-sort", no_argument, &nf, 1},
                                  {"version", no_argument, &vf, 1},
                                  {0, 0, 0, 0}};


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
    default:
      printf("pstree: invalid option -- '%s'\n", argv[optind-1]);
    }
  }

  printf("nf=%d, pf=%d, vf=%d\n", nf, pf, vf);

  return 0;
}
