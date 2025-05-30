/**
    \author: (c) Copyright Two Roads Technological Solutions Pvt Ltd 2011
     Address:
         Suite No 353, Evoma, #14, Bhattarhalli,
  Old Madras Road, Near Garden City College,
  KR Puram, Bangalore 560049, India
  +91 80 4190 3551
*/

#include "dvctrade/Signals/signal_creator.hpp"
#include <boost/algorithm/string/trim.hpp>

namespace HFSAT {
  void SignalCreator::CreateSignalInstance(DebugLogger& _dbglogger_, const Watch& _watch_,
					   const int _tradingdate_,
					   std::string _signal_filename_, 
					   std::vector<Signal_BaseSignal*>& _signal_vec_) {
    std::ifstream t_signal_listfile_;
    t_signal_listfile_.open(_signal_filename_.c_str(), std::ifstream::in);
    
    if (t_signal_listfile_.is_open()) {
      const int kSignalFileLineBufferLen = 1024;
      char readline_buffer_[kSignalFileLineBufferLen];
      bzero(readline_buffer_, kSignalFileLineBufferLen);
	
      while (t_signal_listfile_.good()) {
	bzero(readline_buffer_, kSignalFileLineBufferLen);
	t_signal_listfile_.getline(readline_buffer_, kSignalFileLineBufferLen);
	PerishableStringTokenizer st_(readline_buffer_, kSignalFileLineBufferLen);
	const std::vector<const char*> &tokens_ = st_.GetTokens();
	if (tokens_.size() == 2) {
	  if (strcmp(tokens_[0], "ARAdjustedSPortReturns") == 0) {
	    // should we create param object inside signal ( may be, no big deal )
	    ParamSet_ARAdjustedSPortReturns* t_paramset_ = new ParamSet_ARAdjustedSPortReturns(tokens_[1], _tradingdate_);
	    _signal_vec_.push_back(new Signal_ARAdjustedSPortReturns(_dbglogger_, _watch_, *t_paramset_));	
	  }
	}
      }
      t_signal_listfile_.close();
    }
  }
}

