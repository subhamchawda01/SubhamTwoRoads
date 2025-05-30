/**
    \file IndicatorsCode/avg_abs_trend.cpp

    \author: (c) Copyright Two Roads Technological Solutions Pvt Ltd 2011
     Address:
         Suite No 354, Evoma, #14, Bhattarhalli,
         Old Madras Road, Near Garden City College,
         KR Puram, Bangalore 560049, India
         +91 80 4190 3551
*/
#include "dvccode/CDef/math_utils.hpp"

#include "dvctrade/Indicators/indicator_util.hpp"
#include "dvctrade/Indicators/avg_abs_trend.hpp"

namespace HFSAT {

void AvgAbsTrend::CollectShortCodes(std::vector<std::string>& _shortcodes_affecting_this_indicator_,
                                    std::vector<std::string>& _ors_source_needed_vec_,
                                    const std::vector<const char*>& r_tokens_) {
  VectorUtils::UniqueVectorAdd(_shortcodes_affecting_this_indicator_, (std::string)r_tokens_[3]);
}

AvgAbsTrend* AvgAbsTrend::GetUniqueInstance(DebugLogger& t_dbglogger_, const Watch& r_watch_,
                                            const std::vector<const char*>& r_tokens_, PriceType_t _basepx_pxtype_) {
  // INDICATOR _this_weight_ _indicator_string_ _indep_market_view_ _trend_seconds _average_seconds_ _price_type_
  if (r_tokens_.size() < 7) {
    ExitVerbose(kModelCreationIndicatorLineLessArgs, t_dbglogger_,
                "INDICATOR weight AvgAbsTrend _indep_market_view_ _trend_seconds _average_seconds_ _price_type_");
  }
  ShortcodeSecurityMarketViewMap::StaticCheckValid(r_tokens_[3]);
  return GetUniqueInstance(t_dbglogger_, r_watch_,
                           *(ShortcodeSecurityMarketViewMap::StaticGetSecurityMarketView(r_tokens_[3])),
                           atof(r_tokens_[4]), atof(r_tokens_[5]), StringToPriceType_t(r_tokens_[6]));
}

AvgAbsTrend* AvgAbsTrend::GetUniqueInstance(DebugLogger& t_dbglogger_, const Watch& r_watch_,
                                            const SecurityMarketView& _indep_market_view_, double _trend_seconds,
                                            double _average_seconds_, PriceType_t _price_type_) {
  std::ostringstream t_temp_oss_;
  t_temp_oss_ << VarName() << ' ' << _indep_market_view_.secname() << ' ' << _trend_seconds << ' ' << _average_seconds_
              << ' ' << PriceType_t_To_String(_price_type_);
  std::string concise_indicator_description_(t_temp_oss_.str());

  static std::map<std::string, AvgAbsTrend*> concise_indicator_description_map_;
  if (concise_indicator_description_map_.find(concise_indicator_description_) ==
      concise_indicator_description_map_.end()) {
    concise_indicator_description_map_[concise_indicator_description_] =
        new AvgAbsTrend(t_dbglogger_, r_watch_, concise_indicator_description_, _indep_market_view_, _trend_seconds,
                        _average_seconds_, _price_type_);
  }
  return concise_indicator_description_map_[concise_indicator_description_];
}

AvgAbsTrend::AvgAbsTrend(DebugLogger& t_dbglogger_, const Watch& r_watch_,
                         const std::string& concise_indicator_description_,
                         const SecurityMarketView& _indep_market_view_, double _trend_seconds, double _average_seconds_,
                         PriceType_t _price_type_)
    : CommonIndicator(t_dbglogger_, r_watch_, concise_indicator_description_),
      indep_market_view_(_indep_market_view_),
      price_type_(_price_type_),
      moving_avg_abs_trend_(0),
      last_abs_trend_recorded_(0),
      current_indep_abs_trend_(0),
      p_indep_indicator_(NULL) {
  trend_history_msecs_ = std::max(20, (int)round(1000 * _average_seconds_));
  last_new_page_msecs_ = 0;
  page_width_msecs_ = 500;
  decay_page_factor_ = 0.95;
  inv_decay_sum_ = 0.05;
  SetTimeDecayWeights();
  p_indep_indicator_ =
      SimpleTrend::GetUniqueInstance(t_dbglogger_, r_watch_, _indep_market_view_.shortcode(), _trend_seconds, _price_type_);
  p_indep_indicator_->add_unweighted_indicator_listener(1u, this);
}

void AvgAbsTrend::WhyNotReady() {
  if (!is_ready_) {
    if (!(indep_market_view_.is_ready())) {
      DBGLOG_TIME_CLASS << indep_market_view_.secname() << " is_ready_complex = false " << DBGLOG_ENDL_FLUSH;
      DBGLOG_DUMP;
    }
  }
}

void AvgAbsTrend::OnIndicatorUpdate(const unsigned int& _indicator_index_, const double& _new_value_) {
  current_indep_abs_trend_ = std::fabs(_new_value_);

  if (!is_ready_) {
    if (indep_market_view_.is_ready()) {
      is_ready_ = true;
      InitializeValues();
    }
  } else if (!data_interrupted_) {
    if (watch_.msecs_from_midnight() - last_new_page_msecs_ < page_width_msecs_) {
      moving_avg_abs_trend_ += inv_decay_sum_ * (current_indep_abs_trend_ - last_abs_trend_recorded_);
    } else {
      int num_pages_to_add_ = (int)floor((watch_.msecs_from_midnight() - last_new_page_msecs_) / page_width_msecs_);
      if (num_pages_to_add_ >= (int)decay_vector_.size()) {
        InitializeValues();
      } else {
        if (num_pages_to_add_ == 1) {
          moving_avg_abs_trend_ =
              (current_indep_abs_trend_ * inv_decay_sum_) + (moving_avg_abs_trend_ * decay_vector_[1]);
        } else {  // num_pages_to_add_ >= 2 < decay_vector_.size ( )
          moving_avg_abs_trend_ =
              (current_indep_abs_trend_ * inv_decay_sum_) +
              (last_abs_trend_recorded_ * inv_decay_sum_ * decay_vector_sums_[(num_pages_to_add_ - 1)]) +
              (moving_avg_abs_trend_ * decay_vector_[num_pages_to_add_]);
        }
        last_new_page_msecs_ += (num_pages_to_add_ * page_width_msecs_);
      }
    }

    last_abs_trend_recorded_ = current_indep_abs_trend_;
    indicator_value_ = moving_avg_abs_trend_;

    NotifyIndicatorListeners(indicator_value_);
  }
}

void AvgAbsTrend::InitializeValues() {
  moving_avg_abs_trend_ = 0;
  last_abs_trend_recorded_ = 0;
  last_new_page_msecs_ = watch_.msecs_from_midnight() - watch_.msecs_from_midnight() % page_width_msecs_;
  indicator_value_ = 0;
}

// market_interrupt_listener interface
void AvgAbsTrend::OnMarketDataInterrupted(const unsigned int _security_id_, const int msecs_since_last_receive_) {
  if (indep_market_view_.security_id() == _security_id_) {
    data_interrupted_ = true;
    indicator_value_ = current_indep_abs_trend_;
    NotifyIndicatorListeners(indicator_value_);
  }
}

void AvgAbsTrend::OnMarketDataResumed(const unsigned int _security_id_) {
  if (indep_market_view_.security_id() == _security_id_) {
    InitializeValues();
    data_interrupted_ = false;
  }
}
}
