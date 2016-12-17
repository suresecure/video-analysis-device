//#include <iostream>

//#include <errno.h>
//#include <arpa/inet.h>

//#include "system_ctrl.h"
//using namespace video_analysis_device;

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/ioctl.h>
int main() {

  int fd = open("/dev/watchdog", O_WRONLY);
  int ret = 0;
  if (fd == -1) {
    perror("watchdog");
    exit(EXIT_FAILURE);
  }
  int timeout = 45;
  //ioctl(fd, WDIOC_SETTIMEOUT, &timeout);
  //printf("The timeout was set to %d seconds\n", timeout);
  ioctl(fd, WDIOC_GETTIMEOUT, &timeout);
  printf("The timeout was is %d seconds\n", timeout);
  //while (1) {
    //ret = write(fd, "\0", 1);
    //if (ret != 1) {
      //ret = -1;
      //break;
    //}
    //sleep(10);
  //}
  close(fd);
  return ret;
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
