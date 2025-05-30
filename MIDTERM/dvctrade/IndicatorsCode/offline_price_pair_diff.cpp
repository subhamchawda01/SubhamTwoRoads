/**
    \file IndicatorsCode/offline_price_pair_diff.cpp

    \author: (c) Copyright Two Roads Technological Solutions Pvt Ltd 2011
     Address:
         Suite 217, Level 2, Prestige Omega,
         No 104, EPIP Zone, Whitefield,
         Bangalore - 560066, India
         +91 80 4060 0717
 */
#include "dvctrade/Indicators/offline_price_pair_diff.hpp"

namespace HFSAT {

void OfflinePricePairDiff::CollectShortCodes(std::vector<std::string>& _shortcodes_affecting_this_indicator_,
                                             std::vector<std::string>& _ors_source_needed_vec_,
                                             const std::vector<const char*>& r_tokens_) {
  VectorUtils::UniqueVectorAdd(_shortcodes_affecting_this_indicator_, (std::string)r_tokens_[3]);
  VectorUtils::UniqueVectorAdd(_shortcodes_affecting_this_indicator_, (std::string)r_tokens_[4]);
}

OfflinePricePairDiff* OfflinePricePairDiff::GetUniqueInstance(DebugLogger& t_dbglogger_, const Watch& r_watch_,
                                                              const std::vector<const char*>& r_tokens_,
                                                              PriceType_t _basepx_pxtype_) {
  // INDICATOR _this_weight_ _indicator_string_ _dep_market_view_ _indep_market_view_vec_ _alpha_ _beta_ _price_type_
  if (r_tokens_.size() < 8) {
    ExitVerbose(
        kModelCreationIndicatorLineLessArgs, t_dbglogger_,
        "INDICATOR weight OfflinePricePairDiff _dep_market_view_ _indep_market_view_ _alpha_ _beta_ _price_type_ ");
  }
  return GetUniqueInstance(t_dbglogger_, r_watch_,
                           *(ShortcodeSecurityMarketViewMap::StaticGetSecurityMarketView(r_tokens_[3])),
                           *(ShortcodeSecurityMarketViewMap::StaticGetSecurityMarketView(r_tokens_[4])),
                           atof(r_tokens_[5]), atof(r_tokens_[6]), StringToPriceType_t(r_tokens_[7]));
}

OfflinePricePairDiff* OfflinePricePairDiff::GetUniqueInstance(DebugLogger& t_dbglogger_, const Watch& r_watch_,
                                                              const SecurityMarketView& _dep_market_view_,
                                                              const SecurityMarketView& _indep_market_view_,
                                                              double _alpha_, double _beta_, PriceType_t _price_type_) {
  std::ostringstream t_temp_oss_;
  t_temp_oss_ << VarName() << ' ' << _dep_market_view_.secname() << ' ' << _indep_market_view_.secname() << ' '
              << _alpha_ << ' ' << _beta_ << ' ' << PriceType_t_To_String(_price_type_);
  std::string concise_indicator_description_(t_temp_oss_.str());

  static std::map<std::string, OfflinePricePairDiff*> concise_indicator_description_map_;
  if (concise_indicator_description_map_.find(concise_indicator_description_) ==
      concise_indicator_description_map_.end()) {
    concise_indicator_description_map_[concise_indicator_description_] =
        new OfflinePricePairDiff(t_dbglogger_, r_watch_, concise_indicator_description_, _dep_market_view_,
                                 _indep_market_view_, _alpha_, _beta_, _price_type_);
  }
  return concise_indicator_description_map_[concise_indicator_description_];
}

OfflinePricePairDiff::OfflinePricePairDiff(DebugLogger& t_dbglogger_, const Watch& r_watch_,
                                           const std::string& concise_indicator_description_,
                                           const SecurityMarketView& _dep_market_view_,
                                           const SecurityMarketView& _indep_market_view_, double _alpha_, double _beta_,
                                           PriceType_t _price_type_)
    : CommonIndicator(t_dbglogger_, r_watch_, concise_indicator_description_),
      dep_market_view_(_dep_market_view_),
      indep_market_view_(_indep_market_view_),
      alpha_(_alpha_),
      beta_(_beta_),
      current_dep_price_(0.0),
      current_indep_price_(0.0),
      dep_interrupted_(false),
      indep_interrupted_(false),
      price_type_(_price_type_) {
  if (!indep_market_view_.subscribe_price_type(this, price_type_)) {
    PriceType_t t_error_price_type_ = price_type_;
    std::cerr << typeid(*this).name() << ':' << __func__ << ':' << __LINE__ << ' ' << concise_indicator_description_
              << " passed " << t_error_price_type_ << " to indep " << std::endl;
    exit(1);
  }

  if (!dep_market_view_.subscribe_price_type(this, price_type_)) {
    PriceType_t t_error_price_type_ = price_type_;
    std::cerr << typeid(*this).name() << ':' << __func__ << ':' << __LINE__ << ' ' << concise_indicator_description_
              << " passed " << t_error_price_type_ << " to dep " << std::endl;
    exit(1);
  }
}

void OfflinePricePairDiff::OnMarketUpdate(const unsigned int t_security_id_,
                                          const MarketUpdateInfo& _market_update_info_) {
  if (t_security_id_ == dep_market_view_.security_id()) {
    current_dep_price_ = SecurityMarketView::GetPriceFromType(price_type_, _market_update_info_);
  } else {
    current_indep_price_ = SecurityMarketView::GetPriceFromType(price_type_, _market_update_info_);
  }

  if (!is_ready_) {
    if (indep_market_view_.is_ready_complex(2) && dep_market_view_.is_ready_complex(2)) {
      is_ready_ = true;
      InitializeValues();
    }
  } else if (!data_interrupted_) {
    indicator_value_ = beta_ * current_indep_price_ - alpha_ * current_dep_price_;
    NotifyIndicatorListeners(indicator_value_);
  }
}

void OfflinePricePairDiff::InitializeValues() { indicator_value_ = 0; }

void OfflinePricePairDiff::OnMarketDataInterrupted(const unsigned int _security_id_,
                                                   const int msecs_since_last_receive_) {
  if (indep_market_view_.security_id() == _security_id_) {
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

void OfflinePricePairDiff::OnMarketDataResumed(const unsigned int _security_id_) {
  if (data_interrupted_) {
    if (indep_market_view_.security_id() == _security_id_) {
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
