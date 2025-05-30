/**
    \file IndicatorsCode/diff_price_ebs.cpp

    \author: (c) Copyright Two Roads Technological Solutions Pvt Ltd 2011
     Address:
   Suite No 353, Evoma, #14, Bhattarhalli,
   Old Madras Road, Near Garden City College,
   KR Puram, Bangalore 560049, India
   +91 80 4190 3551
 */
#include "dvccode/CDef/math_utils.hpp"
#include "dvctrade/Indicators/indicator_util.hpp"
#include "dvctrade/Indicators/diff_price_ebs.hpp"

namespace HFSAT {

void DiffPriceEBS::CollectShortCodes(std::vector<std::string>& _shortcodes_affecting_this_indicator_,
                                     std::vector<std::string>& _ors_source_needed_vec_,
                                     const std::vector<const char*>& r_tokens_) {
  VectorUtils::UniqueVectorAdd(_shortcodes_affecting_this_indicator_, (std::string)r_tokens_[3]);
}

DiffPriceEBS* DiffPriceEBS::GetUniqueInstance(DebugLogger& t_dbglogger_, const Watch& r_watch_,
                                              const std::vector<const char*>& r_tokens_, PriceType_t _basepx_pxtype_) {
  ShortcodeSecurityMarketViewMap::StaticCheckValid(std::string(r_tokens_[3]));
  PriceType_t t_price_type_ = kPriceTypeMax;
  if (r_tokens_.size() < 6) {
    ExitVerbose(kModelCreationIndicatorLineLessArgs, t_dbglogger_,
                " DiffpriceEBS Incorrect Syntax. Correct syntax would b INDICATOR _this_weight_ _indicator_string_ "
                "dep_shc_ _fractional_sec_ _price_type_");
  } else {
    t_price_type_ = StringToPriceType_t(r_tokens_[5]);
  }

  return GetUniqueInstance(
      t_dbglogger_, r_watch_, *(ShortcodeSecurityMarketViewMap::StaticGetSecurityMarketView(std::string(r_tokens_[3]))),
      *(ShortcodeSecurityMarketViewMap::StaticGetSecurityMarketView(std::string(r_tokens_[4]))), t_price_type_);
}

DiffPriceEBS* DiffPriceEBS::GetUniqueInstance(DebugLogger& t_dbglogger_, const Watch& r_watch_,
                                              SecurityMarketView& t_indep1_market_view_,
                                              SecurityMarketView& t_indep2_market_view_, PriceType_t _price_type_) {
  std::ostringstream t_temp_oss_;
  t_temp_oss_ << VarName() << ' ' << t_indep1_market_view_.secname() << ' ' << t_indep2_market_view_.secname() << ' '
              << PriceType_t_To_String(_price_type_);
  std::string concise_indicator_description_(t_temp_oss_.str());

  static std::map<std::string, DiffPriceEBS*> concise_indicator_description_map_;

  if (concise_indicator_description_map_.find(concise_indicator_description_) ==
      concise_indicator_description_map_.end()) {
    concise_indicator_description_map_[concise_indicator_description_] =
        new DiffPriceEBS(t_dbglogger_, r_watch_, concise_indicator_description_, t_indep1_market_view_,
                         t_indep2_market_view_, _price_type_);
  }
  return concise_indicator_description_map_[concise_indicator_description_];
}

DiffPriceEBS::DiffPriceEBS(DebugLogger& t_dbglogger_, const Watch& r_watch_,
                           const std::string& concise_indicator_description_, SecurityMarketView& t_indep1_market_view_,
                           SecurityMarketView& t_indep2_market_view_, PriceType_t _price_type_)
    : CommonIndicator(t_dbglogger_, r_watch_, concise_indicator_description_),
      indep1_market_view_(t_indep1_market_view_),
      indep2_market_view_(t_indep2_market_view_),
      last_ebs_update_msecs_(0),
      price_type_(_price_type_),
      indep1_data_interrupted_(false),
      indep2_data_interrupted_(false),
      decay_factor_(1.0),
      ebs_security_id_(100000),
      current_indep1_price_(0.0),
      current_indep2_price_(0.0) {
  // since we using mktsizewprice in below indicator value computation
  if (!indep1_market_view_.subscribe_price_type(this, _price_type_)) {
    PriceType_t t_error_price_type_ = _price_type_;
    std::cerr << typeid(*this).name() << ':' << __func__ << ':' << __LINE__ << ' ' << " passed " << t_error_price_type_
              << std::endl;
  }
  if (!indep2_market_view_.subscribe_price_type(this, _price_type_)) {
    PriceType_t t_error_price_type_ = _price_type_;
    std::cerr << typeid(*this).name() << ':' << __func__ << ':' << __LINE__ << ' ' << " passed " << t_error_price_type_
              << std::endl;
  }

  if (indep1_market_view_.this_smv_exch_source_ == kExchSourceEBS) {
    ebs_security_id_ = indep1_market_view_.security_id();
  } else if (indep2_market_view_.this_smv_exch_source_ == kExchSourceEBS) {
    ebs_security_id_ = indep2_market_view_.security_id();
  } else {
    ExitVerbose(kExitErrorCodeGeneral, t_dbglogger_, " DiffpriceEBS requires atleast one security to be from ebs");
  }
}

void DiffPriceEBS::OnTradePrint(const unsigned int _security_id_, const TradePrintInfo& _trade_print_info_,
                                const MarketUpdateInfo& _market_update_info_) {
  OnMarketUpdate(_security_id_, _market_update_info_);
}

void DiffPriceEBS::OnMarketUpdate(const unsigned int _security_id_, const MarketUpdateInfo& _market_update_info_) {
  if (!is_ready_) {
    if (indep1_market_view_.is_ready_complex(2) && indep2_market_view_.is_ready_complex(2)) {
      is_ready_ = true;
      indicator_value_ = 0;
      NotifyIndicatorListeners(indicator_value_);
    }
  } else if (!data_interrupted_) {
    if (_security_id_ == ebs_security_id_) {
      last_ebs_update_msecs_ = watch_.msecs_from_midnight();
    }

    if (indep1_market_view_.security_id() == _security_id_) {
      current_indep1_price_ = SecurityMarketView::GetPriceFromType(price_type_, _market_update_info_);
    } else {
      current_indep2_price_ = SecurityMarketView::GetPriceFromType(price_type_, _market_update_info_);
    }

    int msecs_diff_ = watch_.msecs_from_midnight() - last_ebs_update_msecs_;

    if (msecs_diff_ < DECAY_START_MSECS) {
      decay_factor_ = 1;
    } else {
      decay_factor_ = 1 / (1 + (msecs_diff_ - DECAY_START_MSECS) * 0.4);
    }

    indicator_value_ = (current_indep2_price_ - current_indep1_price_) * decay_factor_;
  }
}

void DiffPriceEBS::OnMarketDataInterrupted(const unsigned int _security_id_, const int msecs_since_last_receive_) {
  if (indep1_market_view_.security_id() == _security_id_) {
    indep1_data_interrupted_ = true;
    data_interrupted_ = true;
    indicator_value_ = 0;
    NotifyIndicatorListeners(indicator_value_);
  } else if (indep2_market_view_.security_id() == _security_id_) {
    indep2_data_interrupted_ = true;
    data_interrupted_ = true;
    indicator_value_ = 0;
    NotifyIndicatorListeners(indicator_value_);
  }
}

void DiffPriceEBS::OnMarketDataResumed(const unsigned int _security_id_) {
  if (indep1_market_view_.security_id() == _security_id_) {
    indep1_data_interrupted_ = false;
  } else if (indep2_market_view_.security_id() == _security_id_) {
    indep2_data_interrupted_ = false;
  }

  if (!indep1_data_interrupted_ && !indep2_data_interrupted_) {
    data_interrupted_ = true;
  }
}
}
