/**
    \file IndicatorsCode/convex_trend.cpp

    \author: (c) Copyright Two Roads Technological Solutions Pvt Ltd 2011
     Address:
         Suite No 353, Evoma, #14, Bhattarhalli,
         Old Madras Road, Near Garden City College,
         KR Puram, Bangalore 560049, India
         +91 80 4190 3551
*/
#include "dvccode/CommonDataStructures/vector_utils.hpp"

#include "dvctrade/Indicators/indicator_util.hpp"
#include "dvctrade/Indicators/convex_trend.hpp"

namespace HFSAT {

void ConvexTrend::CollectShortCodes(std::vector<std::string>& _shortcodes_affecting_this_indicator_,
                                    std::vector<std::string>& _ors_source_needed_vec_,
                                    const std::vector<const char*>& r_tokens_) {
  VectorUtils::UniqueVectorAdd(_shortcodes_affecting_this_indicator_, (std::string)r_tokens_[3]);
}

ConvexTrend* ConvexTrend::GetUniqueInstance(DebugLogger& t_dbglogger_, const Watch& r_watch_,
                                            const std::vector<const char*>& r_tokens_, PriceType_t _basepx_pxtype_) {
  // INDICATOR _this_weight_ _indicator_string_ _indep_market_view_ _fractional_seconds_st_ _fractional_seconds_lt_
  // _price_type_
  ShortcodeSecurityMarketViewMap::StaticCheckValid(r_tokens_[3]);
  if ((r_tokens_.size() >= 7) && (atof(r_tokens_[4]) <= atof(r_tokens_[5]))) {
    return GetUniqueInstance(t_dbglogger_, r_watch_,
                             *(ShortcodeSecurityMarketViewMap::StaticGetSecurityMarketView(r_tokens_[3])),
                             atof(r_tokens_[4]), atof(r_tokens_[5]), StringToPriceType_t(r_tokens_[6]));
  } else {
    return NULL;
  }
}

ConvexTrend* ConvexTrend::GetUniqueInstance(DebugLogger& t_dbglogger_, const Watch& r_watch_,
                                            const SecurityMarketView& _indep_market_view_,
                                            double _fractional_st_seconds_, double _fractional_lt_seconds_,
                                            PriceType_t _price_type_) {
  std::ostringstream t_temp_oss_;
  t_temp_oss_ << VarName() << ' ' << _indep_market_view_.secname() << ' ' << _fractional_st_seconds_ << ' '
              << _fractional_lt_seconds_ << ' ' << PriceType_t_To_String(_price_type_);
  std::string concise_indicator_description_(t_temp_oss_.str());

  static std::map<std::string, ConvexTrend*> concise_indicator_description_map_;
  if (concise_indicator_description_map_.find(concise_indicator_description_) ==
      concise_indicator_description_map_.end()) {
    concise_indicator_description_map_[concise_indicator_description_] =
        new ConvexTrend(t_dbglogger_, r_watch_, concise_indicator_description_, _indep_market_view_,
                        _fractional_st_seconds_, _fractional_lt_seconds_, _price_type_);
  }
  return concise_indicator_description_map_[concise_indicator_description_];
}

ConvexTrend::ConvexTrend(DebugLogger& t_dbglogger_, const Watch& r_watch_,
                         const std::string& concise_indicator_description_,
                         const SecurityMarketView& _indep_market_view_, double _fractional_st_seconds_,
                         double _fractional_lt_seconds_, PriceType_t _price_type_)
    : IndicatorListener(),
      CommonIndicator(t_dbglogger_, r_watch_, concise_indicator_description_),
      indep_market_view_(_indep_market_view_),
      st_trend_value_(0.00),
      lt_trend_value_(0.00) {
  p_st_indicator_ =
      SimpleTrend::GetUniqueInstance(t_dbglogger_, r_watch_, indep_market_view_.shortcode(), _fractional_st_seconds_, _price_type_);
  p_st_indicator_->add_unweighted_indicator_listener(0, this);

  p_lt_indicator_ =
      SimpleTrend::GetUniqueInstance(t_dbglogger_, r_watch_, indep_market_view_.shortcode(), _fractional_lt_seconds_, _price_type_);
  p_lt_indicator_->add_unweighted_indicator_listener(1, this);
}

void ConvexTrend::WhyNotReady() {
  if (!is_ready_) {
    if (!(indep_market_view_.is_ready_complex(2))) {
      DBGLOG_TIME_CLASS << indep_market_view_.secname() << " is_ready_complex = false " << DBGLOG_ENDL_FLUSH;
      DBGLOG_DUMP;
    }
  }
}

void ConvexTrend::OnIndicatorUpdate(const unsigned int& _indicator_index_, const double& _new_value_) {
  if (!is_ready_) {
    is_ready_ = true;
    indicator_value_ = 0;
    NotifyIndicatorListeners(0);
  } else {
    if (_indicator_index_ == 0u) {
      st_trend_value_ = _new_value_;
    } else {
      lt_trend_value_ = _new_value_;
    }

    bool st_pos_ = (st_trend_value_ < 0) ? false : true;
    bool lt_pos_ = (lt_trend_value_ < 0) ? false : true;
    if (st_pos_ != lt_pos_) {  // if short term trend is in an opposite direction to long term trend then subtract it
                               // form long term trend
      indicator_value_ = st_trend_value_ + lt_trend_value_;
    } else {  // else just report long term trend
      indicator_value_ = lt_trend_value_;
    }

    NotifyIndicatorListeners(indicator_value_);
  }
}

void ConvexTrend::OnMarketDataInterrupted(const unsigned int _security_id_, const int msecs_since_last_receive_) {
  if (indep_market_view_.security_id() == _security_id_) {
    data_interrupted_ = true;
    indicator_value_ = 0;
    NotifyIndicatorListeners(indicator_value_);
  }
}

void ConvexTrend::OnMarketDataResumed(const unsigned int _security_id_) {
  if (indep_market_view_.security_id() == _security_id_) {
    data_interrupted_ = false;
  } else
    return;
}
}
