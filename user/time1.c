// time1: run a command and report its elapsed (wall-clock) time in ticks.

#include "kernel/types.h"
#include "user/user.h"

int
main(int argc, char *argv[])
{
  int start, pid, status;

  if (argc < 2) {
    fprintf(2, "usage: time1 command [args ...]\n");
    exit(1);
  }

  start = uptime();

  pid = fork();
  if (pid < 0) {
    fprintf(2, "time1: fork failed\n");
    exit(1);
  }
  if (pid == 0) {
    // argv is null-terminated, so argv + 1 is the command's own argv.
    exec(argv[1], argv + 1);
    fprintf(2, "time1: exec %s failed\n", argv[1]);
    exit(1);
  }

  if (wait(&status) < 0) {
    fprintf(2, "time1: wait failed\n");
    exit(1);
  }
  if (status != 0)
    printf("command exited with non-zero status %d\n", status);
  printf("elapsed time: %d ticks\n", uptime() - start);
  exit(0);
}
