// prioritytest: check getpriority(), setpriority(), priority
// inheritance across fork(), and getprocs() error handling.

#include "kernel/param.h"
#include "kernel/types.h"
#include "kernel/pstat.h"
#include "user/user.h"

static int failures;

static void
check(char *what, int got, int want)
{
  printf("%s: %d (expected %d) %s\n", what, got, want,
         got == want ? "ok" : "FAIL");
  if (got != want)
    failures++;
}

int
main(void)
{
  int fds[2], pid, pspid, status;
  char c;

  printf("initial priority: %d\n", getpriority());
  check("setpriority(25)", setpriority(25), 0);
  check("getpriority()", getpriority(), 25);
  check("setpriority(-1)", setpriority(-1), -1);
  check("setpriority(50)", setpriority(50), -1);
  check("priority after bad calls", getpriority(), 25);
  check("setpriority(49)", setpriority(49), 0);
  check("setpriority(0)", setpriority(0), 0);
  check("getprocs(bad pointer)", getprocs((struct pstat *)0xffffffffff), -1);
  setpriority(25);

  if (pipe(fds) < 0) {
    fprintf(2, "prioritytest: pipe failed\n");
    exit(1);
  }
  pid = fork();
  if (pid < 0) {
    fprintf(2, "prioritytest: fork failed\n");
    exit(1);
  }
  if (pid == 0) {
    // Report the inherited priority, then switch to 10 and stay
    // alive until the parent has run ps.
    close(fds[0]);
    c = getpriority();
    setpriority(10);
    write(fds[1], &c, 1);
    pause(1000);
    exit(0);
  }
  close(fds[1]);
  if (read(fds[0], &c, 1) != 1) {
    fprintf(2, "prioritytest: child did not report\n");
    exit(1);
  }
  check("child inherited priority", c, 25);

  printf("ps (parent should show 25, child 10):\n");
  pspid = fork();
  if (pspid < 0) {
    fprintf(2, "prioritytest: fork failed\n");
    exit(1);
  }
  if (pspid == 0) {
    char *argv[] = { "ps", 0 };
    exec("ps", argv);
    fprintf(2, "prioritytest: exec ps failed\n");
    exit(1);
  }
  if (wait(&status) != pspid || status != 0) {
    fprintf(2, "prioritytest: ps failed\n");
    failures++;
  }

  kill(pid);
  wait(0);
  printf("%s\n", failures ? "SOME TESTS FAILED" : "all priority tests passed");
  exit(failures ? 1 : 0);
}
