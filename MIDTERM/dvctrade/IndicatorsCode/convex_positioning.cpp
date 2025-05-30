/**
    \file IndicatorsCode/convex_positioning.cpp

    \author: (c) Copyright Two Roads Technological Solutions Pvt Ltd 2011
     Address:
         Suite 217, Level 2, Prestige Omega,
         No 104, EPIP Zone, Whitefield,
         Bangalore - 560066, India
         +91 80 4060 0717
*/
#include "dvccode/CDef/math_utils.hpp"
#include "dvctrade/Indicators/indicator_util.hpp"
#include "dvctrade/Indicators/convex_positioning.hpp"

namespace HFSAT {

void ConvexPositioning::CollectShortCodes(std::vector<std::string>& _shortcodes_affecting_this_indicator_,
                                          std::vector<std::string>& _ors_source_needed_vec_,
                                          const std::vector<const char*>& r_tokens_) {
  VectorUtils::UniqueVectorAdd(_shortcodes_affecting_this_indicator_, (std::string)r_tokens_[3]);
}

ConvexPositioning* ConvexPositioning::GetUniqueInstance(DebugLogger& t_dbglogger_, const Watch& r_watch_,
                                                        const std::vector<const char*>& r_tokens_,
                                                        PriceType_t _basepx_pxtype_) {
  // INDICATOR _this_weight_ _indicator_string_ _indep_market_view_ _num_trades_halflife_
  return GetUniqueInstance(t_dbglogger_, r_watch_,
                           *(ShortcodeSecurityMarketViewMap::StaticGetSecurityMarketView(r_tokens_[3])),
                           atoi(r_tokens_[4]), atoi(r_tokens_[5]), _basepx_pxtype_);
}

ConvexPositioning* ConvexPositioning::GetUniqueInstance(DebugLogger& t_dbglogger_, const Watch& r_watch_,
                                                        SecurityMarketView& _indep_market_view_,
                                                        int _bucket_size_short_, int _bucket_size_long_,
                                                        PriceType_t _basepx_pxtype_) {
  std::ostringstream t_temp_oss_;
  t_temp_oss_ << VarName() << ' ' << _indep_market_view_.secname() << ' ' << _bucket_size_short_ << ' '
              << _bucket_size_long_ << ' ' << PriceType_t_To_String(_basepx_pxtype_);
  std::string concise_indicator_description_(t_temp_oss_.str());

  static std::map<std::string, ConvexPositioning*> concise_indicator_description_map_;
  if (concise_indicator_description_map_.find(concise_indicator_description_) ==
      concise_indicator_description_map_.end()) {
    concise_indicator_description_map_[concise_indicator_description_] =
        new ConvexPositioning(t_dbglogger_, r_watch_, concise_indicator_description_, _indep_market_view_,
                              _bucket_size_short_, _bucket_size_long_);
  }

  return concise_indicator_description_map_[concise_indicator_description_];
}

ConvexPositioning::ConvexPositioning(DebugLogger& t_dbglogger_, const Watch& r_watch_,
                                     const std::string& concise_indicator_description_,
                                     SecurityMarketView& _indep_market_view_, int _bucket_size_short_,
                                     int _bucket_size_long_)
    : CommonIndicator(t_dbglogger_, r_watch_, concise_indicator_description_),
      indep_market_view_(_indep_market_view_),
      p_st_indicator_(Positioning::GetUniqueInstance(t_dbglogger_, r_watch_, _indep_market_view_, _bucket_size_short_,
                                                     HFSAT::kPriceTypeMidprice)),
      p_lt_indicator_(Positioning::GetUniqueInstance(t_dbglogger_, r_watch_, _indep_market_view_, _bucket_size_long_,
                                                     HFSAT::kPriceTypeMidprice)),
      st_positioning_value_(0),
      lt_positioning_value_(0) {
  double ratio_lt_st_ = std::max(1.00, sqrt(_bucket_size_long_ / _bucket_size_short_));
  p_st_indicator_->add_indicator_listener(0, this, 1.00);
  p_lt_indicator_->add_indicator_listener(1, this, (1.00 / ratio_lt_st_));
}

void ConvexPositioning::OnIndicatorUpdate(const unsigned int& _indicator_index_, const double& _new_value_) {
  if (!is_ready_) {
    is_ready_ = true;
    NotifyIndicatorListeners(0);
  } else if (!data_interrupted_) {
    if (_indicator_index_ == 0u) {
      st_positioning_value_ = _new_value_;
    } else {
      lt_positioning_value_ = _new_value_;
    }
    indicator_value_ = st_positioning_value_ + lt_positioning_value_;

    if (p_st_indicator_->IsDataInterrupted()) indicator_value_ = 0;

    NotifyIndicatorListeners(indicator_value_);
  }
}

void ConvexPositioning::OnMarketDataInterrupted(const unsigned int _security_id_, const int msecs_since_last_receive_) {
  if (indep_market_view_.security_id() == _security_id_) {
    data_interrupted_ = true;
    indicator_value_ = 0;
    NotifyIndicatorListeners(indicator_value_);
  }
}

void ConvexPositioning::OnMarketDataResumed(const unsigned int _security_id_) {
  if (indep_market_view_.security_id() == _security_id_) {
    data_interrupted_ = false;
  }
}
}
