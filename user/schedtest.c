// schedtest: start CPU-bound children at the same moment, each at its
// own priority, and report each child's response time (ticks until it
// first ran) and turnaround time (ticks until it finished).
//
// usage: schedtest work prio [prio ...]
//   work is the CPU time each child needs, in units of about one tick.

#include "kernel/param.h"
#include "kernel/types.h"
#include "kernel/pstat.h"
#include "user/user.h"

#define MAXKIDS 16
#define UNIT 40000000 // loop iterations in about one tick under QEMU

static int
isnumber(char *s)
{
  if (*s == 0)
    return 0;
  for (; *s; s++)
    if (*s < '0' || *s > '9')
      return 0;
  return 1;
}

// volatile so the compiler cannot delete the busy loop.
static volatile int sink;

static void
spin(int units)
{
  int i, j;

  for (i = 0; i < units; i++)
    for (j = 0; j < UNIT; j++)
      sink++;
}

int
main(int argc, char *argv[])
{
  int fds[2], pids[MAXKIDS], prio[MAXKIDS], starts[MAXKIDS];
  int i, n, work, pid, status, t0, start, done;
  int tat, totresp, tottat;
  struct rusage ru;

  if (argc < 3) {
    fprintf(2, "usage: schedtest work prio [prio ...]\n");
    exit(1);
  }
  if (!isnumber(argv[1]) || (work = atoi(argv[1])) <= 0) {
    fprintf(2, "schedtest: work must be a positive number\n");
    exit(1);
  }
  n = argc - 2;
  if (n > MAXKIDS) {
    fprintf(2, "schedtest: at most %d children\n", MAXKIDS);
    exit(1);
  }
  for (i = 0; i < n; i++) {
    if (!isnumber(argv[i + 2]) || (prio[i] = atoi(argv[i + 2])) > MAXPRIORITY) {
      fprintf(2, "schedtest: priority must be a number from 0 to %d\n",
              MAXPRIORITY);
      exit(1);
    }
  }

  // Run above the children so that we notice each exit right away.
  setpriority(MAXPRIORITY);
  if (pipe(fds) < 0) {
    fprintf(2, "schedtest: pipe failed\n");
    exit(1);
  }

  for (i = 0; i < n; i++) {
    pid = fork();
    if (pid < 0) {
      // Children still blocked in read() see EOF when we exit.
      fprintf(2, "schedtest: fork failed\n");
      exit(1);
    }
    if (pid == 0) {
      close(fds[1]);
      setpriority(prio[i]);
      // Block until the parent sends the common start time.
      if (read(fds[0], &t0, sizeof(t0)) != sizeof(t0))
        exit(-1);
      start = uptime();
      spin(work);
      exit(start - t0);
    }
    pids[i] = pid;
  }
  close(fds[0]);

  // Give every child time to set its priority and block in read().
  pause(5);

  // One write makes all children runnable in the same tick.
  t0 = uptime();
  for (i = 0; i < n; i++)
    starts[i] = t0;
  if (write(fds[1], starts, n * sizeof(int)) != n * sizeof(int)) {
    fprintf(2, "schedtest: write failed\n");
    exit(1);
  }
  close(fds[1]);

  printf("finish\tpid\tprio\tresponse\tturnaround\tcputime\n");
  totresp = tottat = 0;
  for (done = 0; done < n; done++) {
    pid = wait2(&status, &ru);
    if (pid < 0) {
      fprintf(2, "schedtest: wait2 failed\n");
      exit(1);
    }
    tat = uptime() - t0;
    for (i = 0; i < n && pids[i] != pid; i++)
      ;
    if (i == n || status < 0) {
      fprintf(2, "schedtest: child %d failed (status %d)\n", pid, status);
      exit(1);
    }
    printf("%d\t%d\t%d\t%d\t\t%d\t\t%d\n", done + 1, pid, prio[i], status, tat,
           ru.cputime);
    totresp += status;
    tottat += tat;
  }

  // Averages to one decimal place, using integer arithmetic.
  printf("average response: %d.%d ticks, average turnaround: %d.%d ticks\n",
         totresp / n, totresp * 10 / n % 10, tottat / n, tottat * 10 / n % 10);
  exit(0);
}
