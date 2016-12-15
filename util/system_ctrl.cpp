#include <time.h>

#include <unistd.h>
#include <sys/reboot.h>

#include "system_ctrl.h"
namespace video_analysis_device {
// static int SetLocalSystemtime(time_t *t);
// static int GetLocalSystemtime(const time_t *t)
//{
//}
int SystemCtrl::GetUTCSystemTime(time_t *t) {
  struct timespec tp;
  int ret = clock_gettime(CLOCK_REALTIME, &tp);
  *t = tp.tv_sec;
  return ret;
}
int SystemCtrl::SetUTCSystemTime(const time_t *t) {
  struct timespec tp;
  tp.tv_sec = *t;
  tp.tv_nsec = 0;
  return clock_settime(CLOCK_REALTIME, &tp);
}

int SystemCtrl::RebootSystem() {
  sync();
  reboot(RB_AUTOBOOT);
}
}
