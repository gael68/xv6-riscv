// pexec: run a command at the given priority.

#include "kernel/param.h"
#include "kernel/types.h"
#include "user/user.h"

int
main(int argc, char *argv[])
{
  int pid, status;
  char *s;

  if (argc < 3) {
    fprintf(2, "usage: pexec priority command [args ...]\n");
    exit(1);
  }

  // atoi() would quietly turn "abc" into priority 0.
  for (s = argv[1]; *s >= '0' && *s <= '9'; s++)
    ;
  if (s == argv[1] || *s != 0 || setpriority(atoi(argv[1])) < 0) {
    fprintf(2, "pexec: priority must be a number from 0 to %d\n", MAXPRIORITY);
    exit(1);
  }

  // The child inherits the priority just set.
  pid = fork();
  if (pid < 0) {
    fprintf(2, "pexec: fork failed\n");
    exit(1);
  }
  if (pid == 0) {
    exec(argv[2], argv + 2);
    fprintf(2, "pexec: exec %s failed\n", argv[2]);
    exit(1);
  }

  if (wait(&status) < 0) {
    fprintf(2, "pexec: wait failed\n");
    exit(1);
  }
  if (status != 0)
    printf("pexec: %s exited with status %d\n", argv[2], status);
  exit(status);
}
