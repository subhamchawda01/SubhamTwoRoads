/*
 * vx_spot_price.cpp
 *
 *  Created on: 28-Sep-2015
 *      Author: raghuram
 */
#include "dvccode/CDef/math_utils.hpp"
#include "dvctrade/Indicators/indicator_util.hpp"
#include "dvctrade/Indicators/vx_spot_price.hpp"
#include "dvctrade/Indicators/simple_price_type.hpp"

namespace Kalman {
// using namespace Kalman;

void VXSpotPrice::CollectShortCodes(std::vector<std::string>& _shortcodes_affecting_this_indicator_,
                                    std::vector<std::string>& _ors_source_needed_vec_,
                                    const std::vector<const char*>& r_tokens_) {
  HFSAT::VectorUtils::UniqueVectorAdd(_shortcodes_affecting_this_indicator_, (std::string)r_tokens_[3]);
}

VXSpotPrice* VXSpotPrice::GetUniqueInstance(HFSAT::DebugLogger& t_dbglogger_, const HFSAT::Watch& r_watch_,
                                            const std::vector<const char*>& r_tokens_,
                                            HFSAT::PriceType_t _basepx_pxtype_) {
  // INDICATOR _this_weight_ _indicator_string_ _dep_market_view_ _price_type_
  if (r_tokens_.size() >= 11) {
    return GetUniqueInstance(
        t_dbglogger_, r_watch_, *(HFSAT::ShortcodeSecurityMarketViewMap::StaticGetSecurityMarketView(r_tokens_[3])),
        atof(r_tokens_[4]), atof(r_tokens_[5]), atof(r_tokens_[6]), atof(r_tokens_[7]), atof(r_tokens_[8]),
        atof(r_tokens_[9]), atof(r_tokens_[10]), atof(r_tokens_[11]), HFSAT::StringToPriceType_t(r_tokens_[12]));
  } else {
    HFSAT::ExitVerbose(
        HFSAT::kExitErrorCodeGeneral,
        "insufficient inputs to VXSpotPrice : INDICATOR _this_weight_ _indicator_string_ _dep_market_view_ "
        "_price_type_\n");
    return NULL;  // wont reach here , just to remove warning
  }
}

VXSpotPrice* VXSpotPrice::GetUniqueInstance(HFSAT::DebugLogger& t_dbglogger_, const HFSAT::Watch& r_watch_,
                                            HFSAT::SecurityMarketView& _indep_market_view_, double _ir_, double _mrl_,
                                            double _mrs_, double _vol_vol_, double _init_spot_price_,
                                            double _init_spot_var_, double _power_, double _rho_,
                                            HFSAT::PriceType_t _price_type_) {
  static std::map<std::string, VXSpotPrice*> concise_indicator_description_map_;
  std::cout << "inside indicator " << std::endl;
  std::ostringstream t_temp_oss_;
  t_temp_oss_ << VXSpotPrice::VarName() << ' ' << _indep_market_view_.secname() << ' ' << _ir_ << ' ' << _mrl_ << ' '
              << _mrs_ << ' ' << _vol_vol_ << ' ' << _init_spot_price_ << ' ' << _init_spot_var_ << ' ' << _power_
              << ' ' << _rho_ << ' ' << PriceType_t_To_String(_price_type_);

  std::string concise_indicator_description_(t_temp_oss_.str());
  if (concise_indicator_description_map_.find(concise_indicator_description_) ==
      concise_indicator_description_map_.end()) {
    concise_indicator_description_map_[concise_indicator_description_] =
        new VXSpotPrice(t_dbglogger_, r_watch_, concise_indicator_description_, _indep_market_view_, _ir_, _mrl_, _mrs_,
                        _vol_vol_, _init_spot_price_, _init_spot_var_, _power_, _rho_, _price_type_);
  }
  return concise_indicator_description_map_[concise_indicator_description_];
}

VXSpotPrice::VXSpotPrice(HFSAT::DebugLogger& t_dbglogger_, const HFSAT::Watch& r_watch_,
                         const std::string& concise_indicator_description_,
                         HFSAT::SecurityMarketView& _indep_market_view_, double _ir_, double _mrl_, double _mrs_,
                         double _vol_vol_, double _init_spot_price_, double _init_spot_var_, double _power_,
                         double _rho_, HFSAT::PriceType_t _price_type_)
    : HFSAT::CommonIndicator(t_dbglogger_, r_watch_, concise_indicator_description_),
      indep_market_view_(_indep_market_view_),
      r_(_ir_),
      theta_(_mrl_),
      k_(_mrs_),
      v_(_vol_vol_),
      init_spot_price_(_init_spot_price_),
      init_spot_var_(_init_spot_var_),
      p_(_power_),
      rho_(_rho_),
      price_type_(_price_type_) {
  std::cout << "inside constructor " << std::endl;

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

  std::cout << n << " " << nu << " " << nw << " " << m << " " << nv << std::endl;
  Kalman::EKFilter<double, 1>::Vector temp_vec(1);
  temp_vec(1) = _init_spot_price_;
  current_spot_price_ = _init_spot_price_;
  last_spot_price_ = _init_spot_price_;

  Kalman::EKFilter<double, 1>::Matrix temp_mat(1, 1);
  temp_mat(1, 1) = _init_spot_var_;
  init(temp_vec, temp_mat);
}

void VXSpotPrice::makeProcess() {
  Kalman::EKFilter<double, 1>::Vector x_(x.size());
  double a = 0;

  a = (k_ * (theta_ - last_spot_price_) * dt_) +
      (std::pow(last_spot_price_, p_ - 1) * v_ * rho_ * log(current_indep_price_ / last_indep_price_)) -
      (std::pow(last_spot_price_, p_ - 1) * v_ * rho_ * r_ * dt_) +
      (0.5 * std::pow(last_spot_price_, p_ + 1) * rho_ * r_ * dt_);

  x_(1) = x(1) * (1 + a);
  x.swap(x_);
}

void VXSpotPrice::makeMeasure() { z(1) = -0.5 * std::pow(last_spot_price_, 2) * dt_; }
void VXSpotPrice::makeA() {
  double a = 0;

  a = (-k_ * dt_) +
      ((p_ - 1) * std::pow(last_spot_price_, p_ - 2) * v_ * rho_ * log(current_indep_price_ / last_indep_price_)) -
      ((p_ - 1) * std::pow(last_spot_price_, p_ - 2) * v_ * rho_ * r_ * dt_) +
      (0.5 * (p_ + 1) * std::pow(last_spot_price_, p_) * v_ * rho_ * dt_);

  A(1, 1) = 1 + a;
}

void VXSpotPrice::makeW() {
  W(1, 1) = std::pow(last_spot_price_, p_) * v_ * pow(1 - rho_ * rho_, 0.5) * dt_;
  ;
}
void VXSpotPrice::makeQ() { Q(1, 1) = 1; }

void VXSpotPrice::makeH() { H(1, 1) = -last_spot_price_ * dt_; }
void VXSpotPrice::makeV() { V(1, 1) = last_spot_price_ * dt_; }
void VXSpotPrice::makeR() { R(1, 1) = 1; }

void VXSpotPrice::OnTradePrint(const unsigned int _security_id_, const HFSAT::TradePrintInfo& _trade_print_info_,
                               const HFSAT::MarketUpdateInfo& _market_update_info_) {
  if (!is_ready_) {
    if (indep_market_view_.is_ready_complex(2)) {
      is_ready_ = true;
      current_time_ = watch_.msecs_from_midnight();
      last_updated_time_ = watch_.msecs_from_midnight();
      current_indep_price_ = HFSAT::SecurityMarketView::GetPriceFromType(price_type_, _market_update_info_);
      last_indep_price_ = current_indep_price_;
    }
  } else {
    current_time_ = watch_.msecs_from_midnight();

    if ((current_time_ - last_updated_time_) > 1000) {
      dt_ = ((double)(current_time_ - last_updated_time_)) / 1000;
      last_indep_price_ = current_indep_price_;
      current_indep_price_ = HFSAT::SecurityMarketView::GetPriceFromType(price_type_, _market_update_info_);
      Kalman::EKFilter<double, 1>::Vector temp_z(1);
      temp_z(1) = log(current_indep_price_) - log(last_indep_price_) - (r_ * dt_);
      last_spot_price_ = current_spot_price_;
      Kalman::EKFilter<double, 1>::Vector temp_vec(1, 0.0);

      //    std::cout << A(1, 1) << " " << W(1, 1) << " " << V(1, 1) << " " << R(1, 1) << std::endl;
      this->step(temp_vec, temp_z);
      //    std::cout << A(1, 1) << " " << W(1, 1) << " " << V(1, 1) << " " << R(1, 1) << std::endl;
      current_spot_price_ = std::abs(x(1));
      indicator_value_ = current_spot_price_;
      //    std::cout << current_spot_price_ << std::endl;
      //    std::cout << "==============" << std::endl;

      last_updated_time_ = current_time_;
      NotifyIndicatorListeners(indicator_value_);
    }
  }
}

void VXSpotPrice::OnMarketUpdate(const unsigned int _security_id_,
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
