#define NPROC       64                // maximum number of processes
#define NCPU        8                 // maximum number of CPUs
#define NOFILE      16                // open files per process
#define NFILE       100               // open files per system
#define NINODE      50                // maximum number of active i-nodes
#define NDEV        10                // maximum major device number
#define ROOTDEV     1                 // device number of file system root disk
#define MAXARG      32                // max exec arguments
#define MAXOPBLOCKS 10                // max # of blocks any FS op writes
#define LOGBLOCKS   (MAXOPBLOCKS * 3) // max data blocks in on-disk log
#define NBUF        (MAXOPBLOCKS * 3) // size of disk block cache
#define FSSIZE      2000              // size of file system in blocks
#define MAXPATH     128               // maximum file path name
#define USERSTACK   1                 // user stack pages
#define MAXPRIORITY 49                // highest priority setpriority() allows

// Scheduling policy, chosen at compile time by setting SCHEDPOLICY.
// "make SCHEDPOLICY=SCHED_RR" overrides it (after "make clean").
#define SCHED_RR    0                 // round robin over RUNNABLE processes
#define SCHED_PRIO  1                 // highest priority first
#ifndef SCHEDPOLICY
#define SCHEDPOLICY SCHED_PRIO
#endif

// Aging for SCHED_PRIO: a RUNNABLE process gains one level of effective
// priority for every AGINGTICKS ticks it waits, up to MAXEFFPRIORITY.
#ifndef AGING
#define AGING       1                 // 0 disables aging
#endif
#define AGINGTICKS  1
#define MAXEFFPRIORITY 99
