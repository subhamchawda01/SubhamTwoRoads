/**
    \file IndicatorsCode/online_ratio_pairs_mktevents_port.cpp

    \author: (c) Copyright Two Roads Technological Solutions Pvt Ltd 2011
     Address:
         Suite No 353, Evoma, #14, Bhattarhalli,
         Old Madras Road, Near Garden City College,
         KR Puram, Bangalore 560049, India
         +91 80 4190 3551
*/
#include "dvccode/CDef/math_utils.hpp"
#include "dvctrade/Indicators/indicator_util.hpp"
#include "dvctrade/Indicators/online_ratio_pairs_mktevents_port.hpp"

namespace HFSAT {

void OnlineRatioPairsMktEventsPort::CollectShortCodes(std::vector<std::string>& _shortcodes_affecting_this_indicator_,
                                                      std::vector<std::string>& _ors_source_needed_vec_,
                                                      const std::vector<const char*>& r_tokens_) {
  VectorUtils::UniqueVectorAdd(_shortcodes_affecting_this_indicator_, (std::string)r_tokens_[3]);
  IndicatorUtil::AddPortfolioShortCodeVec((std::string)r_tokens_[4], _shortcodes_affecting_this_indicator_);
}

OnlineRatioPairsMktEventsPort* OnlineRatioPairsMktEventsPort::GetUniqueInstance(
    DebugLogger& t_dbglogger_, const Watch& r_watch_, const std::vector<const char*>& r_tokens_,
    PriceType_t _basepx_pxtype_) {
  // INDICATOR _this_weight_ _indicator_string_ _dep_market_view_ _portfolio_descriptor_shortcode_ _num_events_halflife_
  // _price_type_
  return GetUniqueInstance(
      t_dbglogger_, r_watch_, *(ShortcodeSecurityMarketViewMap::StaticGetSecurityMarketView(r_tokens_[3])),
      (std::string)(r_tokens_[4]), std::max(0, atoi(r_tokens_[5])), StringToPriceType_t(r_tokens_[6]));
}

OnlineRatioPairsMktEventsPort* OnlineRatioPairsMktEventsPort::GetUniqueInstance(
    DebugLogger& t_dbglogger_, const Watch& r_watch_, SecurityMarketView& _dep_market_view_,
    std::string _portfolio_descriptor_shortcode_, unsigned int _num_events_halflife_, PriceType_t _price_type_) {
  std::ostringstream t_temp_oss_;
  t_temp_oss_ << VarName() << ' ' << _dep_market_view_.secname() << ' ' << _portfolio_descriptor_shortcode_ << ' '
              << _num_events_halflife_ << ' ' << PriceType_t_To_String(_price_type_);

  std::string concise_indicator_description_(t_temp_oss_.str());

  static std::map<std::string, OnlineRatioPairsMktEventsPort*> concise_indicator_description_map_;

  if (concise_indicator_description_map_.find(concise_indicator_description_) ==
      concise_indicator_description_map_.end()) {
    concise_indicator_description_map_[concise_indicator_description_] =
        new OnlineRatioPairsMktEventsPort(t_dbglogger_, r_watch_, concise_indicator_description_, _dep_market_view_,
                                          _portfolio_descriptor_shortcode_, _num_events_halflife_, _price_type_);
  }
  return concise_indicator_description_map_[concise_indicator_description_];
}

OnlineRatioPairsMktEventsPort::OnlineRatioPairsMktEventsPort(DebugLogger& t_dbglogger_, const Watch& r_watch_,
                                                             const std::string& concise_indicator_description_,
                                                             SecurityMarketView& _dep_market_view_,
                                                             std::string _portfolio_descriptor_shortcode_,
                                                             unsigned int _num_events_halflife_,
                                                             PriceType_t _price_type_)
    : CommonIndicator(t_dbglogger_, r_watch_, concise_indicator_description_),
      dep_market_view_(_dep_market_view_),
      indep_portfolio_price__(
          PCAPortPrice::GetUniqueInstance(t_dbglogger_, r_watch_, _portfolio_descriptor_shortcode_, _price_type_)),
      trend_history_num_events_halflife_(std::max(3u, _num_events_halflife_)),
      price_type_(_price_type_),
      moving_avg_ratio_(1),
      decay_page_factor_(MathUtils::CalcDecayFactor(std::max(3u, _num_events_halflife_))),
      inv_decay_sum_(1 - MathUtils::CalcDecayFactor(std::max(3u, _num_events_halflife_))),
      current_dep_price_(0),
      current_indep_price_(0),
      current_price_ratio_(1) {
  if (!dep_market_view_.subscribe_price_type(this, price_type_)) {
    PriceType_t t_error_price_type_ = price_type_;
    std::cerr << typeid(*this).name() << ':' << __func__ << ':' << __LINE__ << ' ' << " passed " << t_error_price_type_
              << std::endl;
  }
  indep_portfolio_price__->AddPriceChangeListener(this);
}

void OnlineRatioPairsMktEventsPort::OnPortfolioPriceChange(double _new_price_) {
  current_indep_price_ = _new_price_;

  if (!is_ready_) {
    if (dep_market_view_.is_ready_complex(2) && indep_portfolio_price__->is_ready()) {
      is_ready_ = true;
      InitializeValues();
    }
  } else {
    UpdateRatioVariables();
  }
}

void OnlineRatioPairsMktEventsPort::OnPortfolioPriceReset(double _new_price_, double _old_price_,
                                                          unsigned int is_data_interrupted_) {
  if (is_data_interrupted_ == 1u) {
    data_interrupted_ = true;
    indicator_value_ = 0;
    NotifyIndicatorListeners(indicator_value_);
  } else if (is_data_interrupted_ == 2u) {
    InitializeValues();
    data_interrupted_ = false;
  } else if (is_data_interrupted_ == 0u) {
    current_indep_price_ = _new_price_;

    double last_price_ratio_ = current_price_ratio_;
    current_price_ratio_ = current_dep_price_ / current_indep_price_;
    moving_avg_ratio_ = moving_avg_ratio_ + current_price_ratio_ - last_price_ratio_;
  }
}

void OnlineRatioPairsMktEventsPort::OnMarketUpdate(
    const unsigned int _security_id_,
    const MarketUpdateInfo& cr_market_update_info_) {  // this must be an update of dependant
  // hence using GetPriceFromType and not dep_market_view_.price_from_type
  current_dep_price_ = SecurityMarketView::GetPriceFromType(price_type_, cr_market_update_info_);

  if (!is_ready_) {
    if (dep_market_view_.is_ready_complex(2) && indep_portfolio_price__->is_ready()) {
      is_ready_ = true;
      InitializeValues();
    }
  } else {
    UpdateRatioVariables();
  }
}

void OnlineRatioPairsMktEventsPort::UpdateRatioVariables() {
  current_price_ratio_ = current_dep_price_ / current_indep_price_;

  moving_avg_ratio_ = (current_price_ratio_ * inv_decay_sum_) + (moving_avg_ratio_ * decay_page_factor_);

  indicator_value_ = moving_avg_ratio_ - current_price_ratio_;

  if (data_interrupted_) indicator_value_ = 0;

  NotifyIndicatorListeners(indicator_value_);
}

void OnlineRatioPairsMktEventsPort::InitializeValues() {
  moving_avg_ratio_ = current_price_ratio_;

  indicator_value_ = 0;
}
void OnlineRatioPairsMktEventsPort::OnMarketDataInterrupted(const unsigned int _security_id_,
                                                            const int msecs_since_last_receive_) {
  data_interrupted_ = true;
  indicator_value_ = 0;
  NotifyIndicatorListeners(indicator_value_);
}

void OnlineRatioPairsMktEventsPort::OnMarketDataResumed(const unsigned int _security_id_) {
  InitializeValues();
  data_interrupted_ = false;
}
}
