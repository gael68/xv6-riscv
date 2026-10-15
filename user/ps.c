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
  uint now, age;
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

  now = uptime();

  printf("pid\tstate\t\tsize\tage\tprio\tcputime\tppid\tname\n");
  for (i=0; i<nprocs; i++) {
    state = states[uproc[i].state];
    // Age is how long a process has been waiting to run.
    age = uproc[i].state == RUNNABLE ? now - uproc[i].readytime : 0;
    printf("%d\t%s\t%ld\t%d\t%d\t%d\t%d\t%s\n", uproc[i].pid, state,
                   uproc[i].size, age, uproc[i].priority, uproc[i].cputime,
                   uproc[i].ppid, uproc[i].name);
  }

  exit(0);
}

