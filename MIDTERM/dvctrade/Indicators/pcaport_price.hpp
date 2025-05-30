/**
    \file Indicators/pcaport_price.hpp

    \author: (c) Copyright Two Roads Technological Solutions Pvt Ltd 2011
     Address:
         Suite No 353, Evoma, #14, Bhattarhalli,
         Old Madras Road, Near Garden City College,
         KR Puram, Bangalore 560049, India
         +91 80 4190 3551
 */

#ifndef BASE_INDICATORS_PCAPORT_PRICE_H
#define BASE_INDICATORS_PCAPORT_PRICE_H

#include "baseinfra/MarketAdapter/security_market_view_change_listener.hpp"
#include "dvctrade/Indicators/portfolio_price_change_listener.hpp"

#include "dvccode/CDef/debug_logger.hpp"
#include "dvccode/CommonTradeUtils/watch.hpp"

#include "baseinfra/MarketAdapter/security_market_view.hpp"
#include "baseinfra/TradeUtils/market_update_manager_listener.hpp"
#include "dvccode/CommonTradeUtils/historic_price_manager.hpp"

#define USE_PCA_PORTFOLIO_PRICE

namespace HFSAT {

class PCAPortPrice : public SecurityMarketViewChangeListener,
                     public MarketDataInterruptedListener,
                     public SecurityMarketViewStatusListener {
 protected:
  // variables
  DebugLogger& dbglogger_;
  const Watch& watch_;
  SecurityNameIndexer& sec_name_indexer_;

  std::string portfolio_descriptor_shortcode_;
  const PriceType_t price_type_;

  double current_price_;  ///< Default value = 1000, using a large enough value so that this does not approach zero and
  /// indicators liek OnlineComputedPairsPort can wrok for now

  std::vector<PortfolioPriceChangeListener*> price_change_listener_vec_;

  bool is_ready_;

  std::vector<double> security_id_weight_map_;
  std::vector<double> current_security_id_weight_map_;
  std::vector<bool> data_interrupted_map_;
  std::vector<bool> security_id_is_ready_map_;
  std::vector<double> security_id_last_price_map_;
  std::vector<SecurityMarketView*> indep_market_view_vec_;  ///< right now only used in WhyNotReady
  std::vector<int> security_id_to_id_vec_;
  std::vector<MktStatus_t> market_status_vec_;
  double min_price_increment_;

  double sum_of_eigen_compo_by_stdev_;  ///< only used by classes which need the normalization constant
  bool is_pca_;

  // functions
  PCAPortPrice(DebugLogger& _dbglogger_, const Watch& _watch_, const std::string& t_portfolio_descriptor_shortcode_,
               const PriceType_t& _price_type_);

 public:
  static void CollectShortCodes(std::vector<std::string>& _shortcodes_affecting_this_indicator_,
                                const std::string& t_portfolio_descriptor_shortcode_);

  static PCAPortPrice* GetUniqueInstance(DebugLogger& _dbglogger_, const Watch& _watch_,
                                         const std::string& t_portfolio_descriptor_shortcode_,
                                         const PriceType_t& _price_type_);

  ~PCAPortPrice() {}

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

  inline double get_normalizing_factor() const { return sum_of_eigen_compo_by_stdev_; }
  void WhyNotReady();

  // data interrupt listener interface
  void OnMarketDataInterrupted(const unsigned int _security_id_, const int msecs_since_last_receive_);
  void OnMarketDataResumed(const unsigned int _security_id_);
  void OnMarketStatusChange(const unsigned int _security_id_, const MktStatus_t _new_market_status_);

 protected:
  bool AreAllReady();
};
}

#endif  // BASE_INDICATORS_PCAPORT_PRICE_H
