/**
    \file IndicatorsCode/simple_trend.cpp

    \author: (c) Copyright Two Roads Technological Solutions Pvt Ltd 2011
     Address:
         Suite No 353, Evoma, #14, Bhattarhalli,
         Old Madras Road, Near Garden City College,
         KR Puram, Bangalore 560049, India
         +91 80 4190 3551
 */
#include "dvccode/CDef/math_utils.hpp"

#include "dvctrade/Indicators/indicator_util.hpp"
#include "dvctrade/Indicators/returns_diff_synthetic_index.hpp"

namespace HFSAT {

void ReturnsDiffSyntheticIndex::CollectShortCodes(std::vector<std::string>& _shortcodes_affecting_this_indicator_,
                                                  std::vector<std::string>& _ors_source_needed_vec_,
                                                  const std::vector<const char*>& r_tokens_) {
  VectorUtils::UniqueVectorAdd(_shortcodes_affecting_this_indicator_, (std::string)r_tokens_[3]);
  SyntheticIndex::CollectConstituents(_shortcodes_affecting_this_indicator_, (std::string)r_tokens_[3],
                                      atoi(r_tokens_[5]));
}

ReturnsDiffSyntheticIndex* ReturnsDiffSyntheticIndex::GetUniqueInstance(DebugLogger& t_dbglogger_,
                                                                        const Watch& r_watch_,
                                                                        const std::vector<const char*>& r_tokens_,
                                                                        PriceType_t _basepx_pxtype_) {
  // INDICATOR _this_weight_ _indicator_string_ _indep_market_view_ _fractional_seconds_ _use_fut_ _price_type_
  if (r_tokens_.size() < 7) {
    ExitVerbose(kModelCreationIndicatorLineLessArgs, t_dbglogger_,
                "   ReturnsDiffSyntheticIndex Incorrect Syntax. Correct syntax would b  INDICATOR _this_weight_ "
                "_indicator_string_ _dep_market_view_ _fractional_seconds_ _use_fut_  _price_type_ ");
  }

  ShortcodeSecurityMarketViewMap::StaticCheckValid(r_tokens_[3]);
  return GetUniqueInstance(t_dbglogger_, r_watch_,
                           *(ShortcodeSecurityMarketViewMap::StaticGetSecurityMarketView(r_tokens_[3])),
                           atof(r_tokens_[4]), atoi(r_tokens_[5]), StringToPriceType_t(r_tokens_[6]));
}

ReturnsDiffSyntheticIndex* ReturnsDiffSyntheticIndex::GetUniqueInstance(DebugLogger& t_dbglogger_,
                                                                        const Watch& r_watch_,
                                                                        const SecurityMarketView& _indep_market_view_,
                                                                        double _fractional_seconds_, int _use_fut_,
                                                                        PriceType_t _price_type_) {
  std::ostringstream t_temp_oss_;
  t_temp_oss_ << VarName() << ' ' << _indep_market_view_.secname() << ' ' << _fractional_seconds_ << ' ' << _use_fut_
              << ' ' << PriceType_t_To_String(_price_type_);
  std::string concise_indicator_description_(t_temp_oss_.str());

  static std::map<std::string, ReturnsDiffSyntheticIndex*> concise_indicator_description_map_;
  if (concise_indicator_description_map_.find(concise_indicator_description_) ==
      concise_indicator_description_map_.end()) {
    concise_indicator_description_map_[concise_indicator_description_] =
        new ReturnsDiffSyntheticIndex(t_dbglogger_, r_watch_, concise_indicator_description_, _indep_market_view_,
                                      _fractional_seconds_, _use_fut_, _price_type_);
  }
  return concise_indicator_description_map_[concise_indicator_description_];
}

ReturnsDiffSyntheticIndex::ReturnsDiffSyntheticIndex(DebugLogger& t_dbglogger_, const Watch& r_watch_,
                                                     const std::string& concise_indicator_description_,
                                                     const SecurityMarketView& _indep_market_view_,
                                                     double _fractional_seconds_, int _use_fut_,
                                                     PriceType_t _price_type_)
    : CommonIndicator(t_dbglogger_, r_watch_, concise_indicator_description_),
      indep_market_view_(_indep_market_view_),
      price_type_(_price_type_),
      moving_avg_synthetic_price_(0),
      synth_index_value_(0),
      returns_index_value_(0),
      last_synthetic_price_recorded_(0),
      current_synthetic_price_(0),
      time_decay_calculator_ ( std::max(20, (int)round(1000 * _fractional_seconds_)), 0, 500, 0.95 ),
      use_fut_(_use_fut_) {
  
  trend_history_msecs_ = std::max(20, (int)round(1000 * _fractional_seconds_));
  last_new_page_msecs_ = 0;
  page_width_msecs_ = 500;
  decay_page_factor_ = 0.95;
  inv_decay_sum_ = 0.05;
  SetTimeDecayWeights();

  time_decay_calculator_.SetTimeDecayWeights ( ) ;
  
  if (!indep_market_view_.subscribe_price_type(this, _price_type_)) {
    PriceType_t t_error_price_type_ = _price_type_;
    std::cerr << typeid(*this).name() << ':' << __func__ << ':' << __LINE__ << ' ' << concise_indicator_description_
              << " passed " << t_error_price_type_ << std::endl;
  }

  synth_index = SyntheticIndex::GetUniqueInstance(t_dbglogger_, r_watch_, (SecurityMarketView*)&_indep_market_view_,
                                                  _price_type_, use_fut_);
  returns_index =
      SimpleReturns::GetUniqueInstance(t_dbglogger_, r_watch_, _indep_market_view_, _fractional_seconds_, _price_type_);

  synth_index->add_indicator_listener(0, this, 1.0);
  returns_index->add_indicator_listener(1, this, 1.0);
}

void ReturnsDiffSyntheticIndex::WhyNotReady() {
  if (!is_ready_) {
    if (!(indep_market_view_.is_ready_complex(2))) {
      DBGLOG_TIME_CLASS << indep_market_view_.secname() << " is_ready_complex = false " << DBGLOG_ENDL_FLUSH;
      DBGLOG_DUMP;
    }
  }
}

void ReturnsDiffSyntheticIndex::OnIndicatorUpdate(const unsigned int& _indicator_index_, const double& _new_value_) {
  if (_indicator_index_ == 1)
    returns_index_value_ = _new_value_;
  else
    synth_index_value_ = _new_value_;

  if (!synth_index->IsIndicatorReady() || !returns_index->IsIndicatorReady()) {
    return;
  }

  if (!is_ready_) {
    is_ready_ = true;
    InitializeValues();
  } else {
    if (watch_.msecs_from_midnight() - last_new_page_msecs_ < page_width_msecs_) {
      moving_avg_synthetic_price_ += inv_decay_sum_ * (synth_index_value_ - last_synthetic_price_recorded_);
    } else {
      int num_pages_to_add_ = (int)floor((watch_.msecs_from_midnight() - last_new_page_msecs_) / page_width_msecs_);
      if (num_pages_to_add_ >= (int)decay_vector_.size()) {
        InitializeValues();
      } else {
        if (num_pages_to_add_ == 1) {
          moving_avg_synthetic_price_ =
              (synth_index_value_ * inv_decay_sum_) + (moving_avg_synthetic_price_ * decay_vector_[1]);
        } else {  // num_pages_to_add_ >= 2 < decay_vector_.size ( )
          moving_avg_synthetic_price_ =
              (synth_index_value_ * inv_decay_sum_) +
              (last_synthetic_price_recorded_ * inv_decay_sum_ * decay_vector_sums_[(num_pages_to_add_ - 1)]) +
              (moving_avg_synthetic_price_ * decay_vector_[num_pages_to_add_]);
        }
        last_new_page_msecs_ += (num_pages_to_add_ * page_width_msecs_);
      }
    }

    last_synthetic_price_recorded_ = synth_index_value_;

    indicator_value_ =
        ((synth_index_value_ - moving_avg_synthetic_price_) / moving_avg_synthetic_price_) - returns_index_value_;

    if (synth_index->IsDataInterrupted() || returns_index->IsDataInterrupted()) {
      indicator_value_ = 0;
    }

    NotifyIndicatorListeners(indicator_value_);
  }
}

// market_interrupt_listener interface
void ReturnsDiffSyntheticIndex::OnMarketDataInterrupted(const unsigned int _security_id_,
                                                        const int msecs_since_last_receive_) {
  if (indep_market_view_.security_id() == _security_id_) {
    data_interrupted_ = true;
    indicator_value_ = 0;
    NotifyIndicatorListeners(indicator_value_);
  }
}

void ReturnsDiffSyntheticIndex::OnMarketDataResumed(const unsigned int _security_id_) {
  if (indep_market_view_.security_id() == _security_id_) {
    InitializeValues();
    data_interrupted_ = false;
  }
}

void ReturnsDiffSyntheticIndex::InitializeValues() {
  moving_avg_synthetic_price_ = synth_index_value_;
  last_synthetic_price_recorded_ = synth_index_value_;
  indicator_value_ = 0;
  last_new_page_msecs_ = watch_.msecs_from_midnight() - watch_.msecs_from_midnight() % page_width_msecs_;
  time_decay_calculator_.InitializeValues(watch_.msecs_from_midnight());
}
}
