/**
    \author: (c) Copyright Two Roads Technological Solutions Pvt Ltd 2011
     Address:
         Suite No 353, Evoma, #14, Bhattarhalli,
  Old Madras Road, Near Garden City College,
  KR Puram, Bangalore 560049, India
  +91 80 4190 3551
*/

#pragma once

#include "dvctrade/Signals/base_signal.hpp"
#include "dvctrade/Signals/ar_adjusted_sreturns.hpp"

namespace HFSAT {
  class SignalCreator {
  public:
    static void CreateSignalInstance(DebugLogger& _dbglogger_, const Watch& _watch_,
				     const int _tradingdate_,
				     std::string _signal_filename_, 
				     std::vector<Signal_BaseSignal*>& _signal_vec_);
  };
}
