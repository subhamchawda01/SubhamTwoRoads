/**
    \file IndicatorsCode/di1_curve_adjusted_price.cpp
    \author: (c) Copyright Two Roads Technological Solutions Pvt Ltd 2011
     Address:
         Suite No 353, Evoma, #14, Bhattarhalli,
         Old Madras Road, Near Garden City College,
         KR Puram, Bangalore 560049, India
         +91 80 4190 3551
*/
//#include "dvccode/CDef/math_utils.hpp"
#include "dvctrade/Indicators/indicator_util.hpp"
#include "dvctrade/Indicators/di1_curve_adjusted_price.hpp"

namespace HFSAT {

void DI1CurveAdjustedPrice::CollectShortCodes(std::vector<std::string>& _shortcodes_affecting_this_indicator_,
                                              std::vector<std::string>& _ors_source_needed_vec_,
                                              const std::vector<const char*>& r_tokens_) {
  if (r_tokens_.size() > 6u)  // doesnt mean indicator string syntax is valid .
  {
    VectorUtils::UniqueVectorAdd(_shortcodes_affecting_this_indicator_, (std::string)r_tokens_[3]);  // dep
    VectorUtils::UniqueVectorAdd(_shortcodes_affecting_this_indicator_, (std::string)r_tokens_[4]);  // indep1
    VectorUtils::UniqueVectorAdd(_shortcodes_affecting_this_indicator_, (std::string)r_tokens_[5]);  // indep2
  }
}

DI1CurveAdjustedPrice* DI1CurveAdjustedPrice::GetUniqueInstance(DebugLogger& t_dbglogger_, const Watch& r_watch_,
                                                                const std::vector<const char*>& r_tokens_,
                                                                PriceType_t _base_price_type_) {
  if (r_tokens_.size() < 7u) {
    ExitVerbose(kExitErrorCodeGeneral, "DI1CurveAdjustedPrice needs 8 tokens");
    return NULL;
  }

  // INDICATOR 1.00 DI1CurveAdjustedPrice BR_DI1N_0 BR_DI_0 BR_DI_1 PRICE_TYPE HALFLIFE
  ShortcodeSecurityMarketViewMap::StaticCheckValid(r_tokens_[3]);  // dep
  ShortcodeSecurityMarketViewMap::StaticCheckValid(r_tokens_[4]);  // indep1
  ShortcodeSecurityMarketViewMap::StaticCheckValid(r_tokens_[5]);  // indep2

  return GetUniqueInstance(t_dbglogger_, r_watch_,
                           *(ShortcodeSecurityMarketViewMap::StaticGetSecurityMarketView(
                               r_tokens_[3])),  // dep is not necessary to hold for now but just in case...
                           *(ShortcodeSecurityMarketViewMap::StaticGetSecurityMarketView(r_tokens_[4])),
                           *(ShortcodeSecurityMarketViewMap::StaticGetSecurityMarketView(r_tokens_[5])),
                           StringToPriceType_t(r_tokens_[6]));
}

DI1CurveAdjustedPrice* DI1CurveAdjustedPrice::GetUniqueInstance(DebugLogger& t_dbglogger_, const Watch& r_watch_,
                                                                const SecurityMarketView& _dep_market_view_,
                                                                const SecurityMarketView& _indep1_market_view_,
                                                                const SecurityMarketView& _indep2_market_view_,
                                                                PriceType_t t_price_type_) {
  std::ostringstream t_temp_oss_;

  t_temp_oss_ << VarName() << ' ' << _dep_market_view_.secname() << ' ' << _indep1_market_view_.secname() << ' '
              << _indep2_market_view_.secname() << ' ' << ' ' << t_price_type_;
  std::string concise_indicator_description_(t_temp_oss_.str());

  static std::map<std::string, DI1CurveAdjustedPrice*> concise_indicator_description_map_;
  if (concise_indicator_description_map_.find(concise_indicator_description_) ==
      concise_indicator_description_map_.end()) {
    concise_indicator_description_map_[concise_indicator_description_] =
        new DI1CurveAdjustedPrice(t_dbglogger_, r_watch_, concise_indicator_description_, _dep_market_view_,
                                  _indep1_market_view_, _indep2_market_view_, t_price_type_);
  }
  return concise_indicator_description_map_[concise_indicator_description_];
}

DI1CurveAdjustedPrice::DI1CurveAdjustedPrice(DebugLogger& t_dbglogger_, const Watch& r_watch_,
                                             const std::string& concise_indicator_description_,
                                             const SecurityMarketView& _dep_market_view_,
                                             const SecurityMarketView& _indep1_market_view_,
                                             const SecurityMarketView& _indep2_market_view_, PriceType_t _price_type_)
    : CommonIndicator(t_dbglogger_, r_watch_, concise_indicator_description_),
      dep_market_view_(_dep_market_view_),
      indep1_market_view_(_indep1_market_view_),
      indep2_market_view_(_indep2_market_view_),
      price_type_t(_price_type_),
      current_dep_price_(0),
      current_indep1_price_(0),  // just so we dont declare these everytime
      current_indep2_price_(0),  // just so we dont declare these everytime
      dep_term_(CurveUtils::_get_term_(r_watch_.YYYYMMDD(), dep_market_view_.secname())),
      indep1_term_(CurveUtils::_get_term_(r_watch_.YYYYMMDD(), indep1_market_view_.secname())),
      indep2_term_(CurveUtils::_get_term_(r_watch_.YYYYMMDD(), indep2_market_view_.secname())),
      dep_interrupted_(false),
      indep1_interrupted_(false),
      indep2_interrupted_(false)

{
  alpha1_ = (indep2_term_ - dep_term_) / (indep2_term_ - indep1_term_) * (indep1_term_ / dep_term_);
  alpha2_ = -(indep1_term_ - dep_term_) / (indep2_term_ - indep1_term_) * (indep2_term_ / dep_term_);

  //    alpha1_ = ( indep2_term_ - dep_term_ )  / ( indep2_term_ - indep1_term_ ) ;
  //    alpha2_ = - ( indep1_term_ - dep_term_ ) / ( indep2_term_ - indep1_term_ ) ;

  if ((!indep1_market_view_.subscribe_price_type(this, price_type_t)) ||
      (!indep2_market_view_.subscribe_price_type(this, price_type_t)) ||
      (!dep_market_view_.subscribe_price_type(this, price_type_t))) {
    PriceType_t t_error_price_type_ = price_type_t;
    std::cerr << typeid(*this).name() << ':' << __func__ << ':' << __LINE__ << ' ' << concise_indicator_description_
              << " passed " << t_error_price_type_ << std::endl;
  }
}

void DI1CurveAdjustedPrice::OnMarketUpdate(const unsigned int _security_id_,
                                           const MarketUpdateInfo& _market_update_info_) {
  if (indep1_market_view_.security_id() == _security_id_) {
    current_indep1_price_ = SecurityMarketView::GetPriceFromType(price_type_t, _market_update_info_);
  } else if (indep2_market_view_.security_id() == _security_id_) {
    current_indep2_price_ = SecurityMarketView::GetPriceFromType(price_type_t, _market_update_info_);
  } else if (dep_market_view_.security_id() == _security_id_) {
    current_dep_price_ = SecurityMarketView::GetPriceFromType(price_type_t, _market_update_info_);
  }

  if (!is_ready_) {
    if (indep1_market_view_.is_ready_complex(2) && indep2_market_view_.is_ready_complex(2) &&
        dep_market_view_.is_ready_complex(2)) {
      is_ready_ = true;
    }
  } else if (!data_interrupted_) {
    current_predicted_price_ = (alpha1_ * current_indep1_price_ + alpha2_ * current_indep2_price_);
    indicator_value_ = current_predicted_price_ - current_dep_price_;
    NotifyIndicatorListeners(indicator_value_);
  }
}

// market_interrupt_listener interface
void DI1CurveAdjustedPrice::OnMarketDataInterrupted(const unsigned int _security_id_,
                                                    const int msecs_since_last_receive_) {
  if (indep1_market_view_.security_id() == _security_id_) {
    indep1_interrupted_ = true;
  } else if (indep2_market_view_.security_id() == _security_id_) {
    indep2_interrupted_ = true;
  } else if (dep_market_view_.security_id() == _security_id_) {
    dep_interrupted_ = true;
  } else {
    return;
  }

  data_interrupted_ = true;
  InitializeValues();
  NotifyIndicatorListeners(indicator_value_);
}

void DI1CurveAdjustedPrice::OnMarketDataResumed(const unsigned int _security_id_) {
  if (indep1_market_view_.security_id() == _security_id_) {
    indep1_interrupted_ = false;
  } else if (indep2_market_view_.security_id() == _security_id_) {
    indep2_interrupted_ = false;
  } else if (dep_market_view_.security_id() == _security_id_) {
    dep_interrupted_ = false;
  } else {
    return;
  }

  if ((!indep1_interrupted_) && (!indep2_interrupted_) && (!dep_interrupted_)) {
    InitializeValues();
    data_interrupted_ = false;
  }
}

void DI1CurveAdjustedPrice::InitializeValues() {
  current_dep_price_ = 0;
  current_indep1_price_ = 0;
  current_indep2_price_ = 0;
  indicator_value_ = 0;
}
}
