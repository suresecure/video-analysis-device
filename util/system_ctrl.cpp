#include <time.h>
#include <iostream>
#include <fstream>

#include <unistd.h>
#include <arpa/inet.h>
#include <sys/reboot.h>

#include "system_ctrl.h"
namespace video_analysis_device {
// static int SetLocalSystemtime(time_t *t);
// static int GetLocalSystemtime(const time_t *t)
//{
//}
//
static bool ValidIpaddr(const std::string &addr) {
  struct sockaddr_in sa;
  if (inet_pton(AF_INET, addr.c_str(), &(sa.sin_addr)) > 0)
    return true;
  else
    return false;
}
static bool ValidNetmask(const std::string &netmask) {
  struct sockaddr_in sa;
  if (inet_pton(AF_INET, netmask.c_str(), &(sa.sin_addr)) <= 0)
    return false;
  unsigned long prefix_length = sa.sin_addr.s_addr;
  if (prefix_length == 0)
    return false;
  uint32_t y = prefix_length;
  y = ~htonl(y);
  uint32_t z = y + 1;
  std::cout<<std::hex<<prefix_length << "\t"<<y<<"\t"<<z<<std::endl;
  return (z & y) == 0;
}
int SystemCtrl::SetNetworkConf(int idx, const std::string &ipaddr,
                               const std::string &netmask,
                               const std::string &default_gateway,
                               const std::string &dns) {
  if (!ValidIpaddr(ipaddr) || !ValidIpaddr(default_gateway) ||
      !ValidIpaddr(dns) || !ValidNetmask(netmask))
    return -1;

  std::ofstream interfaces("tmpcfgfiles/interfaces");
  interfaces << "auto lo\n";
  interfaces << "iface lo inet loopback\n\n";
  interfaces << "auto eth0\n";
  interfaces << "iface eth0 inet static\n";
  interfaces << "    address " << ipaddr << "\n";
  interfaces << "    netmask " << netmask << "\n";
  interfaces << "    gateway " << default_gateway << "\n";
  interfaces.close();
  std::ofstream resolv_conf("tmpcfgfiles/resolv.conf");
  resolv_conf << "nameserver " << dns << "\n";
  resolv_conf.close();
  return 0;
}
int SystemCtrl::GetNetworkConf(int idx, std::string &ipaddr,
                               std::string &netmask,
                               std::string &default_gateway, std::string &dns) {
  std::ifstream interfaces("cfgfiles/interfaces");
  std::string word;
  while (interfaces >> word) {
    if (word == "address") {
      interfaces >> word;
      ipaddr = word;
    } else if (word == "netmask") {
      interfaces >> word;
      netmask = word;
    } else if (word == "gateway") {
      interfaces >> word;
      default_gateway = word;
    }
  }
  interfaces.close();
  std::ifstream resolv_conf("cfgfiles/resolv.conf");
  while (resolv_conf >> word) {
    if (word == "nameserver") {
      resolv_conf >> word;
      dns = word;
    }
  }
  resolv_conf.close();
  return 0;
}

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
