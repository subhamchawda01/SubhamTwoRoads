/**
    \file IndicatorsCode/implied_vol_spread.cpp

    \author: (c) Copyright Two Roads Technological Solutions Pvt Ltd 2011
     Address:
         Suite 217, Level 2, Prestige Omega,
         No 104, EPIP Zone, Whitefield,
         Bangalore - 560066, India
         +91 80 4060 0717
 */
#include "dvccode/CDef/math_utils.hpp"

#include "dvctrade/Indicators/indicator_util.hpp"
#include "dvctrade/Indicators/implied_vol_spread.hpp"

namespace HFSAT {

void ImpliedVolSpread::CollectShortCodes(std::vector<std::string>& _shortcodes_affecting_this_indicator_,
                                         std::vector<std::string>& _ors_source_needed_vec_,
                                         const std::vector<const char*>& r_tokens_) {
  std::string dep_shc_ = r_tokens_[3];

  if (!(NSESecurityDefinitions::IsOption(r_tokens_[3]))) {
    // SBIN Nth Step
    dep_shc_ = NSESecurityDefinitions::GetOptionShortcodeForUnderlyingInCurrSchemeFromContractNumber(
        r_tokens_[3], -1, atoi(r_tokens_[6]), atoi(r_tokens_[7]));
  }

  VectorUtils::UniqueVectorAdd(_shortcodes_affecting_this_indicator_, dep_shc_);

  // if ITM, we will use FUT0
  std::string indep_shc_ = NSESecurityDefinitions::GetPrevOptionInCurrentSchema(dep_shc_);

  // for NIFTY we observed 2 step work better than 1 step
  if (dep_shc_.compare(0, 9, "NSE_NIFTY") == 0) {
    indep_shc_ = NSESecurityDefinitions::GetPrevOptionInCurrentSchema(indep_shc_);
  }

  if (NSESecurityDefinitions::GetMoneynessFromShortCode(indep_shc_) > 0) {
    indep_shc_ = NSESecurityDefinitions::GetFutureShortcodeFromOptionShortCode(indep_shc_);
  }

  VectorUtils::UniqueVectorAdd(_shortcodes_affecting_this_indicator_, indep_shc_);
}

ImpliedVolSpread* ImpliedVolSpread::GetUniqueInstance(DebugLogger& t_dbglogger_, const Watch& r_watch_,
                                                      const std::vector<const char*>& r_tokens_,
                                                      PriceType_t _basepx_pxtype_) {
  // INDICATOR _this_weight_ _indicator_string_ underlying _fractional_seconds_ _price_type_ is_call_ num_steps_
  if (r_tokens_.size() < 6) {
    ExitVerbose(kModelCreationIndicatorLineLessArgs, t_dbglogger_,
                "INDICATOR weight ImpliedVolSpread underlying _fractional_seconds_ _price_type_");
  }

  // This indicator is written for OTM options
  std::string dep_shc_ = r_tokens_[3];
  if (!(NSESecurityDefinitions::IsOption(dep_shc_))) {
    if (r_tokens_.size() < 8) {
      ExitVerbose(
          kModelCreationIndicatorLineLessArgs, t_dbglogger_,
          "INDICATOR weight ImpliedVolSpread underlying _fractional_seconds_ _price_type_  _is_call_ _num_steps_ ");
    }
    dep_shc_ = NSESecurityDefinitions::GetOptionShortcodeForUnderlyingInCurrSchemeFromContractNumber(
        r_tokens_[3], -1, atoi(r_tokens_[6]), atoi(r_tokens_[7]));
    if (dep_shc_.empty()) {
      ExitVerbose(kExitErrorCodeGeneral, t_dbglogger_,
                  "No options Shortcode Exists :  ImpliedVolSpread underlying _fractional_seconds_ _price_type_  "
                  "_is_call_ _num_steps_ ");
    }
  }

  // ShortcodeSecurityMarketViewMap::StaticCheckValid(NSESecurityDefinitions::GetPrevOptionInCurrentSchema(dep_shc_));
  return GetUniqueInstance(t_dbglogger_, r_watch_,
                           *(ShortcodeSecurityMarketViewMap::StaticGetSecurityMarketView(dep_shc_)), atof(r_tokens_[4]),
                           StringToPriceType_t(r_tokens_[5]));
}

ImpliedVolSpread* ImpliedVolSpread::GetUniqueInstance(DebugLogger& t_dbglogger_, const Watch& r_watch_,
                                                      const SecurityMarketView& _dep_market_view_,
                                                      double _fractional_seconds_, PriceType_t _price_type_) {
  std::ostringstream t_temp_oss_;
  t_temp_oss_ << VarName() << ' ' << _dep_market_view_.secname() << ' ' << _fractional_seconds_ << ' '
              << PriceType_t_To_String(_price_type_);
  std::string concise_indicator_description_(t_temp_oss_.str());

  // store as basepointer ( since we dont use to referece but to desctroy
  // static std::map<std::string, ImpliedVolSpread*> concise_indicator_description_map_;
  if (global_concise_indicator_description_map_.find(concise_indicator_description_) ==
      global_concise_indicator_description_map_.end()) {
    global_concise_indicator_description_map_[concise_indicator_description_] = new ImpliedVolSpread(
        t_dbglogger_, r_watch_, concise_indicator_description_, _dep_market_view_, _fractional_seconds_, _price_type_);
  }
  return dynamic_cast<ImpliedVolSpread*>(global_concise_indicator_description_map_[concise_indicator_description_]);
}

ImpliedVolSpread::ImpliedVolSpread(DebugLogger& t_dbglogger_, const Watch& r_watch_,
                                   const std::string& concise_indicator_description_,
                                   const SecurityMarketView& _dep_market_view_, double _fractional_seconds_,
                                   PriceType_t _price_type_)
    : CommonIndicator(t_dbglogger_, r_watch_, concise_indicator_description_),
      dep_market_view_(_dep_market_view_),
      price_type_(_price_type_),
      moving_avg_dep_(0),
      last_dep_iv_(0),
      current_dep_iv_(0),
      moving_avg_indep_(0),
      last_indep_iv_(0),
      current_indep_iv_(0),
      stdev_dep_(-1),
      stdev_indep_(-1),
      dep_interrupted_(false),
      indep_interrupted_(false) {
  trend_history_msecs_ = std::max(20, (int)round(1000 * _fractional_seconds_));
  last_new_page_msecs_ = 0;
  page_width_msecs_ = 500;
  decay_page_factor_ = 0.95;
  inv_decay_sum_ = 0.05;
  SetTimeDecayWeights();

  bool is_option_ = true;

  std::string indep_shc_ = NSESecurityDefinitions::GetPrevOptionInCurrentSchema(dep_market_view_.shortcode());
  if (indep_shc_.compare(0, 9, "NSE_NIFTY") == 0) {
    indep_shc_ = NSESecurityDefinitions::GetPrevOptionInCurrentSchema(indep_shc_);
  }

  if (NSESecurityDefinitions::GetMoneynessFromShortCode(indep_shc_) > 0) {
    indep_shc_ = NSESecurityDefinitions::GetFutureShortcodeFromOptionShortCode(indep_shc_);
    is_option_ = false;
  }

  indep_market_view_ = ShortcodeSecurityMarketViewMap::StaticGetSecurityMarketView(indep_shc_);

  if (!dep_market_view_.subscribe_price_type(this, kPriceTypeImpliedVol)) {
    PriceType_t t_error_price_type_ = _price_type_;
    std::cerr << typeid(*this).name() << ':' << __func__ << ':' << __LINE__ << ' ' << concise_indicator_description_
              << " passed " << t_error_price_type_ << std::endl;
  }

  // for now we are using a hack, i.e using mid_price if futures else implied_vol
  if (is_option_ && !indep_market_view_->subscribe_price_type(this, kPriceTypeImpliedVol)) {
    PriceType_t t_error_price_type_ = kPriceTypeImpliedVol;
    std::cerr << typeid(*this).name() << ':' << __func__ << ':' << __LINE__ << ' ' << concise_indicator_description_
              << " passed " << t_error_price_type_ << std::endl;
  } else if (!indep_market_view_->subscribe_price_type(this, kPriceTypeMidprice)) {
    PriceType_t t_error_price_type_ = kPriceTypeMidprice;
    std::cerr << typeid(*this).name() << ':' << __func__ << ':' << __LINE__ << ' ' << concise_indicator_description_
              << " passed " << t_error_price_type_ << std::endl;
  }

  // for now lets use price stdev ( should be iv stdev )
  SlowStdevCalculator* std_1 =
      SlowStdevCalculator::GetUniqueInstance(t_dbglogger_, r_watch_, dep_market_view_.shortcode());
  if (std_1 != NULL) {
    std_1->AddSlowStdevCalculatorListener(this);
  }

  SlowStdevCalculator* std_2 =
      SlowStdevCalculator::GetUniqueInstance(t_dbglogger_, r_watch_, indep_market_view_->shortcode());
  if (std_2 != NULL) {
    std_2->AddSlowStdevCalculatorListener(this);
  }
}

void ImpliedVolSpread::OnStdevUpdate(const unsigned int _security_id_, const double& _new_stdev_value_) {
  if (_security_id_ == dep_market_view_.security_id()) {
    stdev_dep_ = _new_stdev_value_;
  } else {
    stdev_indep_ = _new_stdev_value_;
  }
}

void ImpliedVolSpread::WhyNotReady() {
  if (!is_ready_) {
    if (!(indep_market_view_->is_ready_complex2(2))) {
      DBGLOG_TIME_CLASS << indep_market_view_->secname() << " is_ready_complex2 = false " << DBGLOG_ENDL_FLUSH;
      DBGLOG_DUMP;
    }
  }
}

void ImpliedVolSpread::OnMarketUpdate(const unsigned int _security_id_, const MarketUpdateInfo& _market_update_info_) {
  if (_security_id_ == dep_market_view_.security_id()) {
    current_dep_iv_ = SecurityMarketView::GetPriceFromType(kPriceTypeImpliedVol, _market_update_info_);
  } else {
    current_indep_iv_ = SecurityMarketView::GetPriceFromType(kPriceTypeImpliedVol, _market_update_info_);
  }

  if (!is_ready_) {
    if (indep_market_view_->is_ready_complex2(2) && dep_market_view_.is_ready_complex2(2)) {
      is_ready_ = true;
      InitializeValues();
    }
  } else if (!data_interrupted_) {
    if (watch_.msecs_from_midnight() - last_new_page_msecs_ < page_width_msecs_) {
      moving_avg_dep_ += inv_decay_sum_ * (current_dep_iv_ - last_dep_iv_);
      moving_avg_indep_ += inv_decay_sum_ * (current_indep_iv_ - last_indep_iv_);
    } else {
      int num_pages_to_add_ = (int)floor((watch_.msecs_from_midnight() - last_new_page_msecs_) / page_width_msecs_);
      if (num_pages_to_add_ >= (int)decay_vector_.size()) {
        InitializeValues();
      } else {
        if (num_pages_to_add_ == 1) {
          moving_avg_dep_ = (current_dep_iv_ * inv_decay_sum_) + (moving_avg_dep_ * decay_vector_[1]);
          moving_avg_indep_ = (current_indep_iv_ * inv_decay_sum_) + (moving_avg_indep_ * decay_vector_[1]);
        } else {  // num_pages_to_add_ >= 2 < decay_vector_.size ( )
          moving_avg_dep_ = (current_dep_iv_ * inv_decay_sum_) +
                            (last_dep_iv_ * inv_decay_sum_ * decay_vector_sums_[(num_pages_to_add_ - 1)]) +
                            (moving_avg_dep_ * decay_vector_[num_pages_to_add_]);
          moving_avg_indep_ = (current_indep_iv_ * inv_decay_sum_) +
                              (last_indep_iv_ * inv_decay_sum_ * decay_vector_sums_[(num_pages_to_add_ - 1)]) +
                              (moving_avg_indep_ * decay_vector_[num_pages_to_add_]);
        }
        last_new_page_msecs_ += (num_pages_to_add_ * page_width_msecs_);
      }
    }

    last_dep_iv_ = current_dep_iv_;
    last_indep_iv_ = current_indep_iv_;

    indicator_value_ =
        (current_indep_iv_ - moving_avg_indep_) / stdev_indep_ - (current_dep_iv_ - moving_avg_dep_) / stdev_dep_;

    NotifyIndicatorListeners(indicator_value_);
  }
}

void ImpliedVolSpread::InitializeValues() {
  moving_avg_dep_ = current_dep_iv_;
  moving_avg_indep_ = current_indep_iv_;

  last_indep_iv_ = current_indep_iv_;
  last_dep_iv_ = current_dep_iv_;

  last_new_page_msecs_ = watch_.msecs_from_midnight() - watch_.msecs_from_midnight() % page_width_msecs_;
  indicator_value_ = 0;
}

// market_interrupt_listener interface
void ImpliedVolSpread::OnMarketDataInterrupted(const unsigned int _security_id_, const int msecs_since_last_receive_) {
  if (indep_market_view_->security_id() == _security_id_) {
    indep_interrupted_ = true;
  } else if (dep_market_view_.security_id() == _security_id_) {
    dep_interrupted_ = true;
  }
  if (indep_interrupted_ || dep_interrupted_) {
    data_interrupted_ = true;
    indicator_value_ = 0;
    NotifyIndicatorListeners(indicator_value_);
  }
}

void ImpliedVolSpread::OnMarketDataResumed(const unsigned int _security_id_) {
  if (data_interrupted_) {
    if (indep_market_view_->security_id() == _security_id_) {
      indep_interrupted_ = false;
    } else if (dep_market_view_.security_id() == _security_id_) {
      dep_interrupted_ = false;
    }
    if (!(dep_interrupted_ || indep_interrupted_)) {
      InitializeValues();
      data_interrupted_ = false;
    }
  }
}
}
