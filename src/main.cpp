#include "runner.hpp"
#include "evals.hpp"
#include "tasks.hpp"
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

int main(int argc, char**argv) {
  //rankFeatures();
  //evalNormalizeRigid();
  //evalTasks();
  //bruteSubmission();
  //bruteSolve();
  int only_sid = -1;

  // if (argc == 1) {
  //   if (argv[1][0] == 'e') {
  //     evalEvals(1);
  //   }
  //   else if (argv[1][0] == 't') {
  //     evalTasks();
  //   }
  //   else {
  //     exit(0);
  //   }
  // }

  if (argc >= 2) {
    only_sid = atoi(argv[1]);
    printf("Running only task # %d\n", only_sid);
  }
  int maxdepth = -1;
  if (argc >= 3) {
    maxdepth = atoi(argv[2]);
    printf("Using max depth %d\n", maxdepth);
  }
  int mindepth = -1;
  if (argc >= 4) {
    mindepth = atoi(argv[3]);
    printf("Using min depth %d\n", mindepth);
  }
  int force_func = -1;
  if (argc >= 5) {
    force_func = atoi(argv[4]);
    printf("Forcing func %d\n", force_func);
  }
  run(only_sid, maxdepth, mindepth, force_func);
}
