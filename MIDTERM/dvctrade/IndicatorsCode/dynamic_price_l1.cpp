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
#include "dvctrade/Indicators/dynamic_price_l1.hpp"

namespace HFSAT {

void DynamicPriceL1::CollectShortCodes(std::vector<std::string> &_shortcodes_affecting_this_indicator_,
                                       std::vector<std::string> &_ors_source_needed_vec_,
                                       const std::vector<const char *> &r_tokens_) {
  VectorUtils::UniqueVectorAdd(_shortcodes_affecting_this_indicator_, (std::string)r_tokens_[3]);
}

DynamicPriceL1 *DynamicPriceL1::GetUniqueInstance(DebugLogger &t_dbglogger_, const Watch &r_watch_,
                                                  const std::vector<const char *> &r_tokens_,
                                                  PriceType_t _basepx_pxtype_) {
  ShortcodeSecurityMarketViewMap::StaticCheckValid(std::string(r_tokens_[3]));

  if (r_tokens_.size() < 7) {
    ExitVerbose(kModelCreationIndicatorLineLessArgs, t_dbglogger_,
                " DynamicPriceL1 Incorrect Syntax. Correct syntax would b INDICATOR _this_weight_ _indicator_string_ "
                "dep_shc_ _trade_sec_ _percentile_ _price_type_");
  }
  return GetUniqueInstance(t_dbglogger_, r_watch_,
                           *(ShortcodeSecurityMarketViewMap::StaticGetSecurityMarketView(std::string(r_tokens_[3]))),
                           atof(r_tokens_[4]), atof(r_tokens_[5]), StringToPriceType_t(r_tokens_[6]), _basepx_pxtype_);
}

DynamicPriceL1 *DynamicPriceL1::GetUniqueInstance(DebugLogger &t_dbglogger_, const Watch &r_watch_,
                                                  SecurityMarketView &t_indep_market_view_,
                                                  double _trade_fractional_seconds_, double _percentile_,
                                                  PriceType_t _price_type_, PriceType_t _base_price_type_) {
  std::ostringstream t_temp_oss_;
  t_temp_oss_ << VarName() << ' ' << t_indep_market_view_.secname() << ' ' << _trade_fractional_seconds_ << ' ' << ' '
              << _percentile_ << ' ' << PriceType_t_To_String(_price_type_) << ' '
              << PriceType_t_To_String(_base_price_type_);
  std::string concise_indicator_description_(t_temp_oss_.str());

  static std::map<std::string, DynamicPriceL1 *> concise_indicator_description_map_;

  if (concise_indicator_description_map_.find(concise_indicator_description_) ==
      concise_indicator_description_map_.end()) {
    concise_indicator_description_map_[concise_indicator_description_] =
        new DynamicPriceL1(t_dbglogger_, r_watch_, concise_indicator_description_, t_indep_market_view_,
                           _trade_fractional_seconds_, _percentile_, _price_type_, _base_price_type_);
  }
  return concise_indicator_description_map_[concise_indicator_description_];
}

DynamicPriceL1::DynamicPriceL1(DebugLogger &t_dbglogger_, const Watch &r_watch_,
                               const std::string &concise_indicator_description_,
                               SecurityMarketView &t_indep_market_view_, double _trade_fractional_seconds_,
                               double _percentile_, PriceType_t _price_type_, PriceType_t _basepx_pxtype_)
    : CommonIndicator(t_dbglogger_, r_watch_, concise_indicator_description_),
      indep_market_view_(t_indep_market_view_),
      time_decayed_trade_info_manager_(*TimeDecayedTradeInfoManager::GetUniqueInstance(
          t_dbglogger_, r_watch_, t_indep_market_view_, _trade_fractional_seconds_)),
      trade_price_avg_(0),
      current_price_(0),
      trade_fractional_seconds_(_trade_fractional_seconds_),
      percentile_(_percentile_),
      high_trade_sz_(0),
      price_type_(_price_type_),
      basepx_type_(_basepx_pxtype_) {
  if (!indep_market_view_.subscribe_price_type(this, _price_type_)) {
    PriceType_t t_error_price_type_ = _price_type_;
    std::cerr << typeid(*this).name() << ':' << __func__ << ':' << __LINE__ << ' ' << " passed " << t_error_price_type_
              << std::endl;
  }

  if (!indep_market_view_.subscribe_price_type(this, _basepx_pxtype_)) {
    PriceType_t t_error_price_type_ = _basepx_pxtype_;
    std::cerr << typeid(*this).name() << ':' << __func__ << ':' << __LINE__ << ' ' << " passed " << t_error_price_type_
              << std::endl;
  }

  time_decayed_trade_info_manager_.compute_sumpxsz();
  time_decayed_trade_info_manager_.compute_sumsz();
  t_indep_market_view_.subscribe_tradeprints(this);
}

void DynamicPriceL1::OnMarketUpdate(const unsigned int _security_id_, const MarketUpdateInfo &_market_update_info_) {
#define MIN_SIGNIFICANT_SUM_SZ_TRADED 1.00

  if (!is_ready_) {
    if (indep_market_view_.is_ready_complex(2)) {
      is_ready_ = true;
      GetHistoricalValues();
    }
  } else if (!data_interrupted_) {
    double trade_ratio_;

    if (time_decayed_trade_info_manager_.sumsz_ >= MIN_SIGNIFICANT_SUM_SZ_TRADED) {
      trade_price_avg_ = (time_decayed_trade_info_manager_.sumpxsz_ / time_decayed_trade_info_manager_.sumsz_);
      trade_ratio_ = (time_decayed_trade_info_manager_.sumsz_ / high_trade_sz_);
    } else {
      trade_price_avg_ = 0;
      trade_ratio_ = 0;
    }

    double trade_fac_ = pow(trade_ratio_, 3) / (1 + pow(trade_ratio_, 3));

    current_price_ = SecurityMarketView::GetPriceFromType(price_type_, _market_update_info_);

    current_price_ = (1 - trade_fac_) * current_price_ + trade_fac_ * trade_price_avg_;
    indicator_value_ = (current_price_ - SecurityMarketView::GetPriceFromType(basepx_type_, _market_update_info_));
    NotifyIndicatorListeners(indicator_value_);
  }
}

void DynamicPriceL1::OnMarketDataInterrupted(const unsigned int _security_id_, const int msecs_since_last_receive_) {
  if (indep_market_view_.security_id() == _security_id_) {
    data_interrupted_ = true;
    indicator_value_ = 0;
    NotifyIndicatorListeners(indicator_value_);
  }
}

void DynamicPriceL1::OnMarketDataResumed(const unsigned int _security_id_) {
  if (indep_market_view_.security_id() == _security_id_) {
    data_interrupted_ = false;
  }
}

void DynamicPriceL1::GetHistoricalValues() {
  high_trade_sz_ = HFSAT::SampleDataUtil::GetPercentileForPeriod(indep_market_view_.shortcode(), watch_.YYYYMMDD(), 60,
                                                                 trading_start_mfm_, trading_end_mfm_,
                                                                 std::string("RollingAvgTradeSize300"), percentile_);
  double avg_trade_size_ = HFSAT::SampleDataUtil::GetPercentileForPeriod(
      indep_market_view_.shortcode(), watch_.YYYYMMDD(), 60, trading_start_mfm_, trading_end_mfm_,
      std::string("RollingAvgTradeSize300"), 0.5);
  high_trade_sz_ = avg_trade_size_ + 30 * (high_trade_sz_ - avg_trade_size_);  // This is to scale from sampling
                                                                               // distribution to normal percentile.30
                                                                               // is used as square root of 900.
}
}
