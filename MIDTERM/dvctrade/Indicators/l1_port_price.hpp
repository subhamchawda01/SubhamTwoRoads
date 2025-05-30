/**
    \file Indicators/l1_port_price.hpp

    \author: (c) Copyright Two Roads Technological Solutions Pvt Ltd 2011
     Address:
         Suite No 162, Evoma, #14, Bhattarhalli,
         Old Madras Road, Near Garden City College,
         KR Puram, Bangalore 560049, India
         +91 80 4190 3551
*/
#ifndef BASE_INDICATORS_L1_PORT_PRICE_H
#define BASE_INDICATORS_L1_PORT_PRICE_H

#include "dvctrade/Indicators/common_indicator.hpp"
#include "dvctrade/Indicators/pcaport_price.hpp"

namespace HFSAT {

class L1PortPrice : public CommonIndicator {
 protected:
  // variables
  PCAPortPrice* portfolio_price_;
  const PriceType_t price_type_;

  // functions
 public:
  static void CollectShortCodes(std::vector<std::string>& _shortcodes_affecting_this_indicator_,
                                std::vector<std::string>& _ors_source_needed_vec_,
                                const std::vector<const char*>& _tokens_);

  static L1PortPrice* GetUniqueInstance(DebugLogger& _dbglogger_, const Watch& _watch_,
                                        const std::vector<const char*>& _tokens_, PriceType_t _basepx_pxtype_);

  static L1PortPrice* GetUniqueInstance(DebugLogger& _dbglogger_, const Watch& _watch_,
                                        const std::string _portfolio_shc_, PriceType_t _price_type_);

 protected:
  L1PortPrice(DebugLogger& _dbglogger_, const Watch& _watch_, const std::string& concise_indicator_description_,
              const std::string _portfolio_shc_, PriceType_t _price_type_);

 public:
  ~L1PortPrice() {}

  // listener interface
  void OnMarketUpdate(const unsigned int _security_id_, const MarketUpdateInfo& _market_update_info_) {}
  inline void OnTradePrint(const unsigned int _security_id_, const TradePrintInfo& _trade_print_info_,
                           const MarketUpdateInfo& _market_update_info_) {
    OnMarketUpdate(_security_id_, _market_update_info_);
  }

  inline void OnPortfolioPriceChange(double _new_price_);
  inline void OnPortfolioPriceReset(double t_new_price_, double t_old_price_, unsigned int is_data_interrupted_);
  bool GetReadinessRequired(const std::string& r_dep_shortcode_, const std::vector<const char*>& tokens_) const {
    return true;
  }
  // functions
  static std::string VarName() { return "L1PortPrice"; }

  void WhyNotReady();
  inline void SubscribeDataInterrupts(MarketUpdateManager& market_update_manager_) {
    market_update_manager_.AddMarketDataInterruptedListener(portfolio_price_);
  }

  void OnMarketDataInterrupted(const unsigned int _security_id_, const int msecs_since_last_receive_);
  void OnMarketDataResumed(const unsigned int _security_id_);

 protected:
  void InitializeValues();
};
}

#endif  // BASE_INDICATORS_STUDENT_PRICE_H
