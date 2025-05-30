/**
   \file IndicatorsCode/online_beta_trend.cpp

   \author: (c) Copyright Two Roads Technological Solutions Pvt Ltd 2011
   Address:
   Suite No 351, Evoma, #14, Bhattarhalli,
   Old Madras Road, Near Garden City College,
   KR Puram, Bangalore 560049, India
   +91 80 4190 3551
*/
#include "dvccode/CDef/math_utils.hpp"

#include "dvctrade/Indicators/indicator_util.hpp"
#include "dvctrade/Indicators/online_beta_trend.hpp"

namespace HFSAT {
void OnlineBetaTrend::CollectShortCodes(std::vector<std::string>& _shortcodes_affecting_this_indicator_,
                                        std::vector<std::string>& _ors_source_needed_vec_,
                                        const std::vector<const char*>& r_tokens_) {
  VectorUtils::UniqueVectorAdd(_shortcodes_affecting_this_indicator_, (std::string)r_tokens_[3]);
  std::string t_source_shortcode_ = (std::string)r_tokens_[4];
  if (IndicatorUtil::IsPortfolioShortcode(t_source_shortcode_)) {
    IndicatorUtil::AddPortfolioShortCodeVec(t_source_shortcode_, _shortcodes_affecting_this_indicator_);
  } else {
    VectorUtils::UniqueVectorAdd(_shortcodes_affecting_this_indicator_, t_source_shortcode_);
  }
}

OnlineBetaTrend* OnlineBetaTrend::GetUniqueInstance(DebugLogger& t_dbglogger_, const Watch& r_watch_,
                                                    const std::vector<const char*>& r_tokens_,
                                                    PriceType_t _basepx_pxtype_) {
  // INDICATOR _this_weight_ _indicator_string_ _dep_market_view_  _source_code_ t_trend_history_secs_ _t_price_type_
  if (r_tokens_.size() < 7) {
    std::cerr << "insufficient arguments to INDICATOR OnlineBetaTrend, correct syntax : _this_weight_ "
                 "_indicator_string_ _dep_market_view_  _indep_market_view_ t_trend_history_secs_ _t_beta_secs_ "
                 "_t_price_type_\n";
    exit(1);
  }
  int beta_secs_;
  PriceType_t _price_type_;
  if (r_tokens_.size() > 7) {
    beta_secs_ = atoi(r_tokens_[6]);
    _price_type_ = StringToPriceType_t(r_tokens_[7]);
  } else {
    beta_secs_ = atoi(r_tokens_[5]);
    _price_type_ = StringToPriceType_t(r_tokens_[6]);
  }
  ShortcodeSecurityMarketViewMap::StaticCheckValid(r_tokens_[3]);
  // ShortcodeSecurityMarketViewMap::StaticCheckValid ( r_tokens_[4] );
  return GetUniqueInstance(t_dbglogger_, r_watch_,
                           *(ShortcodeSecurityMarketViewMap::StaticGetSecurityMarketView(r_tokens_[3])),
                           (std::string)r_tokens_[4], atoi(r_tokens_[5]), beta_secs_, _price_type_);
}

OnlineBetaTrend* OnlineBetaTrend::GetUniqueInstance(DebugLogger& t_dbglogger_, const Watch& r_watch_,
                                                    const SecurityMarketView& _dep_market_view_,
                                                    std::string _source_shortcode_,
                                                    const unsigned int t_trend_history_secs_,
                                                    const unsigned int _beta_secs_, PriceType_t _t_price_type_) {
  std::ostringstream t_temp_oss_;
  t_temp_oss_ << VarName() << ' ' << _dep_market_view_.secname() << ' ' << _source_shortcode_ << ' '
              << t_trend_history_secs_ << ' ' << _beta_secs_ << ' ' << _t_price_type_;
  std::string concise_indicator_description_(t_temp_oss_.str());

  static std::map<std::string, OnlineBetaTrend*> concise_indicator_description_map_;
  if (concise_indicator_description_map_.find(concise_indicator_description_) ==
      concise_indicator_description_map_.end()) {
    concise_indicator_description_map_[concise_indicator_description_] =
        new OnlineBetaTrend(t_dbglogger_, r_watch_, concise_indicator_description_, _dep_market_view_,
                            _source_shortcode_, t_trend_history_secs_, _beta_secs_, _t_price_type_);
  }
  return concise_indicator_description_map_[concise_indicator_description_];
}

OnlineBetaTrend::OnlineBetaTrend(DebugLogger& t_dbglogger_, const Watch& r_watch_,
                                 const std::string& concise_indicator_description_,
                                 const SecurityMarketView& _dep_market_view_, std::string _source_shortcode_,
                                 const unsigned int t_trend_history_secs_, const unsigned int _beta_secs_,
                                 PriceType_t _t_price_type_)
    : CommonIndicator(t_dbglogger_, r_watch_, concise_indicator_description_),
      beta_history_msecs_(std::max(100u, 1000u * _beta_secs_)),
      dep_market_view_(_dep_market_view_),
      moving_covariance_(0),
      dep_trend_indicator_(*(SimpleTrend::GetUniqueInstance(t_dbglogger_, r_watch_, _dep_market_view_.shortcode(),
                                                            t_trend_history_secs_, _t_price_type_))),
      // indep_trend_indicator_ ( *( SimpleTrend::GetUniqueInstance ( t_dbglogger_, r_watch_, _indep_market_view_.shortcode(),
      // t_trend_history_secs_, _t_price_type_ ) ) ),
      lrdb_(OfflineReturnsLRDB::GetUniqueInstance(t_dbglogger_, r_watch_, _dep_market_view_.shortcode())),
      last_lrinfo_updated_msecs_(0),
      current_lrinfo_(0.0, 0.0),
      current_projection_multiplier_(0.0),
      dep_updated_(false),
      indep_updated_(false),
      last_dep_trend_recorded_(0),
      last_indep_trend_recorded_(0),
      current_dep_trend_(0),
      current_indep_trend_(0),
      dep_moving_avg_trend_(0),
      indep_moving_avg_trend_(0),
      moving_avg_squared_trend_indep_(0),
      dep_indep_moving_avg_trend_(0),
      // min_unbiased_l2_norm_ (_indep_market_view_.min_price_increment ( ) * _indep_market_view_.min_price_increment (
      // )/25.0),
      source_shortcode_(_source_shortcode_),
      upper_bound_(std::numeric_limits<double>::max()),
      lower_bound_(-1 * std::numeric_limits<double>::max()) {
  watch_.subscribe_BigTimePeriod(this);
  if (IndicatorUtil::IsPortfolioShortcode(_source_shortcode_)) {
    indep_trend_indicator_ = SimpleTrendPort::GetUniqueInstance(t_dbglogger_, r_watch_, _source_shortcode_,
                                                                t_trend_history_secs_, _t_price_type_);
    double min_price_increment_ =
        PCAPortPrice::GetUniqueInstance(t_dbglogger_, r_watch_, _source_shortcode_, kPriceTypeMktSizeWPrice)
            ->min_price_increment();
    min_unbiased_l2_norm_ = min_price_increment_ * min_price_increment_ / 25.0;
  } else {
    ShortcodeSecurityMarketViewMap::StaticCheckValid(_source_shortcode_);
    SecurityMarketView& _indep_market_view_ =
        *(ShortcodeSecurityMarketViewMap::StaticGetSecurityMarketView(_source_shortcode_));
    indep_trend_indicator_ = SimpleTrend::GetUniqueInstance(t_dbglogger_, r_watch_, _indep_market_view_.shortcode(),
                                                            t_trend_history_secs_, _t_price_type_);
    min_unbiased_l2_norm_ =
        _indep_market_view_.min_price_increment() * _indep_market_view_.min_price_increment() / 25.0;
  }
  trend_history_msecs_ = std::max(100u, (1000u * _beta_secs_));
  last_new_page_msecs_ = 0;
  page_width_msecs_ = 500;
  decay_page_factor_ = 0.95;
  inv_decay_sum_ = 0.05;
  SetTimeDecayWeights();
  dep_trend_indicator_.add_unweighted_indicator_listener(0, this);
  indep_trend_indicator_->add_unweighted_indicator_listener(1, this);

  UpdateLRInfo();
}

void OnlineBetaTrend::OnIndicatorUpdate(const unsigned int& _indicator_index_, const double& _new_value_) {
  if (!is_ready_) {
    if (_indicator_index_ == 0) {
      current_dep_trend_ = _new_value_;
      dep_updated_ = true;
    }
    if (_indicator_index_ == 1) {
      current_indep_trend_ = _new_value_;
      indep_updated_ = true;
    }
    if (dep_updated_ && indep_updated_) {
      is_ready_ = true;
      InitializeValues();
    }
  } else {
    if (_indicator_index_ == 0) {
      current_dep_trend_ = _new_value_;
    }
    if (_indicator_index_ == 1) {
      current_indep_trend_ = _new_value_;
    }

    if (watch_.msecs_from_midnight() - last_new_page_msecs_ < page_width_msecs_) {
      dep_moving_avg_trend_ += inv_decay_sum_ * (current_dep_trend_ - last_dep_trend_recorded_);
      indep_moving_avg_trend_ += inv_decay_sum_ * (current_indep_trend_ - last_indep_trend_recorded_);
      moving_avg_squared_trend_indep_ += inv_decay_sum_ * ((current_indep_trend_ * current_indep_trend_) -
                                                           (last_indep_trend_recorded_ * last_indep_trend_recorded_));
      dep_indep_moving_avg_trend_ += inv_decay_sum_ * (current_dep_trend_ * current_indep_trend_ -
                                                       last_dep_trend_recorded_ * last_indep_trend_recorded_);
    } else {  // new page(s)
      int num_pages_to_add_ = (int)floor((watch_.msecs_from_midnight() - last_new_page_msecs_) / page_width_msecs_);
      if (num_pages_to_add_ >= (int)decay_vector_.size()) {
        InitializeValues();
      } else {
        if (num_pages_to_add_ == 1) {
          dep_moving_avg_trend_ = (current_dep_trend_ * inv_decay_sum_) + (dep_moving_avg_trend_ * decay_page_factor_);
          indep_moving_avg_trend_ =
              (current_indep_trend_ * inv_decay_sum_) + (indep_moving_avg_trend_ * decay_page_factor_);
          moving_avg_squared_trend_indep_ = (current_indep_trend_ * current_indep_trend_ * inv_decay_sum_) +
                                            (moving_avg_squared_trend_indep_ * decay_page_factor_);
          dep_indep_moving_avg_trend_ = (current_dep_trend_ * current_indep_trend_ * inv_decay_sum_) +
                                        (dep_indep_moving_avg_trend_ * decay_page_factor_);
        } else {  // num_pages_to_add_ >= 2 < decay_vector_.size ( )
          dep_moving_avg_trend_ =
              (current_dep_trend_ * inv_decay_sum_) +
              (last_dep_trend_recorded_ * inv_decay_sum_ * decay_vector_sums_[(num_pages_to_add_ - 1)]) +
              (dep_moving_avg_trend_ * decay_vector_[num_pages_to_add_]);
          indep_moving_avg_trend_ =
              (current_indep_trend_ * inv_decay_sum_) +
              (last_indep_trend_recorded_ * inv_decay_sum_ * decay_vector_sums_[(num_pages_to_add_ - 1)]) +
              (indep_moving_avg_trend_ * decay_vector_[num_pages_to_add_]);
          moving_avg_squared_trend_indep_ = (current_indep_trend_ * current_indep_trend_ * inv_decay_sum_) +
                                            (last_indep_trend_recorded_ * last_indep_trend_recorded_ * inv_decay_sum_ *
                                             decay_vector_sums_[(num_pages_to_add_ - 1)]) +
                                            (moving_avg_squared_trend_indep_ * decay_vector_[num_pages_to_add_]);
          dep_indep_moving_avg_trend_ = inv_decay_sum_ * current_dep_trend_ * current_indep_trend_ +
                                        (last_dep_trend_recorded_ * last_indep_trend_recorded_ * inv_decay_sum_ *
                                         decay_vector_sums_[(num_pages_to_add_ - 1)]) +
                                        (dep_indep_moving_avg_trend_ * decay_vector_[num_pages_to_add_]);
        }
        last_new_page_msecs_ += (num_pages_to_add_ * page_width_msecs_);
      }
    }

    double unbiased_l2_norm_indep_ = std::max(
        min_unbiased_l2_norm_, moving_avg_squared_trend_indep_ - (indep_moving_avg_trend_ * indep_moving_avg_trend_));
    last_indep_trend_recorded_ = current_indep_trend_;
    last_dep_trend_recorded_ = current_dep_trend_;

    moving_covariance_ = dep_indep_moving_avg_trend_ - dep_moving_avg_trend_ * indep_moving_avg_trend_;
    indicator_value_ =
        moving_covariance_ /
        (unbiased_l2_norm_indep_);  //// beta = corr*std_dep/std_indep; corr = covariance /(std_dep*std_indep);
    if (indicator_value_ > upper_bound_) indicator_value_ = upper_bound_;
    if (indicator_value_ < lower_bound_) indicator_value_ = lower_bound_;

    if (data_interrupted_) indicator_value_ = 0;
    NotifyIndicatorListeners((indicator_value_));
  }
}

void OnlineBetaTrend::InitializeValues() {
  moving_covariance_ = 0;
  dep_moving_avg_trend_ = current_dep_trend_;
  indep_moving_avg_trend_ = current_indep_trend_;
  moving_avg_squared_trend_indep_ = current_indep_trend_ * current_indep_trend_;
  dep_indep_moving_avg_trend_ = current_dep_trend_ * current_indep_trend_;
  last_indep_trend_recorded_ = current_indep_trend_;
  last_dep_trend_recorded_ = current_dep_trend_;
  last_new_page_msecs_ = watch_.msecs_from_midnight() - (watch_.msecs_from_midnight() % page_width_msecs_);
  indicator_value_ = current_projection_multiplier_;
}

void OnlineBetaTrend::UpdateLRInfo() {
  if ((last_lrinfo_updated_msecs_ == 0) ||
      (watch_.msecs_from_midnight() - last_lrinfo_updated_msecs_ > TENMINUTESMSECS)) {
    current_lrinfo_ = lrdb_.GetLRCoeff(dep_market_view_.shortcode(), source_shortcode_);
    ComputeMultiplier();

    if (dbglogger_.CheckLoggingLevel(LRDB_INFO)) {
      DBGLOG_TIME_CLASS_FUNC << "lrinfo ( " << dep_market_view_.shortcode() << ", " << source_shortcode_ << " ) "
                             << current_lrinfo_.lr_coeff_ << ' ' << current_lrinfo_.lr_correlation_ << " -> "
                             << current_projection_multiplier_ << DBGLOG_ENDL_FLUSH;
    }

    last_lrinfo_updated_msecs_ = watch_.msecs_from_midnight() - (watch_.msecs_from_midnight() % TENMINUTESMSECS);
  }
}
void OnlineBetaTrend::OnMarketDataInterrupted(const unsigned int _security_id_, const int msecs_since_last_receive_) {
  data_interrupted_ = true;
  indicator_value_ = 0;
  NotifyIndicatorListeners(indicator_value_);
}

void OnlineBetaTrend::OnMarketDataResumed(const unsigned int _security_id_) {
  InitializeValues();
  data_interrupted_ = false;
}
}
