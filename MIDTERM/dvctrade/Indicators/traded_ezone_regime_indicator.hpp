
#ifndef BASE_INDICATORS_TRADED_EZONE_REGIME_H
#define BASE_INDICATORS_TRADED_EZONE_REGIME_H

#include "dvctrade/Indicators/common_indicator.hpp"
#include "baseinfra/MarketAdapter/base_market_view_manager.hpp"
#include "dvccode/CommonTradeUtils/economic_events_manager.hpp"

namespace HFSAT {

/// Class returning current price minus moving average,
/// i.e. exponentially time-decaying moving average
class TradedEzoneRegimeIndicator : public CommonIndicator, public TimePeriodListener {
 protected:
  EconomicEventsManager eco_event_manager_;

  // functions
 protected:
  TradedEzoneRegimeIndicator(DebugLogger& t_dbglogger_, const Watch& r_watch_,
                             const std::string& concise_indicator_description_, const std::string& traded_ezone_,
                             PriceType_t _price_type_);

 public:
  ~TradedEzoneRegimeIndicator() {}
  static void CollectShortCodes(std::vector<std::string>& _shortcodes_affecting_this_indicator_,
                                std::vector<std::string>& _ors_source_needed_vec_,
                                const std::vector<const char*>& r_tokens_);

  static TradedEzoneRegimeIndicator* GetUniqueInstance(DebugLogger& t_dbglogger_, const Watch& r_watch_,
                                                       const std::vector<const char*>& r_tokens_,
                                                       PriceType_t _price_type_);

  static TradedEzoneRegimeIndicator* GetUniqueInstance(DebugLogger& t_dbglogger_, const Watch& r_watch_,
                                                       const std::string& traded_ezone_, PriceType_t _price_type_);

  inline void OnMarketUpdate(const unsigned int _security_id_, const MarketUpdateInfo& _market_update_info_) {}
  inline void OnTradePrint(const unsigned int _security_id_, const TradePrintInfo& _trade_print_info_,
                           const MarketUpdateInfo& _market_update_info_) {}

  bool IsReady() { return is_ready_; }

  inline void OnPortfolioPriceChange(double _new_price_) {}
  inline void OnPortfolioPriceReset(double t_new_price_, double t_old_price_, unsigned int is_data_interrupted_) {}

  // functions
  static std::string VarName() { return "TradedEzoneRegimeIndicator"; }

  void WhyNotReady() {}

  void OnMarketDataInterrupted(const unsigned int _security_id_, const int msecs_since_last_receive_) {}
  void OnMarketDataResumed(const unsigned int _security_id_) {}

  // listener interface
  void OnTimePeriodUpdate(const int num_pages_to_add_);
};
}

#endif  // BASE_INDICATORS_TRADED_EZONE_REGIME_H
