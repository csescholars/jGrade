#ifndef __MKPATH_H
#define __MKPATH_H

#include <sys/types.h>
#include <sys/stat.h>
#include <errno.h>
#include <libgen.h>
#include <string.h>
#include <stdlib.h>

int mkpath(const char *s, mode_t mode);

#endif
