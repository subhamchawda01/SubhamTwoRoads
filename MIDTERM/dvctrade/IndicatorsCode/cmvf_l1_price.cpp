/**
    \file IndicatorsCode/cmvf_l1_price.cpp

    \author: (c) Copyright Two Roads Technological Solutions Pvt Ltd 2011
     Address:
   Suite 217, Level 2, Prestige Omega,
   No 104, EPIP Zone, Whitefield,
   Bangalore - 560066, India
   +91 80 4060 0717
 */
#include "dvccode/CDef/math_utils.hpp"

#include "dvctrade/Indicators/indicator_util.hpp"
#include "dvctrade/Indicators/cmvf_l1_price.hpp"
#include "baseinfra/BaseUtils/curve_utils.hpp"

namespace HFSAT {

void CMVFL1Price::CollectShortCodes(std::vector<std::string>& _shortcodes_affecting_this_indicator_,
                                    std::vector<std::string>& _ors_source_needed_vec_,
                                    const std::vector<const char*>& r_tokens_) {
  VectorUtils::UniqueVectorAdd(_shortcodes_affecting_this_indicator_, (std::string)r_tokens_[3]);
  VectorUtils::UniqueVectorAdd(_shortcodes_affecting_this_indicator_, (std::string)r_tokens_[4]);
}

CMVFL1Price* CMVFL1Price::GetUniqueInstance(DebugLogger& t_dbglogger_, const Watch& r_watch_,
                                            const std::vector<const char*>& r_tokens_, PriceType_t _basepx_pxtype_) {
  // INDICATOR _this_weight_ _indicator_string_ _sec_market_view_ _price_type_
  if (r_tokens_.size() < 6) {
    ExitVerbose(kModelCreationIndicatorLineLessArgs, t_dbglogger_,
                "INDICATOR weight CMVFL1Price _indep1_market_view_ _indep2_market_view_ _price_type_ ");
  }
  ShortcodeSecurityMarketViewMap::StaticCheckValid(r_tokens_[3]);
  return GetUniqueInstance(
      t_dbglogger_, r_watch_, *(ShortcodeSecurityMarketViewMap::StaticGetSecurityMarketView(r_tokens_[3])),
      *(ShortcodeSecurityMarketViewMap::StaticGetSecurityMarketView(r_tokens_[4])), StringToPriceType_t(r_tokens_[5]));
}

CMVFL1Price* CMVFL1Price::GetUniqueInstance(DebugLogger& t_dbglogger_, const Watch& r_watch_,
                                            const SecurityMarketView& _indep1_market_view_,
                                            const SecurityMarketView& _indep2_market_view_, PriceType_t _price_type_) {
  std::ostringstream t_temp_oss_;
  t_temp_oss_ << VarName() << ' ' << _indep1_market_view_.secname() << ' ' << _indep2_market_view_.secname() << ' '
              << PriceType_t_To_String(_price_type_);
  std::string concise_indicator_description_(t_temp_oss_.str());

  static std::map<std::string, CMVFL1Price*> concise_indicator_description_map_;
  if (concise_indicator_description_map_.find(concise_indicator_description_) ==
      concise_indicator_description_map_.end()) {
    concise_indicator_description_map_[concise_indicator_description_] =
        new CMVFL1Price(t_dbglogger_, r_watch_, concise_indicator_description_, _indep1_market_view_,
                        _indep2_market_view_, _price_type_);
  }
  return concise_indicator_description_map_[concise_indicator_description_];
}

CMVFL1Price::CMVFL1Price(DebugLogger& t_dbglogger_, const Watch& r_watch_,
                         const std::string& concise_indicator_description_,
                         const SecurityMarketView& _indep1_market_view_, const SecurityMarketView& _indep2_market_view_,
                         PriceType_t _price_type_)
    : CommonIndicator(t_dbglogger_, r_watch_, concise_indicator_description_),
      indep1_market_view_(_indep1_market_view_),
      indep2_market_view_(_indep2_market_view_),
      price_type_(_price_type_),
      indep1_l1_price_(0),
      indep2_l1_price_(0),
      alpha_(0.0) {
  int indep1_term_ = HFSAT::CurveUtils::_get_term_(watch_.YYYYMMDD(), indep1_market_view_.secname());
  int indep2_term_ = HFSAT::CurveUtils::_get_term_(watch_.YYYYMMDD(), indep2_market_view_.secname());

  if (indep1_term_ >= indep2_term_) {
    ExitVerbose(kModelCreationIndicatorIncorrectArgs, t_dbglogger_,
                "For constant maturity, maturity of first shc should be less than maturity second shc");
  }
  int maturity_ = (indep2_term_ + indep1_term_) / 2;

  alpha_ = ((float)(indep2_term_ - maturity_)) /
           (maturity_);  // implicit assumptions that indep2 has higher maturity than indep1 and indep2_term > maturity

  if (!indep1_market_view_.subscribe_price_type(this, price_type_) ||
      !indep2_market_view_.subscribe_price_type(this, price_type_)) {
    PriceType_t t_error_price_type_ = _price_type_;
    std::cerr << typeid(*this).name() << ':' << __func__ << ':' << __LINE__ << ' ' << concise_indicator_description_
              << " passed " << t_error_price_type_ << std::endl;
  }
}

void CMVFL1Price::WhyNotReady() {
  if (!is_ready_) {
    if (!(indep1_market_view_.is_ready_complex(2))) {
      DBGLOG_TIME_CLASS << indep2_market_view_.secname() << " is_ready_complex = false " << DBGLOG_ENDL_FLUSH;
      DBGLOG_DUMP;
    }
    if (!(indep2_market_view_.is_ready_complex(2))) {
      DBGLOG_TIME_CLASS << indep2_market_view_.secname() << " is_ready_complex = false " << DBGLOG_ENDL_FLUSH;
      DBGLOG_DUMP;
    }
  }
}

void CMVFL1Price::OnMarketUpdate(const unsigned int _security_id_, const MarketUpdateInfo& _market_update_info_) {
  if (_security_id_ == indep1_market_view_.security_id()) {
    indep1_l1_price_ = SecurityMarketView::GetPriceFromType(price_type_, _market_update_info_);
  } else if (_security_id_ == indep2_market_view_.security_id()) {
    indep2_l1_price_ = SecurityMarketView::GetPriceFromType(price_type_, _market_update_info_);
  } else {
    return;
  }

  if (!is_ready_) {
    if (indep1_market_view_.is_ready_complex(2) && indep2_market_view_.is_ready_complex(2)) {
      is_ready_ = true;
      InitializeValues();
    }
  } else if (!data_interrupted_) {
    indicator_value_ = indep1_l1_price_ * alpha_ + indep2_l1_price_ * (1 - alpha_);
    NotifyIndicatorListeners(indicator_value_);
  }
}

void CMVFL1Price::InitializeValues() {
  indep1_l1_price_ = 0;
  indep2_l1_price_ = 0;
  indicator_value_ = 0;
}

// market_interrupt_listener interface
void CMVFL1Price::OnMarketDataInterrupted(const unsigned int _security_id_, const int msecs_since_last_receive_) {
  // Emitting the same value is the best thing to do here....so doing nothing
}

void CMVFL1Price::OnMarketDataResumed(const unsigned int _security_id_) {
  // See explanantion above in OnMarketDataInterrupted
}
}
