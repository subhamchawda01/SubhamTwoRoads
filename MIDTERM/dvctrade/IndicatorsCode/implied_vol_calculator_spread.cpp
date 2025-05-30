/**
    \file IndicatorsCode/implied_vol_calculator_spread.cpp

    \author: (c) Copyright Two Roads Technological Solutions Pvt Ltd 2011
     Address:
         Suite No 353, Evoma, #14, Bhattarhalli,
         Old Madras Road, Near Garden City College,
         KR Puram, Bangalore 560049, India
         +91 80 4190 3551
*/

#include "dvctrade/Indicators/implied_vol_calculator_spread.hpp"

namespace HFSAT {

void ImpliedVolCalculatorSpread::CollectShortCodes(std::vector<std::string>& _shortcodes_affecting_this_indicator_,
                                                   std::vector<std::string>& _ors_source_needed_vec_,
                                                   const std::vector<const char*>& r_tokens_) {
  VectorUtils::UniqueVectorAdd(_shortcodes_affecting_this_indicator_, (std::string)r_tokens_[3]);
  VectorUtils::UniqueVectorAdd(_shortcodes_affecting_this_indicator_,
                               NSESecurityDefinitions::GetOppositeContractShc((std::string)r_tokens_[3]));
}

ImpliedVolCalculatorSpread* ImpliedVolCalculatorSpread::GetUniqueInstance(DebugLogger& t_dbglogger_,
                                                                          const Watch& r_watch_,
                                                                          const std::vector<const char*>& r_tokens_,
                                                                          PriceType_t _basepx_pxtype_) {
  // INDICATOR _this_weight_ _indicator_string_ _dep_market_view_ _price_type_
  ShortcodeSecurityMarketViewMap::StaticCheckValid(r_tokens_[3]);
  ShortcodeSecurityMarketViewMap::StaticCheckValid(NSESecurityDefinitions::GetOppositeContractShc(r_tokens_[3]));
  ShortcodeSecurityMarketViewMap::StaticCheckValid(
      NSESecurityDefinitions::GetFutureShortcodeFromOptionShortCode(r_tokens_[3]));
  return GetUniqueInstance(t_dbglogger_, r_watch_,
                           *(ShortcodeSecurityMarketViewMap::StaticGetSecurityMarketView(r_tokens_[3])),
                           StringToPriceType_t(r_tokens_[4]));
}

ImpliedVolCalculatorSpread* ImpliedVolCalculatorSpread::GetUniqueInstance(DebugLogger& t_dbglogger_,
                                                                          const Watch& r_watch_,
                                                                          const SecurityMarketView& _dep_market_view_,
                                                                          PriceType_t _price_type_) {
  std::ostringstream t_temp_oss_;
  t_temp_oss_ << VarName() << ' ' << _dep_market_view_.secname() << ' ' << PriceType_t_To_String(_price_type_);
  std::string concise_indicator_description_(t_temp_oss_.str());

  static std::map<std::string, ImpliedVolCalculatorSpread*> concise_indicator_description_map_;
  if (concise_indicator_description_map_.find(concise_indicator_description_) ==
      concise_indicator_description_map_.end()) {
    concise_indicator_description_map_[concise_indicator_description_] = new ImpliedVolCalculatorSpread(
        t_dbglogger_, r_watch_, concise_indicator_description_, _dep_market_view_, _price_type_);
  }
  return concise_indicator_description_map_[concise_indicator_description_];
}

ImpliedVolCalculatorSpread::ImpliedVolCalculatorSpread(DebugLogger& t_dbglogger_, const Watch& r_watch_,
                                                       const std::string& concise_indicator_description_,
                                                       const SecurityMarketView& _dep_market_view_,
                                                       PriceType_t _price_type_)
    : CommonIndicator(t_dbglogger_, r_watch_, concise_indicator_description_),
      dep_market_view_(_dep_market_view_),
      opp_market_view_(*(ShortcodeSecurityMarketViewMap::StaticGetSecurityMarketView(
          NSESecurityDefinitions::GetOppositeContractShc(dep_market_view_.shortcode())))),
      fut_market_view_(*(ShortcodeSecurityMarketViewMap::StaticGetSecurityMarketView(
          NSESecurityDefinitions::GetFutureShortcodeFromOptionShortCode(dep_market_view_.shortcode())))),
      price_type_(_price_type_),
      dep_bid_implied_vol_(0),
      dep_ask_implied_vol_(0),
      opp_bid_implied_vol_(0),
      opp_ask_implied_vol_(0),
      prev_dep_bid_price_(0),
      prev_dep_ask_price_(0),
      prev_opp_bid_price_(0),
      prev_opp_ask_price_(0),
      fut_price_(0) {
  if (!dep_market_view_.subscribe_price_type(this, price_type_)) {
    PriceType_t t_error_price_type_ = price_type_;
    std::cerr << typeid(*this).name() << ':' << __func__ << ':' << __LINE__ << ' ' << concise_indicator_description_
              << " passed " << t_error_price_type_ << std::endl;
  }

  if (!opp_market_view_.subscribe_price_type(this, price_type_)) {
    PriceType_t t_error_price_type_ = price_type_;
    std::cerr << typeid(*this).name() << ':' << __func__ << ':' << __LINE__ << ' ' << concise_indicator_description_
              << " passed " << t_error_price_type_ << std::endl;
  }

  if (!fut_market_view_.subscribe_price_type(this, price_type_)) {
    PriceType_t t_error_price_type_ = price_type_;
    std::cerr << typeid(*this).name() << ':' << __func__ << ':' << __LINE__ << ' ' << concise_indicator_description_
              << " passed " << t_error_price_type_ << std::endl;
  }

  dep_option_ = OptionObject::GetUniqueInstance(t_dbglogger_, r_watch_, dep_market_view_.shortcode());
  opp_option_ = OptionObject::GetUniqueInstance(t_dbglogger_, r_watch_, opp_market_view_.shortcode());
}

void ImpliedVolCalculatorSpread::WhyNotReady() {
  if (!is_ready_) {
    if (!(dep_market_view_.is_ready())) {
      DBGLOG_TIME_CLASS << dep_market_view_.secname() << " is_ready_complex = false " << DBGLOG_ENDL_FLUSH;
      DBGLOG_DUMP;
    }

    if (!(opp_market_view_.is_ready())) {
      DBGLOG_TIME_CLASS << opp_market_view_.secname() << " is_ready_complex = false " << DBGLOG_ENDL_FLUSH;
      DBGLOG_DUMP;
    }
  }
}

void ImpliedVolCalculatorSpread::InitializeValues() {
  indicator_value_ = 0;
  fut_price_ = fut_market_view_.mid_price();  // We are taking the mid price for first time, then we will update in
                                              // OnMarketUpdate with price_type_
}

void ImpliedVolCalculatorSpread::OnMarketUpdate(const unsigned int _security_id_,
                                                const MarketUpdateInfo& _market_update_info_) {
  if (!is_ready_) {
    if ((dep_market_view_.is_ready_complex2(1)) && (opp_market_view_.is_ready()) &&
        (fut_market_view_.is_ready_complex(2))) {
      is_ready_ = true;
      InitializeValues();
    }
  } else if (!data_interrupted_) {
    if (_security_id_ == dep_market_view_.security_id()) {
      if (dep_market_view_.bestbid_price() != prev_dep_bid_price_) {
        dep_bid_implied_vol_ = dep_option_->MktImpliedVol(fut_price_, dep_market_view_.bestbid_price());
        prev_dep_bid_price_ = dep_market_view_.bestbid_price();
      }
      if (dep_market_view_.bestask_price() != prev_dep_ask_price_) {
        dep_ask_implied_vol_ = dep_option_->MktImpliedVol(fut_price_, dep_market_view_.bestask_price());
        prev_dep_ask_price_ = dep_market_view_.bestask_price();
      }
    } else if (_security_id_ == opp_market_view_.security_id()) {
      if (opp_market_view_.bestbid_price() != prev_opp_bid_price_) {
        opp_bid_implied_vol_ = opp_option_->MktImpliedVol(fut_price_, opp_market_view_.bestbid_price());
        prev_opp_bid_price_ = opp_market_view_.bestbid_price();
      }
      if (opp_market_view_.bestask_price() != prev_opp_ask_price_) {
        opp_ask_implied_vol_ = opp_option_->MktImpliedVol(fut_price_, opp_market_view_.bestask_price());
        prev_opp_ask_price_ = opp_market_view_.bestask_price();
      }
    } else {
      double curr_fut_price_ = SecurityMarketView::GetPriceFromType(price_type_, _market_update_info_);
      if (fut_price_ != curr_fut_price_) {
        dep_bid_implied_vol_ = dep_option_->MktImpliedVol(curr_fut_price_, dep_market_view_.bestbid_price());
        dep_ask_implied_vol_ = dep_option_->MktImpliedVol(curr_fut_price_, dep_market_view_.bestask_price());
        opp_bid_implied_vol_ = opp_option_->MktImpliedVol(curr_fut_price_, opp_market_view_.bestbid_price());
        opp_ask_implied_vol_ = opp_option_->MktImpliedVol(curr_fut_price_, opp_market_view_.bestask_price());
      }
    }

    double max_bid_ = std::max(dep_bid_implied_vol_, opp_bid_implied_vol_);
    double min_ask_ = ((dep_ask_implied_vol_ == 0) || (opp_ask_implied_vol_ == 0))
                          ? (dep_ask_implied_vol_ + opp_ask_implied_vol_)
                          : std::min(dep_ask_implied_vol_, opp_ask_implied_vol_);

    double bid_implied_vol_;
    double ask_implied_vol_;

    if ((min_ask_ != 0) && (max_bid_ != 0)) {
      bid_implied_vol_ = std::min(min_ask_, max_bid_);
      ask_implied_vol_ = std::max(min_ask_, max_bid_);
      ;
      indicator_value_ = (bid_implied_vol_ + ask_implied_vol_) / 2;
    } else if (min_ask_ == 0) {
      bid_implied_vol_ = max_bid_;
      ask_implied_vol_ = 0;
      indicator_value_ = max_bid_;
    } else {
      bid_implied_vol_ = 0;
      ask_implied_vol_ = min_ask_;
      indicator_value_ = min_ask_;
    }

    indicator_value_ *= 100;  // Changing the value to percentage

    if (indicator_value_ > 0) NotifyIndicatorListeners(indicator_value_);
  }
}

// market_interrupt_listener interface
void ImpliedVolCalculatorSpread::OnMarketDataInterrupted(const unsigned int _security_id_,
                                                         const int msecs_since_last_receive_) {
  if ((dep_market_view_.security_id() == _security_id_)) {
    data_interrupted_ = true;
    indicator_value_ = 0;
    NotifyIndicatorListeners(indicator_value_);
  }
}

void ImpliedVolCalculatorSpread::OnMarketDataResumed(const unsigned int _security_id_) {
  if ((dep_market_view_.security_id() == _security_id_)) {
    InitializeValues();
    data_interrupted_ = false;
  }
}
}
