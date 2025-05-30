/**
    \file IndicatorsCode/volume_weighted_simple_trend_mktevents.cpp

    \author: (c) Copyright Two Roads Technological Solutions Pvt Ltd 2011
     Address:
         Suite No 353, Evoma, #14, Bhattarhalli,
         Old Madras Road, Near Garden City College,
         KR Puram, Bangalore 560049, India
         +91 80 4190 3551
 */
#include "dvccode/CDef/math_utils.hpp"

#include "dvctrade/Indicators/indicator_util.hpp"
#include "dvctrade/Indicators/volume_weighted_simple_trend_mktevents.hpp"

namespace HFSAT {

void VolumeWeightedSimpleTrendMktEvents::CollectShortCodes(
    std::vector<std::string>& _shortcodes_affecting_this_indicator_, std::vector<std::string>& _ors_source_needed_vec_,
    const std::vector<const char*>& r_tokens_) {
  VectorUtils::UniqueVectorAdd(_shortcodes_affecting_this_indicator_, (std::string)r_tokens_[3]);
}

VolumeWeightedSimpleTrendMktEvents* VolumeWeightedSimpleTrendMktEvents::GetUniqueInstance(
    DebugLogger& t_dbglogger_, const Watch& r_watch_, const std::vector<const char*>& r_tokens_,
    PriceType_t _basepx_pxtype_) {
  if (r_tokens_.size() < 8) {
    ExitVerbose(kModelCreationIndicatorLineLessArgs, t_dbglogger_,
                "INDICATOR weight SimpleTrendMktEvents _indep_market_view_ _num_events_halflife_ "
                "_volume_ratio_duration_ _volume_ratio_exponent_ _price_type_ ");
  }
  return GetUniqueInstance(t_dbglogger_, r_watch_,
                           *(ShortcodeSecurityMarketViewMap::StaticGetSecurityMarketView(r_tokens_[3])),
                           (unsigned int)std::max(0, atoi(r_tokens_[4])), atof(r_tokens_[5]), atof(r_tokens_[6]),
                           StringToPriceType_t(r_tokens_[7]));
}

VolumeWeightedSimpleTrendMktEvents* VolumeWeightedSimpleTrendMktEvents::GetUniqueInstance(
    DebugLogger& t_dbglogger_, const Watch& r_watch_, const SecurityMarketView& _indep_market_view_,
    unsigned int _num_events_halflife_, double _volume_ratio_duration_, double _volume_ratio_exponent_,
    PriceType_t _price_type_) {
  std::ostringstream t_temp_oss_;
  t_temp_oss_ << VarName() << ' ' << _indep_market_view_.secname() << ' ' << _num_events_halflife_ << ' '
              << _volume_ratio_duration_ << ' ' << _volume_ratio_exponent_ << ' '
              << PriceType_t_To_String(_price_type_);
  std::string concise_indicator_description_(t_temp_oss_.str());

  static std::map<std::string, VolumeWeightedSimpleTrendMktEvents*> concise_indicator_description_map_;
  if (concise_indicator_description_map_.find(concise_indicator_description_) ==
      concise_indicator_description_map_.end()) {
    concise_indicator_description_map_[concise_indicator_description_] = new VolumeWeightedSimpleTrendMktEvents(
        t_dbglogger_, r_watch_, concise_indicator_description_, _indep_market_view_, _num_events_halflife_,
        _volume_ratio_duration_, _volume_ratio_exponent_, _price_type_);
  }
  return concise_indicator_description_map_[concise_indicator_description_];
}

VolumeWeightedSimpleTrendMktEvents::VolumeWeightedSimpleTrendMktEvents(
    DebugLogger& t_dbglogger_, const Watch& r_watch_, const std::string& concise_indicator_description_,
    const SecurityMarketView& _indep_market_view_, unsigned int _num_events_halflife_, double _volume_ratio_duration_,
    double _volume_ratio_exponent_, PriceType_t _price_type_)
    : CommonIndicator(t_dbglogger_, r_watch_, concise_indicator_description_),
      indep_market_view_(_indep_market_view_),
      trend_history_num_events_halflife_(std::max((unsigned int)3, _num_events_halflife_)),
      price_type_(_price_type_),
      moving_avg_price_(0),
      inv_decay_sum_(1),
      current_indep_price_(0.0),
      recent_scaled_volume_calculator_(*(RecentScaledVolumeCalculator::GetUniqueInstance(
          t_dbglogger_, r_watch_, _indep_market_view_, _volume_ratio_duration_))),
      volume_ratio_calculator_(*(VolumeRatioCalculator::GetUniqueInstance(t_dbglogger_, r_watch_, _indep_market_view_,
                                                                          _volume_ratio_duration_))),
      current_volume_ratio_(0.0),
      scaled_volume_(0.0),
      volume_factor_(0.0),
      volume_ratio_exponent_(std::max(0.1, _volume_ratio_exponent_)) {
  decay_page_factor_ = MathUtils::CalcDecayFactor(trend_history_num_events_halflife_);
  inv_decay_sum_ = (1 - decay_page_factor_);

  if (!indep_market_view_.subscribe_price_type(this, price_type_)) {
    PriceType_t t_error_price_type_ = price_type_;
    std::cerr << typeid(*this).name() << ':' << __func__ << ':' << __LINE__ << ' ' << concise_indicator_description_
              << " passed " << t_error_price_type_ << std::endl;
  }

  volume_ratio_calculator_.AddVolumeRatioListener(1u, this);
  recent_scaled_volume_calculator_.AddRecentScaledVolumeListener(1u, this);
}

void VolumeWeightedSimpleTrendMktEvents::OnVolumeRatioUpdate(const unsigned int r_security_id_,
                                                             const double& r_new_volume_ratio_) {
  current_volume_ratio_ = std::pow(r_new_volume_ratio_, volume_ratio_exponent_);
  current_volume_ratio_ = std::max(1.0, std::min(current_volume_ratio_, 2.0));
  volume_factor_ = current_volume_ratio_ * scaled_volume_;
}

void VolumeWeightedSimpleTrendMktEvents::OnScaledVolumeUpdate(const unsigned int r_security_id_,
                                                              const double& r_new_scaled_volume_) {
  scaled_volume_ = std::pow(r_new_scaled_volume_, volume_ratio_exponent_);
  scaled_volume_ = std::max(1.0, std::min(scaled_volume_, 2.0));
  volume_factor_ = current_volume_ratio_ * scaled_volume_;
}

void VolumeWeightedSimpleTrendMktEvents::OnMarketUpdate(const unsigned int _security_id_,
                                                        const MarketUpdateInfo& cr_market_update_info_) {
  current_indep_price_ = SecurityMarketView::GetPriceFromType(price_type_, cr_market_update_info_);

  if (!is_ready_) {
    if (indep_market_view_.is_ready() &&
        (indep_market_view_.spread_increments() < (2 * indep_market_view_.normal_spread_increments()))) {
      is_ready_ = true;

      current_indep_price_ = SecurityMarketView::GetPriceFromType(price_type_, cr_market_update_info_);
      moving_avg_price_ = current_indep_price_;
      indicator_value_ = 0;
    }
  } else if (!data_interrupted_) {
    current_indep_price_ = SecurityMarketView::GetPriceFromType(price_type_, cr_market_update_info_);
    moving_avg_price_ = (current_indep_price_ * inv_decay_sum_) + (moving_avg_price_ * decay_page_factor_);

    indicator_value_ = current_indep_price_ - moving_avg_price_;
  }

  indicator_value_ = indicator_value_ * volume_factor_;

  NotifyIndicatorListeners(indicator_value_);
}

void VolumeWeightedSimpleTrendMktEvents::InitializeValues() {
  indicator_value_ = 0;
  moving_avg_price_ = current_indep_price_;
}
// market_interrupt_listener interface
void VolumeWeightedSimpleTrendMktEvents::OnMarketDataInterrupted(const unsigned int _security_id_,
                                                                 const int msecs_since_last_receive_) {
  if (indep_market_view_.security_id() == _security_id_) {
    data_interrupted_ = true;
    indicator_value_ = 0;
    NotifyIndicatorListeners(indicator_value_);
  }
}

void VolumeWeightedSimpleTrendMktEvents::OnMarketDataResumed(const unsigned int _security_id_) {
  if (indep_market_view_.security_id() == _security_id_) {
    InitializeValues();
    data_interrupted_ = false;
  }
}
}
