/**
    \file IndicatorsCode/diff_pair_price_type.cpp

    \author: (c) Copyright Two Roads Technological Solutions Pvt Ltd 2011
     Address:
         Suite No 353, Evoma, #14, Bhattarhalli,
         Old Madras Road, Near Garden City College,
         KR Puram, Bangalore 560049, India
         +91 80 4190 3551
*/
#include "dvccode/CDef/math_utils.hpp"
#include "dvctrade/Indicators/indicator_util.hpp"
#include "dvctrade/Indicators/diff_pair_price_type.hpp"

namespace HFSAT {

void DiffPairPriceType::CollectShortCodes(std::vector<std::string>& t_shortcodes_affecting_this_indicator_,
                                          std::vector<std::string>& t_ors_source_needed_vec_,
                                          const std::vector<const char*>& r_tokens_) {
  VectorUtils::UniqueVectorAdd(t_shortcodes_affecting_this_indicator_, (std::string)r_tokens_[3]);
}

DiffPairPriceType* DiffPairPriceType::GetUniqueInstance(DebugLogger& t_dbglogger_, const Watch& r_watch_,
                                                        const std::vector<const char*>& r_tokens_,
                                                        PriceType_t _basepx_pxtype_) {
  ShortcodeSecurityMarketViewMap::StaticCheckValid(r_tokens_[3]);
  return GetUniqueInstance(t_dbglogger_, r_watch_,
                           *(ShortcodeSecurityMarketViewMap::StaticGetSecurityMarketView(r_tokens_[3])),
                           StringToPriceType_t(r_tokens_[4]), StringToPriceType_t(r_tokens_[5]));
}

DiffPairPriceType* DiffPairPriceType::GetUniqueInstance(DebugLogger& t_dbglogger_, const Watch& r_watch_,
                                                        SecurityMarketView& t_indep_market_view_,
                                                        PriceType_t t_base_price_type_,
                                                        PriceType_t t_target_price_type_) {
  std::ostringstream t_temp_oss_;
  t_temp_oss_ << VarName() << ' ' << t_indep_market_view_.secname() << ' ' << PriceType_t_To_String(t_base_price_type_)
              << ' ' << PriceType_t_To_String(t_target_price_type_);
  std::string concise_indicator_description_(t_temp_oss_.str());

  static std::map<std::string, DiffPairPriceType*> concise_indicator_description_map_;

  if (concise_indicator_description_map_.find(concise_indicator_description_) ==
      concise_indicator_description_map_.end()) {
    concise_indicator_description_map_[concise_indicator_description_] =
        new DiffPairPriceType(t_dbglogger_, r_watch_, concise_indicator_description_, t_indep_market_view_,
                              t_base_price_type_, t_target_price_type_);
  }
  return concise_indicator_description_map_[concise_indicator_description_];
}

DiffPairPriceType::DiffPairPriceType(DebugLogger& t_dbglogger_, const Watch& r_watch_,
                                     const std::string& concise_indicator_description_,
                                     SecurityMarketView& t_indep_market_view_, PriceType_t t_base_price_type_,
                                     PriceType_t t_target_price_type_)
    : CommonIndicator(t_dbglogger_, r_watch_, concise_indicator_description_),
      indep_market_view_(t_indep_market_view_),
      base_price_type_(t_base_price_type_),
      target_price_type_(t_target_price_type_) {
  if (!t_indep_market_view_.subscribe_price_type(this, base_price_type_)) {
    PriceType_t t_error_price_type_ = base_price_type_;
    std::cerr << typeid(*this).name() << ':' << __func__ << ':' << __LINE__ << ' ' << concise_indicator_description_
              << " passed " << t_error_price_type_ << " to DEP " << std::endl;
  }
  if (!t_indep_market_view_.subscribe_price_type(this, target_price_type_)) {
    PriceType_t t_error_price_type_ = target_price_type_;
    std::cerr << typeid(*this).name() << ':' << __func__ << ':' << __LINE__ << ' ' << concise_indicator_description_
              << " passed " << t_error_price_type_ << std::endl;
  }
}

void DiffPairPriceType::OnMarketUpdate(const unsigned int t_security_id_,
                                       const MarketUpdateInfo& t_market_update_info_) {
  if (!is_ready_) {
    if (indep_market_view_.is_ready_complex(2)) {
      is_ready_ = true;
      indicator_value_ = 0;
      NotifyIndicatorListeners(indicator_value_);
    }
  } else {
    indicator_value_ = SecurityMarketView::GetPriceFromType(target_price_type_, t_market_update_info_) -
                       SecurityMarketView::GetPriceFromType(base_price_type_, t_market_update_info_);

    if (data_interrupted_) indicator_value_ = 0;

    NotifyIndicatorListeners(indicator_value_);
  }
}

void DiffPairPriceType::OnMarketDataInterrupted(const unsigned int _security_id_, const int msecs_since_last_receive_) {
  if (indep_market_view_.security_id() == _security_id_) {
    data_interrupted_ = true;
    indicator_value_ = 0;
    NotifyIndicatorListeners(indicator_value_);
  }
}

void DiffPairPriceType::OnMarketDataResumed(const unsigned int _security_id_) {
  if (indep_market_view_.security_id() == _security_id_) {
    data_interrupted_ = false;
  }
}
}
