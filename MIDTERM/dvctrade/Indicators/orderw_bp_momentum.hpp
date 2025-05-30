/**
    \file Indicators/orderw_bp_momentum.hpp

    \author: (c) Copyright Two Roads Technological Solutions Pvt Ltd 2011
     Address:
         Suite No 353, Evoma, #14, Bhattarhalli,
         Old Madras Road, Near Garden City College,
         KR Puram, Bangalore 560049, India
         +91 80 4190 3551
*/
#ifndef BASE_INDICATORS_ORDERW_BP_MOMENTUM_H
#define BASE_INDICATORS_ORDERW_BP_MOMENTUM_H

#include "dvccode/ExternalData/external_time_listener.hpp"
#include "dvctrade/Indicators/common_indicator.hpp"
#include "baseinfra/MarketAdapter/base_market_view_manager.hpp"

namespace HFSAT {

#define MAX_LEVEL_DECAY_VECTOR_SIZE 12u
#define MAX_TIME_DECAY_VECTOR_SIZE 64
/// Time Decayed Level Decayed Book Pressure Indicator
class OrderWBPMomentum : public CommonIndicator, public SMVPLChangeListener, public TimePeriodListener {
 protected:
  // variables
  SecurityMarketView& indep_market_view_;

  const int num_levels_;
  const double level_decay_factor_;
  const int book_change_history_halflife_msecs_;

  // computational variables
  double level_decay_vector_[MAX_LEVEL_DECAY_VECTOR_SIZE];
  double time_decay_vector_[MAX_TIME_DECAY_VECTOR_SIZE];

  double bidpressure_;
  double askpressure_;

  // functions
 protected:
  OrderWBPMomentum(DebugLogger& _dbglogger_, const Watch& _watch_, const std::string& concise_indicator_description_,
                   SecurityMarketView& _indep_market_view_, unsigned int _num_levels_, double _level_decay_factor_,
                   double _fractional_seconds_);

 public:
  static void CollectShortCodes(std::vector<std::string>& _shortcodes_affecting_this_indicator_,
                                std::vector<std::string>& _ors_source_needed_vec_,
                                const std::vector<const char*>& _tokens_);

  static OrderWBPMomentum* GetUniqueInstance(DebugLogger& _dbglogger_, const Watch& _watch_,
                                             const std::vector<const char*>& _tokens_, PriceType_t _basepx_pxtype_);

  static OrderWBPMomentum* GetUniqueInstance(DebugLogger& _dbglogger_, const Watch& _watch_,
                                             SecurityMarketView& _indep_market_view_, unsigned int _num_levels_,
                                             double _decay_factor_, double _fractional_seconds_,
                                             PriceType_t _basepx_pxtype_);

 public:
  ~OrderWBPMomentum() {}

  // listener interface
  void OnMarketUpdate(const unsigned int _security_id_, const MarketUpdateInfo& _market_update_info_);
  void OnTradePrint(const unsigned int _security_id_, const TradePrintInfo& _trade_print_info_,
                    const MarketUpdateInfo& _market_update_info_) {}
  void OnPortfolioPriceChange(double _new_price_) {}
  void OnPortfolioPriceReset(double _new_price_, double _old_price_, unsigned int is_data_interrupted_) {}
  void OnMarketDataInterrupted(const unsigned int _security_id_, const int msecs_since_last_receive_);
  void OnMarketDataResumed(const unsigned int _security_id_);
  // functions

  void OnPLNew(const unsigned int t_security_id_, const MarketUpdateInfo& r_market_update_info_,
               const TradeType_t t_buysell_, const int t_level_added_, const int t_old_size_, const int t_new_size_,
               const int t_old_ordercount_, const int t_new_ordercount_, const int t_int_price_,
               const int t_int_price_level_, const bool t_is_intermediate_message_);

  void OnPLDelete(const unsigned int t_security_id_, const MarketUpdateInfo& r_market_update_info_,
                  const TradeType_t t_buysell_, const int t_level_removed_, const int t_old_size_,
                  const int t_old_ordercount_, const int t_int_price_, const int t_int_price_level_,
                  const bool t_is_intermediate_message_);

  void OnPLChange(const unsigned int t_security_id_, const MarketUpdateInfo& r_market_update_info_,
                  const TradeType_t t_buysell_, const int t_level_changed_, const int t_int_price_,
                  const int t_int_price_level_, const int t_old_size_, const int t_new_size_,
                  const int t_old_ordercount_, const int t_new_ordercount_, const bool t_is_intermediate_message_);

  inline void OnTimePeriodUpdate(const int num_pages_to_add_) {
    if (num_pages_to_add_ < MAX_TIME_DECAY_VECTOR_SIZE) {
      bidpressure_ *= time_decay_vector_[num_pages_to_add_];
      askpressure_ *= time_decay_vector_[num_pages_to_add_];
    } else {
      bidpressure_ = 0;
      askpressure_ = 0;
    }
  }
#undef MAX_TIME_DECAY_VECTOR_SIZE
#undef MAX_LEVEL_DECAY_VECTOR_SIZE

  // functions
  static std::string VarName() { return "OrderWBPMomentum"; }
};
}

#endif  // BASE_INDICATORS_ORDERW_BP_MOMENTUM_H
