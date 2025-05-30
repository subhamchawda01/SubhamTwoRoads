/*
 * spread_implied_vol_price.hpp
 *
 *  Created on: 15-Oct-2015
 *      Author: raghuram
 */

#ifndef INDICATORSCODE_SPREAD_IMPLIED_VOL_PRICE_HPP_
#define INDICATORSCODE_SPREAD_IMPLIED_VOL_PRICE_HPP_

#include "dvctrade/Indicators/common_indicator.hpp"
#include "dvctrade/Indicators/exponential_moving_average.hpp"
#include "baseinfra/MarketAdapter/base_market_view_manager.hpp"

namespace HFSAT {

/// Indicator that computes the trend ( current price - moving average )
/// then scales it by the standard deviation, to see what multiple of the stdev is this move
/// Returns that
class SpreadImpliedVolPrice : public CommonIndicator, public IndicatorListener {
 protected:
  // variables
  const std::vector<SecurityMarketView*> all_market_views_;

  double last_vx_spot_;
  double curr_vx_spot_;

  std::vector<double> last_price_vec_;
  std::vector<double> last_imp_vol_vec_;
  std::vector<double> last_imp_var_vec_;

  std::vector<double> curr_price_vec_;
  std::vector<double> curr_imp_vol_vec_;
  std::vector<double> curr_imp_var_vec_;

  double fractional_seconds_;
  std::vector<ExponentialMovingAverage*> moving_avg_ind_vec_;
  PriceType_t price_type_;
  double ttm_0_;
  std::vector<bool> ready_vec_;
  unsigned int num_updated_;

  double current_sp_price_ = 0;

  // functions
 public:
  static void CollectShortCodes(std::vector<std::string>& _shortcodes_affecting_this_indicator_,
                                std::vector<std::string>& _ors_source_needed_vec_,
                                const std::vector<const char*>& _tokens_);

  static SpreadImpliedVolPrice* GetUniqueInstance(DebugLogger& _dbglogger_, const Watch& _watch_,
                                                  const std::vector<const char*>& _tokens_,
                                                  PriceType_t _basepx_pxtype_);

  static SpreadImpliedVolPrice* GetUniqueInstance(DebugLogger& _dbglogger_, const Watch& _watch_,
                                                  const std::vector<SecurityMarketView*> _all_market_views_,
                                                  double _fractional_seconds_, PriceType_t _price_type_);

 protected:
  SpreadImpliedVolPrice(DebugLogger& _dbglogger_, const Watch& _watch_,
                        const std::string& concise_indicator_description_,
                        const std::vector<SecurityMarketView*> _all_market_views_, double _fractional_seconds_,
                        PriceType_t _price_type_);

 public:
  ~SpreadImpliedVolPrice() {}

  // listener interface
  void OnMarketUpdate(const unsigned int _security_id_, const MarketUpdateInfo& _market_update_info_){};
  inline void OnTradePrint(const unsigned int _security_id_, const TradePrintInfo& _trade_print_info_,
                           const MarketUpdateInfo& _market_update_info_) {
    OnMarketUpdate(_security_id_, _market_update_info_);
  }

  void OnIndicatorUpdate(const unsigned int& _indicator_index_, const double& _new_value_);
  void OnIndicatorUpdate(const unsigned int& _indicator_index_, const double& _new_value_decrease_,
                         const double& _new_value_nochange_, const double& _new_value_increase_){};

  inline void OnPortfolioPriceChange(double _new_price_) {}
  inline void OnPortfolioPriceReset(double t_new_price_, double t_old_price_, unsigned int is_data_interrupted_) {}

  // functions
  static std::string VarName() { return "SpreadImpliedVolPrice"; }

  void OnMarketDataInterrupted(const unsigned int _security_id_, const int msecs_since_last_receive_){};
  void OnMarketDataResumed(const unsigned int _security_id_);

  void InitializeValues();

 protected:
  static std::vector<std::string> GetShortCodes(std::string dep_shortcode);
  double GetVXSpot(double p_0, double p_1);
};
}

#endif /* INDICATORSCODE_SPREAD_IMPLIED_VOL_PRICE_HPP_ */
