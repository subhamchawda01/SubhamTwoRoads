/**
   \file SimMarketMaker/security_delay_stats.hpp

   \author: (c) Copyright Two Roads Technological Solutions Pvt Ltd 2011
   Address:
   Suite No 353, Evoma, #14, Bhattarhalli,
   Old Madras Road, Near Garden City College,
   KR Puram, Bangalore 560049, India
   +91 80 4190 3551
 */

#pragma once

#include "baseinfra/SimMarketMaker/ors_message_stats.hpp"
#include "baseinfra/MarketAdapter/security_market_view.hpp"

namespace HFSAT {
class GatewayShcMapping {
 public:
  GatewayShcMapping(std::string exch, std::string gateway, std::vector<std::string> shortcode_set,
                    int validfrom_YYYYMMDD)
      : exch_(exch), gateway_(gateway), shortcode_set_(shortcode_set), validfrom_YYYYMMDD_(validfrom_YYYYMMDD) {}

  std::string exch_;
  std::string gateway_;
  std::vector<std::string> shortcode_set_;
  int validfrom_YYYYMMDD_;
};

// The output struct for GetDelayAPIs
struct DelayOutput {
 public:
  DelayOutput() : delay(ttime_t(0, 0)), seqd_time(ttime_t(0, 0)) {}
  DelayOutput(ttime_t delay, ttime_t seqd_time) : delay(delay), seqd_time(seqd_time) {}

  ttime_t delay;      // the actual delay
  ttime_t seqd_time;  // time in live trading on which delay was calculated
};

class SecurityDelayStats {
 public:
  static SecurityDelayStats& GetUniqueInstance(SecurityMarketView* smv, Watch& watch,
                                               TradingLocation_t trading_location, int _normal_delay_usecs);
  static void ResetUniqueInstance(const unsigned security_id, Watch& watch, TradingLocation_t trading_location,
                                  int _normal_delay_usecs);
  static void ResetAllUniqueInstance();
  bool GetSendConfDelay(ttime_t current_time, DelayOutput& send_conf_delay, double seq2conf_multiplier_,
                        int seq2conf_addend_);
  bool GetSendMktDelay(ttime_t current_time, DelayOutput& send_mkt_delay, double seq2conf_multiplier_,
                       int seq2conf_addend_);
  bool GetCancelConfDelay(ttime_t current_time, DelayOutput& cxl_conf_delay, double seq2conf_multiplier_,
                          int seq2conf_addend_);
  bool GetCancelMktDelay(ttime_t current_time, DelayOutput& cxl_mkt_delay, double seq2conf_multiplier_,
                         int seq2conf_addend_);
  bool GetXthPercentileSendConfDelay(int x, ttime_t& send_conf_delay);
  bool GetXthPercentileSendMktDelay(int x, ttime_t& send_mkt_delay);
  bool GetXthPercentileCancelConfDelay(int x, ttime_t& cxl_conf_delay);
  bool GetXthPercentileCancelMktDelay(int x, ttime_t& cxl_mkt_delay);
  ttime_t GetMedianConfDelay() { return ors_message_stats_->GetMedianConfDelay(); }
  ttime_t GetMedianMktDelay() { return ors_message_stats_->GetMedianMktDelay(); }
  std::map<ttime_t, OrsOrderInfo>& GetSeqTimeToOrsOrderMap();
  std::map<ttime_t, OrsOrderInfo>& GetCxlSeqTimeToOrsOrderMap();
  static void LoadGatewayShcMapping(int tradingdate_YYYYMMDD);
  static std::vector<std::string> GetShortCodesInSameGateway(const std::string shortcode);
  void PrintAllDelays();
  void GetTimeStampsInRange(ttime_t start_time, ttime_t end_time, std::vector<ttime_t>& t_ts_vec_);

 private:
  SecurityDelayStats(Watch& watch, SecurityMarketView* smv, TradingLocation_t trading_location,
                     int _normal_delay_usecs);
  ~SecurityDelayStats();
  void GenerateSortedDelays();
  static bool IsNumber(const std::string& str);

  ORSMessageStats* ors_message_stats_;
  DebugLogger* dbglogger_;
  std::map<ttime_t, OrsOrderInfo> cxlseq_time_to_ors_order_map_;
  std::map<ttime_t, OrsOrderInfo> seq_time_to_ors_order_map_;

  std::vector<ttime_t> send_delays_sorted_;
  std::vector<ttime_t> send_mkt_delays_sorted_;
  std::vector<ttime_t> cxl_delays_sorted_;
  std::vector<ttime_t> cxl_mkt_delays_sorted_;
  static std::vector<std::vector<std::string>> shortcodes_in_same_gateway_;
  static std::map<std::pair<unsigned, unsigned>, SecurityDelayStats*> secid_tradingdate_to_delay_stats_map_;
};
}
