/**
    \author: (c) Copyright Two Roads Technological Solutions Pvt Ltd 2011
     Address:
         Suite No 353, Evoma, #14, Bhattarhalli,
  Old Madras Road, Near Garden City College,
  KR Puram, Bangalore 560049, India
  +91 80 4190 3551
*/

#include "dvctrade/ExecLogic/sbe_risk_reader.hpp"
#include "dvccode/CommonDataStructures/perishable_string_tokenizer.hpp"

namespace HFSAT {

SBERiskReader::SBERiskReader(HFSAT::Watch& watch_t, HFSAT::DebugLogger& dbglogger_t)
  : exec_logic_(nullptr),
    watch_(watch_t),
    dbglogger_(dbglogger_t),
    exec_start_time_(-1) {
  struct timeval tv;
  gettimeofday(&tv, NULL);
  exec_start_time_ = tv.tv_sec * 1000 * 1000 * 1000;  // nanosec
}

void SBERiskReader::SubscribeRiskChanges(SignalBasedTrading* t_exec_logic_) {
  exec_logic_ = t_exec_logic_;
}

void SBERiskReader::NotifyOrderListeners(std::string _instrument_, std::string _order_id_,
					  int _order_size_, double _ref_px_) {
  if (exec_logic_ != nullptr) {
    exec_logic_->OnNewOrderFromStrategy(_instrument_, _order_id_, _order_size_, _ref_px_);
  } else
    dbglogger_ << "notifyOrderListeners:: Listener not added for instrument: " << _instrument_ << "\n";
}

void SBERiskReader::NotifyPositionListeners(std::string _instrument_, std::string _strat_id_,
					    int _position_size_, double _new_ref_px_) {
  if (exec_logic_ != nullptr) {
    exec_logic_->OnNewPositionFromStrategy(_instrument_, _strat_id_, _position_size_, _new_ref_px_);
  } else
    dbglogger_ << "NotifyPositionListeners:: Listener not added for instrument: " << _instrument_ << "\n";
}

}
