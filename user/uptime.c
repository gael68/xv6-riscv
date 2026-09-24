#include "kernel/types.h"
#include "user/user.h"

int
main(int argc, char *argv[])
{
  printf("up %d clock ticks\n", uptime());
  exit(0);
}
