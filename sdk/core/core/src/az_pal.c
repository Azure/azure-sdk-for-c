#include <az_pal.h>
#include <az_time_internal.h>

#ifdef _WIN32
#include <windows.h>
#else
#include <unistd.h>
#endif

#include <_az_cfg.h>

#ifdef _WIN32
void az_pal_sleep(int32_t milliseconds) {
  if (milliseconds > 0) {
    Sleep(milliseconds);
  }
}
#else
void az_pal_sleep(int32_t milliseconds) {
  if (milliseconds > 0) {
    (void)usleep(milliseconds * _az_TIME_MICROSECONDS_PER_MILLISECOND);
  }
}
#endif
