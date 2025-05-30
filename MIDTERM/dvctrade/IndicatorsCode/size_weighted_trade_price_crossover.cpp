/**
    \file IndicatorsCode/size_weighted_trade_price_crossover.cpp

    \author: (c) Copyright Two Roads Technological Solutions Pvt Ltd 2011
     Address:
         Suite No 353, Evoma, #14, Bhattarhalli,
         Old Madras Road, Near Garden City College,
         KR Puram, Bangalore 560049, India
         +91 80 4190 3551
*/
#include "dvccode/CommonDataStructures/vector_utils.hpp"

#include "dvctrade/Indicators/indicator_util.hpp"
#include "dvctrade/Indicators/size_weighted_trade_price_crossover.hpp"

namespace HFSAT {

void SizeWeightedTradePriceCrossover::CollectShortCodes(std::vector<std::string>& _shortcodes_affecting_this_indicator_,
                                                        std::vector<std::string>& _ors_source_needed_vec_,
                                                        const std::vector<const char*>& r_tokens_) {
  VectorUtils::UniqueVectorAdd(_shortcodes_affecting_this_indicator_, (std::string)r_tokens_[3]);
}

SizeWeightedTradePriceCrossover* SizeWeightedTradePriceCrossover::GetUniqueInstance(
    DebugLogger& t_dbglogger_, const Watch& r_watch_, const std::vector<const char*>& r_tokens_,
    PriceType_t _basepx_pxtype_) {
  // INDICATOR _this_weight_ _indicator_string_ _indep_market_view_ _fractional_seconds_st_ _fractional_seconds_lt_
  // _price_type_
  ShortcodeSecurityMarketViewMap::StaticCheckValid(r_tokens_[3]);
  if ((r_tokens_.size() >= 6) && (atof(r_tokens_[4]) <= atof(r_tokens_[5]))) {
    return GetUniqueInstance(t_dbglogger_, r_watch_,
                             *(ShortcodeSecurityMarketViewMap::StaticGetSecurityMarketView(r_tokens_[3])),
                             atof(r_tokens_[4]), atof(r_tokens_[5]), _basepx_pxtype_);
  } else {
    return NULL;
  }
}

SizeWeightedTradePriceCrossover* SizeWeightedTradePriceCrossover::GetUniqueInstance(
    DebugLogger& t_dbglogger_, const Watch& r_watch_, SecurityMarketView& _indep_market_view_,
    double _fractional_st_seconds_, double _fractional_lt_seconds_, PriceType_t _price_type_) {
  std::ostringstream t_temp_oss_;
  t_temp_oss_ << VarName() << ' ' << _indep_market_view_.secname() << ' ' << _fractional_st_seconds_ << ' '
              << _fractional_lt_seconds_;
  std::string concise_indicator_description_(t_temp_oss_.str());

  static std::map<std::string, SizeWeightedTradePriceCrossover*> concise_indicator_description_map_;
  if (concise_indicator_description_map_.find(concise_indicator_description_) ==
      concise_indicator_description_map_.end()) {
    concise_indicator_description_map_[concise_indicator_description_] =
        new SizeWeightedTradePriceCrossover(t_dbglogger_, r_watch_, concise_indicator_description_, _indep_market_view_,
                                            _fractional_st_seconds_, _fractional_lt_seconds_);
  }
  return concise_indicator_description_map_[concise_indicator_description_];
}

SizeWeightedTradePriceCrossover::SizeWeightedTradePriceCrossover(DebugLogger& t_dbglogger_, const Watch& r_watch_,
                                                                 const std::string& concise_indicator_description_,
                                                                 SecurityMarketView& _indep_market_view_,
                                                                 double _fractional_st_seconds_,
                                                                 double _fractional_lt_seconds_)
    : CommonIndicator(t_dbglogger_, r_watch_, concise_indicator_description_),
      indep_market_view_(_indep_market_view_),
      st_swtp_value_(0.00),
      lt_swtp_value_(0.00),
      st_time_decayed_trade_info_manager_(*(TimeDecayedTradeInfoManager::GetUniqueInstance(
          t_dbglogger_, r_watch_, _indep_market_view_, _fractional_st_seconds_))),
      lt_time_decayed_trade_info_manager_(*(TimeDecayedTradeInfoManager::GetUniqueInstance(
          t_dbglogger_, r_watch_, _indep_market_view_, _fractional_lt_seconds_))) {
  st_time_decayed_trade_info_manager_.compute_sumpxsz();
  st_time_decayed_trade_info_manager_.compute_sumsz();
  lt_time_decayed_trade_info_manager_.compute_sumpxsz();
  lt_time_decayed_trade_info_manager_.compute_sumsz();
  if (!indep_market_view_.subscribe_price_type(this, kPriceTypeMktSizeWPrice)) {
    PriceType_t t_error_price_type_ = kPriceTypeMktSizeWPrice;
    std::cerr << typeid(*this).name() << ':' << __func__ << ':' << __LINE__ << ' ' << concise_indicator_description_
              << " passed " << t_error_price_type_ << std::endl;
  }

  indep_market_view_.subscribe_tradeprints(this);
}

void SizeWeightedTradePriceCrossover::WhyNotReady() {
  if (!is_ready_) {
    if (!(indep_market_view_.is_ready_complex(2))) {
      DBGLOG_TIME_CLASS << indep_market_view_.secname() << " is_ready_complex = false " << DBGLOG_ENDL_FLUSH;
      DBGLOG_DUMP;
    }
  }
}

void SizeWeightedTradePriceCrossover::OnMarketUpdate(const unsigned int _security_id_,
                                                     const MarketUpdateInfo& cr_market_update_info_) {
#define MIN_SIGNIFICANT_SUM_SZ_TRADED 1.00
  double current_indep_price_ = SecurityMarketView::GetPriceFromType(kPriceTypeMktSizeWPrice, cr_market_update_info_);

  if (st_time_decayed_trade_info_manager_.sumsz_ >= MIN_SIGNIFICANT_SUM_SZ_TRADED) {
    st_swtp_value_ = (st_time_decayed_trade_info_manager_.sumpxsz_ / st_time_decayed_trade_info_manager_.sumsz_);
  } else {
    st_swtp_value_ = current_indep_price_;
  }

  if (lt_time_decayed_trade_info_manager_.sumsz_ >= MIN_SIGNIFICANT_SUM_SZ_TRADED) {
    lt_swtp_value_ = (lt_time_decayed_trade_info_manager_.sumpxsz_ / lt_time_decayed_trade_info_manager_.sumsz_);
  } else {
    lt_swtp_value_ = current_indep_price_;
  }

  indicator_value_ = st_swtp_value_ - lt_swtp_value_;

  NotifyIndicatorListeners(indicator_value_);
}

void SizeWeightedTradePriceCrossover::OnTradePrint(const unsigned int _security_id_,
                                                   const TradePrintInfo& _trade_print_info_,
                                                   const MarketUpdateInfo& cr_market_update_info_) {
#define MIN_SIGNIFICANT_SUM_SZ_TRADED 1.00
  double current_indep_price_ = SecurityMarketView::GetPriceFromType(kPriceTypeMktSizeWPrice, cr_market_update_info_);

  if (st_time_decayed_trade_info_manager_.sumsz_ >= MIN_SIGNIFICANT_SUM_SZ_TRADED) {
    st_swtp_value_ = (st_time_decayed_trade_info_manager_.sumpxsz_ / st_time_decayed_trade_info_manager_.sumsz_);
  } else {
    st_swtp_value_ = current_indep_price_;
  }

  if (lt_time_decayed_trade_info_manager_.sumsz_ >= MIN_SIGNIFICANT_SUM_SZ_TRADED) {
    lt_swtp_value_ = (lt_time_decayed_trade_info_manager_.sumpxsz_ / lt_time_decayed_trade_info_manager_.sumsz_);
  } else {
    lt_swtp_value_ = current_indep_price_;
  }

  indicator_value_ = st_swtp_value_ - lt_swtp_value_;

  NotifyIndicatorListeners(indicator_value_);
}

void SizeWeightedTradePriceCrossover::OnMarketDataInterrupted(const unsigned int _security_id_,
                                                              const int msecs_since_last_receive_) {
  if (indep_market_view_.security_id() == _security_id_) {
    data_interrupted_ = true;
    indicator_value_ = 0;
    NotifyIndicatorListeners(indicator_value_);
  }
}

void SizeWeightedTradePriceCrossover::OnMarketDataResumed(const unsigned int _security_id_) {
  if (indep_market_view_.security_id() == _security_id_) {
    data_interrupted_ = false;
  } else
    return;
}
}
