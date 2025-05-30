#pragma once

#include "dvccode/ExternalData/external_time_listener.hpp"
#include "midterm/GeneralizedLogic/nse_simple_exec_logic.hpp"
#include "midterm/GeneralizedLogic/nse_exec_logic_order_reader.hpp"
#define LARGE_NUM_BACKUP_ORDS 30
#define BACKUP_PROCESSING_MSECS_BEGIN 13560000

struct RVOrderInfo_t {
  std::string instrument_;
  int strat_id_;
  int lot_size_;
  double ref_px_;
  std::string order_id_;
};
struct StraddleOrderInfo_t {
  std::string shortcode_;
  std::string instrument_; // Here instrument is like NIFTY
  int strat_id_;
  int lots_;
  double ref_px_;
  std::string order_id_;
  double strike_;
  int expiry_;
};
struct GeneralOrderInfo_t {
  // Here general_shortcode_ is synthetically designed, contains positions,
  // constraints etc.
  // COMPLEX%2%NSE_BANKNIFTY_P0_A|NSE_BANKNIFTY_C0_A%-5|-5%1?-1?abs_leq?1%
  std::string general_shortcode_;
  std::string order_id_;
  int strat_id_;
  double ref_px_;
  std::string order_tag_;
  NSE_SIMPLEEXEC::OrderType_t order_type_;
};

typedef std::multimap<uint64_t, GeneralOrderInfo_t>::iterator
    general_order_info_itr;
typedef std::pair<general_order_info_itr, general_order_info_itr>
    general_Range_itr;

namespace NSE_SIMPLEEXEC {
class SimpleNseExecLogicOrderReaderFromFile
    : public SimpleNseExecLogicOrderReader {
  std::multimap<uint64_t, GeneralOrderInfo_t>
      general_order_time_to_order_info_map_; // sim mode
  std::string orders_file_;
  int num_backup_orders_read_;
  bool backup_file_read_;

public:
  SimpleNseExecLogicOrderReaderFromFile(
      HFSAT::Watch &watch_t, HFSAT::DebugLogger &dbglogger_t, bool is_live_t,
      std::string orders_file_t,
      std::string backup_file_t,
      std::map<std::string, SyntheticLegInfo> leg_info_t,
      NseExecLogicHelper *exec_logic_helper_t);

  void LoadAllOrdersInSimMode() override;
  void OnTimePeriodUpdate(const int num_pages_to_add_) override;
  void LookupOrdersInSimMode() override;
  void ReadLiveOrdersFromFile(std::string fname, bool time_checks = true);
  void HandleVanillaOrdersInSim();
  void HandleGeneralizedOrdersInSim();
};
}
