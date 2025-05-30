/**
    \file IndicatorsCode/simple_price_type.cpp

    \author: (c) Copyright Two Roads Technological Solutions Pvt Ltd 2011
     Address:
         Suite No 353, Evoma, #14, Bhattarhalli,
         Old Madras Road, Near Garden City College,
         KR Puram, Bangalore 560049, India
         +91 80 4190 3551
*/
#include "dvccode/CDef/math_utils.hpp"
#include "dvctrade/Indicators/indicator_util.hpp"
#include "dvctrade/Indicators/simple_price_type.hpp"

namespace HFSAT {

void SimplePriceType::CollectShortCodes(std::vector<std::string>& _shortcodes_affecting_this_indicator_,
                                        std::vector<std::string>& _ors_source_needed_vec_,
                                        const std::vector<const char*>& r_tokens_) {
  VectorUtils::UniqueVectorAdd(_shortcodes_affecting_this_indicator_, (std::string)r_tokens_[3]);
}

SimplePriceType* SimplePriceType::GetUniqueInstance(DebugLogger& t_dbglogger_, const Watch& r_watch_,
                                                    const std::vector<const char*>& r_tokens_,
                                                    PriceType_t _basepx_pxtype_) {
  // INDICATOR _this_weight_ _indicator_string_ _dep_market_view_ _price_type_
  if (r_tokens_.size() >= 5) {
    return GetUniqueInstance(t_dbglogger_, r_watch_,
                             *(ShortcodeSecurityMarketViewMap::StaticGetSecurityMarketView(r_tokens_[3])),
                             StringToPriceType_t(r_tokens_[4]));
  } else {
    ExitVerbose(kExitErrorCodeGeneral,
                "insufficient inputs to SimplePriceType : INDICATOR _this_weight_ _indicator_string_ _dep_market_view_ "
                "_price_type_\n");
    return NULL;  // wont reach here , just to remove warning
  }
}

SimplePriceType* SimplePriceType::GetUniqueInstance(DebugLogger& t_dbglogger_, const Watch& r_watch_,
                                                    SecurityMarketView& _indep_market_view_, PriceType_t _price_type_) {
  static std::map<std::string, SimplePriceType*> concise_indicator_description_map_;

  std::ostringstream t_temp_oss_;
  t_temp_oss_ << VarName() << ' ' << _indep_market_view_.secname() << ' ' << PriceType_t_To_String(_price_type_);

  std::string concise_indicator_description_(t_temp_oss_.str());
  if (concise_indicator_description_map_.find(concise_indicator_description_) ==
      concise_indicator_description_map_.end()) {
    concise_indicator_description_map_[concise_indicator_description_] =
        new SimplePriceType(t_dbglogger_, r_watch_, concise_indicator_description_, _indep_market_view_, _price_type_);
  }
  return concise_indicator_description_map_[concise_indicator_description_];
}

SimplePriceType::SimplePriceType(DebugLogger& t_dbglogger_, const Watch& r_watch_,
                                 const std::string& concise_indicator_description_,
                                 SecurityMarketView& _indep_market_view_, PriceType_t _price_type_)
    : CommonIndicator(t_dbglogger_, r_watch_, concise_indicator_description_),
      indep_market_view_(_indep_market_view_),
      price_type_(_price_type_) {
  if (!indep_market_view_.subscribe_price_type(this, price_type_)) {
    PriceType_t t_error_price_type_ = price_type_;
    std::cerr << typeid(*this).name() << ':' << __func__ << ':' << __LINE__ << ' ' << concise_indicator_description_
              << " passed " << t_error_price_type_ << std::endl;
  }
}

void SimplePriceType::OnMarketUpdate(const unsigned int _security_id_, const MarketUpdateInfo& _market_update_info_) {
  if (!is_ready_) {
    if (indep_market_view_.is_ready_complex(2)) {
      is_ready_ = true;
      indicator_value_ = SecurityMarketView::GetPriceFromType(price_type_, _market_update_info_);
      NotifyIndicatorListeners(indicator_value_);
    }
  } else {
    indicator_value_ = SecurityMarketView::GetPriceFromType(price_type_, _market_update_info_);
    NotifyIndicatorListeners(indicator_value_);
  }
}
void SimplePriceType::OnMarketDataInterrupted(const unsigned int _security_id_, const int msecs_since_last_receive_) {
  data_interrupted_ = true;
}

void SimplePriceType::OnMarketDataResumed(const unsigned int _security_id_) { data_interrupted_ = false; }
}
