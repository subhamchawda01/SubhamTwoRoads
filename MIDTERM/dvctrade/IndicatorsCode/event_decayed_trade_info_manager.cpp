/**
    \file IndicatorsCode/event_decayed_trade_info_manager.cpp

    \author: (c) Copyright Two Roads Technological Solutions Pvt Ltd 2011
     Address:
         Suite No 353, Evoma, #14, Bhattarhalli,
         Old Madras Road, Near Garden City College,
         KR Puram, Bangalore 560049, India
         +91 80 4190 3551
*/
#include <sstream>

#include "dvccode/CDef/math_utils.hpp"

#include "dvctrade/Indicators/event_decayed_trade_info_manager.hpp"

namespace HFSAT {

EventDecayedTradeInfoManager* EventDecayedTradeInfoManager::GetUniqueInstance(DebugLogger& t_dbglogger_,
                                                                              const Watch& r_watch_,
                                                                              SecurityMarketView& _indep_market_view_,
                                                                              unsigned int _num_events_halflife_) {
  std::ostringstream t_temp_oss_;
  t_temp_oss_ << VarName() << ' ' << _indep_market_view_.secname() << ' ' << _num_events_halflife_;

  std::string concise_indicator_description_(t_temp_oss_.str());

  static std::map<std::string, EventDecayedTradeInfoManager*> concise_indicator_description_map_;

  if (concise_indicator_description_map_.find(concise_indicator_description_) ==
      concise_indicator_description_map_.end()) {
    concise_indicator_description_map_[concise_indicator_description_] =
        new EventDecayedTradeInfoManager(t_dbglogger_, r_watch_, _indep_market_view_, _num_events_halflife_);
  }
  return concise_indicator_description_map_[concise_indicator_description_];
}

EventDecayedTradeInfoManager::EventDecayedTradeInfoManager(DebugLogger& t_dbglogger_, const Watch& r_watch_,
                                                           SecurityMarketView& _indep_market_view_,
                                                           unsigned int _num_events_halflife_)
    : dbglogger_(t_dbglogger_),
      watch_(r_watch_),
      indep_market_view_(_indep_market_view_),
      trade_history_halflife_mktevents_(std::max((unsigned int)1, _num_events_halflife_)),
      decay_page_factor_(0.95),
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
      computing_lvlsumtdifffsz_(false),
      computing_lvlsumtdiffnsz_(false),
      sumtdiffsz_(0),
      sumtdiffsqrtsz_(0),
      sumsz_(0),
      sumsqrtsz_(0),
      sumtype_(0),
      sumcoeffs_(0),
      sumpx_(0),
      sumtypesz_(0),
      sumtypesqrtsz_(0),
      sumpxsz_(0),
      sumpxsqrtsz_(0),
      sumtdiffnsz_(0),
      sumtdifffsz_(0),
      sumtdifffsqrtsz_(0),
      sumtdiffsqrtfsqrtsz_(0),
      lvlsumtdiffsz_(0),
      lvlsumtdifffsz_(0),
      lvlsumtdiffnsz_(0) {
  decay_page_factor_ = MathUtils::CalcDecayFactor(trade_history_halflife_mktevents_);
  InitializeValues();

  indep_market_view_.subscribe_tradeprints(this);
  indep_market_view_.subscribe_price_type(this, kPriceTypeMktSizeWPrice);  // only needed for decaying values
}

void EventDecayedTradeInfoManager::OnMarketUpdate(const unsigned int _security_id_,
                                                  const MarketUpdateInfo& cr_market_update_info_) {
  DecayCurrentValues();
}

void EventDecayedTradeInfoManager::OnTradePrint(const unsigned int _security_id_,
                                                const TradePrintInfo& cr_trade_print_info_,
                                                const MarketUpdateInfo& cr_market_update_info_) {
  DecayCurrentValues();

  if (computing_sumtdiffsz_) {
    sumtdiffsz_ += cr_trade_print_info_.tradepx_mktpx_diff_ * cr_trade_print_info_.size_traded_;
  }

  if (computing_sumtdiffsqrtsz_) {
    sumtdiffsqrtsz_ += cr_trade_print_info_.tradepx_mktpx_diff_ * cr_trade_print_info_.sqrt_size_traded_;
  }

  if (computing_sumsz_) {
    sumsz_ += cr_trade_print_info_.size_traded_;
  }

  if (computing_sumsqrtsz_) {
    sumsqrtsz_ += cr_trade_print_info_.sqrt_size_traded_;
  }

  if (computing_sumtype_) {
    sumtype_ += cr_trade_print_info_.int_trade_type_;
  }

  if (computing_sumcoeffs_) {
    sumcoeffs_++;
  }

  if (computing_sumpx_) {
    sumpx_ += cr_trade_print_info_.trade_price_;
  }

  if (computing_sumtypesz_) {
    sumtypesz_ += cr_trade_print_info_.int_trade_type_ * cr_trade_print_info_.size_traded_;
  }

  if (computing_sumtypesqrtsz_) {
    sumtypesqrtsz_ += cr_trade_print_info_.int_trade_type_ * cr_trade_print_info_.sqrt_size_traded_;
  }

  if (computing_sumpxsz_) {
    sumpxsz_ += cr_trade_print_info_.trade_price_ * cr_trade_print_info_.size_traded_;
  }

  if (computing_sumpxsqrtsz_) {
    sumpxsqrtsz_ += cr_trade_print_info_.trade_price_ * cr_trade_print_info_.sqrt_size_traded_;
  }

  if (computing_sumtdiffnsz_) {
    sumtdiffnsz_ += (cr_trade_print_info_.tradepx_mktpx_diff_ * cr_trade_print_info_.trade_impact_);
  }

  if (computing_sumtdifffsz_) {
    sumtdifffsz_ += (cr_trade_print_info_.tradepx_mktpx_diff_ * cr_trade_print_info_.trade_impact_ *
                     cr_trade_print_info_.size_traded_);
  }

  if (computing_sumtdifffsqrtsz_) {
    sumtdifffsqrtsz_ += (cr_trade_print_info_.tradepx_mktpx_diff_ * cr_trade_print_info_.trade_impact_ *
                         cr_trade_print_info_.sqrt_size_traded_);
  }

  if (computing_sumtdiffsqrtfsqrtsz_) {
    sumtdiffsqrtfsqrtsz_ += (cr_trade_print_info_.tradepx_mktpx_diff_ * cr_trade_print_info_.sqrt_trade_impact_ *
                             cr_trade_print_info_.sqrt_size_traded_);
  }

  if (computing_lvlsumtdiffsz_) {
    if (cr_market_update_info_.mid_price_ != cr_market_update_info_.pretrade_mid_price_) {
      lvlsumtdiffsz_ /= 2.0;
    }
    lvlsumtdiffsz_ += cr_trade_print_info_.tradepx_mktpx_diff_ * cr_trade_print_info_.size_traded_;
  }
  if (computing_lvlsumtdifffsz_) {
    if (cr_market_update_info_.mid_price_ != cr_market_update_info_.pretrade_mid_price_) {
      lvlsumtdifffsz_ /= 2.0;
    }
    lvlsumtdifffsz_ += cr_trade_print_info_.tradepx_mktpx_diff_ * cr_trade_print_info_.trade_impact_ *
                       cr_trade_print_info_.size_traded_;
  }
  if (computing_lvlsumtdiffnsz_) {
    if (cr_market_update_info_.mid_price_ != cr_market_update_info_.pretrade_mid_price_) {
      lvlsumtdiffnsz_ /= 2.0;
    }
    lvlsumtdiffnsz_ += cr_trade_print_info_.tradepx_mktpx_diff_ * cr_trade_print_info_.trade_impact_;
  }
}

void EventDecayedTradeInfoManager::compute_sumtdiffsz() {
  indep_market_view_.ComputeTradepxMktpxDiff();
  computing_sumtdiffsz_ = true;
}

void EventDecayedTradeInfoManager::compute_sumtdiffsqrtsz() {
  indep_market_view_.ComputeTradepxMktpxDiff();
  indep_market_view_.ComputeSqrtSizeTraded();
  computing_sumtdiffsqrtsz_ = true;
}

void EventDecayedTradeInfoManager::compute_sumsz() { computing_sumsz_ = true; }

void EventDecayedTradeInfoManager::compute_sumsqrtsz() {
  indep_market_view_.ComputeSqrtSizeTraded();
  computing_sumsqrtsz_ = true;
}

void EventDecayedTradeInfoManager::compute_sumtype() {
  indep_market_view_.ComputeIntTradeType();
  computing_sumtype_ = true;
}

void EventDecayedTradeInfoManager::compute_sumcoeffs() { computing_sumcoeffs_ = true; }

void EventDecayedTradeInfoManager::compute_sumpx() { computing_sumpx_ = true; }

void EventDecayedTradeInfoManager::compute_sumtypesz() {
  indep_market_view_.ComputeIntTradeType();
  computing_sumtypesz_ = true;
}

void EventDecayedTradeInfoManager::compute_sumtypesqrtsz() {
  indep_market_view_.ComputeIntTradeType();
  indep_market_view_.ComputeSqrtSizeTraded();
  computing_sumtypesqrtsz_ = true;
}

void EventDecayedTradeInfoManager::compute_sumpxsz() { computing_sumpxsz_ = true; }

void EventDecayedTradeInfoManager::compute_sumpxsqrtsz() {
  indep_market_view_.ComputeSqrtSizeTraded();
  computing_sumpxsqrtsz_ = true;
}

void EventDecayedTradeInfoManager::compute_sumtdiffnsz() {
  indep_market_view_.ComputeTradepxMktpxDiff();
  indep_market_view_.ComputeTradeImpact();
  computing_sumtdiffnsz_ = true;
}

void EventDecayedTradeInfoManager::compute_sumtdifffsz() {
  indep_market_view_.ComputeTradepxMktpxDiff();
  indep_market_view_.ComputeTradeImpact();
  computing_sumtdifffsz_ = true;
}

void EventDecayedTradeInfoManager::compute_sumtdifffsqrtsz() {
  indep_market_view_.ComputeTradepxMktpxDiff();
  indep_market_view_.ComputeSqrtSizeTraded();
  indep_market_view_.ComputeTradeImpact();
  computing_sumtdifffsqrtsz_ = true;
}

void EventDecayedTradeInfoManager::compute_sumtdiffsqrtfsqrtsz() {
  indep_market_view_.ComputeTradepxMktpxDiff();
  indep_market_view_.ComputeSqrtSizeTraded();
  indep_market_view_.ComputeSqrtTradeImpact();
  computing_sumtdiffsqrtfsqrtsz_ = true;
}

void EventDecayedTradeInfoManager::compute_lvlsumtdiffsz() {
  indep_market_view_.subscribe_price_type(this, kPriceTypeMidprice);  // only needed for decaying values
  indep_market_view_.ComputeTradepxMktpxDiff();
  indep_market_view_.SetStorePreTrade();
  computing_lvlsumtdiffsz_ = true;
}

void EventDecayedTradeInfoManager::compute_lvlsumtdifffsz() {
  indep_market_view_.subscribe_price_type(this, kPriceTypeMidprice);  // only needed for decaying values
  indep_market_view_.ComputeTradepxMktpxDiff();
  indep_market_view_.ComputeTradeImpact();
  indep_market_view_.SetStorePreTrade();
  computing_lvlsumtdifffsz_ = true;
}

void EventDecayedTradeInfoManager::compute_lvlsumtdiffnsz() {
  indep_market_view_.subscribe_price_type(this, kPriceTypeMidprice);  // only needed for decaying values
  indep_market_view_.ComputeTradepxMktpxDiff();
  indep_market_view_.ComputeTradeImpact();
  indep_market_view_.SetStorePreTrade();
  computing_lvlsumtdiffnsz_ = true;
}

void EventDecayedTradeInfoManager::DecayCurrentValues() {
  if (computing_sumtdiffsz_) {
    sumtdiffsz_ *= decay_page_factor_;
  }

  if (computing_sumtdiffsqrtsz_) {
    sumtdiffsqrtsz_ *= decay_page_factor_;
  }

  if (computing_sumsz_) {
    sumsz_ *= decay_page_factor_;
  }

  if (computing_sumsqrtsz_) {
    sumsqrtsz_ *= decay_page_factor_;
  }

  if (computing_sumtype_) {
    sumtype_ *= decay_page_factor_;
  }

  if (computing_sumcoeffs_) {
    sumcoeffs_ *= decay_page_factor_;
  }

  if (computing_sumpx_) {
    sumpx_ *= decay_page_factor_;
  }

  if (computing_sumtypesz_) {
    sumtypesz_ *= decay_page_factor_;
  }

  if (computing_sumtypesqrtsz_) {
    sumtypesqrtsz_ *= decay_page_factor_;
  }

  if (computing_sumpxsz_) {
    sumpxsz_ *= decay_page_factor_;
  }

  if (computing_sumpxsqrtsz_) {
    sumpxsqrtsz_ *= decay_page_factor_;
  }
  if (computing_sumtdiffnsz_) {
    sumtdiffnsz_ *= decay_page_factor_;
  }
  if (computing_sumtdifffsz_) {
    sumtdifffsz_ *= decay_page_factor_;
  }
  if (computing_sumtdifffsqrtsz_) {
    sumtdifffsqrtsz_ *= decay_page_factor_;
  }
  if (computing_sumtdiffsqrtfsqrtsz_) {
    sumtdiffsqrtfsqrtsz_ *= decay_page_factor_;
  }
  if (computing_lvlsumtdiffsz_) {
    lvlsumtdiffsz_ *= decay_page_factor_;
  }
  if (computing_lvlsumtdifffsz_) {
    lvlsumtdifffsz_ *= decay_page_factor_;
  }
  if (computing_lvlsumtdiffnsz_) {
    lvlsumtdiffnsz_ *= decay_page_factor_;
  }
}

void EventDecayedTradeInfoManager::InitializeValues() {
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

  lvlsumtdiffsz_ = 0;
  lvlsumtdifffsz_ = 0;
  lvlsumtdiffnsz_ = 0;
}
}
