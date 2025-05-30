/**
    \file IndicatorsCode/simple_spread.cpp
    \author: (c) Copyright Two Roads Technological Solutions Pvt Ltd 2011
     Address:
         Suite No 353, Evoma, #14, Bhattarhalli,
         Old Madras Road, Near Garden City College,
         KR Puram, Bangalore 560049, India
         +91 80 4190 3551
*/
//#include "dvccode/CDef/math_utils.hpp"
#include "dvctrade/Indicators/indicator_util.hpp"
#include "dvctrade/Indicators/simple_spread.hpp"

namespace HFSAT {

void SimpleSpread::CollectShortCodes(std::vector<std::string>& _shortcodes_affecting_this_indicator_,
                                     std::vector<std::string>& _ors_source_needed_vec_,
                                     const std::vector<const char*>& r_tokens_) {
  if (r_tokens_.size() > 5u)  // doesnt mean indicator string syntax is valid .
  {
    VectorUtils::UniqueVectorAdd(_shortcodes_affecting_this_indicator_, (std::string)r_tokens_[3]);  // indep1
    VectorUtils::UniqueVectorAdd(_shortcodes_affecting_this_indicator_, (std::string)r_tokens_[4]);  // indep2
  }
}

SimpleSpread* SimpleSpread::GetUniqueInstance(DebugLogger& t_dbglogger_, const Watch& r_watch_,
                                              const std::vector<const char*>& r_tokens_,
                                              PriceType_t _base_price_type_) {
  if (r_tokens_.size() < 6u) {
    ExitVerbose(kExitErrorCodeGeneral, "SimpleSpread needs 6 tokens");
    return NULL;
  }

  // INDICATOR 1.00 SimpleSpread BR_DI_0 BR_DI_1 PRICE_TYPE
  ShortcodeSecurityMarketViewMap::StaticCheckValid(r_tokens_[3]);  // indep1
  ShortcodeSecurityMarketViewMap::StaticCheckValid(r_tokens_[4]);  // indep2

  return GetUniqueInstance(
      t_dbglogger_, r_watch_, *(ShortcodeSecurityMarketViewMap::StaticGetSecurityMarketView(r_tokens_[3])),
      *(ShortcodeSecurityMarketViewMap::StaticGetSecurityMarketView(r_tokens_[4])), StringToPriceType_t(r_tokens_[5]));
}

SimpleSpread* SimpleSpread::GetUniqueInstance(DebugLogger& t_dbglogger_, const Watch& r_watch_,
                                              const SecurityMarketView& _indep1_market_view_,
                                              const SecurityMarketView& _indep2_market_view_,
                                              PriceType_t t_price_type_) {
  std::ostringstream t_temp_oss_;

  t_temp_oss_ << VarName() << ' ' << ' ' << _indep1_market_view_.secname() << ' ' << _indep2_market_view_.secname()
              << ' ' << ' ' << t_price_type_;
  std::string concise_indicator_description_(t_temp_oss_.str());

  static std::map<std::string, SimpleSpread*> concise_indicator_description_map_;
  if (concise_indicator_description_map_.find(concise_indicator_description_) ==
      concise_indicator_description_map_.end()) {
    concise_indicator_description_map_[concise_indicator_description_] =
        new SimpleSpread(t_dbglogger_, r_watch_, concise_indicator_description_, _indep1_market_view_,
                         _indep2_market_view_, t_price_type_);
  }
  return concise_indicator_description_map_[concise_indicator_description_];
}

SimpleSpread::SimpleSpread(DebugLogger& t_dbglogger_, const Watch& r_watch_,
                           const std::string& concise_indicator_description_,
                           const SecurityMarketView& _indep1_market_view_,
                           const SecurityMarketView& _indep2_market_view_, PriceType_t _price_type_)
    : CommonIndicator(t_dbglogger_, r_watch_, concise_indicator_description_),
      indep1_market_view_(_indep1_market_view_),
      indep2_market_view_(_indep2_market_view_),
      price_type_t(_price_type_),
      current_indep1_price_(0),  // just so we dont declare these everytime
      current_indep2_price_(0),  // just so we dont declare these everytime
      indep1_interrupted_(false),
      indep2_interrupted_(false)

{
  if ((!indep1_market_view_.subscribe_price_type(this, price_type_t)) ||
      (!indep2_market_view_.subscribe_price_type(this, price_type_t))) {
    PriceType_t t_error_price_type_ = price_type_t;
    std::cerr << typeid(*this).name() << ':' << __func__ << ':' << __LINE__ << ' ' << concise_indicator_description_
              << " passed " << t_error_price_type_ << std::endl;
  }
}

void SimpleSpread::OnMarketUpdate(const unsigned int _security_id_, const MarketUpdateInfo& _market_update_info_) {
  if (indep1_market_view_.security_id() == _security_id_) {
    current_indep1_price_ = SecurityMarketView::GetPriceFromType(price_type_t, _market_update_info_);
  } else if (indep2_market_view_.security_id() == _security_id_) {
    current_indep2_price_ = SecurityMarketView::GetPriceFromType(price_type_t, _market_update_info_);
  }

  if (!is_ready_) {
    if (indep1_market_view_.is_ready_complex(2) && indep2_market_view_.is_ready_complex(2)) {
      is_ready_ = true;
    }
  } else if (!data_interrupted_) {
    indicator_value_ = current_indep2_price_ - current_indep1_price_;
    NotifyIndicatorListeners(indicator_value_);
  }
}

// market_interrupt_listener interface
void SimpleSpread::OnMarketDataInterrupted(const unsigned int _security_id_, const int msecs_since_last_receive_) {
  if (indep1_market_view_.security_id() == _security_id_) {
    indep1_interrupted_ = true;
  } else if (indep2_market_view_.security_id() == _security_id_) {
    indep2_interrupted_ = true;
  } else {
    return;
  }

  data_interrupted_ = true;
  InitializeValues();
  NotifyIndicatorListeners(indicator_value_);
}

void SimpleSpread::OnMarketDataResumed(const unsigned int _security_id_) {
  if (indep1_market_view_.security_id() == _security_id_) {
    indep1_interrupted_ = false;
  } else if (indep2_market_view_.security_id() == _security_id_) {
    indep2_interrupted_ = false;
  } else {
    return;
  }

  if ((!indep1_interrupted_) && (!indep2_interrupted_)) {
    InitializeValues();
    data_interrupted_ = false;
  }
}

void SimpleSpread::InitializeValues() {
  current_indep1_price_ = 0;
  current_indep2_price_ = 0;
  indicator_value_ = 0;
}
}
