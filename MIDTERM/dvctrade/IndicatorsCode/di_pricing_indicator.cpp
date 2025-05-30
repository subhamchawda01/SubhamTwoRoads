/**
    \file IndicatorsCode/di_pricing_indicator.cpp
    \author: (c) Copyright Two Roads Technological Solutions Pvt Ltd 2011
     Address:
         Suite No 353, Evoma, #14, Bhattarhalli,
         Old Madras Road, Near Garden City College,
         KR Puram, Bangalore 560049, India
         +91 80 4190 3551
 */

#include "dvctrade/Indicators/indicator_util.hpp"
#include "dvctrade/Indicators/di_pricing_indicator.hpp"

namespace HFSAT {

DIPricingIndicator* DIPricingIndicator::GetUniqueInstance(DebugLogger& t_dbglogger_, const Watch& r_watch_,
                                                          const std::vector<const char*>& r_tokens_,
                                                          PriceType_t _base_price_type_) {
  if (r_tokens_.size() < 8u) {
    ExitVerbose(kExitErrorCodeGeneral, "DIPricingIndicator needs 10 tokens");
    return NULL;
  }

  ShortcodeSecurityMarketViewMap::StaticCheckValid(r_tokens_[3]);
  ShortcodeSecurityMarketViewMap::StaticCheckValid(r_tokens_[4]);
  ShortcodeSecurityMarketViewMap::StaticCheckValid(r_tokens_[5]);

  return GetUniqueInstance(t_dbglogger_, r_watch_,
                           *(ShortcodeSecurityMarketViewMap::StaticGetSecurityMarketView(r_tokens_[3])),
                           *(ShortcodeSecurityMarketViewMap::StaticGetSecurityMarketView(r_tokens_[4])),
                           *(ShortcodeSecurityMarketViewMap::StaticGetSecurityMarketView(r_tokens_[5])),
                           atof(r_tokens_[6]), StringToPriceType_t(r_tokens_[7]));
}

DIPricingIndicator* DIPricingIndicator::GetUniqueInstance(DebugLogger& t_dbglogger_, const Watch& r_watch_,
                                                          SecurityMarketView& t_dep_market_view_,
                                                          SecurityMarketView& t_indep_1_market_view_,
                                                          SecurityMarketView& t_indep_2_market_view_,
                                                          double _fractional_seconds_, PriceType_t t_pxtype_)

{
  std::ostringstream t_temp_oss_;
  t_temp_oss_ << VarName() << ' ' << t_dep_market_view_.secname() << ' ' << t_indep_1_market_view_.secname() << ' '
              << t_indep_2_market_view_.secname() << ' ' << t_pxtype_ << ' ' << _fractional_seconds_;
  std::string concise_indicator_description_(t_temp_oss_.str());

  static std::map<std::string, DIPricingIndicator*> concise_indicator_description_map_;

  if (concise_indicator_description_map_.find(concise_indicator_description_) ==
      concise_indicator_description_map_.end()) {
    concise_indicator_description_map_[concise_indicator_description_] =
        new DIPricingIndicator(t_dbglogger_, r_watch_, concise_indicator_description_, t_dep_market_view_,
                               t_indep_1_market_view_, t_indep_2_market_view_, _fractional_seconds_, t_pxtype_);
  }
  return concise_indicator_description_map_[concise_indicator_description_];
}

void DIPricingIndicator::CollectShortCodes(std::vector<std::string>& _shortcodes_affecting_this_indicator_,
                                           std::vector<std::string>& _ors_source_needed_vec_,
                                           const std::vector<const char*>& r_tokens_) {
  if (r_tokens_.size() > 7u) {
    VectorUtils::UniqueVectorAdd(_shortcodes_affecting_this_indicator_, (std::string)r_tokens_[3]);
    VectorUtils::UniqueVectorAdd(_shortcodes_affecting_this_indicator_, (std::string)r_tokens_[4]);
    VectorUtils::UniqueVectorAdd(_shortcodes_affecting_this_indicator_, (std::string)r_tokens_[5]);
  }
}

void DIPricingIndicator::OnMarketUpdate(const unsigned int _security_id_,
                                        const MarketUpdateInfo& _market_update_info_) {
  if (_market_update_info_.mkt_size_weighted_price_ <= (kInvalidPrice + 0.5)) return;

  // update prices irrespective of anything
  if (_security_id_ == dep_security_id_) {
    prev_value_dep_ = SecurityMarketView::GetPriceFromType(t_pxtype_, _market_update_info_) / RATEFACTOR;
    dep_ready_ = true;
  } else if (_security_id_ == indep_1_security_id_) {
    prev_value_indep_1_ = SecurityMarketView::GetPriceFromType(t_pxtype_, _market_update_info_) / RATEFACTOR;
  } else if (_security_id_ == indep_2_security_id_) {
    prev_value_indep_2_ = SecurityMarketView::GetPriceFromType(t_pxtype_, _market_update_info_) / RATEFACTOR;
  }

  if (!is_ready_) {
    if (dep_ready_ && indep_1_ready_ && indep_2_ready_) {
      is_ready_ = true;

      indicator_value_ = ((indep_2_trend_ * indep_2_p_) / (1 + prev_value_indep_2_) +
                          (indep_1_trend_ * indep_1_p_) / (1 + prev_value_indep_1_)) *
                         (1 + prev_value_dep_);
      NotifyIndicatorListeners(indicator_value_);
    }
  } else {
    indicator_value_ = ((indep_2_trend_ * indep_2_p_) / (1 + prev_value_indep_2_) +
                        (indep_1_trend_ * indep_1_p_) / (1 + prev_value_indep_1_)) *
                       (1 + prev_value_dep_);
    NotifyIndicatorListeners(indicator_value_);
  }
}

inline void DIPricingIndicator::OnTradePrint(const unsigned int _security_id_, const TradePrintInfo& _trade_print_info_,
                                             const MarketUpdateInfo& _market_update_info_) {
  OnMarketUpdate(_security_id_, _market_update_info_);
}

inline void DIPricingIndicator::OnMarketDataInterrupted(const unsigned int _security_id_,
                                                        const int msecs_since_last_receive_) {}
inline void DIPricingIndicator::OnMarketDataResumed(const unsigned int _security_id_) {}

void DIPricingIndicator::WhyNotReady() {
  if (!is_ready_) {
    if (!dep_ready_) {
      DBGLOG_TIME_CLASS << dep_market_view_.shortcode() << " " << dep_market_view_.secname() << " not ready"
                        << DBGLOG_ENDL_FLUSH;
      DBGLOG_DUMP;
    }
    if (!indep_1_ready_) {
      DBGLOG_TIME_CLASS << indep_1_market_view_.shortcode() << " " << indep_1_market_view_.secname() << " not ready"
                        << DBGLOG_ENDL_FLUSH;
      DBGLOG_DUMP;
    }
    if (!indep_2_ready_) {
      DBGLOG_TIME_CLASS << indep_2_market_view_.shortcode() << " " << indep_2_market_view_.secname() << " not ready"
                        << DBGLOG_ENDL_FLUSH;
      DBGLOG_DUMP;
    }
  }
}

void DIPricingIndicator::OnIndicatorUpdate(const unsigned int& _indicator_index_, const double& _new_value_) {
  if (!is_ready_) {
    if (_indicator_index_ == 1) indep_1_ready_ = true;
    if (_indicator_index_ == 2) indep_2_ready_ = true;
  } else {
    if (_indicator_index_ == 1) indep_1_trend_ = _new_value_;
    if (_indicator_index_ == 2) indep_2_trend_ = _new_value_;
  }
}

inline void DIPricingIndicator::OnIndicatorUpdate(const unsigned int& _indicator_index_,
                                                  const double& new_value_decrease_, const double& new_value_nochange_,
                                                  const double& new_value_increase_) {
  return;
}

DIPricingIndicator::DIPricingIndicator(DebugLogger& t_dbglogger_, const Watch& r_watch_,
                                       const std::string& concise_indicator_description_,
                                       SecurityMarketView& t_dep_market_view_,
                                       SecurityMarketView& t_indep_1_market_view_,
                                       SecurityMarketView& t_indep_2_market_view_, double _fractional_seconds_,
                                       PriceType_t _t_pxtype_)
    : IndicatorListener(),
      CommonIndicator(t_dbglogger_, r_watch_, concise_indicator_description_),
      dep_market_view_(t_dep_market_view_),
      indep_1_market_view_(t_indep_1_market_view_),
      indep_2_market_view_(t_indep_2_market_view_),
      t_pxtype_(_t_pxtype_),
      dep_security_id_(t_dep_market_view_.security_id()),
      indep_1_security_id_(t_indep_1_market_view_.security_id()),
      indep_2_security_id_(t_indep_2_market_view_.security_id()),
      prev_value_dep_(0),
      prev_value_indep_1_(0),
      prev_value_indep_2_(0),
      dep_ready_(false),
      indep_1_ready_(false),
      indep_2_ready_(false),
      indep_1_trend_(0.0),
      indep_2_trend_(0.0),
      dep_p_(0.0),
      indep_1_p_(0.0),
      indep_2_p_(0.0) {
  // std::string date_str = boost::gregorian::to_iso_string ( boost::gregorian::day_clock::universal_day() ) ; // utc
  // date
  // int today_date_ = atoi ( date_str.c_str() );
  int today_date_ = r_watch_.YYYYMMDD();

  double dep_t_ = CurveUtils::_get_term_(today_date_, t_dep_market_view_.secname());
  double indep_1_t_ = CurveUtils::_get_term_(today_date_, t_indep_1_market_view_.secname());
  double indep_2_t_ = CurveUtils::_get_term_(today_date_, t_indep_2_market_view_.secname());
  dep_p_ = (dep_t_ * (indep_2_t_ - indep_1_t_)) / ((indep_2_t_ - dep_t_) * (dep_t_ - indep_1_t_));
  indep_1_p_ = indep_1_t_ / (dep_t_ - indep_1_t_);
  indep_2_p_ = indep_2_t_ / (indep_2_t_ - dep_t_);

  indep_1_p_ = indep_1_p_ / dep_p_;  // small optimization to prevent division on each update ( indep_1_t * ( indep_2_t_
                                     // - dep_t_ ) ) / dep_t_ * ( indep_2_t_ - indep_1_t_ ) == alpha1_
  indep_2_p_ = indep_2_p_ / dep_p_;

  indep_1_trend_indicator_ =
      SimpleTrend::GetUniqueInstance(t_dbglogger_, r_watch_, indep_1_market_view_.shortcode(), _fractional_seconds_, t_pxtype_);
  indep_2_trend_indicator_ =
      SimpleTrend::GetUniqueInstance(t_dbglogger_, r_watch_, indep_2_market_view_.shortcode(), _fractional_seconds_, t_pxtype_);
  indep_1_trend_indicator_->add_indicator_listener(1, this, 1.00);
  indep_2_trend_indicator_->add_indicator_listener(2, this, 1.00);

  if (!t_dep_market_view_.subscribe_price_type(this, t_pxtype_) ||
      !t_indep_1_market_view_.subscribe_price_type(this, t_pxtype_) ||
      !t_indep_2_market_view_.subscribe_price_type(this, t_pxtype_)) {
    std::cerr << typeid(*this).name() << ':' << __func__ << ':' << __LINE__ << ' ' << " passed " << t_pxtype_
              << std::endl;
  }
}
}
