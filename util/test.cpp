#include <iostream>

#include <errno.h>

#include "system_ctrl.h"
using namespace video_analysis_device;
int main()
{
  //time_t t;
  //SystemCtrl::GetUTCSystemTime(&t);
  //std::cout<<t<<std::endl;
  //t = t+60;
  //std::cout<<t<<std::endl;
  //if(-1 == SystemCtrl::SetUTCSystemTime(&t))
  //{
    //int errsv = errno;
    //std::cout<<errsv<<std::endl;
  //}
  SystemCtrl::RebootSystem();
  return 0;
}
