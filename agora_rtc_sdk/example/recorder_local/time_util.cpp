#include "time_util.h"
#include <cstdio>
#include <ctime>
#include <mutex>


struct Time2UTCInfo {
  std::mutex locker;
  struct timeval utc_base;
  struct timespec base_spec;
  bool init_once_flag;
  Time2UTCInfo()
    :init_once_flag(false)
    {}
};

static Time2UTCInfo utcInfo;

void InitUtc() {
    //get base UTC time, it's only got once.
    if(!utcInfo.init_once_flag)
    {
        std::lock_guard<std::mutex> auto_lock(utcInfo.locker);
        if(!utcInfo.init_once_flag)
        {
            gettimeofday(&utcInfo.utc_base, NULL);//get utc
            if(clock_gettime(CLOCK_MONOTONIC, &utcInfo.base_spec) == -1) return;
            utcInfo.init_once_flag = true;
        }
    } 
}

void Time2UTCStrWithSlash_ns(char *buffer, int len)
{
  InitUtc();

  timespec now_spec = {0, 0};
  if(clock_gettime(CLOCK_MONOTONIC, &now_spec) == -1) return;
  now_spec.tv_nsec = 1000ll*1000*1000 + now_spec.tv_nsec + utcInfo.utc_base.tv_usec* 1000 - utcInfo.base_spec.tv_nsec;
  now_spec.tv_sec = utcInfo.utc_base.tv_sec + now_spec.tv_sec - utcInfo.base_spec.tv_sec;

  if(now_spec.tv_nsec >= 2000ll*1000*1000)
  {
      //ns_overflow = true;
      now_spec.tv_nsec -= 2000ll*1000*1000;
      now_spec.tv_sec++;
  }
  else if(now_spec.tv_nsec < 1000ll*1000*1000)
  {
      //ns_underflow = true;
      now_spec.tv_sec--;
  }
  else
      now_spec.tv_nsec -= 1000ll*1000*1000;

  int used = (int)strftime(buffer, len, "%H%M%S", gmtime(&now_spec.tv_sec));

  if(used > 0 && used < len - 5)  // we have enough space
      snprintf(buffer+used, len-used, "_%09u/", (unsigned int)now_spec.tv_nsec);
}

/*
 * monotinic ->0-timezone UTC time stamp
 */
void Time2UTCStr(char *buffer, int len)
{                                                                                                                                                                                             
    //get base UTC time, it's only got once.
    InitUtc();

    timespec now_spec = {0, 0};
    if(clock_gettime(CLOCK_MONOTONIC, &now_spec) == -1) return;
    now_spec.tv_nsec = 1000ll*1000*1000 + now_spec.tv_nsec + utcInfo.utc_base.tv_usec* 1000 - utcInfo.base_spec.tv_nsec;
    now_spec.tv_sec = utcInfo.utc_base.tv_sec + now_spec.tv_sec - utcInfo.base_spec.tv_sec;


    if(now_spec.tv_nsec >= 2000ll*1000*1000)
    {
        //ns_overflow = true;
        now_spec.tv_nsec -= 2000ll*1000*1000;
        now_spec.tv_sec++;
    }
    else if(now_spec.tv_nsec < 1000ll*1000*1000)
    {
        //ns_underflow = true;
        now_spec.tv_sec--;
    }
    else
        now_spec.tv_nsec -= 1000ll*1000*1000;

    int used = (int)strftime(buffer, len, "%Y%m%d%H%M%S", gmtime(&now_spec.tv_sec));

    if(used > 0 && used < len - 4)  // we have enough space
        snprintf(buffer+used, len-used, "%03u", (unsigned int)now_spec.tv_nsec/1000/1000);
}