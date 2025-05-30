/**
    \file IndicatorsCode/tr_sum_ttype.cpp

    \author: (c) Copyright Two Roads Technological Solutions Pvt Ltd 2011
     Address:
         Suite No 353, Evoma, #14, Bhattarhalli,
         Old Madras Road, Near Garden City College,
         KR Puram, Bangalore 560049, India
         +91 80 4190 3551
*/
#include "dvccode/CDef/math_utils.hpp"
#include "dvctrade/Indicators/indicator_util.hpp"
#include "dvccode/CommonTradeUtils/sample_data_util.hpp"
#include "dvctrade/Indicators/tr_avg_tdiff_sqrt_tsize_volfactor.hpp"

namespace HFSAT {

void TRAvgTDiffSqrtTSizeVolfactor::CollectShortCodes(std::vector<std::string>& _shortcodes_affecting_this_indicator_,
                                                     std::vector<std::string>& _ors_source_needed_vec_,
                                                     const std::vector<const char*>& r_tokens_) {
  VectorUtils::UniqueVectorAdd(_shortcodes_affecting_this_indicator_, (std::string)r_tokens_[3]);
}

TRAvgTDiffSqrtTSizeVolfactor* TRAvgTDiffSqrtTSizeVolfactor::GetUniqueInstance(DebugLogger& t_dbglogger_,
                                                                              const Watch& r_watch_,
                                                                              const std::vector<const char*>& r_tokens_,
                                                                              PriceType_t _basepx_pxtype_) {
  // INDICATOR _this_weight_ _indicator_string_ _indep_market_view_ _num_trades_halflife_
  if (r_tokens_.size() >= 7) {
    return GetUniqueInstance(t_dbglogger_, r_watch_,
                             *(ShortcodeSecurityMarketViewMap::StaticGetSecurityMarketView(r_tokens_[3])),
                             atoi(r_tokens_[4]), atof(r_tokens_[5]), StringToPriceType_t(r_tokens_[6]));
  } else {
    ExitVerbose(kExitErrorCodeGeneral,
                "insufficient inputs to SimplePriceType : INDICATOR _this_weight_ _indicator_string_ _dep_market_view_ "
                "frac_trades volfactor_percentile(-1) _price_type_\n");
    return NULL;  // wont reach here , just to remove warning
  }
}

TRAvgTDiffSqrtTSizeVolfactor* TRAvgTDiffSqrtTSizeVolfactor::GetUniqueInstance(
    DebugLogger& t_dbglogger_, const Watch& r_watch_, SecurityMarketView& _indep_market_view_,
    int _num_trades_halflife_, double _volfactor_percentile_, PriceType_t _basepx_pxtype_) {
  std::ostringstream t_temp_oss_;
  t_temp_oss_ << VarName() << ' ' << _indep_market_view_.secname() << ' ' << _num_trades_halflife_ << " "
              << _volfactor_percentile_ << " " << PriceType_t_To_String(_basepx_pxtype_);
  std::string concise_indicator_description_(t_temp_oss_.str());

  static std::map<std::string, TRAvgTDiffSqrtTSizeVolfactor*> concise_indicator_description_map_;
  if (concise_indicator_description_map_.find(concise_indicator_description_) ==
      concise_indicator_description_map_.end()) {
    concise_indicator_description_map_[concise_indicator_description_] =
        new TRAvgTDiffSqrtTSizeVolfactor(t_dbglogger_, r_watch_, concise_indicator_description_, _indep_market_view_,
                                         _num_trades_halflife_, _volfactor_percentile_, _basepx_pxtype_);
  }
  return concise_indicator_description_map_[concise_indicator_description_];
}

TRAvgTDiffSqrtTSizeVolfactor::TRAvgTDiffSqrtTSizeVolfactor(DebugLogger& t_dbglogger_, const Watch& r_watch_,
                                                           const std::string& concise_indicator_description_,
                                                           SecurityMarketView& _indep_market_view_,
                                                           int _num_trades_halflife_, double _volfactor_percentile_,
                                                           PriceType_t _price_type_)
    : CommonIndicator(t_dbglogger_, r_watch_, concise_indicator_description_),
      indep_market_view_(_indep_market_view_),
      volfactor_percentile_(_volfactor_percentile_),
      price_type_(_price_type_),
      decay_page_factor_(MathUtils::CalcDecayFactor(std::max(1, _num_trades_halflife_))),
      cutoff_trades_(0),
      sumtdiffsqrtsz_(0),
      sumsqrtsz_(0),
      trades_measure_(0) {
  if (!_indep_market_view_.subscribe_price_type(this, price_type_)) {
    PriceType_t t_error_price_type_ = price_type_;
    std::cerr << typeid(*this).name() << ':' << __func__ << ':' << __LINE__ << ' ' << concise_indicator_description_
              << " passed " << t_error_price_type_ << std::endl;
  }
  _indep_market_view_.subscribe_tradeprints(this);
  _indep_market_view_.ComputeSqrtSizeTraded();

  if (volfactor_percentile_ > 0) {
    cutoff_trades_ = HFSAT::SampleDataUtil::GetPercentileForPeriod(indep_market_view_.shortcode(), watch_.YYYYMMDD(),
                                                                   30, trading_start_mfm_, trading_end_mfm_, "TRADES",
                                                                   volfactor_percentile_);
    trades_measure_indc_ =
        RecentSimpleTradesMeasure::GetUniqueInstance(t_dbglogger_, r_watch_, indep_market_view_, 300);
    trades_measure_indc_->add_unweighted_indicator_listener(1u, this);
  }
  // std::cout << "cutoff_trades: " << cutoff_trades_ << std::endl;
}

void TRAvgTDiffSqrtTSizeVolfactor::OnMarketUpdate(const unsigned int _security_id_,
                                                  const MarketUpdateInfo& _market_update_info_) {
  if (sumsqrtsz_ > 0) {
    double trade_bias_ = sumtdiffsqrtsz_ / sumsqrtsz_;

    if (volfactor_percentile_ > 0) {
      indicator_value_ = (1 / (1 + exp(cutoff_trades_ - trades_measure_))) * trade_bias_;
    } else {
      indicator_value_ = trade_bias_;
    }
  }
  if (data_interrupted_) indicator_value_ = 0;

  NotifyIndicatorListeners(indicator_value_);
}

void TRAvgTDiffSqrtTSizeVolfactor::OnTradePrint(const unsigned int _security_id_,
                                                const TradePrintInfo& _trade_print_info_,
                                                const MarketUpdateInfo& _market_update_info_) {
  sumtdiffsqrtsz_ *= decay_page_factor_;
  sumsqrtsz_ *= decay_page_factor_;

  sumtdiffsqrtsz_ +=
      (_trade_print_info_.trade_price_ - SecurityMarketView::GetPriceFromType(price_type_, _market_update_info_)) *
      _trade_print_info_.sqrt_size_traded_;
  sumsqrtsz_ += _trade_print_info_.sqrt_size_traded_;

  if (sumsqrtsz_ > 0) {
    // std::cout << "sumsqrtsz: " << sumsqrtsz_ << ", sumtdiffsqrtsz: " << sumtdiffsqrtsz_ << ", trades_measure_: " <<
    // trades_measure_ << std::endl;
    double trade_bias_ = sumtdiffsqrtsz_ / sumsqrtsz_;

    if (volfactor_percentile_ > 0) {
      indicator_value_ = (1 / (1 + exp(cutoff_trades_ - trades_measure_))) * trade_bias_;
    } else {
      indicator_value_ = trade_bias_;
    }
  }
  if (data_interrupted_) indicator_value_ = 0;

  NotifyIndicatorListeners(indicator_value_);
}

void TRAvgTDiffSqrtTSizeVolfactor::OnIndicatorUpdate(const unsigned int& _indicator_index_, const double& _new_value_) {
  if (volfactor_percentile_ > 0) {
    trades_measure_ = _new_value_;
    if (sumsqrtsz_ > 0) {
      double trade_bias_ = sumtdiffsqrtsz_ / sumsqrtsz_;
      indicator_value_ =
          (1 / (1 + exp(cutoff_trades_ - trades_measure_))) * trade_bias_;  // + indep_market_view_.mid_price();
    }
    if (data_interrupted_) indicator_value_ = 0;

    NotifyIndicatorListeners(indicator_value_);
  }
}

void TRAvgTDiffSqrtTSizeVolfactor::OnMarketDataInterrupted(const unsigned int _security_id_,
                                                           const int msecs_since_last_receive_) {
  if (indep_market_view_.security_id() == _security_id_) {
    data_interrupted_ = true;
    indicator_value_ = 0;
    NotifyIndicatorListeners(indicator_value_);
  } else
    return;
}

void TRAvgTDiffSqrtTSizeVolfactor::OnMarketDataResumed(const unsigned int _security_id_) {
  if (indep_market_view_.security_id() == _security_id_) {
    data_interrupted_ = false;
    // trade_decayed_trade_info_manager_.InitializeValues();
  } else
    return;
}
}
