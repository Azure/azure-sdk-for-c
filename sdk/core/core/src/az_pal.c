#include <az_pal.h>

#ifdef _WIN32
#include <windows.h>
#else
#include <unistd.h>
#endif

#include <_az_cfg.h>

#ifdef _WIN32
void az_pal_wait(int32_t milliseconds) {
  if (milliseconds > 0) {
    Sleep(milliseconds);
  }
}
#else
void az_pal_wait(int32_t milliseconds) {
  if (milliseconds > 0) {
    (void)usleep(milliseconds * 1000);
  }
}
#endif
