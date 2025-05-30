/**
    \file IndicatorsCode/smooth_trend.cpp

    \author: (c) Copyright Two Roads Technological Solutions Pvt Ltd 2011
     Address:
         Suite No 353, Evoma, #14, Bhattarhalli,
         Old Madras Road, Near Garden City College,
         KR Puram, Bangalore 560049, India
         +91 80 4190 3551
 */
#include "dvccode/CDef/math_utils.hpp"

#include "dvctrade/Indicators/indicator_util.hpp"
#include "dvctrade/Indicators/smooth_trend.hpp"

namespace HFSAT {

void SmoothTrend::CollectShortCodes(std::vector<std::string>& _shortcodes_affecting_this_indicator_,
                                    std::vector<std::string>& _ors_source_needed_vec_,
                                    const std::vector<const char*>& r_tokens_) {
  VectorUtils::UniqueVectorAdd(_shortcodes_affecting_this_indicator_, (std::string)r_tokens_[3]);
}

SmoothTrend* SmoothTrend::GetUniqueInstance(DebugLogger& t_dbglogger_, const Watch& r_watch_,
                                            const std::vector<const char*>& r_tokens_, PriceType_t _basepx_pxtype_) {
  // INDICATOR _this_weight_ _indicator_string_ _l1_fractional_secs_ _st_fractional_secs_ _price_type_
  ShortcodeSecurityMarketViewMap::StaticCheckValid(r_tokens_[3]);
  return GetUniqueInstance(t_dbglogger_, r_watch_,
                           *(ShortcodeSecurityMarketViewMap::StaticGetSecurityMarketView(r_tokens_[3])),
                           atof(r_tokens_[4]), atof(r_tokens_[5]), StringToPriceType_t(r_tokens_[6]));
}

SmoothTrend* SmoothTrend::GetUniqueInstance(DebugLogger& t_dbglogger_, const Watch& r_watch_,
                                            const SecurityMarketView& _dep_market_view_, double _lt_fractional_seconds_,
                                            double _st_fractional_seconds_, PriceType_t _price_type_) {
  std::ostringstream t_temp_oss_;
  t_temp_oss_ << VarName() << ' ' << _dep_market_view_.secname() << ' ' << _lt_fractional_seconds_ << ' '
              << _st_fractional_seconds_ << ' ' << PriceType_t_To_String(_price_type_);
  std::string concise_indicator_description_(t_temp_oss_.str());

  static std::map<std::string, SmoothTrend*> concise_indicator_description_map_;
  if (concise_indicator_description_map_.find(concise_indicator_description_) ==
      concise_indicator_description_map_.end()) {
    concise_indicator_description_map_[concise_indicator_description_] =
        new SmoothTrend(t_dbglogger_, r_watch_, concise_indicator_description_, _dep_market_view_,
                        _lt_fractional_seconds_, _st_fractional_seconds_, _price_type_);
  }
  return concise_indicator_description_map_[concise_indicator_description_];
}

SmoothTrend::SmoothTrend(DebugLogger& t_dbglogger_, const Watch& r_watch_,
                         const std::string& concise_indicator_description_, const SecurityMarketView& _dep_market_view_,
                         double _lt_fractional_seconds_, double _st_fractional_seconds_, PriceType_t _price_type_)
    : CommonIndicator(t_dbglogger_, r_watch_, concise_indicator_description_),
      dep_market_view_(_dep_market_view_),
      price_type_(_price_type_),
      lt_trend_history_msecs_(std::max(20, (int)round(1000 * _lt_fractional_seconds_))),
      st_trend_history_msecs_(std::max(20, (int)round(1000 * _st_fractional_seconds_))),
      lt_last_new_page_msecs_(0),
      lt_page_width_msecs_(500),
      lt_decay_page_factor_(0.95),
      lt_inv_decay_sum_(0.05),
      st_last_new_page_msecs_(0),
      st_page_width_msecs_(500),
      st_decay_page_factor_(0.95),
      st_inv_decay_sum_(0.05),
      lt_moving_avg_(0),
      st_moving_avg_(0),
      last_dep_price_(0),
      current_dep_price_(0) {
  SetTimeDecayWeights();
  if (!dep_market_view_.subscribe_price_type(this, _price_type_)) {
    PriceType_t t_error_price_type_ = _price_type_;
    std::cerr << typeid(*this).name() << ':' << __func__ << ':' << __LINE__ << ' ' << concise_indicator_description_
              << " passed " << t_error_price_type_ << std::endl;
  }
}

void SmoothTrend::WhyNotReady() {
  if (!is_ready_) {
    if (!(dep_market_view_.is_ready_complex(2))) {
      DBGLOG_TIME_CLASS << dep_market_view_.secname() << " is_ready_complex = false " << DBGLOG_ENDL_FLUSH;
      DBGLOG_DUMP;
    }
  }
}

void SmoothTrend::OnMarketUpdate(const unsigned int _security_id_, const MarketUpdateInfo& _market_update_info_) {
  current_dep_price_ = SecurityMarketView::GetPriceFromType(price_type_, _market_update_info_);

  if (!is_ready_) {
    if (dep_market_view_.is_ready_complex(2)) {
      is_ready_ = true;
      InitializeValues();
    }
  } else if (!data_interrupted_) {
    if (watch_.msecs_from_midnight() - lt_last_new_page_msecs_ < lt_page_width_msecs_) {
      lt_moving_avg_ += lt_inv_decay_sum_ * (current_dep_price_ - last_dep_price_);
    } else {
      int lt_num_pages_to_add_ =
          (int)floor((watch_.msecs_from_midnight() - lt_last_new_page_msecs_) / lt_page_width_msecs_);
      if (lt_num_pages_to_add_ >= (int)lt_decay_vector_.size()) {
        InitializeValues();
      } else {
        if (lt_num_pages_to_add_ == 1) {
          lt_moving_avg_ = (current_dep_price_ * lt_inv_decay_sum_) + (lt_moving_avg_ * lt_decay_vector_[1]);
        } else {  // num_pages_to_add_ >= 2 < decay_vector_.size ( )
          lt_moving_avg_ = (current_dep_price_ * lt_inv_decay_sum_) +
                           (last_dep_price_ * lt_inv_decay_sum_ * lt_decay_vector_sums_[(lt_num_pages_to_add_ - 1)]) +
                           (lt_moving_avg_ * lt_decay_vector_[lt_num_pages_to_add_]);
        }
        lt_last_new_page_msecs_ += (lt_num_pages_to_add_ * lt_page_width_msecs_);
      }
    }

    if (watch_.msecs_from_midnight() - st_last_new_page_msecs_ < st_page_width_msecs_) {
      st_moving_avg_ += st_inv_decay_sum_ * (current_dep_price_ - last_dep_price_);
    } else {
      int st_num_pages_to_add_ =
          (int)floor((watch_.msecs_from_midnight() - st_last_new_page_msecs_) / st_page_width_msecs_);
      if (st_num_pages_to_add_ >= (int)st_decay_vector_.size()) {
        InitializeValues();
      } else {
        if (st_num_pages_to_add_ == 1) {
          st_moving_avg_ = (current_dep_price_ * st_inv_decay_sum_) + (st_moving_avg_ * st_decay_vector_[1]);
        } else {  // num_pages_to_add_ >= 2 < decay_vector_.size ( )
          st_moving_avg_ = (current_dep_price_ * st_inv_decay_sum_) +
                           (last_dep_price_ * st_inv_decay_sum_ * st_decay_vector_sums_[(st_num_pages_to_add_ - 1)]) +
                           (st_moving_avg_ * st_decay_vector_[st_num_pages_to_add_]);
        }
        st_last_new_page_msecs_ += (st_num_pages_to_add_ * st_page_width_msecs_);
      }
    }

    last_dep_price_ = current_dep_price_;
    indicator_value_ = (lt_moving_avg_ - st_moving_avg_);

    // if ( data_interrupted_ )
    //  { // not sure we will need this ... since by definition data is already iterrupted
    //    indicator_value_ = 0;
    //  }

    NotifyIndicatorListeners(indicator_value_);
  }
}

void SmoothTrend::SetTimeDecayWeights() {
  const unsigned int kDecayLength =
      20;  ///< here number of samples are not required to be very high and hence the decaylength target is just 20
  const unsigned int kMinPageWidth = 10;
  const unsigned int kMaxPageWidth =
      200;  ///< keeping kMaxPageWidth low makes the number_fadeoffs_ pretty high and hence keeps lots of sample points

  lt_page_width_msecs_ = std::min(kMaxPageWidth, std::max(kMinPageWidth, (lt_trend_history_msecs_ / kDecayLength)));
  int lt_number_fadeoffs_ = std::max(1, (int)ceil(lt_trend_history_msecs_ / lt_page_width_msecs_));
  lt_decay_page_factor_ = MathUtils::CalcDecayFactor(lt_number_fadeoffs_);
  lt_decay_vector_.resize(2 * lt_number_fadeoffs_);
  lt_decay_vector_sums_.resize(2 * lt_number_fadeoffs_);
  for (auto i = 0u; i < lt_decay_vector_.size(); i++) {
    lt_decay_vector_[i] = pow(lt_decay_page_factor_, (int)i);
  }
  lt_decay_vector_sums_[0] = 0;
  for (unsigned int i = 1; i < lt_decay_vector_sums_.size(); i++) {
    lt_decay_vector_sums_[i] = lt_decay_vector_sums_[i - 1] + lt_decay_vector_[i];
  }
  lt_inv_decay_sum_ = (1 - lt_decay_page_factor_);

  st_page_width_msecs_ = std::min(kMaxPageWidth, std::max(kMinPageWidth, (st_trend_history_msecs_ / kDecayLength)));
  int st_number_fadeoffs_ = std::max(1, (int)ceil(st_trend_history_msecs_ / st_page_width_msecs_));
  st_decay_page_factor_ = MathUtils::CalcDecayFactor(st_number_fadeoffs_);
  st_decay_vector_.resize(2 * st_number_fadeoffs_);
  st_decay_vector_sums_.resize(2 * st_number_fadeoffs_);
  for (auto i = 0u; i < st_decay_vector_.size(); i++) {
    st_decay_vector_[i] = pow(st_decay_page_factor_, (int)i);
  }
  st_decay_vector_sums_[0] = 0;
  for (unsigned int i = 1; i < st_decay_vector_sums_.size(); i++) {
    st_decay_vector_sums_[i] = st_decay_vector_sums_[i - 1] + st_decay_vector_[i];
  }
  st_inv_decay_sum_ = (1 - st_decay_page_factor_);
}

void SmoothTrend::InitializeValues() {
  lt_moving_avg_ = current_dep_price_;
  st_moving_avg_ = current_dep_price_;
  last_dep_price_ = current_dep_price_;
  lt_last_new_page_msecs_ = watch_.msecs_from_midnight() - watch_.msecs_from_midnight() % lt_page_width_msecs_;
  st_last_new_page_msecs_ = watch_.msecs_from_midnight() - watch_.msecs_from_midnight() % st_page_width_msecs_;
  indicator_value_ = 0;
}

// market_interrupt_listener interface
void SmoothTrend::OnMarketDataInterrupted(const unsigned int _security_id_, const int msecs_since_last_receive_) {
  if (dep_market_view_.security_id() == _security_id_) {
    data_interrupted_ = true;
    indicator_value_ = 0;
    NotifyIndicatorListeners(indicator_value_);
  }
}

void SmoothTrend::OnMarketDataResumed(const unsigned int _security_id_) {
  if (dep_market_view_.security_id() == _security_id_) {
    InitializeValues();
    data_interrupted_ = false;
  }
}
}
