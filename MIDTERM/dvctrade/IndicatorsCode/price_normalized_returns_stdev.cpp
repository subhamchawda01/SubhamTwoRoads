/**
    \file IndicatorsCode/price_normalized_returns_stdev.cpp

    \author: (c) Copyright Two Roads Technological Solutions Pvt Ltd 2011
     Address:
         Suite No 353, Evoma, #14, Bhattarhalli,
         Old Madras Road, Near Garden City College,
         KR Puram, Bangalore 560049, India
         +91 80 4190 3551
*/
// INDICATOR 1.00 PriceNormalizedReturnsStdev VX_0 ES_0 100 100 OfflineMixMMS
//#include "dvccode/CDef/math_utils.hpp"
#include "dvctrade/Indicators/indicator_util.hpp"
#include "dvctrade/Indicators/price_normalized_returns_stdev.hpp"

namespace HFSAT {

// INDICATOR 1.00 INDICATORNAME DEP INDEP WINDOW PRICETYPE
void PriceNormalizedReturnsStdev::CollectShortCodes(std::vector<std::string>& _shortcodes_affecting_this_indicator_,
                                                    std::vector<std::string>& _ors_source_needed_vec_,
                                                    const std::vector<const char*>& r_tokens_) {
  if (r_tokens_.size() >= 8u)  // doesnt mean indicator string syntax is valid .
  {
    VectorUtils::UniqueVectorAdd(_shortcodes_affecting_this_indicator_, (std::string)r_tokens_[3]);
    VectorUtils::UniqueVectorAdd(_shortcodes_affecting_this_indicator_, (std::string)r_tokens_[4]);
  }
}

PriceNormalizedReturnsStdev* PriceNormalizedReturnsStdev::GetUniqueInstance(DebugLogger& t_dbglogger_,
                                                                            const Watch& r_watch_,
                                                                            const std::vector<const char*>& r_tokens_,
                                                                            PriceType_t _base_price_type_) {
  if (r_tokens_.size() < 7u) {
    ExitVerbose(kExitErrorCodeGeneral, "PriceNormalizedReturnsStdev needs 7 tokens");
    return NULL;
  }
  ShortcodeSecurityMarketViewMap::StaticCheckValid(r_tokens_[3]);
  ShortcodeSecurityMarketViewMap::StaticCheckValid(r_tokens_[4]);

  return GetUniqueInstance(t_dbglogger_, r_watch_,
                           *(ShortcodeSecurityMarketViewMap::StaticGetSecurityMarketView(r_tokens_[3])),
                           *(ShortcodeSecurityMarketViewMap::StaticGetSecurityMarketView(r_tokens_[4])),
                           atof(r_tokens_[5]), StringToPriceType_t(r_tokens_[6]));
}

PriceNormalizedReturnsStdev* PriceNormalizedReturnsStdev::GetUniqueInstance(
    DebugLogger& t_dbglogger_, const Watch& r_watch_, const SecurityMarketView& _dep_smv_,
    const SecurityMarketView& _indep_smv_, double _returns_window_, PriceType_t t_price_type_) {
  std::ostringstream t_temp_oss_;

  t_temp_oss_ << VarName() << ' ' << _dep_smv_.secname() << ' ' << _indep_smv_.secname() << ' ' << _returns_window_
              << ' ' << t_price_type_;

  std::string concise_indicator_description_(t_temp_oss_.str());
  static std::map<std::string, PriceNormalizedReturnsStdev*> concise_indicator_description_map_;
  if (concise_indicator_description_map_.find(concise_indicator_description_) ==
      concise_indicator_description_map_.end()) {
    concise_indicator_description_map_[concise_indicator_description_] =
        new PriceNormalizedReturnsStdev(t_dbglogger_, r_watch_, concise_indicator_description_, _dep_smv_, _indep_smv_,
                                        _returns_window_, t_price_type_);
  }

  return concise_indicator_description_map_[concise_indicator_description_];
}

PriceNormalizedReturnsStdev::PriceNormalizedReturnsStdev(DebugLogger& t_dbglogger_, const Watch& r_watch_,
                                                         const std::string& concise_indicator_description_,
                                                         const SecurityMarketView& _dep_smv_,
                                                         const SecurityMarketView& _indep_smv_, double _returns_window_,
                                                         PriceType_t _price_type_)
    : IndicatorListener(),
      CommonIndicator(t_dbglogger_, r_watch_, concise_indicator_description_),
      dep_smv_(_dep_smv_),
      dep_price_(0.0),
      price_type_(_price_type_) {
  if (!dep_smv_.subscribe_price_type(this, price_type_)) {
    PriceType_t t_error_price_type_ = price_type_;
    std::cerr << typeid(*this).name() << ':' << __func__ << ':' << __LINE__ << ' ' << concise_indicator_description_
              << " passed " << t_error_price_type_ << std::endl;
  }

  indep_returns_stdev_ =
      ReturnsStdev::GetUniqueInstance(t_dbglogger_, r_watch_, _indep_smv_, _returns_window_, _price_type_);
  indep_returns_stdev_->add_unweighted_indicator_listener(1u, this);
}

void PriceNormalizedReturnsStdev::OnMarketUpdate(const unsigned int _security_id_,
                                                 const MarketUpdateInfo& _market_update_info_) {
  dep_price_ = SecurityMarketView::GetPriceFromType(price_type_, _market_update_info_);
}

void PriceNormalizedReturnsStdev::OnIndicatorUpdate(const unsigned int& indicator_index_, const double& new_value_) {
  if (!is_ready_ && dep_smv_.is_ready_complex(2)) {
    is_ready_ = true;
  } else if (!data_interrupted_ && dep_price_ > 0.0) {
    indicator_value_ = new_value_ / dep_price_;
    NotifyIndicatorListeners(indicator_value_);
  }
}

void PriceNormalizedReturnsStdev::OnMarketDataInterrupted(const unsigned int _security_id_,
                                                          const int msecs_since_last_receive_) {
  data_interrupted_ = true;
  indicator_value_ = 0;
  NotifyIndicatorListeners(indicator_value_);
}

void PriceNormalizedReturnsStdev::OnMarketDataResumed(const unsigned int _security_id_) {
  InitializeValues();
  data_interrupted_ = false;
}

void PriceNormalizedReturnsStdev::InitializeValues() { indicator_value_ = 0; }
}
