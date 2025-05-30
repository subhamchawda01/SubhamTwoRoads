/**
    \file IndicatorsCode/l1_port_price.cpp

    \author: (c) Copyright Two Roads Technological Solutions Pvt Ltd 2011
     Address:
         Suite 217, Level 2, Prestige Omega,
         No 104, EPIP Zone, Whitefield,
         Bangalore - 560066, India
         +91 80 4060 0717
*/
#include "dvccode/CDef/math_utils.hpp"

#include "dvctrade/Indicators/indicator_util.hpp"
#include "dvctrade/Indicators/l1_port_price.hpp"

namespace HFSAT {

void L1PortPrice::CollectShortCodes(std::vector<std::string>& _shortcodes_affecting_this_indicator_,
                                    std::vector<std::string>& _ors_source_needed_vec_,
                                    const std::vector<const char*>& r_tokens_) {
  IndicatorUtil::AddPortfolioShortCodeVec((std::string)r_tokens_[3], _shortcodes_affecting_this_indicator_);
}

L1PortPrice* L1PortPrice::GetUniqueInstance(DebugLogger& t_dbglogger_, const Watch& r_watch_,
                                            const std::vector<const char*>& r_tokens_, PriceType_t _basepx_pxtype_) {
  // INDICATOR _this_weight_ _indicator_string_ _portfolio_shc_ _price_type_
  if (r_tokens_.size() < 5) {
    ExitVerbose(kModelCreationIndicatorLineLessArgs, t_dbglogger_,
                "INDICATOR weight L1PortPrice _portfolio_ _price_type_ ");
  }
  return GetUniqueInstance(t_dbglogger_, r_watch_, (std::string)(r_tokens_[3]), StringToPriceType_t(r_tokens_[4]));
}

L1PortPrice* L1PortPrice::GetUniqueInstance(DebugLogger& t_dbglogger_, const Watch& r_watch_,
                                            const std::string _portfolio_shc__, PriceType_t _price_type_) {
  std::ostringstream t_temp_oss_;
  t_temp_oss_ << VarName() << ' ' << _portfolio_shc__ << ' ' << PriceType_t_To_String(_price_type_);
  std::string concise_indicator_description_(t_temp_oss_.str());

  static std::map<std::string, L1PortPrice*> concise_indicator_description_map_;
  if (concise_indicator_description_map_.find(concise_indicator_description_) ==
      concise_indicator_description_map_.end()) {
    concise_indicator_description_map_[concise_indicator_description_] =
        new L1PortPrice(t_dbglogger_, r_watch_, concise_indicator_description_, _portfolio_shc__, _price_type_);
  }
  return concise_indicator_description_map_[concise_indicator_description_];
}

L1PortPrice::L1PortPrice(DebugLogger& t_dbglogger_, const Watch& r_watch_,
                         const std::string& concise_indicator_description_, const std::string _portfolio_shc_,
                         PriceType_t _price_type_)
    : CommonIndicator(t_dbglogger_, r_watch_, concise_indicator_description_),
      portfolio_price_(PCAPortPrice::GetUniqueInstance(t_dbglogger_, r_watch_, _portfolio_shc_, _price_type_)),
      price_type_(_price_type_) {
  portfolio_price_->AddPriceChangeListener(this);
}

void L1PortPrice::OnPortfolioPriceReset(double t_new_portfolio_price_, double t_old_portfolio_price_,
                                        unsigned int is_data_interrupted_) {
  if (is_data_interrupted_ == 1u) {
    data_interrupted_ = true;
    indicator_value_ = 0;
    NotifyIndicatorListeners(indicator_value_);
  } else if (is_data_interrupted_ == 2u) {
    InitializeValues();
    data_interrupted_ = false;
  } else if (is_data_interrupted_ == 0u) {
    indicator_value_ = t_new_portfolio_price_;
  }
}

void L1PortPrice::OnPortfolioPriceChange(double _new_price_) {
  indicator_value_ = _new_price_;
  if (!is_ready_ && portfolio_price_->is_ready()) {
    is_ready_ = true;
    InitializeValues();
  } else {
    if (data_interrupted_) indicator_value_ = 0;
    NotifyIndicatorListeners(indicator_value_);
  }
}

void L1PortPrice::WhyNotReady() {
  if (!is_ready_) {
    if (!portfolio_price_->is_ready()) {
      portfolio_price_->WhyNotReady();
    }
  }
}

void L1PortPrice::InitializeValues() { indicator_value_ = 0; }

// market_interrupt_listener interface
void L1PortPrice::OnMarketDataInterrupted(const unsigned int _security_id_, const int msecs_since_last_receive_) {
  data_interrupted_ = true;
  indicator_value_ = 0;
  NotifyIndicatorListeners(indicator_value_);
}

void L1PortPrice::OnMarketDataResumed(const unsigned int _security_id_) {
  InitializeValues();
  data_interrupted_ = false;
}
}
