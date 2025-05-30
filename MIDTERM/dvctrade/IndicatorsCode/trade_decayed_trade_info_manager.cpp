/**
   \file IndicatorsCode/trade_decayed_trade_info_manager.cpp

    \author: (c) Copyright Two Roads Technological Solutions Pvt Ltd 2011
     Address:
         Suite No 353, Evoma, #14, Bhattarhalli,
         Old Madras Road, Near Garden City College,
         KR Puram, Bangalore 560049, India
         +91 80 4190 3551
*/
#include <sstream>

#include "dvccode/CDef/math_utils.hpp"

#include "dvctrade/Indicators/trade_decayed_trade_info_manager.hpp"

namespace HFSAT {

TradeDecayedTradeInfoManager* TradeDecayedTradeInfoManager::GetUniqueInstance(DebugLogger& t_dbglogger_,
                                                                              const Watch& r_watch_,
                                                                              SecurityMarketView& _indep_market_view_,
                                                                              unsigned int _num_trades_halflife_) {
  std::ostringstream t_temp_oss_;
  t_temp_oss_ << VarName() << ' ' << _indep_market_view_.secname() << ' ' << _num_trades_halflife_;

  std::string concise_indicator_description_(t_temp_oss_.str());

  static std::map<std::string, TradeDecayedTradeInfoManager*> concise_indicator_description_map_;

  if (concise_indicator_description_map_.find(concise_indicator_description_) ==
      concise_indicator_description_map_.end()) {
    concise_indicator_description_map_[concise_indicator_description_] =
        new TradeDecayedTradeInfoManager(t_dbglogger_, r_watch_, _indep_market_view_, _num_trades_halflife_);
  }
  return concise_indicator_description_map_[concise_indicator_description_];
}

TradeDecayedTradeInfoManager::TradeDecayedTradeInfoManager(DebugLogger& t_dbglogger_, const Watch& r_watch_,
                                                           SecurityMarketView& _indep_market_view_,
                                                           unsigned int _num_trades_halflife_)
    : dbglogger_(t_dbglogger_),
      watch_(r_watch_),
      indep_market_view_(_indep_market_view_),
      trade_history_halflife_trades_(std::max(1u, _num_trades_halflife_)),
      decay_page_factor_(MathUtils::CalcDecayFactor(std::max(1u, _num_trades_halflife_))),
      last_bid_int_price_(0),
      last_ask_int_price_(0),
      computing_sumtdiffsz_(false),
      computing_sumtdiffsqrtsz_(false),
      computing_sumsz_(false),
      computing_sumsqrtsz_(false),
      computing_sumtype_(false),
      computing_sumcoeffs_(false),
      computing_sumpx_(false),
      computing_sumtypesz_(false),
      computing_sumtypesqrtsz_(false),
      computing_sumpxsz_(false),
      computing_sumpxsqrtsz_(false),
      computing_sumtdiffnsz_(false),
      computing_sumtdifffsz_(false),
      computing_sumtdifffsqrtsz_(false),
      computing_sumtdiffsqrtfsqrtsz_(false),
      computing_lvlsumtdiffsz_(false),
      computing_lvlsumtdiffnsz_(false),
      computing_lvlsumtdifffsz_(false),
      sumtdiffsz_(0),           ///< Sum ( tradepx_mktpx_diff_ * size_traded_ )
      sumtdiffsqrtsz_(0),       ///< Sum ( tradepx_mktpx_diff_ * sqrt_size_traded_ )
      sumsz_(0),                ///< Sum ( size_traded_ )
      sumsqrtsz_(0),            ///< Sum ( sqrt_size_traded_ )
      sumtype_(0),              ///< Sum ( int_trade_type_ )
      sumcoeffs_(0),            ///< Sum ( 1 ) ... used primarily in averaging
      sumpx_(0),                ///< Sum ( trade_price_ )
      sumtypesz_(0),            ///< Sum ( int_trade_type_ * size_traded_ )
      sumtypesqrtsz_(0),        ///< Sum ( int_trade_type_ * sqrt_size_traded_ )
      sumpxsz_(0),              ///< Sum ( trade_price_ * size_traded_ )
      sumpxsqrtsz_(0),          ///< Sum ( trade_price_ * sqrt_size_traded_ )
      sumtdiffnsz_(0),          ///< Sum ( tradepx_mktpx_diff_ * trade_impact_ ) for trade_impact_ refer TradePrintInfo
      sumtdifffsz_(0),          ///< Sum ( tradepx_mktpx_diff_ * trade_impact_ * size_traded_ )
      sumtdifffsqrtsz_(0),      ///< Sum ( tradepx_mktpx_diff_ * trade_impact_ * sqrt_size_traded_ )
      sumtdiffsqrtfsqrtsz_(0),  ///< Sum ( tradepx_mktpx_diff_ * sqrt_trade_impact_ * sqrt_size_traded_ )
      lvlsumtdiffsz_(0),        ///< Sum ( tradepx_mktpx_diff_ * size_traded_ ), same as sumtdiffsz_ except the value is
      /// decayed by 1/2.0 when the mid_price changes
      lvlsumtdiffnsz_(0),
      lvlsumtdifffsz_(0) {
  indep_market_view_.subscribe_tradeprints(
      this);  // this does subscribe to market update events only for AdjustLevelVars
}

void TradeDecayedTradeInfoManager::compute_sumtdiffsz() {
  indep_market_view_.ComputeTradepxMktpxDiff();
  computing_sumtdiffsz_ = true;
}

void TradeDecayedTradeInfoManager::compute_sumtdiffsqrtsz() {
  indep_market_view_.ComputeTradepxMktpxDiff();
  indep_market_view_.ComputeSqrtSizeTraded();
  computing_sumtdiffsqrtsz_ = true;
}

void TradeDecayedTradeInfoManager::compute_sumsz() { computing_sumsz_ = true; }

void TradeDecayedTradeInfoManager::compute_sumsqrtsz() {
  indep_market_view_.ComputeSqrtSizeTraded();
  computing_sumsqrtsz_ = true;
}

void TradeDecayedTradeInfoManager::compute_sumtype() {
  indep_market_view_.ComputeIntTradeType();
  computing_sumtype_ = true;
}

void TradeDecayedTradeInfoManager::compute_sumcoeffs() { computing_sumcoeffs_ = true; }

void TradeDecayedTradeInfoManager::compute_sumpx() { computing_sumpx_ = true; }

void TradeDecayedTradeInfoManager::compute_sumtypesz() {
  indep_market_view_.ComputeIntTradeType();
  computing_sumtypesz_ = true;
}

void TradeDecayedTradeInfoManager::compute_sumtypesqrtsz() {
  indep_market_view_.ComputeIntTradeType();
  indep_market_view_.ComputeSqrtSizeTraded();
  computing_sumtypesqrtsz_ = true;
}

void TradeDecayedTradeInfoManager::compute_sumpxsz() { computing_sumpxsz_ = true; }

void TradeDecayedTradeInfoManager::compute_sumpxsqrtsz() {
  indep_market_view_.ComputeSqrtSizeTraded();
  computing_sumpxsqrtsz_ = true;
}

void TradeDecayedTradeInfoManager::compute_sumtdiffnsz() {
  indep_market_view_.ComputeTradepxMktpxDiff();
  indep_market_view_.ComputeTradeImpact();
  computing_sumtdiffnsz_ = true;
}

void TradeDecayedTradeInfoManager::compute_sumtdifffsz() {
  indep_market_view_.ComputeTradepxMktpxDiff();
  indep_market_view_.ComputeTradeImpact();
  computing_sumtdifffsz_ = true;
}

void TradeDecayedTradeInfoManager::compute_sumtdifffsqrtsz() {
  indep_market_view_.ComputeTradepxMktpxDiff();
  indep_market_view_.ComputeSqrtSizeTraded();
  indep_market_view_.ComputeTradeImpact();
  computing_sumtdifffsqrtsz_ = true;
}

void TradeDecayedTradeInfoManager::compute_sumtdiffsqrtfsqrtsz() {
  indep_market_view_.ComputeTradepxMktpxDiff();
  indep_market_view_.ComputeSqrtSizeTraded();
  indep_market_view_.ComputeSqrtTradeImpact();
  computing_sumtdiffsqrtfsqrtsz_ = true;
}

void TradeDecayedTradeInfoManager::compute_lvlsumtdiffsz() {
  indep_market_view_.ComputeTradepxMktpxDiff();
  // no need to subscribe to book events for lvl vars since we already subscribe to tradeprints,
  // which internally subscribes as an l1_price_listeners_
  computing_lvlsumtdiffsz_ = true;
}

void TradeDecayedTradeInfoManager::compute_lvlsumtdiffnsz() {
  indep_market_view_.ComputeTradepxMktpxDiff();
  indep_market_view_.ComputeTradeImpact();
  // no need to subscribe to book events for lvl vars since we already subscribe to tradeprints,
  // which internally subscribes as an l1_price_listeners_
  computing_lvlsumtdiffnsz_ = true;
}

void TradeDecayedTradeInfoManager::compute_lvlsumtdifffsz() {
  indep_market_view_.ComputeTradepxMktpxDiff();
  indep_market_view_.ComputeTradeImpact();
  // no need to subscribe to book events for lvl vars since we already subscribe to tradeprints,
  // which internally subscribes as an l1_price_listeners_
  computing_lvlsumtdifffsz_ = true;
}

void TradeDecayedTradeInfoManager::InitializeValues() {
  sumtdiffsz_ = 0;
  sumtdiffsqrtsz_ = 0;
  sumsz_ = 0;
  sumsqrtsz_ = 0;
  sumtype_ = 0;
  sumcoeffs_ = 0;
  sumpx_ = 0;
  sumtypesz_ = 0;
  sumtypesqrtsz_ = 0;
  sumpxsz_ = 0;
  sumpxsqrtsz_ = 0;
  sumtdiffnsz_ = 0;
  sumtdifffsz_ = 0;
  sumtdifffsqrtsz_ = 0;
  sumtdiffsqrtfsqrtsz_ = 0;
}
}
