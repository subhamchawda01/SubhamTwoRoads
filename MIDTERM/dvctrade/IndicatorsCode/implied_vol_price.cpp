/*
 * implied_vol_indicator.cpp
 *
 *  Created on: 05-Oct-2015
 *      Author: raghuram
 */

#include "dvccode/CDef/math_utils.hpp"

#include "dvctrade/Indicators/indicator_util.hpp"
#include "dvctrade/Indicators/implied_vol_price.hpp"

namespace HFSAT {

void ImpliedVolPrice::CollectShortCodes(std::vector<std::string>& _shortcodes_affecting_this_indicator_,
                                        std::vector<std::string>& _ors_source_needed_vec_,
                                        const std::vector<const char*>& r_tokens_) {
  std::string dep_shortcode = (std::string)r_tokens_[3];
  std::vector<std::string> all_shortcodes = ImpliedVolPrice::GetShortCodes(dep_shortcode);

  for (auto i = 0u; i < all_shortcodes.size(); i++) {
    VectorUtils::UniqueVectorAdd(_shortcodes_affecting_this_indicator_, all_shortcodes[i]);
  }
}

ImpliedVolPrice* ImpliedVolPrice::GetUniqueInstance(DebugLogger& t_dbglogger_, const Watch& r_watch_,
                                                    const std::vector<const char*>& r_tokens_,
                                                    PriceType_t _basepx_pxtype_) {
  // INDICATOR _this_weight_ _indicator_string_ _indep_market_view_ _fractional_seconds_ _price_type_
  // ShortcodeSecurityMarketViewMap::StaticCheckValid(r_tokens_[3]);
  std::vector<SecurityMarketView*> temp_smv_vec;

  std::vector<std::string> all_shortcodes = ImpliedVolPrice::GetShortCodes((std::string)r_tokens_[3]);

  for (auto i = 0u; i < all_shortcodes.size(); i++) {
    temp_smv_vec.push_back(ShortcodeSecurityMarketViewMap::StaticGetSecurityMarketView(all_shortcodes[i]));
  }

  return GetUniqueInstance(t_dbglogger_, r_watch_, temp_smv_vec, atof(r_tokens_[4]), StringToPriceType_t(r_tokens_[5]));
}

ImpliedVolPrice* ImpliedVolPrice::GetUniqueInstance(DebugLogger& t_dbglogger_, const Watch& r_watch_,
                                                    const std::vector<SecurityMarketView*> _all_market_views_,
                                                    double _fractional_seconds_, PriceType_t _price_type_) {
  std::ostringstream t_temp_oss_;
  t_temp_oss_ << VarName() << ' ' << _all_market_views_[_all_market_views_.size() - 1]->secname() << ' '
              << _fractional_seconds_ << ' ' << PriceType_t_To_String(_price_type_);
  std::string concise_indicator_description_(t_temp_oss_.str());

  static std::map<std::string, ImpliedVolPrice*> concise_indicator_description_map_;
  if (concise_indicator_description_map_.find(concise_indicator_description_) ==
      concise_indicator_description_map_.end()) {
    concise_indicator_description_map_[concise_indicator_description_] = new ImpliedVolPrice(
        t_dbglogger_, r_watch_, concise_indicator_description_, _all_market_views_, _fractional_seconds_, _price_type_);
  }
  return concise_indicator_description_map_[concise_indicator_description_];
}

ImpliedVolPrice::ImpliedVolPrice(DebugLogger& t_dbglogger_, const Watch& r_watch_,
                                 const std::string& concise_indicator_description_,
                                 const std::vector<SecurityMarketView*> _all_market_views_, double _fractional_seconds_,
                                 PriceType_t _price_type_)
    : CommonIndicator(t_dbglogger_, r_watch_, concise_indicator_description_),
      all_market_views_(_all_market_views_.begin(), _all_market_views_.end()),
      last_vx_spot_(0),
      curr_vx_spot_(0),
      last_price_vec_(_all_market_views_.size(), 0),
      last_imp_vol_vec_(2, 0),
      last_imp_var_vec_(2, 0),
      curr_price_vec_(_all_market_views_.size(), 0),
      curr_imp_vol_vec_(2, 0),
      curr_imp_var_vec_(2, 0),
      fractional_seconds_(_fractional_seconds_),
      moving_avg_ind_vec_(_all_market_views_.size()),
      price_type_(_price_type_),
      ttm_0_(22.0),
      ready_vec_(_all_market_views_.size(), false),
      num_updated_(false) {
  for (auto i = 0u; i < all_market_views_.size(); i++) {
    moving_avg_ind_vec_[i] = ExponentialMovingAverage::GetUniqueInstance(dbglogger_, watch_, (*all_market_views_[i]),
                                                                         fractional_seconds_, price_type_);
    (moving_avg_ind_vec_[i])->add_unweighted_indicator_listener(i, this);

    if (!(all_market_views_[i])->subscribe_price_type(this, price_type_)) {
      PriceType_t t_error_price_type_ = _price_type_;
      std::cerr << typeid(*this).name() << ':' << __func__ << ':' << __LINE__ << ' ' << concise_indicator_description_
                << " passed " << t_error_price_type_ << std::endl;
    }
  }
}

void ImpliedVolPrice::OnIndicatorUpdate(const unsigned int& _indicator_index_, const double& _new_value_) {
  if (num_updated_ != all_market_views_.size()) {
    if (!ready_vec_[_indicator_index_]) {
      num_updated_++;
      ready_vec_[_indicator_index_] = true;
    }
    last_price_vec_[_indicator_index_] = _new_value_;
    curr_price_vec_[_indicator_index_] = _new_value_;

    if (num_updated_ == all_market_views_.size()) {
      curr_vx_spot_ = ImpliedVolPrice::GetVXSpot(curr_price_vec_[0], curr_price_vec_[1]);
      last_vx_spot_ = curr_vx_spot_;
      curr_imp_var_vec_[0] = ((curr_vx_spot_ * curr_vx_spot_) * ttm_0_ / 252.0) + (curr_price_vec_[0] * 22.0 / 252.0);
      curr_imp_vol_vec_[0] = std::pow(curr_imp_var_vec_[0], 0.5);

      last_imp_var_vec_[0] = curr_imp_var_vec_[0];
      last_imp_vol_vec_[0] = curr_imp_vol_vec_[0];
      curr_imp_var_vec_[1] = ((curr_vx_spot_ * curr_vx_spot_) * ttm_0_ / 252.0);
      curr_imp_vol_vec_[1] = 0;
      for (auto i = 0u; i < all_market_views_.size(); i++) {
        curr_imp_var_vec_[1] = +(curr_price_vec_[i] * curr_price_vec_[i]) * 22.0 / 252;
      }

      last_imp_var_vec_[1] = curr_imp_var_vec_[1];
      curr_imp_vol_vec_[1] = std::pow(curr_imp_var_vec_[1], 0.5);
      last_imp_vol_vec_[1] = curr_imp_vol_vec_[1];
    }
  } else {
    last_price_vec_[_indicator_index_] = curr_price_vec_[_indicator_index_];
    curr_price_vec_[_indicator_index_] = _new_value_;

    if (_indicator_index_ == 0 || _indicator_index_ == 1) {
      last_vx_spot_ = curr_vx_spot_;
      curr_vx_spot_ = GetVXSpot(curr_price_vec_[0], curr_price_vec_[1]);
    }
    last_imp_vol_vec_[0] = curr_imp_vol_vec_[0];
    last_imp_vol_vec_[1] = curr_imp_vol_vec_[1];

    last_imp_var_vec_[0] = curr_imp_var_vec_[0];
    last_imp_var_vec_[1] = curr_imp_var_vec_[1];

    curr_imp_var_vec_[0] = ((curr_vx_spot_ * curr_vx_spot_) * ttm_0_ / 252.0) + (curr_price_vec_[0] * 22.0 / 252.0);
    curr_imp_vol_vec_[0] = std::pow(curr_imp_var_vec_[0], 0.5);
    curr_imp_var_vec_[1] = curr_imp_var_vec_[1] -
                           (last_price_vec_[_indicator_index_] * last_price_vec_[_indicator_index_] * 22.0 / 252.0) +
                           (curr_price_vec_[_indicator_index_] * curr_price_vec_[_indicator_index_] * 22.0 / 252.0);
    if (_indicator_index_ == 0 || _indicator_index_ == 1) {
      curr_imp_var_vec_[1] = curr_imp_var_vec_[1] - (last_vx_spot_ * last_vx_spot_ * ttm_0_ / 252.0) +
                             (curr_vx_spot_ * curr_vx_spot_ * ttm_0_ / 252.0);
    }

    curr_imp_vol_vec_[1] = std::pow(curr_imp_var_vec_[1], 0.5);

    double change_1 = curr_imp_vol_vec_[0] - last_imp_vol_vec_[0];
    // double change_2 = curr_imp_vol_vec_[1] - last_imp_vol_vec_[1];

    double temp_1 = std::pow(change_1 + last_imp_vol_vec_[1], 2.0);

    temp_1 =
        temp_1 -
        (curr_imp_var_vec_[1] -
         ((curr_price_vec_[curr_price_vec_.size() - 1] * curr_price_vec_[curr_price_vec_.size() - 1]) * 22.0 / 252.0));
    temp_1 = std::pow(temp_1 / (22.0 / 252.0), 0.5);
    indicator_value_ = temp_1 - curr_price_vec_[curr_price_vec_.size() - 1];
    NotifyIndicatorListeners(indicator_value_);
  }
}

// void ImpliedVolPrice::InitializeValues() { indicator_value_ = 0; }

// market_interrupt_listener interface
void ImpliedVolPrice::OnMarketDataInterrupted(const unsigned int _security_id_, const int msecs_since_last_receive_) {
  // if (indep_market_view_.security_id() == _security_id_) {
  //   data_interrupted_ = true;
  //    indicator_value_ = 0;
  //   NotifyIndicatorListeners(indicator_value_);
  // } else
  //   return;
}

void ImpliedVolPrice::OnMarketDataResumed(const unsigned int _security_id_) {
  //  if (indep_market_view_.security_id() == _security_id_) {
  // InitializeValues();
  //    data_interrupted_ = false;
  //  } else
  //    return;
}

std::vector<std::string> ImpliedVolPrice::GetShortCodes(std::string dep_shortcode) {
  std::vector<std::string> all_shortcodes;
  std::string temp_str = "VX";
  char temp_char = dep_shortcode[dep_shortcode.size() - 1];
  int ind = temp_char - '0';

  for (int i = 0; i <= ind; i++) {
    temp_str = "VX";
    std::string str_i = std::to_string(i);
    temp_str = temp_str + "_" + str_i;
    all_shortcodes.push_back(temp_str);
  }

  return (all_shortcodes);
}

double ImpliedVolPrice::GetVXSpot(double p_0, double p_1) {
  double vx_spot = p_0 + (1.4755 * (p_0 - p_1) * ttm_0_ / 22);
  return (vx_spot);
}
}
