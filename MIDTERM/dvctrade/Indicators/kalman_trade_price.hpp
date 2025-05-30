/*
 * kalman_trade_price.hpp
 *
 *  Created on: 07-Oct-2015
 *      Author: raghuram
 */

#ifndef INDICATORS_KALMAN_TRADE_PRICE_HPP_
#define INDICATORS_KALMAN_TRADE_PRICE_HPP_

#include "dvctrade/kalman/ekfilter.hpp"
#include "baseinfra/MarketAdapter/security_market_view.hpp"
#include "dvctrade/Indicators/common_indicator.hpp"

namespace Kalman {

/// Simple Indicator which returns the price of the given SecurityMarketView
/// depending on the price_type_ given
class KalmanTradePrice : public Kalman::EKFilter<double, 1>, public HFSAT::CommonIndicator {
 protected:
  // variables

  HFSAT::SecurityMarketView& indep_market_view_;
  double init_state_;
  double init_covar_;

  double sigma_e_;
  double sigma_s_;
  double avg_trade_size_ = 0;
  double current_traded_price_ = 0;
  double current_traded_size_ = 0;
  double current_kalman_price_ = 0;
  double current_mid_price_ = 0;

  double last_traded_price_ = 0;
  double last_mid_price_ = 0;
  const HFSAT::PriceType_t price_type_;

 public:
  static void CollectShortCodes(std::vector<std::string>& _shortcodes_affecting_this_indicator_,
                                std::vector<std::string>& _ors_source_needed_vec_,
                                const std::vector<const char*>& _tokens_);
  static KalmanTradePrice* GetUniqueInstance(HFSAT::DebugLogger& t_dbglogger_, const HFSAT::Watch& r_watch_,
                                             const std::vector<const char*>& r_tokens_,
                                             HFSAT::PriceType_t _basepx_pxtype_);

  static KalmanTradePrice* GetUniqueInstance(HFSAT::DebugLogger& _dbglogger_, const HFSAT::Watch& _watch_,
                                             HFSAT::SecurityMarketView& _indep_market_view_, double _init_covar_,
                                             double _sigma_e_, double _sigma_s_, HFSAT::PriceType_t _price_type_);

  KalmanTradePrice(HFSAT::DebugLogger& _dbglogger_, const HFSAT::Watch& _watch_,
                   const std::string& concise_indicator_description_, HFSAT::SecurityMarketView& _indep_market_view_,
                   double _init_covar_, double _sigma_e_, double _sigma_s_, HFSAT::PriceType_t _price_type_);

  ~KalmanTradePrice() {}

  // listener interface
  void OnMarketUpdate(const unsigned int _security_id_, const HFSAT::MarketUpdateInfo& _market_update_info_);
  void OnTradePrint(const unsigned int _security_id_, const HFSAT::TradePrintInfo& _trade_print_info_,
                    const HFSAT::MarketUpdateInfo& _market_update_info_);

  inline void OnPortfolioPriceChange(double _new_price_) {}
  inline void OnPortfolioPriceReset(double t_new_price_, double t_old_price_, unsigned int is_data_interrupted_) {}

  inline void OnMarketDataInterrupted(const unsigned int _security_id_, const int msecs_since_last_receive_) {}
  inline void OnMarketDataResumed(const unsigned int _security_id_) {}
  // functions

  // functions
  static std::string VarName() { return "Kalman::KalmanTradePrice"; }
  bool GetReadinessRequired(const std::string& r_dep_shortcode_, const std::vector<const char*>& tokens_) const {
    return true;
  }

  // kalman filter functions
  void makeA();
  void makeH();
  void makeV();
  void makeR();
  void makeW();
  void makeQ();
  void makeProcess();
  void makeMeasure();
};
}

#endif /* INDICATORS_KALMAN_TRADE_PRICE_HPP_ */
