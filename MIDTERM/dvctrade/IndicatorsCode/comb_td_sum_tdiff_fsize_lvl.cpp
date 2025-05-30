/**
    \file IndicatorsCode/comb_td_sum_tdiff_fsize_lvl.cpp

    \author: (c) Copyright Two Roads Technological Solutions Pvt Ltd 2011
     Address:
         Suite No 353, Evoma, #14, Bhattarhalli,
         Old Madras Road, Near Garden City College,
         KR Puram, Bangalore 560049, India
         +91 80 4190 3551
*/
#include "dvccode/CommonDataStructures/vector_utils.hpp"

#include "dvctrade/Indicators/indicator_util.hpp"
#include "dvctrade/Indicators/comb_td_sum_tdiff_fsize_lvl.hpp"
#include "dvctrade/Indicators/td_sum_tdiff_fsize_lvl.hpp"

namespace HFSAT {

void CombTDSumTDiffFSizeLvl::CollectShortCodes(std::vector<std::string>& _shortcodes_affecting_this_indicator_,
                                               std::vector<std::string>& _ors_source_needed_vec_,
                                               const std::vector<const char*>& r_tokens_) {
  VectorUtils::UniqueVectorAdd(_shortcodes_affecting_this_indicator_, (std::string)r_tokens_[3]);
}

CombTDSumTDiffFSizeLvl* CombTDSumTDiffFSizeLvl::GetUniqueInstance(DebugLogger& t_dbglogger_, const Watch& r_watch_,
                                                                  const std::vector<const char*>& r_tokens_,
                                                                  PriceType_t _basepx_pxtype_) {
  // INDICATOR _this_weight_ _indicator_string_ _indep_market_view_ _fractional_seconds_st_ _fractional_seconds_lt_
  // _price_type_
  ShortcodeSecurityMarketViewMap::StaticCheckValid(r_tokens_[3]);
  if ((r_tokens_.size() >= 6) && (atof(r_tokens_[4]) <= atof(r_tokens_[5]))) {
    return GetUniqueInstance(t_dbglogger_, r_watch_,
                             *(ShortcodeSecurityMarketViewMap::StaticGetSecurityMarketView(r_tokens_[3])),
                             atof(r_tokens_[4]), atof(r_tokens_[5]));
  } else {
    return NULL;
  }
}

CombTDSumTDiffFSizeLvl* CombTDSumTDiffFSizeLvl::GetUniqueInstance(DebugLogger& t_dbglogger_, const Watch& r_watch_,
                                                                  SecurityMarketView& _indep_market_view_,
                                                                  double _fractional_st_seconds_,
                                                                  double _fractional_lt_seconds_) {
  std::ostringstream t_temp_oss_;
  t_temp_oss_ << VarName() << ' ' << _indep_market_view_.secname() << ' ' << _fractional_st_seconds_ << ' '
              << _fractional_lt_seconds_;
  std::string concise_indicator_description_(t_temp_oss_.str());

  static std::map<std::string, CombTDSumTDiffFSizeLvl*> concise_indicator_description_map_;
  if (concise_indicator_description_map_.find(concise_indicator_description_) ==
      concise_indicator_description_map_.end()) {
    concise_indicator_description_map_[concise_indicator_description_] =
        new CombTDSumTDiffFSizeLvl(t_dbglogger_, r_watch_, concise_indicator_description_, _indep_market_view_,
                                   _fractional_st_seconds_, _fractional_lt_seconds_);
  }
  return concise_indicator_description_map_[concise_indicator_description_];
}

CombTDSumTDiffFSizeLvl::CombTDSumTDiffFSizeLvl(DebugLogger& t_dbglogger_, const Watch& r_watch_,
                                               const std::string& concise_indicator_description_,
                                               SecurityMarketView& _indep_market_view_, double _fractional_st_seconds_,
                                               double _fractional_lt_seconds_)
    : IndicatorListener(),
      CommonIndicator(t_dbglogger_, r_watch_, concise_indicator_description_),
      indep_market_view_(_indep_market_view_),
      st_trend_value_(0.00),
      lt_trend_value_(0.00) {
  p_st_indicator_ =
      TDSumTDiffFSizeLvl::GetUniqueInstance(t_dbglogger_, r_watch_, indep_market_view_, _fractional_st_seconds_);
  p_st_indicator_->add_indicator_listener(0, this, 1.00);

  double ratio_lt_st_ = std::max(1.00, sqrt(_fractional_lt_seconds_ / _fractional_st_seconds_));

  p_lt_indicator_ =
      TDSumTDiffFSizeLvl::GetUniqueInstance(t_dbglogger_, r_watch_, indep_market_view_, _fractional_lt_seconds_);
  p_lt_indicator_->add_indicator_listener(1, this, (1.00 / ratio_lt_st_));
}

void CombTDSumTDiffFSizeLvl::WhyNotReady() {
  if (!is_ready_) {
    if (!(indep_market_view_.is_ready_complex(2))) {
      DBGLOG_TIME_CLASS << indep_market_view_.secname() << " is_ready_complex = false " << DBGLOG_ENDL_FLUSH;
      DBGLOG_DUMP;
    }
  }
}

void CombTDSumTDiffFSizeLvl::OnIndicatorUpdate(const unsigned int& _indicator_index_, const double& _new_value_) {
  if (!is_ready_) {
    is_ready_ = true;
    NotifyIndicatorListeners(0);
  } else {
    if (_indicator_index_ == 0u) {
      st_trend_value_ = _new_value_;
    } else {
      lt_trend_value_ = _new_value_;
    }
    indicator_value_ = st_trend_value_ + lt_trend_value_;

    if (p_st_indicator_->IsDataInterrupted()) indicator_value_ = 0;

    NotifyIndicatorListeners(indicator_value_);
  }
}

void CombTDSumTDiffFSizeLvl::OnMarketDataInterrupted(const unsigned int _security_id_,
                                                     const int msecs_since_last_receive_) {
  if (indep_market_view_.security_id() == _security_id_) {
    data_interrupted_ = true;
    indicator_value_ = 0;
    NotifyIndicatorListeners(indicator_value_);
  } else
    return;
}

void CombTDSumTDiffFSizeLvl::OnMarketDataResumed(const unsigned int _security_id_) {
  if (indep_market_view_.security_id() == _security_id_) {
    data_interrupted_ = false;
  } else
    return;
}
}
