#include "vlog.h"
#include <stdio.h>

int main(int argc, char **argv)
{
  vlog_init();
  vlog(VL_FATAL, VCAT_DETECT, "Must print this");
  vlog(VL_WARNING, VCAT_GENERAL, "This is a log message: %d\n", 5);
  vlog_fini();
  return 0;
}
