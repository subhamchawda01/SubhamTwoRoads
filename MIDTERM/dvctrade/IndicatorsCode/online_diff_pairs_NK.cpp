/**
    \file IndicatorsCode/online_diff_pairs.cpp

    \author: (c) Copyright Two Roads Technological Solutions Pvt Ltd 2011
     Address:
         Suite No 353, Evoma, #14, Bhattarhalli,
         Old Madras Road, Near Garden City College,
         KR Puram, Bangalore 560049, India
         +91 80 4190 3551
*/
#include "dvccode/CDef/math_utils.hpp"
#include "dvctrade/Indicators/indicator_util.hpp"
#include "dvctrade/Indicators/online_diff_pairs_NK.hpp"

namespace HFSAT {

void OnlineDiffPairsNK::CollectShortCodes(std::vector<std::string>& _shortcodes_affecting_this_indicator_,
                                          std::vector<std::string>& _ors_source_needed_vec_,
                                          const std::vector<const char*>& r_tokens_) {
  VectorUtils::UniqueVectorAdd(_shortcodes_affecting_this_indicator_, (std::string)r_tokens_[3]);
  VectorUtils::UniqueVectorAdd(_shortcodes_affecting_this_indicator_, (std::string)r_tokens_[4]);
}

OnlineDiffPairsNK* OnlineDiffPairsNK::GetUniqueInstance(DebugLogger& t_dbglogger_, const Watch& r_watch_,
                                                        const std::vector<const char*>& r_tokens_,
                                                        PriceType_t _basepx_pxtype_) {
  // INDICATOR _this_weight_ _indicator_string_ _dep_market_view_ _indep_market_view_ _fractional_seconds_ _price_type_
  return GetUniqueInstance(t_dbglogger_, r_watch_,
                           *(ShortcodeSecurityMarketViewMap::StaticGetSecurityMarketView(r_tokens_[3])),
                           *(ShortcodeSecurityMarketViewMap::StaticGetSecurityMarketView(r_tokens_[4])),
                           atof(r_tokens_[5]), StringToPriceType_t(r_tokens_[6]));
}

OnlineDiffPairsNK* OnlineDiffPairsNK::GetUniqueInstance(DebugLogger& t_dbglogger_, const Watch& r_watch_,
                                                        SecurityMarketView& _dep_market_view_,
                                                        SecurityMarketView& _indep_market_view_,
                                                        double _fractional_seconds_, PriceType_t _price_type_) {
  std::ostringstream t_temp_oss_;
  t_temp_oss_ << VarName() << ' ' << _dep_market_view_.secname() << ' ' << _indep_market_view_.secname() << ' '
              << _fractional_seconds_ << ' ' << PriceType_t_To_String(_price_type_);
  std::string concise_indicator_description_(t_temp_oss_.str());

  static std::map<std::string, OnlineDiffPairsNK*> concise_indicator_description_map_;
  if (concise_indicator_description_map_.find(concise_indicator_description_) ==
      concise_indicator_description_map_.end()) {
    concise_indicator_description_map_[concise_indicator_description_] =
        new OnlineDiffPairsNK(t_dbglogger_, r_watch_, concise_indicator_description_, _dep_market_view_,
                              _indep_market_view_, _fractional_seconds_, _price_type_);
  }
  return concise_indicator_description_map_[concise_indicator_description_];
}

OnlineDiffPairsNK::OnlineDiffPairsNK(DebugLogger& t_dbglogger_, const Watch& r_watch_,
                                     const std::string& concise_indicator_description_,
                                     SecurityMarketView& _dep_market_view_, SecurityMarketView& _indep_market_view_,
                                     double _fractional_seconds_, PriceType_t _price_type_)
    : CommonIndicator(t_dbglogger_, r_watch_, concise_indicator_description_),
      dep_market_view_(_dep_market_view_),
      indep_market_view_(_indep_market_view_),
      price_type_(_price_type_),
      moving_avg_diff_(0),
      last_price_diff_(1),
      current_dep_price_(0),
      current_indep_price_(0),
      current_price_diff_(0) {
  if (dep_market_view_.security_id() == indep_market_view_.security_id()) {  // added this since for convenience one
                                                                             // could add a combo or portfolio as source
                                                                             // with a security
    // that is also the dependant
    indicator_value_ = 0;
    is_ready_ = true;
  } else {
	trend_history_msecs_ = std::max(MIN_MSEC_HISTORY_INDICATORS, (int)round(1000 * _fractional_seconds_));
	last_new_page_msecs_ = 0;
	page_width_msecs_ = 500;
	decay_page_factor_ = 0.95;
	inv_decay_sum_ = 0.05;
    SetTimeDecayWeights();

    if (!dep_market_view_.subscribe_price_type(this, price_type_)) {
      PriceType_t t_error_price_type_ = price_type_;
      std::cerr << typeid(*this).name() << ':' << __func__ << ':' << __LINE__ << ' ' << concise_indicator_description_
                << " passed " << t_error_price_type_ << " to dep " << std::endl;
    }
    if (!indep_market_view_.subscribe_price_type(this, price_type_)) {
      PriceType_t t_error_price_type_ = price_type_;
      std::cerr << typeid(*this).name() << ':' << __func__ << ':' << __LINE__ << ' ' << concise_indicator_description_
                << " passed " << t_error_price_type_ << std::endl;
    }
  }
}

void OnlineDiffPairsNK::OnMarketUpdate(const unsigned int t_security_id_,
                                       const MarketUpdateInfo& cr_market_update_info_) {
  if (t_security_id_ == dep_market_view_.security_id()) {
    current_dep_price_ = SecurityMarketView::GetPriceFromType(price_type_, cr_market_update_info_);
  } else {
    current_indep_price_ = SecurityMarketView::GetPriceFromType(price_type_, cr_market_update_info_);
  }

  // the fllowing has been added since above we could be potentially be accessing the price when the
  // SMV is not ready
  if (dep_market_view_.IsBidBookEmpty() || dep_market_view_.IsAskBookEmpty() || indep_market_view_.IsBidBookEmpty() ||
      indep_market_view_.IsAskBookEmpty()) {
    return;
  }

  if (!is_ready_) {
    if (dep_market_view_.is_ready_complex(2) && indep_market_view_.is_ready_complex(2)) {
      is_ready_ = true;

      current_price_diff_ = current_indep_price_ - current_dep_price_;

      InitializeValues();
    }
  } else {
    current_price_diff_ = current_indep_price_ - current_dep_price_;

    if (watch_.msecs_from_midnight() - last_new_page_msecs_ <
        page_width_msecs_) {  // no page change ... overwrite values in same page and recalc sums
      moving_avg_diff_ += abs(inv_decay_sum_ * (abs(current_price_diff_) - abs(last_price_diff_)));
    } else {  // at leastone page added
      int num_pages_to_add_ = (int)floor((watch_.msecs_from_midnight() - last_new_page_msecs_) / page_width_msecs_);
      if (num_pages_to_add_ >= (int)decay_vector_.size()) {  // if way too many pages added ... like we took a nap
        InitializeValues();
      } else {
        if (num_pages_to_add_ == 1) {  // exactly 1 page added so optimizations possible
          moving_avg_diff_ = (abs(current_price_diff_) * inv_decay_sum_) + (moving_avg_diff_ * decay_page_factor_);
        } else {  // num_pages_to_add_ >= 2 && num_pages_to_add_ < decay_vector_.size ( )
          moving_avg_diff_ = (abs(current_price_diff_) * inv_decay_sum_) +
                             (last_price_diff_ * inv_decay_sum_ * decay_vector_sums_[(num_pages_to_add_ - 1)]) +
                             (moving_avg_diff_ * decay_vector_[num_pages_to_add_]);
        }
        last_new_page_msecs_ += (num_pages_to_add_ * page_width_msecs_);
      }
    }

    last_price_diff_ = current_price_diff_;
    indicator_value_ = current_price_diff_ - moving_avg_diff_;
    if (current_price_diff_ >= 0) {
      indicator_value_ = current_price_diff_ - moving_avg_diff_;
    } else {
      indicator_value_ = current_price_diff_ + moving_avg_diff_;
    }

    if (std::isnan(indicator_value_)) {
      // std::cerr << __PRETTY_FUNCTION__ << " nan " << std::endl;
      // exit ( 1 );
      indicator_value_ = 0;
    }

    NotifyIndicatorListeners(indicator_value_);
  }
}

void OnlineDiffPairsNK::InitializeValues() {
  moving_avg_diff_ = abs(current_price_diff_);

  last_price_diff_ = current_price_diff_;

  last_new_page_msecs_ = watch_.msecs_from_midnight() - watch_.msecs_from_midnight() % page_width_msecs_;
  indicator_value_ = 0;
}

void OnlineDiffPairsNK::OnMarketDataInterrupted(const unsigned int _security_id_, const int msecs_since_last_receive_) {
  if (indep_market_view_.security_id() == _security_id_) {
    data_interrupted_ = true;
    indicator_value_ = 0;
    NotifyIndicatorListeners(indicator_value_);
  } else
    return;
}

void OnlineDiffPairsNK::OnMarketDataResumed(const unsigned int _security_id_) {
  if (indep_market_view_.security_id() == _security_id_) {
    InitializeValues();
    data_interrupted_ = false;
  } else
    return;
}
}
