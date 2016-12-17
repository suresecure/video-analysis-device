#include <iostream>

#include <errno.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <fcntl.h>

#include "system_ctrl.h"
using namespace video_analysis_device;

int main() {
  int fd = SystemCtrl::OpenWatchdog();
  if (fd == -1) {
    std::cout<<"watchdog"<<std::endl;
    return -1;
  }
  std::cout<<"before get\n";
  std::cout<<"Watchdog timeout is: "<<SystemCtrl::GetWatchdogTimeout(fd);
  std::cout<<"after get\n";
  while (5) {
    std::cout<<"Pat dog\n";
    SystemCtrl::PatWatchdog(fd);
    sleep(10);
  }
  std::cout<<"Disable watchdog\n";
  SystemCtrl::DisableWatchdog(fd);
  SystemCtrl::CloseWatchdog(fd);
  return 0;
  // std::cout<< ValidNetmask("255.0.0.0")<<std::endl;
  // time_t t;
  // SystemCtrl::GetUTCSystemTime(&t);
  // std::cout<<t<<std::endl;
  // t = t+60;
  // std::cout<<t<<std::endl;
  // if(-1 == SystemCtrl::SetUTCSystemTime(&t))
  //{
  // int errsv = errno;
  // std::cout<<errsv<<std::endl;
  //}
  // SystemCtrl::RebootSystem();
  // std::string ipaddr;
  // std::string netmask;
  // std::string default_gateway;
  // std::string dns;
  // SystemCtrl::GetNetworkConf(0, ipaddr, netmask, default_gateway, dns);
  // std::cout << ipaddr << "\t" << netmask << "\t" << default_gateway << "\t"
  //<< dns << std::endl;

  // ipaddr = "192.168.1.100";
  // netmask = "255.255.0.0";
  // default_gateway = "192.168.1.10";
  // dns = "192.168.1.20";
  // std::cout << SystemCtrl::SetNetworkConf(0, ipaddr, netmask,
  // default_gateway,
  // dns);
  return 0;
}
