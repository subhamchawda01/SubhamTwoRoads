#include "dvctrade/ExecLogic/nse_exec_logic_order_reader.hpp"
#include "dvccode/CommonDataStructures/perishable_string_tokenizer.hpp"

namespace NSE_SIMPLEEXEC {

SimpleNseExecLogicOrderReader::SimpleNseExecLogicOrderReader(HFSAT::Watch& watch_t, HFSAT::DebugLogger& dbglogger_t,
                                                             bool is_live_t)
    : instrument_to_exec_logic_map_(),
      instrument_to_last_processed_order_time_map_(),
      watch_(watch_t),
      dbglogger_(dbglogger_t),
      exec_start_time_(-1),
      isLive(is_live_t) {  // Intentionally iniitiazed with max value
  struct timeval tv;
  gettimeofday(&tv, NULL);
  exec_start_time_ = tv.tv_sec * 1000 * 1000 * 1000;  // nanosec
}

void SimpleNseExecLogicOrderReader::SubscribeNewOrders(std::string t_instrument_, SimpleNseExecLogic* t_exec_logic_) {
  instrument_to_exec_logic_map_.insert(std::make_pair(t_instrument_, t_exec_logic_));
}

void SimpleNseExecLogicOrderReader::NotifyOrderListeners(std::string t_instrument_, std::string order_id_,
                                                         int order_size_, double ref_px_) {
  if (instrument_to_exec_logic_map_.find(t_instrument_) != instrument_to_exec_logic_map_.end()) {
    instrument_to_exec_logic_map_[t_instrument_]->OnNewOrderFromStrategy(order_id_, order_size_, ref_px_);
  } else
    dbglogger_ << "notifyOrderListeners:: Listener not added for instrument: " << t_instrument_ << "\n";
}
}
