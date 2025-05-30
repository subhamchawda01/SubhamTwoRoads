#pragma once
#include "dvccode/ExternalData/external_time_listener.hpp"
#include "dvctrade/ExecLogic/nse_simple_exec_logic.hpp"
#include "dvctrade/ExecLogic/nse_exec_logic_order_reader.hpp"

struct RVOrderInfo_t {
  std::string instrument_;
  int strat_id_;
  int lot_size_;
  double ref_px_;
  std::string order_id_;
};
typedef std::multimap<uint64_t, RVOrderInfo_t>::iterator order_info_itr;
typedef std::pair<order_info_itr, order_info_itr> Range_itr;

namespace NSE_SIMPLEEXEC {
class SimpleNseExecLogicOrderReaderFromFile : public SimpleNseExecLogicOrderReader {
  std::multimap<uint64_t, RVOrderInfo_t> order_time_to_order_info_map_;  // sim mode
  std::string orders_file_;

 public:
  SimpleNseExecLogicOrderReaderFromFile(HFSAT::Watch& watch_t, HFSAT::DebugLogger& dbglogger_t, bool is_live_t,
                                        std::string orders_file_t);

  void LoadAllOrdersInSimMode() override;
  void OnTimePeriodUpdate(const int num_pages_to_add_) override;
  void LookupOrdersInSimMode() override;
  void ReadNewLiveOrdersFromFile() override;
};
}
