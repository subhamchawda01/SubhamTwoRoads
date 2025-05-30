// =====================================================================================
// 
//       Filename:  signal_msg.hpp
// 
//    Description:  
// 
//        Version:  1.0
//        Created:  04/20/2023 06:40:03 AM
//       Revision:  none
//       Compiler:  g++
// 
//         Author:  (c) Copyright Two Roads Technological Solutions Pvt Ltd 2011
//
//        Address:  Suite No 162, Evoma, #14, Bhattarhalli,
//                  Old Madras Road, Near Garden City College,
//                  KR Puram, Bangalore 560049, India
//          Phone:  +91 80 4190 3551 
// 
// =====================================================================================

#pragma once

#include <sys/time.h>
#include <string>
#include <sstream>

#define LOG_MONEYNESS_SIZE 9
#define DELTA_PARAM_SIZE 36

namespace HFSAT{

  struct IVCurveData{
    char basename[16];
    int32_t deltaAnchorPoints;
    double deltaAnchorPointLogMoneyNess[LOG_MONEYNESS_SIZE];
    double leftAnchorPointIV;
    double rightAnchorPointIV;
    double delta_param[DELTA_PARAM_SIZE];
    double adjust_val;
    bool is_ready;

    std::string ToString() const{
      std::ostringstream t_temp_oss_;
      t_temp_oss_ << basename << " " << deltaAnchorPoints << " ";

      for(int i=0; i<LOG_MONEYNESS_SIZE; i++){
        t_temp_oss_ << deltaAnchorPointLogMoneyNess[i] << " " ;
      }

      t_temp_oss_ << leftAnchorPointIV << " " << rightAnchorPointIV << " " ;

      for(int i=0; i<DELTA_PARAM_SIZE; i++){
        t_temp_oss_ << delta_param[i] << " " ;
      }

      t_temp_oss_ << adjust_val << " " << is_ready << "\n";

      return t_temp_oss_.str();
    }

  };
}
