/**
    \file IndicatorsCode/stud_price_diff.cpp

    \author: (c) Copyright Two Roads Technological Solutions Pvt Ltd 2011
     Address:
         Suite 217, Level 2, Prestige Omega,
         No 104, EPIP Zone, Whitefield,
         Bangalore - 560066, India
         +91 80 4060 0717
 */
#include "dvccode/CDef/math_utils.hpp"

#include "dvctrade/Indicators/indicator_util.hpp"
#include "dvctrade/Indicators/volume_weighted_stud_price_returns_diff.hpp"

namespace HFSAT {

void VolumeWeightedStudPriceReturnsDiff::CollectShortCodes(
    std::vector<std::string>& _shortcodes_affecting_this_indicator_, std::vector<std::string>& _ors_source_needed_vec_,
    const std::vector<const char*>& r_tokens_) {
  VectorUtils::UniqueVectorAdd(_shortcodes_affecting_this_indicator_, (std::string)r_tokens_[3]);
  VectorUtils::UniqueVectorAdd(_shortcodes_affecting_this_indicator_, (std::string)r_tokens_[4]);
}

VolumeWeightedStudPriceReturnsDiff* VolumeWeightedStudPriceReturnsDiff::GetUniqueInstance(
    DebugLogger& t_dbglogger, const Watch& r_watch, const std::vector<const char*>& r_tokens_,
    PriceType_t _basepx_pxtype_) {
  // INDICATOR _this_weight_ _indicator_string_ dep_market_view _source_shortcode_ _fractional_seconds_
  // stdev_duration include_lrdb_sign_ price_type
  if (r_tokens_.size() < 10) {
    ExitVerbose(kModelCreationIndicatorLineLessArgs, t_dbglogger,
                "INDICATOR weight VolumeWeightedStudPriceReturnsDiff dep_market_view indep_market_view "
                "_fractional_seconds_ "
                "stdev_duration _volume_ratio_duration_ price_type ");
  }
  ShortcodeSecurityMarketViewMap::StaticCheckValid(r_tokens_[3]);
  ShortcodeSecurityMarketViewMap::StaticCheckValid(r_tokens_[4]);
  return GetUniqueInstance(
      t_dbglogger, r_watch, *(ShortcodeSecurityMarketViewMap::StaticGetSecurityMarketView(r_tokens_[3])),
      *(ShortcodeSecurityMarketViewMap::StaticGetSecurityMarketView(r_tokens_[4])), atof(r_tokens_[5]),
      atof(r_tokens_[6]), atof(r_tokens_[7]), atof(r_tokens_[8]), StringToPriceType_t(r_tokens_[9]));
}

VolumeWeightedStudPriceReturnsDiff* VolumeWeightedStudPriceReturnsDiff::GetUniqueInstance(
    DebugLogger& t_dbglogger, const Watch& r_watch, const SecurityMarketView& dep_market_view,
    const SecurityMarketView& indep_market_view, double indicator_duration, double stdev_duration,
    double volume_ratio_duration, double volume_ratio_exponent, PriceType_t price_type) {
  std::ostringstream t_temp_oss_;
  t_temp_oss_ << VarName() << ' ' << dep_market_view.secname() << ' ' << indep_market_view.secname() << ' '
              << indicator_duration << ' ' << stdev_duration << ' ' << volume_ratio_duration << ' '
              << volume_ratio_exponent << ' ' << PriceType_t_To_String(price_type);
  std::string concise_indicator_description_(t_temp_oss_.str());

  static std::map<std::string, VolumeWeightedStudPriceReturnsDiff*> concise_indicator_description_map_;
  if (concise_indicator_description_map_.find(concise_indicator_description_) ==
      concise_indicator_description_map_.end()) {
    concise_indicator_description_map_[concise_indicator_description_] = new VolumeWeightedStudPriceReturnsDiff(
        t_dbglogger, r_watch, concise_indicator_description_, dep_market_view, indep_market_view, indicator_duration,
        stdev_duration, volume_ratio_duration, volume_ratio_exponent, price_type);
  }
  return concise_indicator_description_map_[concise_indicator_description_];
}

VolumeWeightedStudPriceReturnsDiff::VolumeWeightedStudPriceReturnsDiff(
    DebugLogger& t_dbglogger, const Watch& r_watch, const std::string& concise_indicator_description_,
    const SecurityMarketView& dep_market_view, const SecurityMarketView& indep_market_view, double indicator_duration,
    double stdev_duration, double volume_ratio_duration, double volume_ratio_exponent, PriceType_t price_type)
    : CommonIndicator(t_dbglogger, r_watch, concise_indicator_description_),
      dep_market_view_(dep_market_view),
      indep_market_view_(indep_market_view),
      dep_stdev_trend_calculator_(nullptr),
      indep_stdev_trend_calculator_(nullptr),
      recent_scaled_volume_calculator_(nullptr),
      volume_ratio_calculator_(nullptr),
      price_type_(price_type),
      volume_ratio_exponent_(volume_ratio_exponent),
      moving_avg_dep_(0),
      last_dep_price_(0),
      current_dep_price_(0),
      moving_avg_indep_(0),
      last_indep_price_(0),
      current_indep_price_(0),
      stdev_dep_(-1),
      stdev_indep_(-1),
      returns_indep_(0),
      returns_dep_(0),
      volume_ratio_(0),
      scaled_volume_(0),
      volume_factor_(0),
      dep_interrupted_(false),
      indep_interrupted_(false),
      lrdb_sign_(1) {
  //
  trend_history_msecs_ = std::max(20, (int)round(1000 * indicator_duration));
  last_new_page_msecs_ = 0;
  page_width_msecs_ = 500;
  decay_page_factor_ = 0.95;
  inv_decay_sum_ = 0.05;

  SetTimeDecayWeights();
  if (!dep_market_view_.subscribe_price_type(this, price_type)) {
    PriceType_t t_errorprice_type = price_type;
    std::cerr << typeid(*this).name() << ':' << __func__ << ':' << __LINE__ << ' ' << concise_indicator_description_
              << " passed " << t_errorprice_type << std::endl;
  }
  if (!indep_market_view_.subscribe_price_type(this, price_type)) {
    PriceType_t t_errorprice_type = price_type;
    std::cerr << typeid(*this).name() << ':' << __func__ << ':' << __LINE__ << ' ' << concise_indicator_description_
              << " passed " << t_errorprice_type << std::endl;
  }

  double tstdev_duration = std::max(100.0, stdev_duration);
  dep_stdev_trend_calculator_ = SlowStdevReturnsCalculator::GetUniqueInstance(
      t_dbglogger, r_watch, dep_market_view_.shortcode(), tstdev_duration, indicator_duration, price_type);
  indep_stdev_trend_calculator_ = SlowStdevReturnsCalculator::GetUniqueInstance(
      t_dbglogger, r_watch, indep_market_view_.shortcode(), tstdev_duration, indicator_duration, price_type);
  dep_stdev_trend_calculator_->add_unweighted_indicator_listener(1u, this);
  indep_stdev_trend_calculator_->add_unweighted_indicator_listener(2u, this);

  recent_scaled_volume_calculator_ =
      RecentScaledVolumeCalculator::GetUniqueInstance(t_dbglogger, r_watch, indep_market_view, volume_ratio_duration);
  volume_ratio_calculator_ =
      VolumeRatioCalculator::GetUniqueInstance(t_dbglogger, r_watch, indep_market_view, volume_ratio_duration);

  recent_scaled_volume_calculator_->AddRecentScaledVolumeListener(1u, this);
  volume_ratio_calculator_->AddVolumeRatioListener(1u, this);

  if (OfflineReturnsLRDB::GetUniqueInstance(t_dbglogger, r_watch, dep_market_view_.shortcode())
          .GetLRCoeff(dep_market_view_.shortcode(), indep_market_view_.shortcode())
          .lr_correlation_ < 0) {
    lrdb_sign_ = -1;
  }
}

void VolumeWeightedStudPriceReturnsDiff::OnVolumeRatioUpdate(const unsigned int security_id,
                                                             const double& new_volume_ratio) {
  volume_ratio_ = std::max(0.5, std::min(new_volume_ratio, 2.0));
  volume_ratio_ = std::pow(volume_ratio_, volume_ratio_exponent_);
  volume_factor_ = volume_ratio_ * scaled_volume_;
}

void VolumeWeightedStudPriceReturnsDiff::OnScaledVolumeUpdate(const unsigned int security_id,
                                                              const double& new_scaled_volume) {
  scaled_volume_ = std::max(0.5, std::min(new_scaled_volume, 2.0));
  scaled_volume_ = std::pow(scaled_volume_, volume_ratio_exponent_);
  volume_factor_ = volume_ratio_ * scaled_volume_;
}

void VolumeWeightedStudPriceReturnsDiff::OnIndicatorUpdate(const unsigned int& _indicator_index_,
                                                           const double& _new_stdev_value_) {
  if (_indicator_index_ == 1u) {
    stdev_dep_ = _new_stdev_value_;
  } else if (_indicator_index_ == 2u) {
    stdev_indep_ = _new_stdev_value_;
  }
}

void VolumeWeightedStudPriceReturnsDiff::WhyNotReady() {
  if (!is_ready_) {
    if (!(indep_market_view_.is_ready_complex(2))) {
      DBGLOG_TIME_CLASS << indep_market_view_.secname() << " is_ready_complex = false " << DBGLOG_ENDL_FLUSH;
      DBGLOG_DUMP;
    }
  }
}

void VolumeWeightedStudPriceReturnsDiff::OnMarketUpdate(const unsigned int _security_id_,
                                                        const MarketUpdateInfo& _market_update_info_) {
  if (_security_id_ == dep_market_view_.security_id()) {
    current_dep_price_ = SecurityMarketView::GetPriceFromType(price_type_, _market_update_info_);
  } else {
    current_indep_price_ = SecurityMarketView::GetPriceFromType(price_type_, _market_update_info_);
  }

  if (!is_ready_) {
    if (indep_market_view_.is_ready_complex(2) && dep_market_view_.is_ready_complex(2)) {
      is_ready_ = true;
      InitializeValues();
    }
  } else if (!data_interrupted_ && stdev_dep_ > 0 && stdev_indep_ > 0) {
    if (watch_.msecs_from_midnight() - last_new_page_msecs_ < page_width_msecs_) {
      moving_avg_dep_ += inv_decay_sum_ * (current_dep_price_ - last_dep_price_);
      moving_avg_indep_ += inv_decay_sum_ * (current_indep_price_ - last_indep_price_);
    } else {
      int num_pages_to_add_ = (int)floor((watch_.msecs_from_midnight() - last_new_page_msecs_) / page_width_msecs_);
      if (num_pages_to_add_ >= (int)decay_vector_.size()) {
        InitializeValues();
      } else {
        if (num_pages_to_add_ == 1) {
          moving_avg_dep_ = (current_dep_price_ * inv_decay_sum_) + (moving_avg_dep_ * decay_vector_[1]);
          moving_avg_indep_ = (current_indep_price_ * inv_decay_sum_) + (moving_avg_indep_ * decay_vector_[1]);
        } else {  // num_pages_to_add_ >= 2 < decay_vector_.size ( )
          moving_avg_dep_ = (current_dep_price_ * inv_decay_sum_) +
                            (last_dep_price_ * inv_decay_sum_ * decay_vector_sums_[(num_pages_to_add_ - 1)]) +
                            (moving_avg_dep_ * decay_vector_[num_pages_to_add_]);
          moving_avg_indep_ = (current_indep_price_ * inv_decay_sum_) +
                              (last_indep_price_ * inv_decay_sum_ * decay_vector_sums_[(num_pages_to_add_ - 1)]) +
                              (moving_avg_indep_ * decay_vector_[num_pages_to_add_]);
        }
        last_new_page_msecs_ += (num_pages_to_add_ * page_width_msecs_);
      }
    }

    last_dep_price_ = current_dep_price_;
    last_indep_price_ = current_indep_price_;

    if (moving_avg_dep_ != 0 && moving_avg_indep_ != 0) {
      returns_indep_ = (current_indep_price_ - moving_avg_indep_) / moving_avg_indep_;
      returns_dep_ = (current_dep_price_ - moving_avg_dep_) / moving_avg_dep_;

      indicator_value_ = (lrdb_sign_ * returns_indep_ / stdev_indep_ - returns_dep_ / stdev_dep_) * current_dep_price_;
    } else {
      indicator_value_ = 0;
    }
    indicator_value_ *= volume_factor_;
    NotifyIndicatorListeners(indicator_value_);
  }
}

void VolumeWeightedStudPriceReturnsDiff::InitializeValues() {
  moving_avg_dep_ = current_dep_price_;
  moving_avg_indep_ = current_indep_price_;

  last_indep_price_ = current_indep_price_;
  last_dep_price_ = current_dep_price_;

  returns_indep_ = 0;
  returns_dep_ = 0;

  last_new_page_msecs_ = watch_.msecs_from_midnight() - watch_.msecs_from_midnight() % page_width_msecs_;
  indicator_value_ = 0;
}

// market_interrupt_listener interface
void VolumeWeightedStudPriceReturnsDiff::OnMarketDataInterrupted(const unsigned int _security_id_,
                                                                 const int msecs_since_last_receive_) {
  if (indep_market_view_.security_id() == _security_id_) {
    indep_interrupted_ = true;
  } else if (dep_market_view_.security_id() == _security_id_) {
    dep_interrupted_ = true;
  }
  if (indep_interrupted_ || dep_interrupted_) {
    data_interrupted_ = true;
    indicator_value_ = 0;
    NotifyIndicatorListeners(indicator_value_);
  }
}

void VolumeWeightedStudPriceReturnsDiff::OnMarketDataResumed(const unsigned int _security_id_) {
  if (data_interrupted_) {
    if (indep_market_view_.security_id() == _security_id_) {
      indep_interrupted_ = false;
    } else if (dep_market_view_.security_id() == _security_id_) {
      dep_interrupted_ = false;
    }
    if (!(dep_interrupted_ || indep_interrupted_)) {
      InitializeValues();
      data_interrupted_ = false;
    }
  }
}
}
