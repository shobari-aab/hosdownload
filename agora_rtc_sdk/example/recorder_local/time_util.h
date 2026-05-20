#pragma once

#include <sys/stat.h>
#include <sys/time.h>

void Time2UTCStrWithSlash_ns(char *buffer, int len);
void Time2UTCStr(char *buffer, int len);