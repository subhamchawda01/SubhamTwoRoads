/**
    \file Indicators/portfolio_price.hpp

    \author: (c) Copyright Two Roads Technological Solutions Pvt Ltd 2011
     Address:
         Suite No 353, Evoma, #14, Bhattarhalli,
         Old Madras Road, Near Garden City College,
         KR Puram, Bangalore 560049, India
         +91 80 4190 3551
*/
#ifndef BASE_INDICATORS_PORTFOLIO_PRICE_H
#define BASE_INDICATORS_PORTFOLIO_PRICE_H

#include <map>

#include "baseinfra/MarketAdapter/security_market_view_change_listener.hpp"
#include "dvctrade/Indicators/portfolio_price_change_listener.hpp"

#include "dvccode/CDef/debug_logger.hpp"
#include "dvccode/CommonTradeUtils/watch.hpp"

#include "baseinfra/MarketAdapter/security_market_view.hpp"

namespace HFSAT {

/// Basic class that takes a t_portfolio_descriptor_shortcode_,
/// from that calculates the SecurityMarketView objects of interest and subscribes to them.
/// For computation of aggregated price to do minimum computation,
/// the change from last recorded price is calculated
/// and the change multiplied by the weight of that security is added to the current_price_
///
/// THIS CLASS SHOULD ONLY BE USED TO GET PRICE CHANGES OF THE PORTFOLIO
/// TODO look at OnlineComputedPairsPort ... it uses the value returned by PortfolioPrice::current_price() as well
/// Hence weird values like less than 1 or -ve or 0 could be a problem.
///
/// Typically PortfolioPrice should be used with very intuitive portfolios like
/// "UBFUT" or "UEQUI" "CRBCIDXFUT" "GLDOIL" etc
/// But for most dependants we should make a version like
/// "UBFUT2S", etc and compute intelligent weights offline such that
/// (i) when multiplied with weights the stdev ( indep_portfolio_price_ ) and stdev ( dep_market_view_.price ) is same
///
/// So if we make sure that OnlineComputedPairsPort and OnlineComputedNegativelyCorrelatedPairsPort are only used with
/// portfolios like
/// "UBFUT2S" with the above conditions true then we should be fine, since the price value is expected to be in the
/// range of prices of the constituents of the portfolio
/// and hence values are typically expected to be >> 1

class PortfolioPrice : public SecurityMarketViewChangeListener {
 protected:
  // variables
  DebugLogger& dbglogger_;
  const Watch& watch_;

  std::string portfolio_descriptor_shortcode_;
  const PriceType_t price_type_;

  double current_price_;  ///< Default value = 1000, using a large enough value so that this does not approach zero and
  /// indicators liek OnlineComputedPairsPort can wrok for now

  std::vector<PortfolioPriceChangeListener*> price_change_listener_vec_;

  bool is_ready_;

  std::vector<double> security_id_weight_map_;
  std::vector<bool> security_id_is_ready_map_;
  std::vector<double> security_id_last_price_map_;
  std::vector<SecurityMarketView*> indep_market_view_vec_;  ///< right now only used in WhyNotReady

  double min_price_increment_;

  // functions
  PortfolioPrice(DebugLogger& _dbglogger_, const Watch& _watch_, const std::string& t_portfolio_descriptor_shortcode_,
                 const PriceType_t& _price_type_);

 public:
  static void CollectShortCodes(std::vector<std::string>& _shortcodes_affecting_this_indicator_,
                                const std::string& t_portfolio_descriptor_shortcode_);

  static PortfolioPrice* GetUniqueInstance(DebugLogger& _dbglogger_, const Watch& _watch_,
                                           const std::string& t_portfolio_descriptor_shortcode_,
                                           const PriceType_t& _price_type_);

  ~PortfolioPrice() {}

  inline const std::string& shortcode() const { return portfolio_descriptor_shortcode_; }
  inline bool is_ready() const { return is_ready_; }
  inline double current_price() const { return current_price_; }

  inline double min_price_increment() const { return min_price_increment_; }

  void OnMarketUpdate(const unsigned int _security_id_, const MarketUpdateInfo& _market_update_info_);
  inline void OnTradePrint(const unsigned int _security_id_, const TradePrintInfo& _trade_print_info_,
                           const MarketUpdateInfo& _market_update_info_) {
    OnMarketUpdate(_security_id_, _market_update_info_);
  }
  void AddPriceChangeListener(
      PortfolioPriceChangeListener*);  // right now not adding any index unlike indicatorlistenerpair

  inline void OnMarketDataInterrupted(const unsigned int _security_id_, const int msecs_since_last_receive_);
  inline void OnMarketDataResumed(const unsigned int _security_id_);

  void WhyNotReady();

 protected:
  bool AreAllReady();
};
}

#endif  // BASE_INDICATORS_PORTFOLIO_PRICE_H
