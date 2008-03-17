#include <stdint.h>
#include <stdlib.h>
#include <unistd.h>
#include <math.h>
#include <syscall.h>
#include <errno.h>
#include <string.h>
#include <sys/mman.h>
#include <sys/poll.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>

// these are for the itifg calls:
#include "itifgExt.h"
#include "libitifg.h"
