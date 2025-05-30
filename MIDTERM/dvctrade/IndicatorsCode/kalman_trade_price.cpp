/*
 * kalman_trade_price.cpp
 *
 *  Created on: 07-Oct-2015
 *      Author: raghuram
 */

/*
 * vx_spot_price.cpp
 *
 *  Created on: 28-Sep-2015
 *      Author: raghuram
 */
#include "dvccode/CDef/math_utils.hpp"
#include "dvctrade/Indicators/indicator_util.hpp"
#include "dvctrade/Indicators/kalman_trade_price.hpp"
#include "dvctrade/Indicators/simple_price_type.hpp"

namespace Kalman {
// using namespace Kalman;

void KalmanTradePrice::CollectShortCodes(std::vector<std::string>& _shortcodes_affecting_this_indicator_,
                                         std::vector<std::string>& _ors_source_needed_vec_,
                                         const std::vector<const char*>& r_tokens_) {
  HFSAT::VectorUtils::UniqueVectorAdd(_shortcodes_affecting_this_indicator_, (std::string)r_tokens_[3]);
}

KalmanTradePrice* KalmanTradePrice::GetUniqueInstance(HFSAT::DebugLogger& t_dbglogger_, const HFSAT::Watch& r_watch_,
                                                      const std::vector<const char*>& r_tokens_,
                                                      HFSAT::PriceType_t _basepx_pxtype_) {
  // INDICATOR _this_weight_ _indicator_string_ _dep_market_view_ _price_type_
  if (r_tokens_.size() >= 7) {
    return GetUniqueInstance(
        t_dbglogger_, r_watch_, *(HFSAT::ShortcodeSecurityMarketViewMap::StaticGetSecurityMarketView(r_tokens_[3])),
        atof(r_tokens_[4]), atof(r_tokens_[5]), atof(r_tokens_[6]), HFSAT::StringToPriceType_t(r_tokens_[7]));
  } else {
    HFSAT::ExitVerbose(
        HFSAT::kExitErrorCodeGeneral,
        "insufficient inputs to KalmanTradePrice : INDICATOR _this_weight_ _indicator_string_ _dep_market_view_ "
        "_price_type_\n");
    return NULL;  // wont reach here , just to remove warning
  }
}

KalmanTradePrice* KalmanTradePrice::GetUniqueInstance(HFSAT::DebugLogger& t_dbglogger_, const HFSAT::Watch& r_watch_,
                                                      HFSAT::SecurityMarketView& _indep_market_view_,
                                                      double _init_covar_, double _sigma_e_, double _sigma_s_,
                                                      HFSAT::PriceType_t _price_type_) {
  static std::map<std::string, KalmanTradePrice*> concise_indicator_description_map_;
  std::ostringstream t_temp_oss_;
  t_temp_oss_ << KalmanTradePrice::VarName() << ' ' << _indep_market_view_.secname() << ' ' << _init_covar_ << ' '
              << _sigma_e_ << ' ' << _sigma_s_ << ' ' << ' ' << PriceType_t_To_String(_price_type_);

  std::string concise_indicator_description_(t_temp_oss_.str());
  if (concise_indicator_description_map_.find(concise_indicator_description_) ==
      concise_indicator_description_map_.end()) {
    concise_indicator_description_map_[concise_indicator_description_] =
        new KalmanTradePrice(t_dbglogger_, r_watch_, concise_indicator_description_, _indep_market_view_, _init_covar_,
                             _sigma_e_, _sigma_s_, _price_type_);
  }
  return concise_indicator_description_map_[concise_indicator_description_];
}

KalmanTradePrice::KalmanTradePrice(HFSAT::DebugLogger& t_dbglogger_, const HFSAT::Watch& r_watch_,
                                   const std::string& concise_indicator_description_,
                                   HFSAT::SecurityMarketView& _indep_market_view_, double _init_covar_,
                                   double _sigma_e_, double _sigma_s_, HFSAT::PriceType_t _price_type_)
    : HFSAT::CommonIndicator(t_dbglogger_, r_watch_, concise_indicator_description_),
      indep_market_view_(_indep_market_view_),
      init_covar_(_init_covar_),
      sigma_e_(_sigma_e_),
      sigma_s_(_sigma_s_),
      price_type_(_price_type_) {
  if (!indep_market_view_.subscribe_price_type(this, price_type_)) {
    HFSAT::PriceType_t t_error_price_type_ = price_type_;
    std::cerr << typeid(*this).name() << ':' << __func__ << ':' << __LINE__ << ' ' << concise_indicator_description_
              << " passed " << t_error_price_type_ << std::endl;
  }
  setDim(1, 1, 1, 1, 1);
  A.resize(1, 1);
  W.resize(1, 1);
  Q.resize(1, 1);
  H.resize(1, 1);
  V.resize(1, 1);
  R.resize(1, 1);

  // put the below lines in on_trade_print
  /*
    Kalman::EKFilter<double, 1>::Vector temp_vec(1);
    temp_vec(1) = init_state_;

    Kalman::EKFilter<double, 1>::Matrix temp_mat(1, 1);
    temp_mat(1, 1) = init_covar_;
    init(temp_vec, temp_mat);
  */
}

void KalmanTradePrice::makeProcess() {}

void KalmanTradePrice::makeMeasure() { z(1) = x(1); }
void KalmanTradePrice::makeA() { A(1, 1) = 1; }

void KalmanTradePrice::makeW() { W(1, 1) = 1; }
void KalmanTradePrice::makeQ() { Q(1, 1) = sigma_s_; }

void KalmanTradePrice::makeH() { H(1, 1) = 1; }
void KalmanTradePrice::makeV() { V(1, 1) = 1; }
void KalmanTradePrice::makeR() { R(1, 1) = sigma_e_; }

void KalmanTradePrice::OnTradePrint(const unsigned int _security_id_, const HFSAT::TradePrintInfo& _trade_print_info_,
                                    const HFSAT::MarketUpdateInfo& _market_update_info_) {
  if (!is_ready_) {
    if (indep_market_view_.is_ready_complex(2)) {
      is_ready_ = true;
      init_state_ = HFSAT::SecurityMarketView::GetPriceFromType(price_type_, _market_update_info_);
      Kalman::EKFilter<double, 1>::Vector temp_vec(1);
      temp_vec(1) = init_state_;

      Kalman::EKFilter<double, 1>::Matrix temp_mat(1, 1);
      temp_mat(1, 1) = init_covar_;
      init(temp_vec, temp_mat);
      current_mid_price_ = (_market_update_info_.bestbid_price_ + _market_update_info_.bestask_price_) / 2;
    }
  } else {
    last_mid_price_ = current_mid_price_;
    current_mid_price_ = (_market_update_info_.bestbid_price_ + _market_update_info_.bestask_price_) / 2;
    current_traded_price_ = _trade_print_info_.size_traded_;
    current_traded_size_ = _trade_print_info_.trade_price_;

    sigma_e_ = sigma_s_ * (avg_trade_size_ / current_traded_size_);
    Vector temp_z(1);
    temp_z(1) = current_traded_price_;
    Vector temp_u(1, 0.0);

    this->step(temp_u, temp_z);
    indicator_value_ = x(1);

    indicator_value_ = x(1);

    std::cout << watch_.tv().tv_sec << " " << _market_update_info_.bestbid_price_ << " "
              << _market_update_info_.bestask_price_ << " " << _market_update_info_.bestbid_size_ << " "
              << _market_update_info_.bestask_size_ << " " << _market_update_info_.mkt_size_weighted_price_ << " "
              << _trade_print_info_.trade_price_ << " " << _trade_print_info_.size_traded_ << " "
              << _trade_print_info_.buysell_ << " " << (current_mid_price_ - last_mid_price_) / current_mid_price_
              << std::endl;

    NotifyIndicatorListeners(indicator_value_);
  }
}

void KalmanTradePrice::OnMarketUpdate(const unsigned int _security_id_,
                                      const HFSAT::MarketUpdateInfo& _market_update_info_) {
  /*
   if (!is_ready_) {
     if (indep_market_view_.is_ready_complex(2)) {
       is_ready_ = true;
       current_time_ = watch_.tv().tv_usec;
        current_indep_price_ = HFSAT::SecurityMarketView::GetPriceFromType(price_type_, _market_update_info_);
     }
   } else {
     last_updated_time_ = current_time_;
     current_time_ = watch_.tv().tv_usec;
     delta_t_ = (current_time_ - last_updated_time_) * time_factor_;
     last_indep_price_ = current_indep_price_;
     current_indep_price_ = HFSAT::SecurityMarketView::GetPriceFromType(price_type_, _market_update_info_);
     Kalman::EKFilter<double, 1>::Vector temp_z(1);
     temp_z(1) = current_indep_price_;
     last_spot_price_ = current_spot_price_;
     Kalman::EKFilter<double, 1>::Vector temp_vec(1,0.0);
     this->step(temp_vec, temp_z);
     current_spot_price_ = x(1);
     indicator_value_ = current_spot_price_;
     NotifyIndicatorListeners(indicator_value_);
   }

 */
}
}
