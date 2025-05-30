/**
    \file IndicatorsCode/l1_price.cpp

    \author: (c) Copyright Two Roads Technological Solutions Pvt Ltd 2011
     Address:
         Suite 217, Level 2, Prestige Omega,
         No 104, EPIP Zone, Whitefield,
         Bangalore - 560066, India
         +91 80 4060 0717
*/
#include "dvccode/CDef/math_utils.hpp"

#include "dvctrade/Indicators/indicator_util.hpp"
#include "dvctrade/Indicators/l1_price.hpp"

namespace HFSAT {

void L1Price::CollectShortCodes(std::vector<std::string>& _shortcodes_affecting_this_indicator_,
                                std::vector<std::string>& _ors_source_needed_vec_,
                                const std::vector<const char*>& r_tokens_) {
  VectorUtils::UniqueVectorAdd(_shortcodes_affecting_this_indicator_, (std::string)r_tokens_[3]);
}

L1Price* L1Price::GetUniqueInstance(DebugLogger& t_dbglogger_, const Watch& r_watch_,
                                    const std::vector<const char*>& r_tokens_, PriceType_t _basepx_pxtype_) {
  // INDICATOR _this_weight_ _indicator_string_ _sec_market_view_ _price_type_
  if (r_tokens_.size() < 5) {
    ExitVerbose(kModelCreationIndicatorLineLessArgs, t_dbglogger_,
                "INDICATOR weight L1Price _sec_market_view_ _price_type_ ");
  }
  ShortcodeSecurityMarketViewMap::StaticCheckValid(r_tokens_[3]);
  return GetUniqueInstance(t_dbglogger_, r_watch_,
                           *(ShortcodeSecurityMarketViewMap::StaticGetSecurityMarketView(r_tokens_[3])),
                           StringToPriceType_t(r_tokens_[4]));
}

L1Price* L1Price::GetUniqueInstance(DebugLogger& t_dbglogger_, const Watch& r_watch_,
                                    const SecurityMarketView& _sec_market_view_, PriceType_t _price_type_) {
  std::ostringstream t_temp_oss_;
  t_temp_oss_ << VarName() << ' ' << _sec_market_view_.secname() << ' ' << PriceType_t_To_String(_price_type_);
  std::string concise_indicator_description_(t_temp_oss_.str());

  static std::map<std::string, L1Price*> concise_indicator_description_map_;
  if (concise_indicator_description_map_.find(concise_indicator_description_) ==
      concise_indicator_description_map_.end()) {
    concise_indicator_description_map_[concise_indicator_description_] =
        new L1Price(t_dbglogger_, r_watch_, concise_indicator_description_, _sec_market_view_, _price_type_);
  }
  return concise_indicator_description_map_[concise_indicator_description_];
}

L1Price::L1Price(DebugLogger& t_dbglogger_, const Watch& r_watch_, const std::string& concise_indicator_description_,
                 const SecurityMarketView& _sec_market_view_, PriceType_t _price_type_)
    : CommonIndicator(t_dbglogger_, r_watch_, concise_indicator_description_),
      sec_market_view_(_sec_market_view_),
      price_type_(_price_type_),
      price_(0) {
  if (!sec_market_view_.subscribe_price_type(this, price_type_)) {
    PriceType_t t_error_price_type_ = _price_type_;
    std::cerr << typeid(*this).name() << ':' << __func__ << ':' << __LINE__ << ' ' << concise_indicator_description_
              << " passed " << t_error_price_type_ << std::endl;
  }
}

void L1Price::WhyNotReady() {
  if (!is_ready_) {
    if (!(sec_market_view_.is_ready_complex(2))) {
      DBGLOG_TIME_CLASS << sec_market_view_.secname() << " is_ready_complex = false " << DBGLOG_ENDL_FLUSH;
      DBGLOG_DUMP;
    }
  }
}

void L1Price::OnMarketUpdate(const unsigned int _security_id_, const MarketUpdateInfo& _market_update_info_) {
  if (_security_id_ == sec_market_view_.security_id()) {
    price_ = SecurityMarketView::GetPriceFromType(price_type_, _market_update_info_);
  }
  if (!is_ready_) {
    if (sec_market_view_.is_ready_complex(2) ||
	(NSESecurityDefinitions::IsOption(sec_market_view_.shortcode()) && sec_market_view_.is_ready_complex2(1))) {
      is_ready_ = true;
      InitializeValues();
    }
  } else if (!data_interrupted_) {
    indicator_value_ = price_;
    NotifyIndicatorListeners(indicator_value_);
  }
}

void L1Price::InitializeValues() { indicator_value_ = price_; }

// market_interrupt_listener interface
void L1Price::OnMarketDataInterrupted(const unsigned int _security_id_, const int msecs_since_last_receive_) {
  if (sec_market_view_.security_id() == _security_id_) {
    data_interrupted_ = true;
    NotifyIndicatorListeners(indicator_value_);
  }
}

void L1Price::OnMarketDataResumed(const unsigned int _security_id_) {
  if (data_interrupted_) {
    if (sec_market_view_.security_id() == _security_id_) {
      InitializeValues();
      data_interrupted_ = false;
    }
  }
}
}
