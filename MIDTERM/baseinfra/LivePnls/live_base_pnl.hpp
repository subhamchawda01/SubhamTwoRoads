/**
   \file SmartOrderRouting/live_base_pnl.hpp

   \author: (c) Copyright Two Roads Technological Solutions Pvt Ltd 2011
   Address:
   Suite No 353, Evoma, #14, Bhattarhalli,
   Old Madras Road, Near Garden City College,
   KR Puram, Bangalore 560049, India
   +91 80 4190 3551
*/

#pragma once

#include "baseinfra/SmartOrderRouting/base_pnl.hpp"
#include "dvccode/Utils/client_logging_segment_initializer.hpp"

namespace HFSAT {

class LiveBasePNL : public BasePNL,public TimePeriodListener {
 protected:
  HFSAT::CDef::LogBuffer* log_buffer_;
  HFSAT::Utils::ClientLoggingSegmentInitializer* client_logging_segment_initializer_;

 public:
  LiveBasePNL(DebugLogger& t_dbglogger_, Watch& r_watch_, 
              SecurityMarketView& t_dep_market_view_, int t_runtime_id_,
              HFSAT::Utils::ClientLoggingSegmentInitializer* _client_logging_segment_initializer_);

  virtual ~LiveBasePNL(){};

  void OnTimePeriodUpdate(const int num_pages_to_add_);
  void OnExec(int _new_position_, int _exec_quantity_, TradeType_t _buysell_, double _price_, int r_int_price_,
              const int _security_id_);
};
}
