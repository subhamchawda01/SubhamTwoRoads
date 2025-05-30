/**
    \file IndicatorsCode/diff_price_type_spread.cpp

    \author: (c) Copyright Two Roads Technological Solutions Pvt Ltd 2011
     Address:
         Suite No 353, Evoma, #14, Bhattarhalli,
         Old Madras Road, Near Garden City College,
         KR Puram, Bangalore 560049, India
         +91 80 4190 3551
*/
#include "dvccode/CDef/math_utils.hpp"
#include "dvctrade/Indicators/indicator_util.hpp"
#include "dvctrade/Indicators/diff_price_type_spread.hpp"

namespace HFSAT {

void DiffPriceTypeSpread::CollectShortCodes(std::vector<std::string>& _shortcodes_affecting_this_indicator_,
                                            std::vector<std::string>& _ors_source_needed_vec_,
                                            const std::vector<const char*>& r_tokens_) {
  HFSAT::CurveUtils::GetSpreadShortcodes((std::string)r_tokens_[3], _shortcodes_affecting_this_indicator_);
}

DiffPriceTypeSpread* DiffPriceTypeSpread::GetUniqueInstance(DebugLogger& t_dbglogger_, const Watch& r_watch_,
                                                            const std::vector<const char*>& r_tokens_,
                                                            PriceType_t _basepx_pxtype_) {
  // INDICATOR _this_weight_ _indicator_string_ _indep_market_view_ _price_type_
  if (r_tokens_.size() < 5) {
    ExitVerbose(kModelCreationIndicatorLineLessArgs, t_dbglogger_,
                "DiffPriceTypeSpread incorrect syntax. Should be INDICATOR _this_weight_ _indicator_string_ "
                "_indep_market_view_ _price_type_");
  }

  SpreadMarketView* t_spread_market_view_ =
      ShortcodeSpreadMarketViewMap::GetSpreadMarketView(std::string(r_tokens_[3]));
  if (t_spread_market_view_ == NULL) {
    t_spread_market_view_ = new SpreadMarketView(t_dbglogger_, r_watch_, std::string(r_tokens_[3]));
    ShortcodeSpreadMarketViewMap::AddEntry(std::string(r_tokens_[3]), t_spread_market_view_);
  }
  return GetUniqueInstance(t_dbglogger_, r_watch_, *(t_spread_market_view_),
                           StringToPriceType_t(std::string(r_tokens_[4])), _basepx_pxtype_);
}

DiffPriceTypeSpread* DiffPriceTypeSpread::GetUniqueInstance(DebugLogger& t_dbglogger_, const Watch& r_watch_,
                                                            SpreadMarketView& _spread_market_view_,
                                                            PriceType_t _price_type_, PriceType_t _basepx_pxtype_) {
  std::ostringstream t_temp_oss_;
  t_temp_oss_ << VarName() << ' ' << _spread_market_view_.shortcode() << ' ' << PriceType_t_To_String(_price_type_)
              << ' ' << PriceType_t_To_String(_basepx_pxtype_);
  std::string concise_indicator_description_(t_temp_oss_.str());

  static std::map<std::string, DiffPriceTypeSpread*> concise_indicator_description_map_;

  if (concise_indicator_description_map_.find(concise_indicator_description_) ==
      concise_indicator_description_map_.end()) {
    concise_indicator_description_map_[concise_indicator_description_] = new DiffPriceTypeSpread(
        t_dbglogger_, r_watch_, concise_indicator_description_, _spread_market_view_, _price_type_, _basepx_pxtype_);
  }
  return concise_indicator_description_map_[concise_indicator_description_];
}

DiffPriceTypeSpread::DiffPriceTypeSpread(DebugLogger& t_dbglogger_, const Watch& r_watch_,
                                         const std::string& concise_indicator_description_,
                                         SpreadMarketView& _spread_market_view_, PriceType_t _price_type_,
                                         PriceType_t _base_pxtype_)
    : CommonIndicator(t_dbglogger_, r_watch_, concise_indicator_description_),
      spread_market_view_(_spread_market_view_),
      base_price_type_(_base_pxtype_),
      price_type_(_price_type_),
      data_interrupted_vec_(_spread_market_view_.GetShcVec().size(), false) {
  if (!spread_market_view_.SubscribeSpreadMarketView(this)) {
    std::cerr << typeid(*this).name() << ':' << __func__ << ':' << __LINE__ << ' '
              << " can't subscribe to SpreadMarketView for " << spread_market_view_.shortcode() << std::endl;
    exit(0);
  }
  is_ready_ = spread_market_view_.is_ready();
}

void DiffPriceTypeSpread::OnSpreadMarketViewUpdate(const SpreadMarketView& _spread_market_view_) {
  if (!is_ready_) {
    is_ready_ = spread_market_view_.is_ready();
  }

  if (is_ready_ && !data_interrupted_) {
    indicator_value_ =
        _spread_market_view_.price_from_type(price_type_) - _spread_market_view_.price_from_type(base_price_type_);
    NotifyIndicatorListeners(indicator_value_);
  }
}

void DiffPriceTypeSpread::OnMarketDataInterrupted(const unsigned int _security_id_,
                                                  const int msecs_since_last_receive_) {
  std::vector<std::string> shc_vec_ = spread_market_view_.GetShcVec();
  for (auto i = 0u; i < shc_vec_.size(); i++) {
    if (ShortcodeSecurityMarketViewMap::GetUniqueInstance().GetSecurityMarketView(shc_vec_[i])->security_id() ==
        _security_id_) {
      data_interrupted_vec_[i] = true;
      data_interrupted_ = true;
      indicator_value_ = 0;
      NotifyIndicatorListeners(indicator_value_);
      return;
    }
  }
}

void DiffPriceTypeSpread::OnMarketDataResumed(const unsigned int _security_id_) {
  std::vector<std::string> shc_vec_ = spread_market_view_.GetShcVec();
  for (auto i = 0u; i < shc_vec_.size(); i++) {
    if (ShortcodeSecurityMarketViewMap::GetUniqueInstance().GetSecurityMarketView(shc_vec_[i])->security_id() ==
        _security_id_) {
      data_interrupted_vec_[i] = false;
      data_interrupted_ = !(HFSAT::VectorUtils::CheckAllForValue(data_interrupted_vec_, false));
      return;
    }
  }
}
}
