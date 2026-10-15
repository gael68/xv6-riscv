// Shared by the kernel and user programs (ps, time).

enum procstate { UNUSED, USED, SLEEPING, RUNNABLE, RUNNING, ZOMBIE };

struct rusage {
  uint cputime;
};

struct pstat {
  int pid;              // Process ID
  enum procstate state; // Process state
  uint64 size;          // Size of process memory (bytes)
  int ppid;             // Parent process ID
  char name[16];        // Process name
};
