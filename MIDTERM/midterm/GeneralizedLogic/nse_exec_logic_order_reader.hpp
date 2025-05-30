#pragma once
#include "nse_simple_exec_logic.hpp"
#include "dvccode/CommonDataStructures/perishable_string_tokenizer.hpp"
#include "baseinfra/LoggedSources/nse_logged_message_filesource.hpp"
#include "nse_exec_logic_helper.hpp"

#include "dvccode/ExternalData/external_time_listener.hpp"
#include "dvccode/Utils/tcp_server_socket_listener.hpp"
#include "nse_execution_listener_manager.hpp"
#include "nse_generalized_execution_logic.hpp"

/*
 * basically 5:30 hours (5*60 + 30) * 60 * 1000 * 1000 * 1000 = 1.98e+13 nanosec
 */

#define GMTISTTIMEDIFFOFFSET 19800000000000 

// Contains info for a particular security - params, smv, order manager etc.
class SyntheticLegInfo {
public:
  HFSAT::SecurityMarketView *smv_;
  NSE_SIMPLEEXEC::ParamSet *param_;
  int sec_id_;
  HFSAT::NSELoggedMessageFileSource *filesource_;

  SyntheticLegInfo(HFSAT::SecurityMarketView *smv_t,
                   NSE_SIMPLEEXEC::ParamSet *param_t, int sec_id_t,
                   HFSAT::NSELoggedMessageFileSource *filesource_t)
      : smv_(smv_t), param_(param_t), sec_id_(sec_id_t),
        filesource_(filesource_t) {}
  SyntheticLegInfo()
      : smv_(nullptr), param_(nullptr), sec_id_(-1), filesource_(nullptr) {}
};

namespace NSE_SIMPLEEXEC {
class SimpleNseExecLogicOrderReader : public HFSAT::TimePeriodListener {
public:
  std::map<std::string, SimpleNseExecLogic *> instrument_to_exec_logic_map_;
  std::map<uint64_t, uint64_t>
      stratID_to_last_processed_order_time_map_; // live mode
  HFSAT::Watch &watch_;
  HFSAT::DebugLogger &dbglogger_;
  uint64_t exec_start_time_;
  bool isLive;
  std::map<std::string, SyntheticLegInfo>
      leg_info_; // map from security to SyntheticLegInfo
  // contains pointers to all dynamically created execs, from orderID/timestamp
  std::map<std::string, NseGeneralizedExecutionLogic *> live_generalized_execs_;
  std::map<std::string, SimpleNseExecLogic *> synthetic_execs_;
  bool is_pre_monthly_expiry_week;
  bool is_monthly_expiry_week;
  int next_weekly_expiry_;
  int nearest_weekly_expiry_;
  NseExecLogicHelper *exec_logic_helper_;
  std::string bkp_file_;
  std::map<int, std::string> tranche_id_to_strategy_;
public:
  SimpleNseExecLogicOrderReader(
      HFSAT::Watch &watch_t, HFSAT::DebugLogger &dbglogger_t, bool is_live_t,
      std::map<std::string, SyntheticLegInfo> leg_info_t,
      NseExecLogicHelper *exec_logic_helper_t, std::string bkp_file_);
  void SubscribeNewOrders(std::string t_instrument_,
                          SimpleNseExecLogic *t_exec_logic_);
  void NotifyOrderListeners(std::string t_instrument_, std::string order_id_,
                            int order_size_, double ref_px_);
  void NotifyCancelListeners(std::string order_id_, std::string order_tag_);
  void NotifyForceCancelListeners(std::string t_instrument_, std::string order_id_, std::string order_tag_);
  std::string GenerateShortCodeVA(std::string shc);
  void NotifyModifyListeners(std::string t_instrument_, std::string order_id_);
  NSE_SIMPLEEXEC::SimpleNseExecLogic *
  SetupInSim(std::string &shortcode_, HFSAT::SecurityMarketView *smv_,
             NSE_SIMPLEEXEC::ParamSet *);

  void OnTimePeriodUpdate(const int num_pages_to_add_) override {}

  virtual void LookupOrdersInSimMode() {}
  virtual void LoadAllOrdersInSimMode() {}
  virtual void ReadNewLiveOrdersFromFile() {}
};
}
