/**
   \file IndicatorsCode/turn_over_rate.cpp

   \author: (c) Copyright Two Roads Technological Solutions Pvt Ltd 2011
   Address:
   Suite No 353, Evoma, #14, Bhattarhalli,
   Old Madras Road, Near Garden City College,
   KR Puram, Bangalore 560049, India
   +91 80 4190 3551
*/
#include "dvccode/CommonDataStructures/vector_utils.hpp"
#include "dvctrade/Indicators/indicator_util.hpp"
#include "dvctrade/Indicators/turn_over_rate.hpp"

// Q's :
//- how skewed is demand / supply in the current market
//- can trade vol in the window / avg_l1_size_window track it
//- what is does ratio as a function of window ( )

namespace HFSAT {

void TurnOverRate::CollectShortCodes(std::vector<std::string>& r_shortcodes_affecting_this_indicator_,
                                     std::vector<std::string>& r_ors_source_needed_vec_,
                                     const std::vector<const char*>& r_tokens_) {
  VectorUtils::UniqueVectorAdd(r_shortcodes_affecting_this_indicator_, (std::string)r_tokens_[3]);
}

TurnOverRate* TurnOverRate::GetUniqueInstance(DebugLogger& r_dbglogger_, const Watch& r_watch_,
                                              const std::vector<const char*>& r_tokens_, PriceType_t _basepx_pxtype_) {
  if (r_tokens_.size() < 5) {
    ExitVerbose(kModelCreationIndicatorLineLessArgs, r_dbglogger_,
                "INDICATOR weight TurnOverRate _dep_market_view_  _seconds_");
  }

  ShortcodeSecurityMarketViewMap::StaticCheckValid(r_tokens_[3]);
  return GetUniqueInstance(r_dbglogger_, r_watch_,
                           *(ShortcodeSecurityMarketViewMap::StaticGetSecurityMarketView(r_tokens_[3])),
                           atof(r_tokens_[4]));
}

TurnOverRate* TurnOverRate::GetUniqueInstance(DebugLogger& r_dbglogger_, const Watch& r_watch_,
                                              const SecurityMarketView& r_dep_market_view_, double t_secs_) {
  std::ostringstream t_temp_oss_;
  t_temp_oss_ << VarName() << ' ' << r_dep_market_view_.secname() << ' ' << t_secs_;
  std::string concise_indicator_description_(t_temp_oss_.str());

  static std::map<std::string, TurnOverRate*> concise_indicator_description_map_;
  if (concise_indicator_description_map_.find(concise_indicator_description_) ==
      concise_indicator_description_map_.end()) {
    concise_indicator_description_map_[concise_indicator_description_] =
        new TurnOverRate(r_dbglogger_, r_watch_, concise_indicator_description_, r_dep_market_view_, t_secs_);
  }
  return concise_indicator_description_map_[concise_indicator_description_];
}

TurnOverRate::TurnOverRate(DebugLogger& t_dbglogger_, const Watch& r_watch_,
                           const std::string& concise_indicator_description_,
                           const SecurityMarketView& r_dep_market_view_, double t_secs_)
    : CommonIndicator(t_dbglogger_, r_watch_, concise_indicator_description_),
      dep_market_view_(r_dep_market_view_),
      p_recent_simple_volume_measure_(
          RecentSimpleVolumeMeasure::GetUniqueInstance(t_dbglogger_, r_watch_, r_dep_market_view_, t_secs_)),
      moving_avg_sup_(0),
      last_supply_(0),
      current_supply_(0) {
  trend_history_msecs_ = std::max(20, (int)round(1000 * t_secs_));
  last_new_page_msecs_ = 0;
  page_width_msecs_ = 500;
  decay_page_factor_ = 0.95;
  inv_decay_sum_ = 0.05;
  SetTimeDecayWeights();
  if (!dep_market_view_.subscribe_price_type(this, kPriceTypeMidprice))  // for avg_l1 size
  {
    PriceType_t t_error_price_type_ = kPriceTypeMidprice;
    std::cerr << typeid(*this).name() << ':' << __func__ << ':' << __LINE__ << ' ' << concise_indicator_description_
              << " passed " << t_error_price_type_ << std::endl;
  }
  p_recent_simple_volume_measure_->AddRecentSimpleVolumeListener(1u, this);
}

void TurnOverRate::OnVolumeUpdate(unsigned int t_index_, double t_vol_) {
  if (!is_ready_) return;
  indicator_value_ = t_vol_ / moving_avg_sup_;
  for (auto i = 0u; i < tor_listener_pairs_.size(); i++) {
    tor_listener_pairs_[i].OnTorUpdate(indicator_value_);
  }
  NotifyIndicatorListeners(indicator_value_);
}

void TurnOverRate::OnMarketUpdate(const unsigned int _security_id_, const MarketUpdateInfo& _market_update_info_) {
  current_supply_ = (_market_update_info_.bestbid_size_ + _market_update_info_.bestask_size_) / 2;
  if (!is_ready_) {
    if (dep_market_view_.is_ready_complex(2)) {
      is_ready_ = true;
      InitializeValues();
    }
  } else if (!data_interrupted_) {
    if (watch_.msecs_from_midnight() - last_new_page_msecs_ < page_width_msecs_) {
      moving_avg_sup_ += inv_decay_sum_ * (current_supply_ - last_supply_);
    } else {
      int num_pages_to_add_ = (int)floor((watch_.msecs_from_midnight() - last_new_page_msecs_) / page_width_msecs_);
      if (num_pages_to_add_ >= (int)decay_vector_.size()) {
        InitializeValues();
      } else {
        if (num_pages_to_add_ == 1) {
          moving_avg_sup_ = (current_supply_ * inv_decay_sum_) + (moving_avg_sup_ * decay_vector_[1]);
        } else {  // num_pages_to_add_ >= 2 < decay_vector_.size ( )
          moving_avg_sup_ = (current_supply_ * inv_decay_sum_) +
                            (last_supply_ * inv_decay_sum_ * decay_vector_sums_[(num_pages_to_add_ - 1)]) +
                            (moving_avg_sup_ * decay_vector_[num_pages_to_add_]);
        }
        last_new_page_msecs_ += (num_pages_to_add_ * page_width_msecs_);
      }
    }

    last_supply_ = current_supply_;
  }
}

void TurnOverRate::OnMarketDataInterrupted(const unsigned int _security_id_, const int msecs_since_last_receive_) {
  if (dep_market_view_.security_id() == _security_id_) {
    data_interrupted_ = true;
    indicator_value_ = 1;
    NotifyIndicatorListeners(indicator_value_);
  }
}

void TurnOverRate::OnMarketDataResumed(const unsigned int _security_id_) {
  if (dep_market_view_.security_id() == _security_id_) {
    data_interrupted_ = false;
  } else
    return;
}

void TurnOverRate::InitializeValues() {
  moving_avg_sup_ = current_supply_;
  last_supply_ = current_supply_;
  // ratio_value_ = 1;
  last_new_page_msecs_ = watch_.msecs_from_midnight() - watch_.msecs_from_midnight() % page_width_msecs_;
  indicator_value_ = 0;
}

void TurnOverRate::WhyNotReady() {
  if (!is_ready_) {
    if (!(dep_market_view_.is_ready_complex(2))) {
      DBGLOG_TIME_CLASS << dep_market_view_.secname() << " is_ready_complex = false " << DBGLOG_ENDL_FLUSH;
      DBGLOG_DUMP;
    }
  }
}
}
