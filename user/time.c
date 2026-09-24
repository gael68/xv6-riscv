// time: run a command and report its elapsed time, CPU time, and %CPU.

#include "kernel/types.h"
#include "kernel/pstat.h"
#include "user/user.h"

int
main(int argc, char *argv[])
{
  int start, elapsed, pid, pct, status;
  struct rusage ru;

  if (argc < 2) {
    fprintf(2, "usage: time command [args ...]\n");
    exit(1);
  }

  start = uptime();

  pid = fork();
  if (pid < 0) {
    fprintf(2, "time: fork failed\n");
    exit(1);
  }
  if (pid == 0) {
    // argv is null-terminated, so argv + 1 is the command's own argv.
    exec(argv[1], argv + 1);
    fprintf(2, "time: exec %s failed\n", argv[1]);
    exit(1);
  }

  if (wait2(&status, &ru) < 0) {
    fprintf(2, "time: wait2 failed\n");
    exit(1);
  }
  if (status != 0)
    printf("command exited with non-zero status %d\n", status);
  elapsed = uptime() - start;

  // Avoid dividing by zero for commands that finish within one tick.
  pct = elapsed > 0 ? ru.cputime * 100 / elapsed : 0;
  printf("elapsed time: %d ticks, cpu time: %d ticks, %d%% CPU\n",
         elapsed, ru.cputime, pct);
  exit(0);
}
