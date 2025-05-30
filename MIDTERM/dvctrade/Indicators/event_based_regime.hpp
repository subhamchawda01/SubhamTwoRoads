/**
   \file Indicators/size_based_regime.hpp

   \author: (c) Copyright Two Roads Technological Solutions Pvt Ltd 2011
   Address:
   Suite No 351, Evoma, #14, Bhattarhalli,
   Old Madras Road, Near Garden City College,
   KR Puram, Bangalore 560049, India
   +91 80 4190 3551
*/
#pragma once

#include "dvctrade/Indicators/common_indicator.hpp"
#include "dvccode/CommonTradeUtils/economic_events_manager.hpp"
#include "baseinfra/MarketAdapter/shortcode_security_market_view_map.hpp"

namespace HFSAT {

struct EventTime {
  int event_start_mfm_;
  int event_end_mfm_;

  EventTime() : event_start_mfm_(0), event_end_mfm_(0) {}
  EventTime(int _event_start_mfm_, int _event_end_mfm_)
      : event_start_mfm_(_event_start_mfm_), event_end_mfm_(_event_end_mfm_) {}
};

class EventBasedRegime : public CommonIndicator, public TimePeriodListener {
 protected:
  // variables
  SecurityMarketView& dep_market_view_;
  EconomicEventsManager economic_events_manager_;

  EconomicZone_t ezone_traded_;

  int applicable_severity_;
  int event_start_lag_msecs_;
  int event_end_lag_msecs_;
  std::vector<HFSAT::EventTime> event_time_for_day_;
  int current_idx_;
  int event_vec_size_;

 protected:
  EventBasedRegime(DebugLogger& _dbglogger_, const Watch& _watch_, const std::string& concise_indicator_description_,
                   SecurityMarketView& _dep_market_view_, std::string _event_zone_, int _applicable_severity_,
                   int _start_time_lag_mins_, int _end_time_lag_mins_);

 public:
  static void CollectShortCodes(std::vector<std::string>& _shortcodes_affecting_this_indicator_,
                                std::vector<std::string>& _ors_source_needed_vec_,
                                const std::vector<const char*>& _tokens_);

  static EventBasedRegime* GetUniqueInstance(DebugLogger& _dbglogger_, const Watch& _watch_,
                                             const std::vector<const char*>& _tokens_, PriceType_t _basepx_pxtype_);

  static EventBasedRegime* GetUniqueInstance(DebugLogger& _dbglogger_, const Watch& _watch_,
                                             SecurityMarketView& _dep_market_view_, std::string _event_zone_,
                                             int _applicable_severity_, int _start_time_lag_mins_,
                                             int _end_time_lag_mins_);

  ~EventBasedRegime() {}

  // listener interface
  void OnTimePeriodUpdate(const int num_pages_to_add_);

  inline void OnMarketUpdate(const unsigned int _security_id_, const MarketUpdateInfo& _market_update_info_){};
  inline void OnTradePrint(const unsigned int _security_id_, const TradePrintInfo& _trade_print_info_,
                           const MarketUpdateInfo& _market_update_info_){};
  inline void OnPortfolioPriceChange(double _new_price_){};
  inline void OnPortfolioPriceReset(double t_new_price_, double t_old_price_, unsigned int is_data_interrupted_){};

  // functions
  /// Used in ModelCreator to see which shortcodes are core
  bool GetReadinessRequired(const std::string& r_dep_shortcode_, const std::vector<const char*>& tokens_) const {
    return true;
  }

  static std::string VarName() { return "EventBasedRegime"; }

  void WhyNotReady();

  inline void SubscribeDataInterrupts(MarketUpdateManager& market_update_manager_) {}

  void OnMarketDataInterrupted(const unsigned int _security_id_, const int msecs_since_last_receive_);
  void OnMarketDataResumed(const unsigned int _security_id_);

 protected:
  void InitializeValues();
};
}
