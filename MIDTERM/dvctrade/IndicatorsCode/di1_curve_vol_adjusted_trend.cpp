/**
    \file IndicatorsCode/di1_curve_vol_adjusted_trend.cpp

    \author: (c) Copyright Two Roads Technological Solutions Pvt Ltd 2011
     Address:
         Suite No 353, Evoma, #14, Bhattarhalli,
         Old Madras Road, Near Garden City College,
         KR Puram, Bangalore 560049, India
         +91 80 4190 3551
*/
#include "dvccode/CDef/math_utils.hpp"

#include "dvctrade/Indicators/indicator_util.hpp"
#include "dvctrade/Indicators/di1_curve_vol_adjusted_trend.hpp"

namespace HFSAT {

void DI1CurveVolAdjustedTrend::CollectShortCodes(std::vector<std::string>& _shortcodes_affecting_this_indicator_,
                                                 std::vector<std::string>& _ors_source_needed_vec_,
                                                 const std::vector<const char*>& r_tokens_) {
  //	   VectorUtils::UniqueVectorAdd ( _shortcodes_affecting_this_indicator_, (std::string) r_tokens_[3] ) ;
  IndicatorUtil::AddDIPortfolioShortCodeVec(std::string(r_tokens_[3]), _shortcodes_affecting_this_indicator_, -1);
}

DI1CurveVolAdjustedTrend* DI1CurveVolAdjustedTrend::GetUniqueInstance(DebugLogger& t_dbglogger_, const Watch& r_watch_,
                                                                      const std::vector<const char*>& r_tokens_,
                                                                      PriceType_t _basepx_pxtype_) {
  // INDICATOR _this_weight_ _indicator_string_ _indep_market_view_ _fractional_seconds_ _price_type_
  //  ShortcodeSecurityMarketViewMap::StaticCheckValid ( r_tokens_[3] );
  return GetUniqueInstance(t_dbglogger_, r_watch_, r_tokens_[3], atof(r_tokens_[4]), atof(r_tokens_[5]),
                           StringToPriceType_t(r_tokens_[6]));
}

DI1CurveVolAdjustedTrend* DI1CurveVolAdjustedTrend::GetUniqueInstance(DebugLogger& t_dbglogger_, const Watch& r_watch_,
                                                                      const std::string _port_, double _vol_factor_,
                                                                      double _fractional_seconds_,
                                                                      PriceType_t _price_type_) {
  std::ostringstream t_temp_oss_;
  t_temp_oss_ << VarName() << ' ' << _port_ << ' ' << _vol_factor_ << ' ' << _fractional_seconds_ << ' '
              << PriceType_t_To_String(_price_type_);
  std::string concise_indicator_description_(t_temp_oss_.str());

  static std::map<std::string, DI1CurveVolAdjustedTrend*> concise_indicator_description_map_;
  if (concise_indicator_description_map_.find(concise_indicator_description_) ==
      concise_indicator_description_map_.end()) {
    concise_indicator_description_map_[concise_indicator_description_] =
        new DI1CurveVolAdjustedTrend(t_dbglogger_, r_watch_, concise_indicator_description_, _port_, _vol_factor_,
                                     _fractional_seconds_, _price_type_);
  }
  return concise_indicator_description_map_[concise_indicator_description_];
}

DI1CurveVolAdjustedTrend::DI1CurveVolAdjustedTrend(DebugLogger& t_dbglogger_, const Watch& r_watch_,
                                                   const std::string& concise_indicator_description_,
                                                   const std::string _port_, double _vol_factor_,
                                                   double _fractional_seconds_, PriceType_t _price_type_)
    : CommonIndicator(t_dbglogger_, r_watch_, concise_indicator_description_),
      price_type_(_price_type_),
      moving_avg_price_(0),
      last_price_recorded_(0),
      sid_to_weights_(),
      sid_to_price_map_(),
      sid_to_interrupted_map_(),
      current_projected_price_(0),
      is_ready_vec_() {
  std::vector<std::string> port_shortcodes_;
  IndicatorUtil::AddDIPortfolioShortCodeVec(_port_, port_shortcodes_, r_watch_.YYYYMMDD());

  for (auto i = 0u; i < port_shortcodes_.size(); i++) {
    indep_market_view_vec_.push_back(
        (ShortcodeSecurityMarketViewMap::StaticGetSecurityMarketView(port_shortcodes_[i])));
    sid_to_price_map_[indep_market_view_vec_[i]->security_id()] =
        CurveUtils::GetLastDayClosingPrice(r_watch_.YYYYMMDD(), port_shortcodes_[i]);
    sid_to_weights_[indep_market_view_vec_[i]->security_id()] = 0.0;
    sid_to_interrupted_map_[indep_market_view_vec_[i]->security_id()] = false;
    is_ready_vec_.push_back(false);
  }

  //  indep_market_view_vec_.push_back ( ShortcodeSecurityMarketViewMap::StaticGetSecurityMarketView ( _port_.c_str() )
  //  ) ;

  trend_history_msecs_ = std::max(20, (int)round(1000 * _fractional_seconds_));
  last_new_page_msecs_ = 0;
  page_width_msecs_ = 500;
  decay_page_factor_ = 0.95;
  inv_decay_sum_ = 0.05;
  SetTimeDecayWeights();

  for (auto i = 0u; i < indep_market_view_vec_.size(); i++) {
    if (!indep_market_view_vec_[i]->subscribe_price_type(this, price_type_)) {
      PriceType_t t_error_price_type_ = price_type_;
      std::cerr << typeid(*this).name() << ":" << __func__ << ":" << __LINE__ << ' ' << concise_indicator_description_
                << " passed " << t_error_price_type_ << std::endl;
    }
  }

  std::vector<double> indep_dv01_weighted_volume_vec_;

  for (auto i = 0u; i < indep_market_view_vec_.size(); i++) {
    int t_indep_vol_ = CurveUtils::_get_volume_(r_watch_.YYYYMMDD(), indep_market_view_vec_[i]->secname(),
                                                trading_start_mfm_, trading_end_mfm_);
    double t_indep_dv01_ = CurveUtils::stirs_fut_dv01(indep_market_view_vec_[i]->shortcode(), r_watch_.YYYYMMDD(),
                                                      sid_to_price_map_[indep_market_view_vec_[i]->security_id()]);
    indep_dv01_weighted_volume_vec_.push_back(t_indep_vol_ * t_indep_dv01_);
  }

  double t_sum_ = 0;
  for (auto i = 0u; i < indep_market_view_vec_.size(); i++) {
    sid_to_weights_[indep_market_view_vec_[i]->security_id()] = indep_dv01_weighted_volume_vec_[i];
    t_sum_ += sid_to_weights_[indep_market_view_vec_[i]->security_id()];
  }

  for (auto i = 0u; i < indep_market_view_vec_.size(); i++) {
    sid_to_weights_[indep_market_view_vec_[i]->security_id()] /= t_sum_;
  }
}

void DI1CurveVolAdjustedTrend::WhyNotReady() {
  if (!is_ready_) {
    for (auto i = 0u; i < indep_market_view_vec_.size(); i++) {
      if (!(indep_market_view_vec_[i]->is_ready_complex(2))) {
        DBGLOG_TIME_CLASS << indep_market_view_vec_[i]->secname() << " is_ready_complex = false " << DBGLOG_ENDL_FLUSH;
        DBGLOG_DUMP;
      }
    }
  }
}

void DI1CurveVolAdjustedTrend::OnMarketUpdate(const unsigned int _security_id_,
                                              const MarketUpdateInfo& _market_update_info_) {
  double current_indep_price_ = SecurityMarketView::GetPriceFromType(price_type_, _market_update_info_);

  if (!is_ready_) {
    is_ready_ = true;

    for (auto i = 0u; i < indep_market_view_vec_.size(); i++) {
      if (!is_ready_vec_[i]) {
        if (indep_market_view_vec_[i]->is_ready_complex(3)) {
          is_ready_vec_[i] = true;
        } else {
          is_ready_ = false;
        }
      }
    }

    if (is_ready_) {
      InitializeValues();
    }
  } else if (!data_interrupted_) {
    current_projected_price_ +=
        sid_to_weights_[_security_id_] * (current_indep_price_ - sid_to_price_map_[_security_id_]);
    sid_to_price_map_[_security_id_] = current_indep_price_;

    if (watch_.msecs_from_midnight() - last_new_page_msecs_ < page_width_msecs_) {
      moving_avg_price_ += inv_decay_sum_ * (current_projected_price_ - last_price_recorded_);
    } else {
      int num_pages_to_add_ = (int)floor((watch_.msecs_from_midnight() - last_new_page_msecs_) / page_width_msecs_);
      if (num_pages_to_add_ >= (int)decay_vector_.size()) {
        InitializeValues();
      } else {
        if (num_pages_to_add_ == 1) {
          moving_avg_price_ = (current_projected_price_ * inv_decay_sum_) + (moving_avg_price_ * decay_vector_[1]);
        } else {  // num_pages_to_add_ >= 2 < decay_vector_.size ( )
          moving_avg_price_ = (current_projected_price_ * inv_decay_sum_) +
                              (last_price_recorded_ * inv_decay_sum_ * decay_vector_sums_[(num_pages_to_add_ - 1)]) +
                              (moving_avg_price_ * decay_vector_[num_pages_to_add_]);
        }
        last_new_page_msecs_ += (num_pages_to_add_ * page_width_msecs_);
      }
    }

    last_price_recorded_ = current_projected_price_;
    indicator_value_ = (current_projected_price_ - moving_avg_price_);

    NotifyIndicatorListeners(indicator_value_);
  }
}

void DI1CurveVolAdjustedTrend::InitializeValues() {
  moving_avg_price_ = current_projected_price_;
  last_price_recorded_ = current_projected_price_;
  last_new_page_msecs_ = watch_.msecs_from_midnight() - watch_.msecs_from_midnight() % page_width_msecs_;
  indicator_value_ = 0;
}

// market_interrupt_listener interface
void DI1CurveVolAdjustedTrend::OnMarketDataInterrupted(const unsigned int _security_id_,
                                                       const int msecs_since_last_receive_) {
  if (sid_to_interrupted_map_.find(_security_id_) != sid_to_interrupted_map_.end()) {
    sid_to_interrupted_map_[_security_id_] = true;
    data_interrupted_ = true;
    indicator_value_ = 0;
    NotifyIndicatorListeners(indicator_value_);
  }
}

void DI1CurveVolAdjustedTrend::OnMarketDataResumed(const unsigned int _security_id_) {
  if (sid_to_interrupted_map_.find(_security_id_) != sid_to_interrupted_map_.end()) {
    sid_to_interrupted_map_[_security_id_] = false;
    data_interrupted_ = false;
    for (auto i = 0u; i < indep_market_view_vec_.size(); i++) {
      if (sid_to_interrupted_map_[indep_market_view_vec_[i]->security_id()]) {
        data_interrupted_ = true;
        indicator_value_ = 0;
        return;
      }
    }

    InitializeValues();
  }
}
}
