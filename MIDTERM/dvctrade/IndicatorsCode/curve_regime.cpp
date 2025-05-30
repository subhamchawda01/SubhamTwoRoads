/**
   \file IndicatorsCode/curve_regime.cpp

   \author: (c) Copyright Two Roads Technological Solutions Pvt Ltd 2011
   Address:
   Suite No 353, Evoma, #14, Bhattarhalli,
   Old Madras Road, Near Garden City College,
   KR Puram, Bangalore 560049, India
   +91 80 4190 3551
*/
#include <fstream>
#include <strings.h>
#include <stdlib.h>
#include <sstream>

#include "dvccode/CDef/math_utils.hpp"
#include "baseinfra/MarketAdapter/shortcode_security_market_view_map.hpp"
#include "dvctrade/Indicators/curve_regime.hpp"

namespace HFSAT {

void CurveRegime::CollectShortCodes(std::vector<std::string>& _shortcodes_affecting_this_indicator_,
                                    std::vector<std::string>& _ors_source_needed_vec_,
                                    const std::vector<const char*>& r_tokens_) {
  if (r_tokens_.size() > 3)
    VectorUtils::UniqueVectorAdd(_shortcodes_affecting_this_indicator_, (std::string)r_tokens_[3]);
  if (r_tokens_.size() > 4)
    VectorUtils::UniqueVectorAdd(_shortcodes_affecting_this_indicator_, (std::string)r_tokens_[4]);
  if (r_tokens_.size() > 5)
    VectorUtils::UniqueVectorAdd(_shortcodes_affecting_this_indicator_, (std::string)r_tokens_[5]);
}

CurveRegime* CurveRegime::GetUniqueInstance(DebugLogger& t_dbglogger_, const Watch& r_watch_,
                                            const std::vector<const char*>& r_tokens_, PriceType_t _basepx_pxtype_) {
  // INDICATOR  _this_weight_  _indicator_string_  _product_1_ _product_2_ _product_3_
  if (r_tokens_.size() < 6) {
    ExitVerbose(kModelCreationIndicatorLineLessArgs, t_dbglogger_,
                "INDICATOR weight CurveRegime _prod1_market_view_ _prod2_market_view_ _prod3_market_view_");
  }
  ShortcodeSecurityMarketViewMap::StaticCheckValid(r_tokens_[3]);
  ShortcodeSecurityMarketViewMap::StaticCheckValid(r_tokens_[4]);
  ShortcodeSecurityMarketViewMap::StaticCheckValid(r_tokens_[5]);
  return GetUniqueInstance(
      t_dbglogger_, r_watch_, *(ShortcodeSecurityMarketViewMap::StaticGetSecurityMarketView(r_tokens_[3])),
      *(ShortcodeSecurityMarketViewMap::StaticGetSecurityMarketView(r_tokens_[4])),
      *(ShortcodeSecurityMarketViewMap::StaticGetSecurityMarketView(r_tokens_[5])), _basepx_pxtype_);
}

CurveRegime* CurveRegime::GetUniqueInstance(DebugLogger& t_dbglogger_, const Watch& r_watch_,
                                            const SecurityMarketView& _prod1_market_view_,
                                            const SecurityMarketView& _prod2_market_view_,
                                            const SecurityMarketView& _prod3_market_view_,
                                            PriceType_t _basepx_pxtype_) {
  std::ostringstream t_temp_oss_;
  t_temp_oss_ << VarName() << ' ' << _prod1_market_view_.secname() << ' ' << _prod2_market_view_.secname() << ' '
              << _prod3_market_view_.secname() << ' ' << _basepx_pxtype_ << '\n';

  std::string concise_indicator_description_(t_temp_oss_.str());

  static std::map<std::string, CurveRegime*> concise_indicator_description_map_;

  if (concise_indicator_description_map_.find(concise_indicator_description_) ==
      concise_indicator_description_map_.end()) {
    concise_indicator_description_map_[concise_indicator_description_] =
        new CurveRegime(t_dbglogger_, r_watch_, concise_indicator_description_, _prod1_market_view_,
                        _prod2_market_view_, _prod3_market_view_, _basepx_pxtype_);
  }
  return concise_indicator_description_map_[concise_indicator_description_];
}

CurveRegime::CurveRegime(DebugLogger& t_dbglogger_, const Watch& r_watch_,
                         const std::string& concise_indicator_description_,
                         const SecurityMarketView& _prod1_market_view_, const SecurityMarketView& _prod2_market_view_,
                         const SecurityMarketView& _prod3_market_view_, PriceType_t t_basepx_pxtype_)
    : CommonIndicator(t_dbglogger_, r_watch_, concise_indicator_description_),
      prod1_market_view_(_prod1_market_view_),
      prod2_market_view_(_prod2_market_view_),
      prod3_market_view_(_prod3_market_view_),
      prod1_px_(0),
      prod2_px_(0),
      prod3_px_(0),
      prod1_updated_(false),
      prod2_updated_(false),
      prod3_updated_(false),
      price_type_(t_basepx_pxtype_) {
  /// Subscribe to price type here
  if (!prod1_market_view_.subscribe_price_type(this, t_basepx_pxtype_)) {
    PriceType_t t_error_price_type_ = t_basepx_pxtype_;
    std::cerr << typeid(*this).name() << ':' << __func__ << ':' << __LINE__ << ' ' << concise_indicator_description_
              << " passed " << t_error_price_type_ << std::endl;
  }
  if (!prod2_market_view_.subscribe_price_type(this, t_basepx_pxtype_)) {
    PriceType_t t_error_price_type_ = t_basepx_pxtype_;
    std::cerr << typeid(*this).name() << ':' << __func__ << ':' << __LINE__ << ' ' << concise_indicator_description_
              << " passed " << t_error_price_type_ << std::endl;
  }
  if (!prod3_market_view_.subscribe_price_type(this, t_basepx_pxtype_)) {
    PriceType_t t_error_price_type_ = t_basepx_pxtype_;
    std::cerr << typeid(*this).name() << ':' << __func__ << ':' << __LINE__ << ' ' << concise_indicator_description_
              << " passed " << t_error_price_type_ << std::endl;
  }
  InitializeValues();
}

void CurveRegime::OnMarketUpdate(const unsigned int _security_id_, const MarketUpdateInfo& _market_update_info_) {
  if (!is_ready_) {
    if (_security_id_ == prod1_market_view_.security_id()) {
      prod1_updated_ = true;
    } else if (_security_id_ == prod2_market_view_.security_id()) {
      prod2_updated_ = true;
    } else if (_security_id_ == prod3_market_view_.security_id()) {
      prod3_updated_ = true;
    }
    if (prod1_updated_ && prod2_updated_ && prod3_updated_) {
      if (prod1_market_view_.is_ready_complex(2) && prod2_market_view_.is_ready_complex(2) &&
          prod3_market_view_.is_ready_complex(2)) {
        is_ready_ = true;
      }
    } else {
      return;
    }
  }

  if (_security_id_ == prod1_market_view_.security_id()) {
    prod1_px_ = SecurityMarketView::GetPriceFromType(price_type_, _market_update_info_);
  } else if (_security_id_ == prod2_market_view_.security_id()) {
    prod2_px_ = SecurityMarketView::GetPriceFromType(price_type_, _market_update_info_);
  } else if (_security_id_ == prod3_market_view_.security_id()) {
    prod3_px_ = SecurityMarketView::GetPriceFromType(price_type_, _market_update_info_);
  }

  if (prod1_px_ > prod2_px_ && prod2_px_ > prod3_px_) {
    indicator_value_ = 1;
  } else if (prod1_px_ >= prod2_px_ && prod2_px_ <= prod3_px_) {
    indicator_value_ = 4;
  } else if (prod1_px_ <= prod2_px_ && prod2_px_ >= prod3_px_) {
    indicator_value_ = 2;
  } else if (prod1_px_ < prod2_px_ && prod2_px_ < prod3_px_) {
    indicator_value_ = 3;
  }

  NotifyIndicatorListeners(indicator_value_);
}

void CurveRegime::OnTradePrint(const unsigned int _security_id_, const TradePrintInfo& _trade_print_info_,
                               const MarketUpdateInfo& _market_update_info_) {
  OnMarketUpdate(_security_id_, _market_update_info_);
}

void CurveRegime::OnMarketDataResumed(const unsigned int _security_id_) {
  /// we're already using most stable model since interrupt.
}

void CurveRegime::OnMarketDataInterrupted(const unsigned int _security_id_, const int msecs_since_last_receive) {
  InitializeValues();
}

void CurveRegime::InitializeValues() {
  prod1_updated_ = false;
  prod2_updated_ = false;
  prod2_updated_ = false;
  is_ready_ = false;
}
}
