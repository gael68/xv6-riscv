#include "kernel/param.h"
#include "kernel/types.h"
#include "kernel/pstat.h"
#include "user/user.h"

int
main(int argc, char **argv)
{
  struct pstat uproc[NPROC];
  int nprocs;
  int i;
  char *state;
  static char *states[] = {
    [USED]      "used    ",
    [SLEEPING]  "sleeping",
    [RUNNABLE]  "runnable",
    [RUNNING]   "running ",
    [ZOMBIE]    "zombie  "
  };

  nprocs = getprocs(uproc);
  if (nprocs < 0) {
    fprintf(2, "ps: getprocs failed\n");
    exit(1);
  }

  printf("pid\tstate\t\tsize\tprio\tppid\tname\n");
  for (i=0; i<nprocs; i++) {
    state = states[uproc[i].state];
    printf("%d\t%s\t%ld\t%d\t%d\t%s\n", uproc[i].pid, state,
                   uproc[i].size, uproc[i].priority, uproc[i].ppid,
                   uproc[i].name);
  }

  exit(0);
}

