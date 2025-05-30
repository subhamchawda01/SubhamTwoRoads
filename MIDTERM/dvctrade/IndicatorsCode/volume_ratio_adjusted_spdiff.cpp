/**
    \file IndicatorsCode/volume_ratio_adjusted_spdiff.cpp

    \author: (c) Copyright Two Roads Technological Solutions Pvt Ltd 2011
     Address:
         Suite 217, Level 2, Prestige Omega,
         No 104, EPIP Zone, Whitefield,
         Bangalore - 560066, India
         +91 80 4060 0717
 */
#include "dvctrade/Indicators/volume_ratio_adjusted_spdiff.hpp"

namespace HFSAT {

void RSVMRatioAdjSPDiff::CollectShortCodes(std::vector<std::string>& _shortcodes_affecting_this_indicator_,
                                           std::vector<std::string>& _ors_source_needed_vec_,
                                           const std::vector<const char*>& r_tokens_) {
  VectorUtils::UniqueVectorAdd(_shortcodes_affecting_this_indicator_, (std::string)r_tokens_[3]);
  VectorUtils::UniqueVectorAdd(_shortcodes_affecting_this_indicator_, (std::string)r_tokens_[4]);
}

RSVMRatioAdjSPDiff* RSVMRatioAdjSPDiff::GetUniqueInstance(DebugLogger& t_dbglogger_, const Watch& r_watch_,
                                                          const std::vector<const char*>& r_tokens_,
                                                          PriceType_t _basepx_pxtype_) {
  if (r_tokens_.size() < 7) {
    ExitVerbose(
        kModelCreationIndicatorLineLessArgs, t_dbglogger_,
        "INDICATOR weight RSVMRatioAdjSPDiff _dep_market_view_ _indep_market_view_ _fractional_seconds_ _price_type_ ");
  } else if (r_tokens_.size() == 7) {
    ShortcodeSecurityMarketViewMap::StaticCheckValid(r_tokens_[3]);
    ShortcodeSecurityMarketViewMap::StaticCheckValid(r_tokens_[4]);
    return GetUniqueInstance(t_dbglogger_, r_watch_,
                             *(ShortcodeSecurityMarketViewMap::StaticGetSecurityMarketView(r_tokens_[3])),
                             *(ShortcodeSecurityMarketViewMap::StaticGetSecurityMarketView(r_tokens_[4])),
                             atof(r_tokens_[5]), -1, StringToPriceType_t(r_tokens_[6]));
  }
  if (std::string(r_tokens_[7]).compare("#") == 0) {
    ShortcodeSecurityMarketViewMap::StaticCheckValid(r_tokens_[3]);
    ShortcodeSecurityMarketViewMap::StaticCheckValid(r_tokens_[4]);
    return GetUniqueInstance(t_dbglogger_, r_watch_,
                             *(ShortcodeSecurityMarketViewMap::StaticGetSecurityMarketView(r_tokens_[3])),
                             *(ShortcodeSecurityMarketViewMap::StaticGetSecurityMarketView(r_tokens_[4])),
                             atof(r_tokens_[5]), -1, StringToPriceType_t(r_tokens_[6]));
  }
  ShortcodeSecurityMarketViewMap::StaticCheckValid(r_tokens_[3]);
  ShortcodeSecurityMarketViewMap::StaticCheckValid(r_tokens_[4]);
  return GetUniqueInstance(t_dbglogger_, r_watch_,
                           *(ShortcodeSecurityMarketViewMap::StaticGetSecurityMarketView(r_tokens_[3])),
                           *(ShortcodeSecurityMarketViewMap::StaticGetSecurityMarketView(r_tokens_[4])),
                           atof(r_tokens_[5]), atof(r_tokens_[6]), StringToPriceType_t(r_tokens_[7]));
}

RSVMRatioAdjSPDiff* RSVMRatioAdjSPDiff::GetUniqueInstance(DebugLogger& t_dbglogger_, const Watch& r_watch_,
                                                          const SecurityMarketView& _dep_market_view_,
                                                          const SecurityMarketView& _indep_market_view_,
                                                          double _fractional_seconds_, double _lrdb_sign_,
                                                          PriceType_t _price_type_) {
  std::ostringstream t_temp_oss_;
  t_temp_oss_ << VarName() << ' ' << _dep_market_view_.secname() << ' ' << _indep_market_view_.secname() << ' '
              << _fractional_seconds_ << ' ' << _lrdb_sign_ << ' ' << PriceType_t_To_String(_price_type_);
  std::string concise_indicator_description_(t_temp_oss_.str());

  static std::map<std::string, RSVMRatioAdjSPDiff*> concise_indicator_description_map_;
  if (concise_indicator_description_map_.find(concise_indicator_description_) ==
      concise_indicator_description_map_.end()) {
    concise_indicator_description_map_[concise_indicator_description_] =
        new RSVMRatioAdjSPDiff(t_dbglogger_, r_watch_, concise_indicator_description_, _dep_market_view_,
                               _indep_market_view_, _fractional_seconds_, _lrdb_sign_, _price_type_);
  }
  return concise_indicator_description_map_[concise_indicator_description_];
}

RSVMRatioAdjSPDiff::RSVMRatioAdjSPDiff(DebugLogger& t_dbglogger_, const Watch& r_watch_,
                                       const std::string& concise_indicator_description_,
                                       const SecurityMarketView& _dep_market_view_,
                                       const SecurityMarketView& _indep_market_view_, double _fractional_seconds_,
                                       double _lrdb_sign_, PriceType_t _price_type_)
    : CommonIndicator(t_dbglogger_, r_watch_, concise_indicator_description_),
      dep_market_view_(_dep_market_view_),
      indep_market_view_(_indep_market_view_),
      p_dep_volume_(
          RecentSimpleVolumeMeasure::GetUniqueInstance(t_dbglogger_, r_watch_, dep_market_view_, _fractional_seconds_)),
      p_indep_volume_(RecentSimpleVolumeMeasure::GetUniqueInstance(t_dbglogger_, r_watch_, indep_market_view_,
                                                                   _fractional_seconds_)),
      price_type_(_price_type_),
      moving_avg_dep_(0),
      last_dep_price_(0),
      current_dep_price_(0),
      moving_avg_indep_(0),
      last_indep_price_(0),
      current_indep_price_(0),
      stdev_dep_(-1),
      stdev_indep_(-1),
      dep_interrupted_(false),
      indep_interrupted_(false),
      vol_factor_(_fractional_seconds_ / 300.0),
      lrdb_sign_(1) {
  trend_history_msecs_ = std::max(20, (int)round(1000 * _fractional_seconds_));
  last_new_page_msecs_ = 0;
  page_width_msecs_ = 500;
  decay_page_factor_ = 0.95;
  inv_decay_sum_ = 0.05;
  SetTimeDecayWeights();
  if (!dep_market_view_.subscribe_price_type(this, _price_type_)) {
    PriceType_t t_error_price_type_ = _price_type_;
    std::cerr << typeid(*this).name() << ':' << __func__ << ':' << __LINE__ << ' ' << concise_indicator_description_
              << " passed " << t_error_price_type_ << std::endl;
  }
  if (!indep_market_view_.subscribe_price_type(this, _price_type_)) {
    PriceType_t t_error_price_type_ = _price_type_;
    std::cerr << typeid(*this).name() << ':' << __func__ << ':' << __LINE__ << ' ' << concise_indicator_description_
              << " passed " << t_error_price_type_ << std::endl;
  }

  SlowStdevCalculator* std_1 =
      SlowStdevCalculator::GetUniqueInstance(t_dbglogger_, r_watch_, dep_market_view_.shortcode());
  if (std_1 != NULL) {
    std_1->AddSlowStdevCalculatorListener(this);
  }

  SlowStdevCalculator* std_2 =
      SlowStdevCalculator::GetUniqueInstance(t_dbglogger_, r_watch_, indep_market_view_.shortcode());
  if (std_2 != NULL) {
    std_2->AddSlowStdevCalculatorListener(this);
  }
  if (_lrdb_sign_ > 0) {
    if (OfflineReturnsLRDB::GetUniqueInstance(t_dbglogger_, r_watch_, _dep_market_view_.shortcode())
            .GetLRCoeff(dep_market_view_.shortcode(), indep_market_view_.shortcode())
            .lr_correlation_ < 0) {
      lrdb_sign_ = -1;
    }
  } else {
    lrdb_sign_ = 1;
  }
  p_dep_volume_->add_unweighted_indicator_listener(1u, this);
  p_indep_volume_->add_unweighted_indicator_listener(2u, this);
  watch_.subscribe_FifteenSecondPeriod(this);
}

void RSVMRatioAdjSPDiff::OnIndicatorUpdate(const unsigned int& indicator_index_, const double& new_value_) {
  if (indicator_index_ == 1u) {
    current_dep_volume_ = std::min(1.0, new_value_);
    current_dep_volume_ratio_ = std::max(2.0, std::min(0.3, avg_dep_volume_ / current_dep_volume_));
  } else {
    current_indep_volume_ = std::min(1.0, new_value_);
    current_indep_volume_ratio_ = std::max(2.0, std::min(0.3, avg_indep_volume_ / current_indep_volume_));
  }
}

void RSVMRatioAdjSPDiff::OnTimePeriodUpdate(const int num_pages_to_add_) {
  int t_current_slot_ = SampleDataUtil::GetSlotFromMfm(watch_.msecs_from_midnight());
  if (current_slot_ != t_current_slot_) {
    current_slot_ = t_current_slot_;
    if (offline_dep_volume_.find(current_slot_) != offline_dep_volume_.end()) {
      avg_dep_volume_ = offline_dep_volume_[current_slot_];
    }
    if (offline_indep_volume_.find(current_slot_) != offline_indep_volume_.end()) {
      avg_indep_volume_ = offline_indep_volume_[current_slot_];
    }
  }
}

void RSVMRatioAdjSPDiff::OnStdevUpdate(const unsigned int _security_id_, const double& _new_stdev_value_) {
  if (_security_id_ == dep_market_view_.security_id()) {
    stdev_dep_ = _new_stdev_value_;
  } else {
    stdev_indep_ = _new_stdev_value_;
  }
}

void RSVMRatioAdjSPDiff::WhyNotReady() {
  if (!is_ready_) {
    if (!(indep_market_view_.is_ready_complex(2))) {
      DBGLOG_TIME_CLASS << indep_market_view_.secname() << " is_ready_complex = false " << DBGLOG_ENDL_FLUSH;
      DBGLOG_DUMP;
    }
    if (!(dep_market_view_.is_ready_complex(2))) {
      DBGLOG_TIME_CLASS << dep_market_view_.secname() << " is_ready_complex = false " << DBGLOG_ENDL_FLUSH;
      DBGLOG_DUMP;
    }
  }
}

void RSVMRatioAdjSPDiff::OnMarketUpdate(const unsigned int _security_id_,
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
  } else if (!data_interrupted_) {
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

    indicator_value_ = (current_indep_volume_ratio_ / current_dep_volume_ratio_) * lrdb_sign_ *
                           (current_indep_price_ - moving_avg_indep_) / stdev_indep_ -
                       (current_dep_price_ - moving_avg_dep_) / stdev_dep_;

    NotifyIndicatorListeners(indicator_value_);
  }
}

void RSVMRatioAdjSPDiff::InitializeValues() {
  moving_avg_dep_ = current_dep_price_;
  moving_avg_indep_ = current_indep_price_;

  last_indep_price_ = current_indep_price_;
  last_dep_price_ = current_dep_price_;

  last_new_page_msecs_ = watch_.msecs_from_midnight() - watch_.msecs_from_midnight() % page_width_msecs_;

  avg_dep_volume_ =
      HFSAT::SampleDataUtil::GetAvgForPeriod(dep_market_view_.shortcode(), watch_.YYYYMMDD(), 60, trading_start_mfm_,
                                             trading_end_mfm_, "VOL", offline_dep_volume_);
  current_slot_ = SampleDataUtil::GetSlotFromMfm(watch_.msecs_from_midnight());

  for (auto i = 0u; i < offline_dep_volume_.size(); i++) {
    offline_dep_volume_[i] = offline_dep_volume_[i] * vol_factor_;
  }

  if (offline_dep_volume_.find(current_slot_) != offline_dep_volume_.end()) {
    avg_dep_volume_ = offline_dep_volume_[current_slot_];
  }

  avg_indep_volume_ =
      HFSAT::SampleDataUtil::GetAvgForPeriod(indep_market_view_.shortcode(), watch_.YYYYMMDD(), 60, trading_start_mfm_,
                                             trading_end_mfm_, "VOL", offline_indep_volume_);
  if (offline_indep_volume_.find(current_slot_) != offline_indep_volume_.end()) {
    avg_indep_volume_ = offline_indep_volume_[current_slot_];
  }

  current_dep_volume_ = avg_dep_volume_;
  current_dep_volume_ratio_ = 1;
  current_indep_volume_ = avg_indep_volume_;
  current_indep_volume_ratio_ = 1;
  indicator_value_ = 0;
}

// market_interrupt_listener interface
void RSVMRatioAdjSPDiff::OnMarketDataInterrupted(const unsigned int _security_id_,
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

void RSVMRatioAdjSPDiff::OnMarketDataResumed(const unsigned int _security_id_) {
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
