/**
   \file IndicatorsCode/regime_DiffPx_Mod_DiffPx.cpp

   \author: (c) Copyright Two Roads Technological Solutions Pvt Ltd 2011
   Address:
   Suite No 351, Evoma, #14, Bhattarhalli,
   Old Madras Road, Near Garden City College,
   KR Puram, Bangalore 560049, India
   +91 80 4190 3551
 */
#include "dvccode/CDef/math_utils.hpp"

#include "dvctrade/Indicators/indicator_util.hpp"
#include "dvctrade/Indicators/regime_DiffPx_Mod_DiffPx.hpp"

namespace HFSAT {

void RegimeDiffPxModDiffPx::CollectShortCodes(std::vector<std::string>& _shortcodes_affecting_this_indicator_,
                                              std::vector<std::string>& _ors_source_needed_vec_,
                                              const std::vector<const char*>& r_tokens_) {
  VectorUtils::UniqueVectorAdd(_shortcodes_affecting_this_indicator_, (std::string)r_tokens_[3]);
}

RegimeDiffPxModDiffPx* RegimeDiffPxModDiffPx::GetUniqueInstance(DebugLogger& t_dbglogger_, const Watch& r_watch_,
                                                                const std::vector<const char*>& r_tokens_,
                                                                PriceType_t _basepx_pxtype_) {
  // INDICATOR _this_weight_ _indicator_string_ _indep_market_view_ _fractional_seconds_ _price_type_
  // _indicator_threshold_ [_indicator_return_type_ :-1 means actual value, otherwise 1 or 2]
  ShortcodeSecurityMarketViewMap::StaticCheckValid(r_tokens_[3]);
  int return_type = 0;
  if (r_tokens_.size() > 7) return_type = atoi(r_tokens_[7]);
  return GetUniqueInstance(t_dbglogger_, r_watch_,
                           *(ShortcodeSecurityMarketViewMap::StaticGetSecurityMarketView(r_tokens_[3])),
                           atof(r_tokens_[4]), StringToPriceType_t(r_tokens_[5]), atof(r_tokens_[6]), return_type);
}

RegimeDiffPxModDiffPx* RegimeDiffPxModDiffPx::GetUniqueInstance(DebugLogger& t_dbglogger_, const Watch& r_watch_,
                                                                const SecurityMarketView& _indep_market_view_,
                                                                double _fractional_seconds_, PriceType_t _price_type_,
                                                                double _indicator_threshold,
                                                                int _indicator_return_type_) {
  std::ostringstream t_temp_oss_;
  t_temp_oss_ << VarName() << ' ' << _indep_market_view_.secname() << ' ' << _fractional_seconds_ << ' '
              << PriceType_t_To_String(_price_type_) << ' ' << _indicator_threshold << ' ' << _indicator_return_type_;
  std::string concise_indicator_description_(t_temp_oss_.str());

  static std::map<std::string, RegimeDiffPxModDiffPx*> concise_indicator_description_map_;
  if (concise_indicator_description_map_.find(concise_indicator_description_) ==
      concise_indicator_description_map_.end()) {
    concise_indicator_description_map_[concise_indicator_description_] =
        new RegimeDiffPxModDiffPx(t_dbglogger_, r_watch_, concise_indicator_description_, _indep_market_view_,
                                  _fractional_seconds_, _price_type_, _indicator_threshold, _indicator_return_type_);
  }
  return concise_indicator_description_map_[concise_indicator_description_];
}

RegimeDiffPxModDiffPx::RegimeDiffPxModDiffPx(DebugLogger& t_dbglogger_, const Watch& r_watch_,
                                             const std::string& concise_indicator_description_,
                                             const SecurityMarketView& _indep_market_view_, double _fractional_seconds_,
                                             PriceType_t _price_type_, double _indicator_threshold,
                                             int _indicator_return_type_)
    : CommonIndicator(t_dbglogger_, r_watch_, concise_indicator_description_),
      indep_market_view_(_indep_market_view_),
      trend_history_msecs_(std::max(20, (int)round(1000 * _fractional_seconds_))),
      price_type_(_price_type_),
      last_price_change_recorded_(0),
      _indicator_threshold_(_indicator_threshold),
      indicator_return_type_(_indicator_return_type_),
      last_new_page_msecs_(0),
      page_width_msecs_(500),
      decay_page_factor_(0.95),
      inv_decay_sum_(0.05),
      last_price_recorded_(0),
      current_indep_price_(0),
      moving_avg_price_(0),
      up_change_(0),
      down_change_(0),
      last_up_price_change_recorded_(0),
      last_down_price_change_recorded_(0),
      up_cross(false),
      down_cross(false) {
  SetTimeDecayWeights();
  if (!indep_market_view_.subscribe_price_type(this, _price_type_)) {
    PriceType_t t_error_price_type_ = _price_type_;
    std::cerr << typeid(*this).name() << ':' << __func__ << ':' << __LINE__ << ' ' << concise_indicator_description_
              << " passed " << t_error_price_type_ << std::endl;
  }
}

void RegimeDiffPxModDiffPx::WhyNotReady() {
  if (!is_ready_) {
    if (!(indep_market_view_.is_ready_complex(2))) {
      DBGLOG_TIME_CLASS << indep_market_view_.secname() << " is_ready_complex = false " << DBGLOG_ENDL_FLUSH;
      DBGLOG_DUMP;
    }
  }
}

void RegimeDiffPxModDiffPx::OnMarketUpdate(const unsigned int _security_id_,
                                           const MarketUpdateInfo& _market_update_info_) {
  current_indep_price_ = SecurityMarketView::GetPriceFromType(price_type_, _market_update_info_);

  if (!is_ready_) {
    if (indep_market_view_.is_ready_complex(2)) {
      is_ready_ = true;
      InitializeValues();
    }
  } else if (!data_interrupted_) {
    if (watch_.msecs_from_midnight() - last_new_page_msecs_ < page_width_msecs_) {
      moving_avg_price_ += inv_decay_sum_ * (current_indep_price_ - last_price_recorded_);
      if (current_indep_price_ > moving_avg_price_)
        up_change_ += inv_decay_sum_ * (current_indep_price_ - moving_avg_price_ - last_up_price_change_recorded_);
      else
        down_change_ -= inv_decay_sum_ * (current_indep_price_ - moving_avg_price_ + last_down_price_change_recorded_);
    } else {
      int num_pages_to_add_ = (int)floor((watch_.msecs_from_midnight() - last_new_page_msecs_) / page_width_msecs_);
      if (num_pages_to_add_ >= (int)decay_vector_.size()) {
        InitializeValues();
      } else {
        if (num_pages_to_add_ == 1) {
          moving_avg_price_ = (current_indep_price_ * inv_decay_sum_) + (moving_avg_price_ * decay_vector_[1]);
          if (current_indep_price_ > moving_avg_price_)
            up_change_ =
                ((current_indep_price_ - moving_avg_price_) * inv_decay_sum_) + (up_change_ * decay_vector_[1]);
          else
            down_change_ =
                ((-current_indep_price_ + moving_avg_price_) * inv_decay_sum_) + (down_change_ * decay_vector_[1]);
        } else {  // num_pages_to_add_ >= 2 < decay_vector_.size ( )
          moving_avg_price_ = (current_indep_price_ * inv_decay_sum_) +
                              (last_price_recorded_ * inv_decay_sum_ * decay_vector_sums_[(num_pages_to_add_ - 1)]) +
                              (moving_avg_price_ * decay_vector_[num_pages_to_add_]);
          if (current_indep_price_ > moving_avg_price_)
            up_change_ =
                ((current_indep_price_ - moving_avg_price_) * inv_decay_sum_) +
                (last_up_price_change_recorded_ * inv_decay_sum_ * decay_vector_sums_[(num_pages_to_add_ - 1)]) +
                (up_change_ * decay_vector_[num_pages_to_add_]);
          else
            down_change_ =
                ((-current_indep_price_ + moving_avg_price_) * inv_decay_sum_) +
                (last_down_price_change_recorded_ * inv_decay_sum_ * decay_vector_sums_[(num_pages_to_add_ - 1)]) +
                (down_change_ * decay_vector_[num_pages_to_add_]);
        }
        last_new_page_msecs_ += (num_pages_to_add_ * page_width_msecs_);
      }
    }
    last_price_change_recorded_ = current_indep_price_ - moving_avg_price_;
    last_price_recorded_ = current_indep_price_;
    if (last_price_change_recorded_ > 0)
      last_up_price_change_recorded_ = last_price_change_recorded_;
    else
      last_down_price_change_recorded_ = -last_price_change_recorded_;

    indicator_value_ = down_change_;
    if (up_change_ > down_change_) indicator_value_ = up_change_;
    if (up_change_ + down_change_ != 0) indicator_value_ /= (up_change_ + down_change_);

    /*
    if(indicator_value_ > _indicator_threshold_) {
            if(up_cross && indicator_value_ < _indicator_threshold_ + 0.1) {
                    if (up_change_ > down_change_) down_change_*=1.5;
                    else up_change_*=1.5;
                    down_cross = true;
            }
            else down_cross = false;
            if (indicator_value_ > _indicator_threshold_ + 0.1)	up_cross = true;
            else up_cross = false;
    }
    else {
            up_cross = false;
            down_cross = false;
    }
    */

    if (indicator_return_type_ != -1) {
      if (indicator_value_ > _indicator_threshold_)
        indicator_value_ = 2;
      else
        indicator_value_ = 1;
    }
    NotifyIndicatorListeners(indicator_value_);
  }
}

void RegimeDiffPxModDiffPx::SetTimeDecayWeights() {
  const unsigned int kDecayLength =
      20;  ///< here number of samples are not required to be very high and hence the decaylength target is just 20
  const unsigned int kMinPageWidth = 10;
  const unsigned int kMaxPageWidth =
      200;  ///< keeping kMaxPageWidth low makes the number_fadeoffs_ pretty high and hence keeps lots of sample points
  page_width_msecs_ = std::min(kMaxPageWidth, std::max(kMinPageWidth, (trend_history_msecs_ / kDecayLength)));

  int number_fadeoffs_ = std::max(1, (int)ceil(trend_history_msecs_ / page_width_msecs_));

  decay_page_factor_ = MathUtils::CalcDecayFactor(number_fadeoffs_);

  decay_vector_.resize(2 * number_fadeoffs_);
  decay_vector_sums_.resize(2 * number_fadeoffs_);

  for (auto i = 0u; i < decay_vector_.size(); i++) {
    decay_vector_[i] = pow(decay_page_factor_, (int)i);
  }

  decay_vector_sums_[0] = 0;
  for (unsigned int i = 1; i < decay_vector_sums_.size(); i++) {
    decay_vector_sums_[i] = decay_vector_sums_[i - 1] + decay_vector_[i];
  }

  inv_decay_sum_ = (1 - decay_page_factor_);
}

void RegimeDiffPxModDiffPx::InitializeValues() {
  moving_avg_price_ = current_indep_price_;
  last_price_recorded_ = current_indep_price_;
  up_change_ = 0;
  down_change_ = 0;
  last_new_page_msecs_ = watch_.msecs_from_midnight() - (watch_.msecs_from_midnight() % page_width_msecs_);
  indicator_value_ = 0;
}

// market_interrupt_listener interface
void RegimeDiffPxModDiffPx::OnMarketDataInterrupted(const unsigned int _security_id_,
                                                    const int msecs_since_last_receive_) {
  if (indep_market_view_.security_id() == _security_id_) {
    data_interrupted_ = true;
    indicator_value_ = 0;
    NotifyIndicatorListeners(indicator_value_);
  }
}

void RegimeDiffPxModDiffPx::OnMarketDataResumed(const unsigned int _security_id_) {
  if (indep_market_view_.security_id() == _security_id_) {
    InitializeValues();
    data_interrupted_ = false;
  }
}
}
