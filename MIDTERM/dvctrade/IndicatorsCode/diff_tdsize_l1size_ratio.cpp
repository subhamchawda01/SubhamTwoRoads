/**
    \file IndicatorsCode/diff_tdsize_l1size_ratio.cpp

    \author: (c) Copyright Two Roads Technological Solutions Pvt Ltd 2011
     Address:
         Suite No 353, Evoma, #14, Bhattarhalli,
         Old Madras Road, Near Garden City College,
         KR Puram, Bangalore 560049, India
         +91 80 4190 3551
*/
#include "dvccode/CDef/math_utils.hpp"
#include "dvctrade/Indicators/indicator_util.hpp"
#include "dvctrade/Indicators/diff_tdsize_l1size_ratio.hpp"

namespace HFSAT {

void DiffTDSizeL1SizeRatio::CollectShortCodes(std::vector<std::string>& _shortcodes_affecting_this_indicator_,
                                              std::vector<std::string>& _ors_source_needed_vec_,
                                              const std::vector<const char*>& r_tokens_) {
  VectorUtils::UniqueVectorAdd(_shortcodes_affecting_this_indicator_, (std::string)r_tokens_[3]);
}

DiffTDSizeL1SizeRatio* DiffTDSizeL1SizeRatio::GetUniqueInstance(DebugLogger& t_dbglogger_, const Watch& r_watch_,
                                                                const std::vector<const char*>& r_tokens_,
                                                                PriceType_t _basepx_pxtype_) {
  // INDICATOR _this_weight_ _indicator_string_ _indep_market_view_ _fractional_seconds_
  return GetUniqueInstance(t_dbglogger_, r_watch_,
                           *(ShortcodeSecurityMarketViewMap::StaticGetSecurityMarketView(r_tokens_[3])),
                           atof(r_tokens_[4]), _basepx_pxtype_);
}

DiffTDSizeL1SizeRatio* DiffTDSizeL1SizeRatio::GetUniqueInstance(DebugLogger& t_dbglogger_, const Watch& r_watch_,
                                                                SecurityMarketView& _indep_market_view_,
                                                                double _fractional_seconds_,
                                                                PriceType_t _basepx_pxtype_) {
  std::ostringstream t_temp_oss_;
  t_temp_oss_ << VarName() << ' ' << _indep_market_view_.secname() << ' ' << _fractional_seconds_ << " "
              << PriceType_t_To_String(_basepx_pxtype_);
  std::string concise_indicator_description_(t_temp_oss_.str());

  static std::map<std::string, DiffTDSizeL1SizeRatio*> concise_indicator_description_map_;
  if (concise_indicator_description_map_.find(concise_indicator_description_) ==
      concise_indicator_description_map_.end()) {
    concise_indicator_description_map_[concise_indicator_description_] = new DiffTDSizeL1SizeRatio(
        t_dbglogger_, r_watch_, concise_indicator_description_, _indep_market_view_, _fractional_seconds_);
  }
  return concise_indicator_description_map_[concise_indicator_description_];
}

DiffTDSizeL1SizeRatio::DiffTDSizeL1SizeRatio(DebugLogger& t_dbglogger_, const Watch& r_watch_,
                                             const std::string& concise_indicator_description_,
                                             SecurityMarketView& _indep_market_view_, double _fractional_seconds_)
    : CommonIndicator(t_dbglogger_, r_watch_, concise_indicator_description_),
      indep_market_view_(_indep_market_view_),
      moving_avg_l1_size_(0),
      last_l1_size_recorded_(0),
      current_l1_size_(0),
      time_decayed_trade_info_manager_(*(TimeDecayedTradeInfoManager::GetUniqueInstance(
          t_dbglogger_, r_watch_, _indep_market_view_, _fractional_seconds_))) {
  trend_history_msecs_ = std::max(20, (int)round(1000 * _fractional_seconds_));
  last_new_page_msecs_ = 0;
  page_width_msecs_ = 500;
  decay_page_factor_ = 0.95;
  inv_decay_sum_ = 0.05;
  SetTimeDecayWeights();
  time_decayed_trade_info_manager_.compute_sumasz();
  time_decayed_trade_info_manager_.compute_sumbsz();
  indep_market_view_.subscribe_tradeprints(this);
  watch_.subscribe_BigTimePeriod(this);
}

void DiffTDSizeL1SizeRatio::OnTradePrint(const unsigned int _security_id_, const TradePrintInfo& _trade_print_info_,
                                         const MarketUpdateInfo& cr_market_update_info_) {
  if (time_decayed_trade_info_manager_.sumasz_ >= 0 && time_decayed_trade_info_manager_.sumbsz_ >= 0 && is_ready_) {
    indicator_value_ =
        (time_decayed_trade_info_manager_.sumasz_ - time_decayed_trade_info_manager_.sumbsz_) / (moving_avg_l1_size_);
  } else {
    indicator_value_ = 0;
  }
  if (data_interrupted_) {
    indicator_value_ = 0;
  }
  NotifyIndicatorListeners(indicator_value_);
}

void DiffTDSizeL1SizeRatio::OnTimePeriodUpdate(const int num_pages_to_add_) {
  current_l1_size_ =
      (indep_market_view_.market_update_info_.bestask_size_ + indep_market_view_.market_update_info_.bestbid_size_) / 2;

  if (!is_ready_) {
    if (indep_market_view_.is_ready_complex(2)) {
      is_ready_ = true;
      InitializeValues();
    }
  } else if (!data_interrupted_) {
    if (watch_.msecs_from_midnight() - last_new_page_msecs_ < page_width_msecs_) {
      moving_avg_l1_size_ += inv_decay_sum_ * (current_l1_size_ - last_l1_size_recorded_);
    } else {
      int num_pages_to_add_ = (int)floor((watch_.msecs_from_midnight() - last_new_page_msecs_) / page_width_msecs_);
      if (num_pages_to_add_ >= (int)decay_vector_.size()) {
        InitializeValues();
      } else {
        if (num_pages_to_add_ == 1) {
          moving_avg_l1_size_ = (current_l1_size_ * inv_decay_sum_) + (moving_avg_l1_size_ * decay_vector_[1]);
        } else {  // num_pages_to_add_ >= 2 < decay_vector_.size ( )
          moving_avg_l1_size_ = (current_l1_size_ * inv_decay_sum_) + (last_l1_size_recorded_ * inv_decay_sum_ *
                                                                       decay_vector_sums_[(num_pages_to_add_ - 1)]) +
                                (moving_avg_l1_size_ * decay_vector_[num_pages_to_add_]);
        }
        last_new_page_msecs_ += (num_pages_to_add_ * page_width_msecs_);
      }
    }
    last_l1_size_recorded_ = current_l1_size_;
  }
}

void DiffTDSizeL1SizeRatio::OnMarketDataInterrupted(const unsigned int _security_id_,
                                                    const int msecs_since_last_receive_) {
  if (indep_market_view_.security_id() == _security_id_) {
    data_interrupted_ = true;
    indicator_value_ = 0;
    NotifyIndicatorListeners(indicator_value_);
  } else {
    return;
  }
}

void DiffTDSizeL1SizeRatio::OnMarketDataResumed(const unsigned int _security_id_) {
  if (indep_market_view_.security_id() == _security_id_) {
    data_interrupted_ = false;
    time_decayed_trade_info_manager_.InitializeValues();
  } else {
    return;
  }
}

void DiffTDSizeL1SizeRatio::InitializeValues() {
  moving_avg_l1_size_ = current_l1_size_;
  last_l1_size_recorded_ = current_l1_size_;
  last_new_page_msecs_ = watch_.msecs_from_midnight() - watch_.msecs_from_midnight() % page_width_msecs_;
  indicator_value_ = 0;
}
}
