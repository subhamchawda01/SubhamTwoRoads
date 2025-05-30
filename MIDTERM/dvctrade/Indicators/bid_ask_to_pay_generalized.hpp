/**
    \file Indicators/bid_ask_to_pay_generalized.hpp

    \author: (c) Copyright Two Roads Technological Solutions Pvt Ltd 2011
     Address:
         Suite No 353, Evoma, #14, Bhattarhalli,
         Old Madras Road, Near Garden City College,
         KR Puram, Bangalore 560049, India
         +91 80 4190 3551
*/

#ifndef BASE_INDICATORS_BID_ASK_TO_PAY_GENERALIZED
#define BASE_INDICATORS_BID_ASK_TO_PAY_GENERALIZED

#include "dvctrade/Indicators/common_indicator.hpp"
#include "baseinfra/MarketAdapter/base_market_view_manager.hpp"

namespace HFSAT {

class BidAskToPayGeneralized : public CommonIndicator {
 protected:
  // variables
  SecurityMarketView& indep_market_view_;
  PriceType_t price_type_;
  const unsigned int num_levels_;
  const unsigned int quantity_to_seek_;

  // computational variables
  std::vector<double> decay_vector_;

  // functions
 protected:
  BidAskToPayGeneralized(DebugLogger& _dbglogger_, const Watch& _watch_,
                         const std::string& concise_indicator_description_, SecurityMarketView& _indep_market_view_,
                         unsigned int _num_levels_, unsigned int _size_to_seek_, PriceType_t price_type_);

 public:
  static void CollectShortCodes(std::vector<std::string>& _shortcodes_affecting_this_indicator_,
                                std::vector<std::string>& _ors_source_needed_vec_,
                                const std::vector<const char*>& _tokens_);

  static BidAskToPayGeneralized* GetUniqueInstance(DebugLogger& _dbglogger_, const Watch& _watch_,
                                                   const std::vector<const char*>& _tokens_, PriceType_t _basepx_type_);

  static BidAskToPayGeneralized* GetUniqueInstance(DebugLogger& _dbglogger_, const Watch& _watch_,
                                                   SecurityMarketView& _indep_market_view_, unsigned int _num_levels_,
                                                   unsigned int _size_to_seek_, PriceType_t _price_type_);

 public:
  ~BidAskToPayGeneralized() {}

  // listener interface
  void OnMarketUpdate(const unsigned int _security_id_, const MarketUpdateInfo& _market_update_info_);
  inline void OnTradePrint(const unsigned int _security_id_, const TradePrintInfo& _trade_print_info_,
                           const MarketUpdateInfo& _market_update_info_) {
    OnMarketUpdate(_security_id_, _market_update_info_);
  }

  inline void OnPortfolioPriceChange(double _new_price_) {}
  inline void OnPortfolioPriceReset(double t_new_price_, double t_old_price_, unsigned int is_data_interrupted_){};

  // functions
  static std::string VarName() { return "BidAskToPayGeneralized"; }
  void OnMarketDataInterrupted(const unsigned int _security_id_, const int msecs_since_last_receive_);
  void OnMarketDataResumed(const unsigned int _security_id_);
  void OnMarketStatusChange(const unsigned int _security_id_, const MktStatus_t _new_market_status_);
};
}
#endif
