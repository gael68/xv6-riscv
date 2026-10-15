#!/usr/bin/env python3
"""PA2 demo: boot xv6 in QEMU and run each task's commands in order.

usage: python3 demo.py [--no-pause] [--clean] [VAR=value ...]
  --no-pause  run every task without waiting for Enter in between
  --clean     run "make clean" first (needed when switching builds)
  VAR=value   passed to make, e.g. AGING=0 or SCHEDPOLICY=SCHED_RR
              (use together with --clean)

Before each task the relevant source code is printed with line numbers.
"""
import os, pty, re, select, subprocess, sys, time

HERE = os.path.dirname(os.path.abspath(__file__))
PAUSE = "--no-pause" not in sys.argv
MAKEVARS = [a for a in sys.argv[1:] if "=" in a and not a.startswith("-")]

# Each task: (title, [(file, start regex, end regex)], [(command, wait)]).
# wait is "prompt", ("count", regex, n), or ("quiet", seconds). "count"
# counts output since the command was typed; "taskcount" counts since the
# task started, for background output that may already have appeared.
TASKS = [
    ("Task 1: getprocs() system call and ps",
     [("kernel/pstat.h", r"^struct pstat", r"^};"),
      ("kernel/proc.c", r"^// Copy a struct pstat", r"^}"),
      ("kernel/sysproc.c", r"^sys_getprocs", r"^}")],
     [("ps", "prompt"),
      ("sleep 50 &", "prompt"),
      ("ps", "prompt")]),
    ("Task 2: priority, getpriority()/setpriority(), pexec",
     [("kernel/sysproc.c", r"^sys_getpriority", r"^}"),
      ("kernel/sysproc.c", r"^sys_setpriority", r"^}"),
      ("kernel/proc.c", r"np->priority = p->priority", r"np->priority = p->priority")],
     [("prioritytest", "prompt"),
      ("pexec 10 ps", "prompt"),
      ("pexec abc ps", "prompt"),
      ("pexec 5 nosuchcmd", "prompt")]),
    ("Task 3: readytime and age in ps",
     [("kernel/proc.c", r"^yield", r"^}"),
      ("user/ps.c", r"Age is how long", r"uproc\[i\].name\);")],
     [("matmul 10 &; matmul 10 &", ("quiet", 1.0)),
      ("ps", ("count", r"\n\$ ", 1)),
      ("ps", ("count", r"\n\$ ", 1)),
      ("echo waiting for both matmuls", ("taskcount", r"Time: \d+ ticks", 2))]),
    ("Task 4: priority scheduler (param.h selects it)",
     [("kernel/param.h", r"Scheduling policy", r"^#endif"),
      ("kernel/proc.c", r"#if SCHEDPOLICY == SCHED_PRIO", r"^#else")],
     [("schedtest 20 0 10 20 30", "prompt"),
      ("schedtest 20 30 20 10 0", "prompt"),
      ("schedtest 20 5 5 5", "prompt")]),
    ("Task 5: aging",
     [("kernel/param.h", r"^// Aging for SCHED_PRIO", r"MAXEFFPRIORITY 99"),
      ("kernel/proc.c", r"^// Priority the scheduler uses", r"^}")],
     [("pexec 5 matmul 10 &; matmul 10 &", ("quiet", 1.0)),
      ("pexec 10 ps", ("count", r"\n\$ ", 1)),
      ("ps", ("count", r"\n\$ ", 1)),
      ("echo waiting for both matmuls", ("taskcount", r"Time: \d+ ticks", 2)),
      ("schedtest 20 0 10 20 30", "prompt"),
      ("schedtest 20 0 5", "prompt")]),
    ("Extra credit: getpriority / setpriority shell builtins",
     [("user/sh.c", r"^// Run the getpriority and setpriority", r"^}"),
      ("user/sh.c", r"else if \(!prioritycmd", r"else if \(!prioritycmd")],
     [("getpriority", "prompt"),
      ("setpriority 10", "prompt"),
      ("getpriority", "prompt"),
      ("ps", "prompt"),
      ("setpriority 50", "prompt"),
      ("time wc README", "prompt"),
      ("pexec 20 matmul 20 &; pexec 20 matmul 20 &", ("quiet", 1.0)),
      ("setpriority 30", ("quiet", 1.0)),
      ("time wc README", ("count", r"elapsed time", 1)),
      ("ps", ("count", r"\n\$ ", 1)),
      ("setpriority 0", ("quiet", 1.0)),
      ("time wc README", ("count", r"elapsed time", 1)),
      ("echo waiting for both matmuls", ("taskcount", r"Time: \d+ ticks", 2))]),
]


def show_code(path, start, end):
    lines = open(os.path.join(HERE, path)).read().split("\n")
    i = next(k for k, l in enumerate(lines) if re.search(start, l))
    j = next(k for k in range(i, len(lines)) if re.search(end, lines[k]))
    print(f"\n----- {path}:{i + 1}-{j + 1} -----")
    for k in range(i, j + 1):
        print(f"{k + 1:5}  {lines[k]}")


def banner(text):
    print("\n" + "=" * 70 + f"\n{text}\n" + "=" * 70, flush=True)


class Qemu:
    def __init__(self):
        self.master, slave = pty.openpty()
        self.proc = subprocess.Popen(["make", "-s", "qemu"] + MAKEVARS, cwd=HERE, stdin=slave,
                                     stdout=slave, stderr=slave, close_fds=True)
        os.close(slave)
        self.buf = ""

    def read(self, t):
        r, _, _ = select.select([self.master], [], [], t)
        if not r:
            return False
        try:
            d = os.read(self.master, 4096).decode("utf-8", "replace")
        except OSError:
            return False
        self.buf += d.replace("\r", "")
        sys.stdout.write(d)
        sys.stdout.flush()
        return True

    def wait(self, cond, start, limit=300, taskstart=0):
        if isinstance(cond, str):
            cond = (cond,)
        if cond[0] == "taskcount":
            cond, start = ("count",) + tuple(cond[1:]), taskstart
        t0 = last = time.time()
        while time.time() - t0 < limit:
            if self.read(0.1):
                last = time.time()
            seg = self.buf[start:]
            quiet = time.time() - last
            if cond[0] == "prompt" and seg.endswith("$ ") and quiet > 0.5:
                return
            if cond[0] == "count" and len(re.findall(cond[1], seg)) >= cond[2] and quiet > 0.3:
                return
            if cond[0] == "quiet" and quiet > cond[1]:
                return
        print(f"\n[demo: gave up waiting for {cond}]")

    def run(self, cmd, cond, taskstart):
        start = len(self.buf)
        os.write(self.master, (cmd + "\n").encode())
        self.wait(cond, start, taskstart=taskstart)

    def quit(self):
        os.write(self.master, b"\x01x")  # Ctrl-A x exits QEMU
        try:
            self.proc.wait(timeout=5)
        except subprocess.TimeoutExpired:
            self.proc.kill()


def main():
    if "--clean" in sys.argv:
        subprocess.run(["make", "clean"], cwd=HERE, stdout=subprocess.DEVNULL, check=True)
    banner("Building and booting xv6 (make qemu)")
    q = Qemu()
    try:
        q.wait(("count", r"init: starting sh\n\$ ", 1), 0, limit=300)
        for title, code, cmds in TASKS:
            banner(title)
            for path, start, end in code:
                show_code(path, start, end)
            if PAUSE:
                input("\n[Press Enter to run this task's commands] ")
            print("\n$ ", end="", flush=True)
            taskstart = len(q.buf)
            for cmd, cond in cmds:
                q.run(cmd, cond, taskstart)
            if PAUSE:
                input("\n\n[Press Enter for the next task] ")
        banner("Done. Quitting QEMU.")
    finally:
        q.quit()


if __name__ == "__main__":
    main()
