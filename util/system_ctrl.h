#ifndef SYSTEM_CTRL_HEADER
#define SYSTEM_CTRL_HEADER

#include <time.h>

namespace video_analysis_device {
class SystemCtrl {
public:
  static int SetNetworkConf(int idx, const std::string &ipaddr,
                            const std::string &netmask,
                            const std::string &default_gateway,
                            const std::string &dns);
  static int GetNetworkConf(int idx, std::string &ipaddr, std::string &netmask,
                            std::string default_gateway, std::string &dns);

  // static int SetLocalSystemtime(time_t* t);
  // static int GetLocalSystemtime(const time_t* t);
  static int GetUTCSystemTime(time_t *t);
  static int SetUTCSystemTime(const time_t *t);

  static int RebootSystem();
};
}

#endif
