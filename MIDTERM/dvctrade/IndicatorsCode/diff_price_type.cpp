/**
    \file IndicatorsCode/diff_price_type.cpp

    \author: (c) Copyright Two Roads Technological Solutions Pvt Ltd 2011
     Address:
         Suite No 353, Evoma, #14, Bhattarhalli,
         Old Madras Road, Near Garden City College,
         KR Puram, Bangalore 560049, India
         +91 80 4190 3551
*/
#include "dvccode/CDef/math_utils.hpp"
#include "dvctrade/Indicators/indicator_util.hpp"
#include "dvctrade/Indicators/diff_price_type.hpp"

namespace HFSAT {

void DiffPriceType::CollectShortCodes(std::vector<std::string>& _shortcodes_affecting_this_indicator_,
                                      std::vector<std::string>& _ors_source_needed_vec_,
                                      const std::vector<const char*>& r_tokens_) {
  VectorUtils::UniqueVectorAdd(_shortcodes_affecting_this_indicator_, (std::string)r_tokens_[3]);
}

DiffPriceType* DiffPriceType::GetUniqueInstance(DebugLogger& t_dbglogger_, const Watch& r_watch_,
                                                const std::vector<const char*>& r_tokens_,
                                                PriceType_t _basepx_pxtype_) {
  ShortcodeSecurityMarketViewMap::StaticCheckValid(std::string(r_tokens_[3]));
  PriceType_t t_price_type_ = kPriceTypeMax;
  if (r_tokens_.size() < 6) {
    if (r_tokens_.size() < 5) {
      ExitVerbose(kModelCreationIndicatorLineLessArgs, t_dbglogger_,
                  " DiffpriceType Incorrect Syntax. Correct syntax would b INDICATOR _this_weight_ _indicator_string_ "
                  "dep_shc_ _price_type_ _base_type_");
    } else {
      t_dbglogger_ << "DiffpriceType Incorrect Syntax. Correct syntax would b INDICATOR _this_weight_ "
                      "_indicator_string_ dep_shc_  _price_type_ _base_px_type_"
                   << "\n";
      t_dbglogger_.CheckToFlushBuffer();
      t_price_type_ = _basepx_pxtype_;
    }
  } else {
    if (std::string(r_tokens_[5]).compare("#") == 0) {
      t_price_type_ = _basepx_pxtype_;
    } else {
      t_price_type_ = StringToPriceType_t(r_tokens_[5]);
    }
  }

  return GetUniqueInstance(t_dbglogger_, r_watch_,
                           *(ShortcodeSecurityMarketViewMap::StaticGetSecurityMarketView(std::string(r_tokens_[3]))),
                           StringToPriceType_t(r_tokens_[4]), t_price_type_);
}

DiffPriceType* DiffPriceType::GetUniqueInstance(DebugLogger& t_dbglogger_, const Watch& r_watch_,
                                                SecurityMarketView& t_indep_market_view_, PriceType_t _price_type_,
                                                PriceType_t _basepx_pxtype_) {
  std::ostringstream t_temp_oss_;
  t_temp_oss_ << VarName() << ' ' << t_indep_market_view_.secname() << ' ' << PriceType_t_To_String(_price_type_) << ' '
              << PriceType_t_To_String(_basepx_pxtype_);
  std::string concise_indicator_description_(t_temp_oss_.str());

  static std::map<std::string, DiffPriceType*> concise_indicator_description_map_;

  if (concise_indicator_description_map_.find(concise_indicator_description_) ==
      concise_indicator_description_map_.end()) {
    concise_indicator_description_map_[concise_indicator_description_] = new DiffPriceType(
        t_dbglogger_, r_watch_, concise_indicator_description_, t_indep_market_view_, _price_type_, _basepx_pxtype_);
  }
  return concise_indicator_description_map_[concise_indicator_description_];
}

DiffPriceType::DiffPriceType(DebugLogger& t_dbglogger_, const Watch& r_watch_,
                             const std::string& concise_indicator_description_,
                             SecurityMarketView& t_indep_market_view_, PriceType_t _price_type_,
                             PriceType_t _b_price_type_)
    : CommonIndicator(t_dbglogger_, r_watch_, concise_indicator_description_),
      indep_market_view_(t_indep_market_view_),
      price_type_(_price_type_),
      b_price_type_(_b_price_type_) {
  if (!t_indep_market_view_.subscribe_price_type(this, price_type_)) {
    PriceType_t t_error_price_type_ = price_type_;
    std::cerr << typeid(*this).name() << ':' << __func__ << ':' << __LINE__ << ' ' << " passed " << t_error_price_type_
              << std::endl;
  }

  if (!t_indep_market_view_.subscribe_price_type(this, _b_price_type_)) {
    PriceType_t t_error_price_type_ = _b_price_type_;
    std::cerr << typeid(*this).name() << ':' << __func__ << ':' << __LINE__ << ' ' << " passed " << t_error_price_type_
              << std::endl;
  }
}

void DiffPriceType::OnMarketUpdate(const unsigned int _security_id_, const MarketUpdateInfo& _market_update_info_) {
  if (!is_ready_) {
    if (indep_market_view_.is_ready_complex(2)) {
      is_ready_ = true;
      indicator_value_ = 0;
      NotifyIndicatorListeners(indicator_value_);
    }
  } else if (!data_interrupted_) {
    indicator_value_ = SecurityMarketView::GetPriceFromType(price_type_, _market_update_info_) -
                       SecurityMarketView::GetPriceFromType(b_price_type_, _market_update_info_);

    NotifyIndicatorListeners(indicator_value_);
  }
}

void DiffPriceType::OnMarketDataInterrupted(const unsigned int _security_id_, const int msecs_since_last_receive_) {
  if (indep_market_view_.security_id() == _security_id_) {
    data_interrupted_ = true;
    indicator_value_ = 0;
    NotifyIndicatorListeners(indicator_value_);
  } else
    return;
}

void DiffPriceType::OnMarketDataResumed(const unsigned int _security_id_) {
  if (indep_market_view_.security_id() == _security_id_) {
    data_interrupted_ = false;
  } else
    return;
}
}
