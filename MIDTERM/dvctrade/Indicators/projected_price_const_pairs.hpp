/**
    \file Indicators/projected_price_const_pairs.hpp

    \author: (c) Copyright Two Roads Technological Solutions Pvt Ltd 2011
     Address:
         Suite 217, Level 2, Prestige Omega,
         No 104, EPIP Zone, Whitefield,
         Bangalore - 560066, India
         +91 80 4060 0717
 */

#ifndef PROJECTED_PRICE_CONST_PAIRS_HPP_
#define PROJECTED_PRICE_CONST_PAIRS_HPP_

#include "dvctrade/Indicators/common_indicator.hpp"
#include "baseinfra/MarketAdapter/security_market_view.hpp"
#include "dvctrade/Indicators/indicator_util.hpp"
#include "dvctrade/Indicators/pca_weights_manager.hpp"

namespace HFSAT {
class ProjectedPriceConstPairs : public CommonIndicator {
 protected:
  // variables
  const SecurityMarketView &dep_market_view_;
  std::vector<SecurityMarketView *> indep_market_view_vec_;

  PriceType_t price_type_;

  // computational variables

  // last one would be dep in all vectors
  std::vector<double> moving_avg_bid_price_vec_;
  std::vector<double> moving_avg_ask_price_vec_;

  std::vector<double> last_bid_price_vec_;
  std::vector<double> last_ask_price_vec_;
  std::vector<double> volatility_factor_vec_;
  double last_best_bid_price_;
  double last_best_ask_price_;
  double current_dep_price_;
  int last_best_ask_idx_;
  int last_best_bid_idx_;

  std::vector<int> last_new_page_msecs_vec_;
  std::vector<bool> is_ready_vec_;
  std::map<unsigned int, int> security_id_to_idx_map_;
  std::vector<double> min_price_increment_vec_;

  // functions
 public:
  static void CollectShortCodes(std::vector<std::string> &_shortcodes_affecting_this_indicator_,
                                std::vector<std::string> &_ors_source_needed_vec_,
                                const std::vector<const char *> &_tokens_);

  static ProjectedPriceConstPairs *GetUniqueInstance(DebugLogger &_dbglogger_, const Watch &_watch_,
                                                     const std::vector<const char *> &_tokens_,
                                                     PriceType_t _basepx_pxtype_);

  static ProjectedPriceConstPairs *GetUniqueInstance(DebugLogger &_dbglogger_, const Watch &_watch_,
                                                     SecurityMarketView &_dep_market_view_,
                                                     std::string _source_shortcode_, double _fractional_seconds_,
                                                     PriceType_t _price_type_);

 protected:
  ProjectedPriceConstPairs(DebugLogger &_dbglogger_, const Watch &_watch_,
                           const std::string &concise_indicator_description_, SecurityMarketView &_dep_market_view_,
                           std::vector<SecurityMarketView *> &_indep_market_view_vec_, double _fractional_seconds_,
                           PriceType_t _price_type_);

 public:
  ~ProjectedPriceConstPairs() {}

  // listener interface
  void OnMarketUpdate(const unsigned int _security_id_, const MarketUpdateInfo &_market_update_info_);
  inline void OnTradePrint(const unsigned int _security_id_, const TradePrintInfo &_trade_print_info_,
                           const MarketUpdateInfo &_market_update_info_) {
    OnMarketUpdate(_security_id_, _market_update_info_);
  }

  // functions
  /// Used in ModelCreator to see which shortcodes are core
  bool GetReadinessRequired(const std::string &r_dep_shortcode_, const std::vector<const char *> &tokens_) const {
    std::vector<std::string> core_shortcodes_;
    GetCoreShortcodes(r_dep_shortcode_, core_shortcodes_);

    // here tokes 3 and 4 are important

    std::vector<std::string> t_shortcodes_affecting_this_indicator_;

    if (tokens_.size() > 3u) {
      VectorUtils::UniqueVectorAdd(t_shortcodes_affecting_this_indicator_, (std::string)tokens_[3]);
    }
    if (tokens_.size() > 4u) {
      std::string t_source_shortcode_ = (std::string)tokens_[4];
      if (IndicatorUtil::IsPortfolioShortcode(t_source_shortcode_)) {
        IndicatorUtil::AddPortfolioShortCodeVec(t_source_shortcode_, t_shortcodes_affecting_this_indicator_);
      } else {
        VectorUtils::UniqueVectorAdd(t_shortcodes_affecting_this_indicator_, t_source_shortcode_);
      }
    }

    for (auto i = 0u; i < t_shortcodes_affecting_this_indicator_.size(); i++) {
      if (VectorUtils::LinearSearchValue(core_shortcodes_, t_shortcodes_affecting_this_indicator_[i])) {
        return true;
      }
    }
    return false;
  }

  static std::string VarName() { return "ProjectedPriceConstPairs"; }

  void OnMarketDataInterrupted(const unsigned int _security_id_, const int msecs_since_last_receive_);
  void OnMarketDataResumed(const unsigned int _security_id_);
  bool AreAllReady();

  inline void OnPortfolioPriceChange(double _new_price_) {}
  inline void OnPortfolioPriceReset(double t_new_price_, double t_old_price_, unsigned int is_data_interrupted_){};

 protected:
  void InitializeValues(int _update_idx_);
};
}

#endif /* PROJECTED_PRICE_CONST_PAIRS_HPP_ */
