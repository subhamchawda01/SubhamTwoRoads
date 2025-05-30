/**
    \file IndicatorsCode/contract_term.cpp

    \author: (c) Copyright Two Roads Technological Solutions Pvt Ltd 2011
     Address:
         Suite 217, Level 2, Prestige Omega,
         No 104, EPIP Zone, Whitefield,
         Bangalore - 560066, India
         +91 80 4060 0717
*/
#include "dvccode/CDef/math_utils.hpp"

#include "dvctrade/Indicators/indicator_util.hpp"
#include "dvctrade/Indicators/contract_term.hpp"

namespace HFSAT {

void ContractTerm::CollectShortCodes(std::vector<std::string>& _shortcodes_affecting_this_indicator_,
                             std::vector<std::string>& _ors_source_needed_vec_,
                             const std::vector<const char*>& r_tokens_) {
  VectorUtils::UniqueVectorAdd(_shortcodes_affecting_this_indicator_, (std::string)r_tokens_[3]);
}

ContractTerm* ContractTerm::GetUniqueInstance(DebugLogger& t_dbglogger_, const Watch& r_watch_,
                              const std::vector<const char*>& r_tokens_, PriceType_t _basepx_pxtype_) {
  // INDICATOR _this_weight_ _indicator_string_ _sec_market_view_ _price_type_
  if (r_tokens_.size() < 4) {
    ExitVerbose(kModelCreationIndicatorLineLessArgs, t_dbglogger_, "INDICATOR weight ContractTerm _sec_market_view_ ");
  }
  ShortcodeSecurityMarketViewMap::StaticCheckValid(r_tokens_[3]);
  return GetUniqueInstance(t_dbglogger_, r_watch_,
                           *(ShortcodeSecurityMarketViewMap::StaticGetSecurityMarketView(r_tokens_[3])));
}

ContractTerm* ContractTerm::GetUniqueInstance(DebugLogger& t_dbglogger_, const Watch& r_watch_,
                              const SecurityMarketView& _sec_market_view_) {
  std::ostringstream t_temp_oss_;
  t_temp_oss_ << VarName() << ' ' << _sec_market_view_.secname() ;
  std::string concise_indicator_description_(t_temp_oss_.str());

  static std::map<std::string, ContractTerm*> concise_indicator_description_map_;
  if (concise_indicator_description_map_.find(concise_indicator_description_) ==
      concise_indicator_description_map_.end()) {
    concise_indicator_description_map_[concise_indicator_description_] =
        new ContractTerm(t_dbglogger_, r_watch_, concise_indicator_description_, _sec_market_view_);
  }
  return concise_indicator_description_map_[concise_indicator_description_];
}

ContractTerm::ContractTerm(DebugLogger& t_dbglogger_, const Watch& r_watch_, const std::string& concise_indicator_description_,
           const SecurityMarketView& _sec_market_view_)
    : CommonIndicator(t_dbglogger_, r_watch_, concise_indicator_description_),
      sec_market_view_(_sec_market_view_){
  if (!sec_market_view_.subscribe_price_type(this, kPriceTypeMktSizeWPrice)) {
    PriceType_t t_error_price_type_ = kPriceTypeMktSizeWPrice;
    std::cerr << typeid(*this).name() << ':' << __func__ << ':' << __LINE__ << ' ' << concise_indicator_description_
              << " passed " << t_error_price_type_ << std::endl;
  }
  term_ = HFSAT::CurveUtils::_get_term_(r_watch_.YYYYMMDD(), sec_market_view_.secname());

}

void ContractTerm::WhyNotReady() {
  if (!is_ready_) {
    if (!(sec_market_view_.is_ready_complex(2))) {
      DBGLOG_TIME_CLASS << sec_market_view_.secname() << " is_ready_complex = false " << DBGLOG_ENDL_FLUSH;
      DBGLOG_DUMP;
    }
  }
}

void ContractTerm::OnMarketUpdate(const unsigned int _security_id_, const MarketUpdateInfo& _market_update_info_) {
  if (!is_ready_) {
    if (sec_market_view_.is_ready_complex(2)) {
      is_ready_ = true;
      InitializeValues();
    }
  } else if (!data_interrupted_) {
    indicator_value_ = term_;
    NotifyIndicatorListeners(indicator_value_);
  }
}

void ContractTerm::InitializeValues() { indicator_value_ = term_; }

// market_interrupt_listener interface
void ContractTerm::OnMarketDataInterrupted(const unsigned int _security_id_, const int msecs_since_last_receive_) {
  if (sec_market_view_.security_id() == _security_id_) {
    data_interrupted_ = true;
    NotifyIndicatorListeners(indicator_value_);
  }
}

void ContractTerm::OnMarketDataResumed(const unsigned int _security_id_) {
  if (data_interrupted_) {
    if (sec_market_view_.security_id() == _security_id_) {
      InitializeValues();
      data_interrupted_ = false;
    }
  }
}
}
