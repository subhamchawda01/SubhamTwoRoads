/*
 * vx_spot_price.hpp
 *
 *  Created on: 28-Sep-2015
 *      Author: raghuram
 */

#pragma once
#include "dvctrade/kalman/ekfilter.hpp"
#include "baseinfra/MarketAdapter/security_market_view.hpp"
#include "dvctrade/Indicators/common_indicator.hpp"

namespace Kalman {
// static Kalman::EKFilter<double, 1> abc;
// using namespace Kalman;
/// Simple Indicator which returns the price of the given SecurityMarketView
/// depending on the price_type_ given
class VXSpotPrice : public Kalman::EKFilter<double, 1>, public HFSAT::CommonIndicator {
 protected:
  // variables

  HFSAT::SecurityMarketView& indep_market_view_;
  double r_;      // risk free rate
  double theta_;  // mean reversion level;
  double k_;      // mean reversion speed
  double v_;      // vol of vol
  int last_updated_time_;
  int current_time_ = 0;
  double last_indep_price_ = 0;
  double current_indep_price_ = 0;
  double last_spot_price_ = 0;
  double current_spot_price_ = 0;
  double init_spot_price_;
  double init_spot_var_;
  double p_;
  double rho_;
  double dt_ = 0;
  double time_factor_ = 1.0 / (252.0 * 3600 * 8);
  const HFSAT::PriceType_t price_type_;

 public:
  // functions
  //  typedef Kalman::KVector<double, 1, true> Vector;  //!< Vector type.
  //  typedef Kalman::KMatrix<double, 1, true> Matrix;  //!< Matrix type.

  static void CollectShortCodes(std::vector<std::string>& _shortcodes_affecting_this_indicator_,
                                std::vector<std::string>& _ors_source_needed_vec_,
                                const std::vector<const char*>& _tokens_);
  static VXSpotPrice* GetUniqueInstance(HFSAT::DebugLogger& t_dbglogger_, const HFSAT::Watch& r_watch_,
                                        const std::vector<const char*>& r_tokens_, HFSAT::PriceType_t _basepx_pxtype_);

  static VXSpotPrice* GetUniqueInstance(HFSAT::DebugLogger& _dbglogger_, const HFSAT::Watch& _watch_,
                                        HFSAT::SecurityMarketView& _indep_market_view_, double _ir_, double _mrl_,
                                        double _mrs_, double _vol_vol_, double init_spot_price_, double _init_spot_var_,
                                        double _power_, double _rho_, HFSAT::PriceType_t _price_type_);

  VXSpotPrice(HFSAT::DebugLogger& _dbglogger_, const HFSAT::Watch& _watch_,
              const std::string& concise_indicator_description_, HFSAT::SecurityMarketView& _indep_market_view_,
              double _ir_, double _mrl_, double _mrs_, double _vol_vol, double _init_spot_price_,
              double _init_spot_var_, double _power_, double _rho_, HFSAT::PriceType_t _price_type_);

  ~VXSpotPrice() {}

  // listener interface
  void OnMarketUpdate(const unsigned int _security_id_, const HFSAT::MarketUpdateInfo& _market_update_info_);
  void OnTradePrint(const unsigned int _security_id_, const HFSAT::TradePrintInfo& _trade_print_info_,
                    const HFSAT::MarketUpdateInfo& _market_update_info_);
  /*
  inline void OnTradePrint(const unsigned int _security_id_, const HFSAT::TradePrintInfo& _trade_print_info_,
                           const HFSAT::MarketUpdateInfo& _market_update_info_) {
    OnMarketUpdate(_security_id_, _market_update_info_);
  }
  */
  inline void OnPortfolioPriceChange(double _new_price_) {}
  inline void OnPortfolioPriceReset(double t_new_price_, double t_old_price_, unsigned int is_data_interrupted_) {}

  inline void OnMarketDataInterrupted(const unsigned int _security_id_, const int msecs_since_last_receive_) {}
  inline void OnMarketDataResumed(const unsigned int _security_id_) {}
  // functions

  // functions
  static std::string VarName() { return "Kalman::VXSpotPrice"; }
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
