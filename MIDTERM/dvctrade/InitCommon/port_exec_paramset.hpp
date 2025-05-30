/**
    \author: (c) Copyright Two Roads Technological Solutions Pvt Ltd 2011
     Address:
         Suite No 353, Evoma, #14, Bhattarhalli,
  Old Madras Road, Near Garden City College,
  KR Puram, Bangalore 560049, India
  +91 80 4190 3551
*/

#pragma once

#include <string>
#include <fstream>
#include <strings.h>
#include <string.h>
#include <sstream>
#include <vector>
#include "dvccode/CDef/defines.hpp"
// to get access to enum lazily
#include "dvctrade/InitCommon/sbe_paramset.hpp"

namespace HFSAT {

struct PortExecParamSet {
public:
  std::string instrument_; // key
  int trade_cooloff_interval_;
  
  AlgoType_t exec_algo_;
  
  double l1perc_limit_;

  int max_lots_;
  int max_position_; // derived variable from max_lots

  int max_order_lots_;
  int max_order_size_; // derivedd variable from order_lots

  bool use_nonbest_support_;
  
  PortExecParamSet(const std::string& _filename_);
  void ParseParamFile(const std::string& _filename_);
  void scale_lot_values(const int _min_order_size_);
  
  std::string ToString();

};

}
