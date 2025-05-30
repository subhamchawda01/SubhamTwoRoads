#pragma once
#include "dvccode/ExternalData/external_time_listener.hpp"
#include "dvccode/Utils/tcp_server_socket_listener.hpp"
#include "dvctrade/ExecLogic/nse_simple_exec_logic.hpp"

struct NSEProductStratID_t {
  std::string instrument_;
  int strat_id_;
  bool operator<(const NSEProductStratID_t& param) const {
    return std::tie(instrument_, strat_id_) < std::tie(param.instrument_, param.strat_id_);
  }
};

namespace NSE_SIMPLEEXEC {
class SimpleNseExecLogicOrderReader : public HFSAT::TimePeriodListener, public HFSAT::Utils::TCPServerSocketListener {
 public:
  std::map<std::string, SimpleNseExecLogic*> instrument_to_exec_logic_map_;
  std::map<NSEProductStratID_t, uint64_t> instrument_to_last_processed_order_time_map_;  // live mode

  HFSAT::Watch& watch_;
  HFSAT::DebugLogger& dbglogger_;
  uint64_t exec_start_time_;
  bool isLive;

 public:
  SimpleNseExecLogicOrderReader(HFSAT::Watch& watch_t, HFSAT::DebugLogger& dbglogger_t, bool is_live_t);
  void SubscribeNewOrders(std::string t_instrument_, SimpleNseExecLogic* t_exec_logic_);
  void NotifyOrderListeners(std::string t_instrument_, std::string order_id_, int order_size_, double ref_px_);
  void OnTimePeriodUpdate(const int num_pages_to_add_) override {}

  virtual void OnClientRequest(int32_t client_fd, char* buffer, uint32_t const& length) {}
  virtual void Stop() {}  // wait required in tcp mode
  virtual void LookupOrdersInSimMode() {}
  virtual void LoadAllOrdersInSimMode() {}
  virtual void ReadNewLiveOrdersFromFile() {}
};
}
