/*
 * book_size.hpp
 *
 *  Created on: 22-Mar-2016
 *      Author: raghuram
 */

#ifndef DVCTRADE_INDICATORS_BOOK_SIZE_HPP_
#define DVCTRADE_INDICATORS_BOOK_SIZE_HPP_

#include "dvctrade/Indicators/common_indicator.hpp"
#include "baseinfra/MarketAdapter/base_market_view_manager.hpp"

namespace HFSAT {

class BookSize : public CommonIndicator {
 protected:
  // variables
  const SecurityMarketView& sec_market_view_;
  unsigned int level_ = 0;
  std::string side_ = "Bid";

  double size_;
  double index_ = 0;

  // functions
 public:
  static void CollectShortCodes(std::vector<std::string>& _shortcodes_affecting_this_indicator_,
                                std::vector<std::string>& _ors_source_needed_vec_,
                                const std::vector<const char*>& _tokens_);

  static BookSize* GetUniqueInstance(DebugLogger& _dbglogger_, const Watch& _watch_,
                                     const std::vector<const char*>& _tokens_, PriceType_t _basepx_pxtype_);

  static BookSize* GetUniqueInstance(DebugLogger& _dbglogger_, const Watch& _watch_,
                                     const SecurityMarketView& _sec_market_view_, unsigned int _level_,
                                     std::string _side_, PriceType_t _basepx_pxtype_);

 protected:
  BookSize(DebugLogger& _dbglogger_, const Watch& _watch_, const std::string& concise_indicator_description_,
           const SecurityMarketView& _sec_market_view_, unsigned int _level_, std::string _side_,
           PriceType_t _price_type_);

 public:
  ~BookSize() {}

  // listener interface
  void OnMarketUpdate(const unsigned int _security_id_, const MarketUpdateInfo& _market_update_info_);
  inline void OnTradePrint(const unsigned int _security_id_, const TradePrintInfo& _trade_print_info_,
                           const MarketUpdateInfo& _market_update_info_) {
    OnMarketUpdate(_security_id_, _market_update_info_);
  }

  inline void OnPortfolioPriceChange(double _new_price_) {}
  inline void OnPortfolioPriceReset(double t_new_price_, double t_old_price_, unsigned int is_data_interrupted_) {}
  bool GetReadinessRequired(const std::string& r_dep_shortcode_, const std::vector<const char*>& tokens_) const {
    return true;
  }
  // functions
  static std::string VarName() { return "BookSize"; }

  void WhyNotReady();

  void OnMarketDataInterrupted(const unsigned int _security_id_, const int msecs_since_last_receive_) {
    data_interrupted_ = true;
    indicator_value_ = 0;
  }
  void OnMarketDataResumed(const unsigned int _security_id_) {
    data_interrupted_ = false;
    indicator_value_ = 0;
  };

 protected:
  void InitializeValues() { indicator_value_ = 0; };
};
}

#endif /* DVCTRADE_INDICATORS_BOOK_SIZE_HPP_ */
