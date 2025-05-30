/**
    \file IndicatorsCode/time_decayed_trade_info_manager.cpp

    \author: (c) Copyright Two Roads Technological Solutions Pvt Ltd 2011
     Address:
         Suite No 353, Evoma, #14, Bhattarhalli,
         Old Madras Road, Near Garden City College,
         KR Puram, Bangalore 560049, India
         +91 80 4190 3551
 */
#include <sstream>
#include "dvccode/CDef/math_utils.hpp"
#include "baseinfra/TradeUtils/time_decayed_trade_info_manager.hpp"

namespace HFSAT {

TimeDecayedTradeInfoManager* TimeDecayedTradeInfoManager::GetUniqueInstance(DebugLogger& t_dbglogger_,
                                                                            const Watch& r_watch_,
                                                                            SecurityMarketView& _indep_market_view_,
                                                                            double _fractional_seconds_) {
  std::ostringstream t_temp_oss_;
  t_temp_oss_ << VarName() << ' ' << _indep_market_view_.secname() << ' ' << _fractional_seconds_;

  std::string concise_indicator_description_(t_temp_oss_.str());

  static std::map<std::string, TimeDecayedTradeInfoManager*> concise_indicator_description_map_;

  if (concise_indicator_description_map_.find(concise_indicator_description_) ==
      concise_indicator_description_map_.end()) {
    concise_indicator_description_map_[concise_indicator_description_] =
        new TimeDecayedTradeInfoManager(t_dbglogger_, r_watch_, _indep_market_view_, _fractional_seconds_);
  }
  return concise_indicator_description_map_[concise_indicator_description_];
}

TimeDecayedTradeInfoManager::TimeDecayedTradeInfoManager(DebugLogger& t_dbglogger_, const Watch& r_watch_,
                                                         SecurityMarketView& _indep_market_view_,
                                                         double _fractional_seconds_)
    : dbglogger_(t_dbglogger_),
      watch_(r_watch_),
      indep_market_view_(_indep_market_view_),
      trade_history_halflife_msecs_(
          std::max(1,
                   MathUtils::GetCeilMultipleOf((int)round(1000 * _fractional_seconds_), 1))),
      last_new_page_msecs_(0),
      page_width_msecs_(500),
      decay_page_factor_(0.95),
      last_bid_int_price_(0),
      last_ask_int_price_(0),
      min_price_increment_(indep_market_view_.min_price_increment()),
      computing_sumlbtdiffsz_(false),
      computing_sumlbtdiffsz_hvlbtdiff_(false),
      computing_sumsz_hvlbtdiff_(false),
      computing_sumtdiffsz_(false),
      computing_sumtdiffsqrtsz_(false),
      computing_sumsz_(false),
      computing_sumasz_(false),
      computing_sumbsz_(false),
      computing_sumsqrtsz_(false),
      computing_lvlsumsqrtsz_(false),
      computing_sumtype_(false),
      computing_sumcoeffs_(false),
      computing_sumpx_(false),
      computing_sumtypesz_(false),
      computing_sumtypesqrtsz_(false),
      computing_sumpxsz_(false),
      computing_sumpxsqrtsz_(false),
      computing_sumtdiffnsz_(false),
      computing_sumtdifffsz_(false),
      computing_lvlsumtdifffsz_(false),
      computing_onelvlsumtdifffsz_(false),
      computing_lvlsumtdifffszvol_(false),
      computing_sumtdifffsqrtsz_(false),
      computing_sumtdiffsqrtfsqrtsz_(false),
      computing_lvlsumtdiffsz_(false),
      computing_onelvlsumtdiffsz_(false),
      computing_lvlsumtdiffsqrtsz_(false),
      computing_lvlsumsz_(false),
      computing_onelvlsumsz_(false),
      computing_lvlsumtype_(false),
      computing_lvlsumcoeffs_(false),
      computing_onelvlsumcoeffs_(false),
      computing_lvlsumpx_(false),
      computing_onelvlsumpx_(false),
      computing_lvlsumtypesz_(false),
      computing_lvlsumtypesqrtsz_(false),
      computing_lvlsumpxsz_(false),
      computing_onelvlsumpxsz_(false),
      computing_lvlsumpxsqrtsz_(false),
      computing_lvlsumtdiffnsz_(false),
      computing_onelvlsumtdiffnsz_(false),
      computing_lvlsumtdifffsqrtsz_(false),
      computing_lvlsumtdiffsqrtfsqrtsz_(false),
      computing_lvlsumtdiffszvol_(false),
      computing_lvlsumtdiffnszvol_(false),
      computing_sumsignsizepx_(false),
      computing_sumsignszbyl1_(false),
      sumlbtdiffsz_(0),            ///< Sum ( last_book_tdiff_ * size_traded_ )
      sumlbtdiffsz_hvlbtdiff_(0),  ///< Sum ( last_book_tdiff_ * size_traded_ ) for cases when lbtdiff has a high value
      sumsz_hvlbtdiff_(0),         ///< Sum ( size_traded_ ) for cases when lbtdiff has a high value
      sumtdiffsz_(0),              ///< Sum ( tradepx_mktpx_diff_ * size_traded_ )
      sumtdiffsqrtsz_(0),          ///< Sum ( tradepx_mktpx_diff_ * sqrt_size_traded_ )
      sumsz_(0),                   ///< Sum ( size_traded_ )
      sumasz_(0),                  ///< Sum ( size_traded_ / agg_buy )
      sumbsz_(0),                  ///< Sum ( size_traded_ / agg_sell )
      sumsqrtsz_(0),               ///< Sum ( sqrt_size_traded_ )
      lvlsumsqrtsz_(0),
      sumtype_(0),        ///< Sum ( int_trade_type_ )
      sumcoeffs_(0),      ///< Sum ( 1 ) ... used primarily in averaging
      sumpx_(0),          ///< Sum ( trade_price_ )
      sumtypesz_(0),      ///< Sum ( int_trade_type_ * size_traded_ )
      sumtypesqrtsz_(0),  ///< Sum ( int_trade_type_ * sqrt_size_traded_ )
      sumpxsz_(0),        ///< Sum ( trade_price_ * size_traded_ )
      sumpxsqrtsz_(0),    ///< Sum ( trade_price_ * sqrt_size_traded_ )
      sumtdiffnsz_(0),    ///< Sum ( tradepx_mktpx_diff_ * trade_impact_ ) for trade_impact_ refer TradePrintInfo
      sumtdifffsz_(0),    ///< Sum ( tradepx_mktpx_diff_ * trade_impact_ * size_traded_ )
      lvlsumtdifffsz_(0),
      onelvlsumtdifffsz_(0),
      sumtdifffsqrtsz_(0),  ///< Sum ( tradepx_mktpx_diff_ * trade_impact_ * sqrt_size_traded_ )
      sumtdiffsqrtfsqrtsz_(0),
      lvlsumtdiffsz_(0),
      onelvlsumtdiffsz_(0),
      lvlsumtdiffsqrtsz_(0),
      lvlsumsz_(0),
      lvlsumtype_(0),
      lvlsumcoeffs_(0),
      onelvlsumcoeffs_(0),
      lvlsumpx_(0),
      onelvlsumpx_(0),
      lvlsumtypesz_(0),
      lvlsumtypesqrtsz_(0),
      lvlsumpxsz_(0),
      lvlsumpxsqrtsz_(0),
      lvlsumtdiffnsz_(0),
      onelvlsumtdiffnsz_(0),
      lvlsumtdifffsqrtsz_(0),
      lvlsumtdiffsqrtfsqrtsz_(0),
      onelvlsumsz_(0),
      onelvlsumpxsz_(0),
      onelvlmaxsumsz_(0),
      sumsignsizepx_(0) {
  watch_.subscribe_TimePeriod(this);

  SetTimeDecayWeights();
  InitializeValues();

  indep_market_view_.subscribe_tradeprints(this);

  /// this is done to get a constant source of pings to be able to decay values ...
  /// If this does not improve values dramatically
  /// then we can think about removing this
  indep_market_view_.subscribe_price_type(this, kPriceTypeMktSizeWPrice);
}

inline void TimeDecayedTradeInfoManager::AdjustLevelVars() {
  if (computing_lvlsumtdiffsz_) {
    lvlsumtdiffsz_ /= 2.0;
  }

  if (computing_onelvlsumtdiffsz_) {
    onelvlsumtdiffsz_ = 0;
  }

  if (computing_lvlsumtdiffsqrtsz_) {
    lvlsumtdiffsqrtfsqrtsz_ /= 2.0;
  }

  if (computing_lvlsumsz_) {
    lvlsumsz_ /= 2.0;
  }

  if (computing_onelvlsumsz_) {
    onelvlsumsz_ = 0.0;
    onelvlmaxsumsz_ = 0.0;
  }

  if (computing_lvlsumsqrtsz_) {
    lvlsumsqrtsz_ /= 2.0;
  }

  if (computing_lvlsumtype_) {
    lvlsumtype_ /= 2.0;
  }
  if (computing_lvlsumcoeffs_) {
    lvlsumcoeffs_ /= 2.0;
  }

  if (computing_onelvlsumcoeffs_) {
    onelvlsumcoeffs_ = 0;
  }

  if (computing_lvlsumpx_) {
    lvlsumpx_ /= 2.0;
  }

  if (computing_onelvlsumpx_) {
    onelvlsumpx_ = 0;
  }

  if (computing_lvlsumtypesz_) {
    lvlsumtypesz_ /= 2.0;
  }
  if (computing_lvlsumtypesqrtsz_) {
    lvlsumtypesqrtsz_ /= 2.0;
  }
  if (computing_lvlsumpxsz_) {
    lvlsumpxsz_ /= 2.0;
  }
  if (computing_onelvlsumpxsz_) {
    onelvlsumpxsz_ = 0.0;
  }
  if (computing_lvlsumpxsqrtsz_) {
    lvlsumpxsqrtsz_ /= 2.0;
  }

  if (computing_lvlsumtdiffnsz_) {
    lvlsumtdiffnsz_ /= 2.0;
  }
  if (computing_onelvlsumtdiffnsz_) {
    onelvlsumtdiffnsz_ = 0;
  }

  if (computing_lvlsumtdifffsz_) {
    lvlsumtdifffsz_ /= 2.0;
  }
  if (computing_onelvlsumtdifffsz_) {
    onelvlsumtdifffsz_ = 0;
  }

  if (computing_lvlsumtdifffsqrtsz_) {
    lvlsumtdifffsqrtsz_ /= 2.0;
  }
  if (computing_lvlsumtdiffsqrtfsqrtsz_) {
    lvlsumtdiffsqrtfsqrtsz_ /= 2.0;
  }

}

void TimeDecayedTradeInfoManager::OnTimePeriodUpdate(const int num_pages_to_add_) { DecayCurrentValues(); }

void TimeDecayedTradeInfoManager::OnMarketUpdate(const unsigned int _security_id_,
                                                 const MarketUpdateInfo& _market_update_info_) {
  DecayCurrentValues();

  if ((last_bid_int_price_ != _market_update_info_.bestbid_int_price_) ||
      (last_ask_int_price_ != _market_update_info_.bestask_int_price_)) {
    AdjustLevelVars();
    last_bid_int_price_ = _market_update_info_.bestbid_int_price_;
    last_ask_int_price_ = _market_update_info_.bestask_int_price_;
  }
}

void TimeDecayedTradeInfoManager::OnTradePrint(const unsigned int _security_id_,
                                               const TradePrintInfo& _trade_print_info_,
                                               const MarketUpdateInfo& _market_update_info_) {
  DecayCurrentValues();

  if ((last_bid_int_price_ != _market_update_info_.bestbid_int_price_) ||
      (last_ask_int_price_ != _market_update_info_.bestask_int_price_)) {
    AdjustLevelVars();
    last_bid_int_price_ = _market_update_info_.bestbid_int_price_;
    last_ask_int_price_ = _market_update_info_.bestask_int_price_;
  }

  if (computing_sumlbtdiffsz_) {
    sumlbtdiffsz_ += _trade_print_info_.last_book_tdiff_ * _trade_print_info_.size_traded_;
  }

  if (computing_sumlbtdiffsz_hvlbtdiff_ || computing_sumsz_hvlbtdiff_) {
    if ((fabs(_trade_print_info_.last_book_tdiff_) > (0.25 * min_price_increment_)) &&
        (fabs(_trade_print_info_.last_book_tdiff_) < (2.25 * min_price_increment_))) {
      sumlbtdiffsz_hvlbtdiff_ += _trade_print_info_.last_book_tdiff_ * _trade_print_info_.size_traded_;
      sumsz_hvlbtdiff_ += _trade_print_info_.size_traded_;
    }
  }

  if (computing_sumtdiffsz_) {
    sumtdiffsz_ += _trade_print_info_.tradepx_mktpx_diff_ * _trade_print_info_.size_traded_;
  }
  if (computing_lvlsumtdiffsz_) {
    lvlsumtdiffsz_ += _trade_print_info_.tradepx_mktpx_diff_ * _trade_print_info_.size_traded_;
  }
  if (computing_onelvlsumtdiffsz_) {
    onelvlsumtdiffsz_ += _trade_print_info_.tradepx_mktpx_diff_ * _trade_print_info_.size_traded_;
  }

  if (computing_sumtdiffsqrtsz_) {
    sumtdiffsqrtsz_ += _trade_print_info_.tradepx_mktpx_diff_ * _trade_print_info_.sqrt_size_traded_;
  }

  if (computing_sumsz_) {
    sumsz_ += _trade_print_info_.size_traded_;
  }

  if (computing_sumasz_ && _trade_print_info_.buysell_ == kTradeTypeBuy) {
    sumasz_ += _trade_print_info_.size_traded_;
  }
  if (computing_sumbsz_ && _trade_print_info_.buysell_ == kTradeTypeSell) {
    sumbsz_ += _trade_print_info_.size_traded_;
  }

  if (computing_sumsqrtsz_) {
    sumsqrtsz_ += _trade_print_info_.sqrt_size_traded_;
  }
  if (computing_lvlsumsqrtsz_) {
    lvlsumsqrtsz_ += _trade_print_info_.sqrt_size_traded_;
  }

  if (computing_sumtype_) {
    sumtype_ += _trade_print_info_.int_trade_type_;
  }

  if (computing_sumcoeffs_) {
    sumcoeffs_ += 1.0;
  }
  if (computing_lvlsumcoeffs_) {
    lvlsumcoeffs_ += 1.0;
  }
  if (computing_onelvlsumcoeffs_) {
    onelvlsumcoeffs_ += 1.0;
  }

  if (computing_sumpx_) {
    sumpx_ += _trade_print_info_.trade_price_;
  }
  if (computing_lvlsumpx_) {
    lvlsumpx_ += _trade_print_info_.trade_price_;
  }
  if (computing_onelvlsumpx_) {
    onelvlsumpx_ += _trade_print_info_.trade_price_;
  }

  if (computing_sumtypesz_) {
    sumtypesz_ += _trade_print_info_.int_trade_type_ * _trade_print_info_.size_traded_;
  }

  if (computing_sumtypesqrtsz_) {
    sumtypesqrtsz_ += _trade_print_info_.int_trade_type_ * _trade_print_info_.sqrt_size_traded_;
  }

  if (computing_sumpxsz_) {
    sumpxsz_ += _trade_print_info_.trade_price_ * _trade_print_info_.size_traded_;
  }

  if (computing_sumpxsqrtsz_) {
    sumpxsqrtsz_ += _trade_print_info_.trade_price_ * _trade_print_info_.sqrt_size_traded_;
  }

  if (computing_sumtdiffnsz_) {
    sumtdiffnsz_ += (_trade_print_info_.tradepx_mktpx_diff_ * _trade_print_info_.trade_impact_);
  }

  if (computing_sumtdifffsz_) {
    sumtdifffsz_ +=
        (_trade_print_info_.tradepx_mktpx_diff_ * _trade_print_info_.trade_impact_ * _trade_print_info_.size_traded_);
  }
  if (computing_lvlsumtdifffsz_) {
    lvlsumtdifffsz_ +=
        (_trade_print_info_.tradepx_mktpx_diff_ * _trade_print_info_.trade_impact_ * _trade_print_info_.size_traded_);
  }
  if (computing_onelvlsumtdifffsz_) {
    onelvlsumtdifffsz_ +=
        (_trade_print_info_.tradepx_mktpx_diff_ * _trade_print_info_.trade_impact_ * _trade_print_info_.size_traded_);
  }

  if (computing_sumtdifffsqrtsz_) {
    sumtdifffsqrtsz_ += (_trade_print_info_.tradepx_mktpx_diff_ * _trade_print_info_.trade_impact_ *
                         _trade_print_info_.sqrt_size_traded_);
  }

  if (computing_sumtdiffsqrtfsqrtsz_) {
    sumtdiffsqrtfsqrtsz_ += (_trade_print_info_.tradepx_mktpx_diff_ * _trade_print_info_.sqrt_trade_impact_ *
                             _trade_print_info_.sqrt_size_traded_);
  }

  if (computing_lvlsumtdiffsqrtsz_) {
    lvlsumtdiffsqrtsz_ += _trade_print_info_.tradepx_mktpx_diff_ * _trade_print_info_.sqrt_size_traded_;
  }

  if (computing_lvlsumsz_) {
    lvlsumsz_ += _trade_print_info_.size_traded_;
  }

  if (computing_onelvlsumsz_) {
    onelvlsumsz_ += _trade_print_info_.size_traded_;
    if (onelvlmaxsumsz_ < onelvlsumsz_) onelvlmaxsumsz_ = onelvlsumsz_;
  }

  if (computing_lvlsumtype_) {
    lvlsumtype_ += _trade_print_info_.int_trade_type_;
  }

  if (computing_lvlsumtypesz_) {
    lvlsumtypesz_ += _trade_print_info_.int_trade_type_ * _trade_print_info_.size_traded_;
  }

  if (computing_lvlsumtypesqrtsz_) {
    lvlsumtypesqrtsz_ += _trade_print_info_.int_trade_type_ * _trade_print_info_.sqrt_size_traded_;
  }

  if (computing_lvlsumpxsz_) {
    lvlsumpxsz_ += _trade_print_info_.trade_price_ * _trade_print_info_.size_traded_;
  }

  if (computing_onelvlsumpxsz_) {
    onelvlsumpxsz_ += _trade_print_info_.trade_price_ * _trade_print_info_.size_traded_;
  }

  if (computing_lvlsumpxsqrtsz_) {
    lvlsumpxsqrtsz_ += _trade_print_info_.trade_price_ * _trade_print_info_.sqrt_size_traded_;
  }

  if (computing_lvlsumtdiffnsz_) {
    lvlsumtdiffnsz_ += (_trade_print_info_.tradepx_mktpx_diff_ * _trade_print_info_.trade_impact_);
  }

  if (computing_onelvlsumtdiffnsz_) {
    onelvlsumtdiffnsz_ += (_trade_print_info_.tradepx_mktpx_diff_ * _trade_print_info_.trade_impact_);
  }

  if (computing_lvlsumtdifffsqrtsz_) {
    lvlsumtdifffsqrtsz_ += (_trade_print_info_.tradepx_mktpx_diff_ * _trade_print_info_.trade_impact_ *
                            _trade_print_info_.sqrt_size_traded_);
  }

  if (computing_lvlsumtdiffsqrtfsqrtsz_) {
    lvlsumtdiffsqrtfsqrtsz_ += (_trade_print_info_.tradepx_mktpx_diff_ * _trade_print_info_.sqrt_trade_impact_ *
                                _trade_print_info_.sqrt_size_traded_);
  }


  if (computing_sumsignsizepx_) {
    sumsignsizepx_ +=
        _trade_print_info_.int_trade_type_ * _trade_print_info_.size_traded_ * _trade_print_info_.trade_price_;
  }

}

void TimeDecayedTradeInfoManager::compute_sumlbtdiffsz() {
  indep_market_view_.ComputeLBTDiff();
  computing_sumlbtdiffsz_ = true;
}

void TimeDecayedTradeInfoManager::compute_sumlbtdiffsz_hvlbtdiff() {
  indep_market_view_.ComputeLBTDiff();
  computing_sumlbtdiffsz_hvlbtdiff_ = true;
}

void TimeDecayedTradeInfoManager::compute_sumsz_hvlbtdiff() {
  indep_market_view_.ComputeLBTDiff();
  computing_sumsz_hvlbtdiff_ = true;
}

void TimeDecayedTradeInfoManager::compute_sumtdiffsz() {
  indep_market_view_.ComputeTradepxMktpxDiff();
  computing_sumtdiffsz_ = true;
}
void TimeDecayedTradeInfoManager::compute_lvlsumtdiffsz() {
  indep_market_view_.ComputeTradepxMktpxDiff();
  computing_lvlsumtdiffsz_ = true;
}
void TimeDecayedTradeInfoManager::compute_onelvlsumtdiffsz() {
  indep_market_view_.ComputeTradepxMktpxDiff();
  computing_onelvlsumtdiffsz_ = true;
}

void TimeDecayedTradeInfoManager::compute_sumtdiffsqrtsz() {
  indep_market_view_.ComputeTradepxMktpxDiff();
  indep_market_view_.ComputeSqrtSizeTraded();
  computing_sumtdiffsqrtsz_ = true;
}

void TimeDecayedTradeInfoManager::compute_sumsz() { computing_sumsz_ = true; }

void TimeDecayedTradeInfoManager::compute_sumasz() { computing_sumasz_ = true; }

void TimeDecayedTradeInfoManager::compute_sumbsz() { computing_sumbsz_ = true; }

void TimeDecayedTradeInfoManager::compute_sumsqrtsz() {
  indep_market_view_.ComputeSqrtSizeTraded();
  computing_sumsqrtsz_ = true;
}
void TimeDecayedTradeInfoManager::compute_lvlsumsqrtsz() {
  indep_market_view_.ComputeSqrtSizeTraded();
  computing_lvlsumsqrtsz_ = true;
}

void TimeDecayedTradeInfoManager::compute_sumtype() {
  indep_market_view_.ComputeIntTradeType();
  computing_sumtype_ = true;
}

void TimeDecayedTradeInfoManager::compute_sumcoeffs() { computing_sumcoeffs_ = true; }

void TimeDecayedTradeInfoManager::compute_sumpx() { computing_sumpx_ = true; }

void TimeDecayedTradeInfoManager::compute_sumtypesz() {
  indep_market_view_.ComputeIntTradeType();
  computing_sumtypesz_ = true;
}

void TimeDecayedTradeInfoManager::compute_sumtypesqrtsz() {
  indep_market_view_.ComputeIntTradeType();
  indep_market_view_.ComputeSqrtSizeTraded();
  computing_sumtypesqrtsz_ = true;
}

void TimeDecayedTradeInfoManager::compute_sumpxsz() { computing_sumpxsz_ = true; }

void TimeDecayedTradeInfoManager::compute_sumpxsqrtsz() {
  indep_market_view_.ComputeSqrtSizeTraded();
  computing_sumpxsqrtsz_ = true;
}

void TimeDecayedTradeInfoManager::compute_sumtdiffnsz() {
  indep_market_view_.ComputeTradepxMktpxDiff();
  indep_market_view_.ComputeTradeImpact();
  computing_sumtdiffnsz_ = true;
}

void TimeDecayedTradeInfoManager::compute_sumtdifffsz() {
  indep_market_view_.ComputeTradepxMktpxDiff();
  indep_market_view_.ComputeTradeImpact();
  computing_sumtdifffsz_ = true;
}
void TimeDecayedTradeInfoManager::compute_lvlsumtdifffsz() {
  indep_market_view_.ComputeTradepxMktpxDiff();
  indep_market_view_.ComputeTradeImpact();
  computing_lvlsumtdifffsz_ = true;
}

void TimeDecayedTradeInfoManager::compute_onelvlsumtdifffsz() {
  indep_market_view_.ComputeTradepxMktpxDiff();
  indep_market_view_.ComputeTradeImpact();
  computing_onelvlsumtdifffsz_ = true;
}

void TimeDecayedTradeInfoManager::compute_sumtdifffsqrtsz() {
  indep_market_view_.ComputeTradepxMktpxDiff();
  indep_market_view_.ComputeSqrtSizeTraded();
  indep_market_view_.ComputeTradeImpact();
  computing_sumtdifffsqrtsz_ = true;
}

void TimeDecayedTradeInfoManager::compute_sumtdiffsqrtfsqrtsz() {
  indep_market_view_.ComputeTradepxMktpxDiff();
  indep_market_view_.ComputeSqrtSizeTraded();
  indep_market_view_.ComputeSqrtTradeImpact();
  computing_sumtdiffsqrtfsqrtsz_ = true;
}

void TimeDecayedTradeInfoManager::compute_lvlsumtdiffsqrtsz() {
  indep_market_view_.ComputeTradepxMktpxDiff();
  indep_market_view_.ComputeSqrtSizeTraded();
  computing_lvlsumtdiffsqrtsz_ = true;
}
void TimeDecayedTradeInfoManager::compute_lvlsumsz() { computing_lvlsumsz_ = true; }
void TimeDecayedTradeInfoManager::compute_onelvlsumsz() { computing_onelvlsumsz_ = true; }

void TimeDecayedTradeInfoManager::compute_lvlsumtype() {
  indep_market_view_.ComputeIntTradeType();
  computing_lvlsumtype_ = true;
}

void TimeDecayedTradeInfoManager::compute_lvlsumcoeffs() { computing_lvlsumcoeffs_ = true; }

void TimeDecayedTradeInfoManager::compute_onelvlsumcoeffs() { computing_onelvlsumcoeffs_ = true; }

void TimeDecayedTradeInfoManager::compute_lvlsumpx() { computing_lvlsumpx_ = true; }

void TimeDecayedTradeInfoManager::compute_onelvlsumpx() { computing_onelvlsumpx_ = true; }

void TimeDecayedTradeInfoManager::compute_lvlsumtypesz() {
  indep_market_view_.ComputeIntTradeType();
  computing_lvlsumtypesz_ = true;
}
void TimeDecayedTradeInfoManager::compute_lvlsumtypesqrtsz() {
  indep_market_view_.ComputeIntTradeType();
  indep_market_view_.ComputeSqrtSizeTraded();
  computing_lvlsumtypesqrtsz_ = true;
}
void TimeDecayedTradeInfoManager::compute_lvlsumpxsz() { computing_lvlsumpxsz_ = true; }
void TimeDecayedTradeInfoManager::compute_onelvlsumpxsz() { computing_onelvlsumpxsz_ = true; }
void TimeDecayedTradeInfoManager::compute_lvlsumpxsqrtsz() {
  indep_market_view_.ComputeSqrtSizeTraded();
  computing_lvlsumpxsqrtsz_ = true;
}

void TimeDecayedTradeInfoManager::compute_lvlsumtdiffnsz() {
  indep_market_view_.ComputeTradepxMktpxDiff();
  indep_market_view_.ComputeTradeImpact();
  computing_lvlsumtdiffnsz_ = true;
}

void TimeDecayedTradeInfoManager::compute_onelvlsumtdiffnsz() {
  indep_market_view_.ComputeTradepxMktpxDiff();
  indep_market_view_.ComputeTradeImpact();
  computing_onelvlsumtdiffnsz_ = true;
}

void TimeDecayedTradeInfoManager::compute_lvlsumtdifffsqrtsz() {
  indep_market_view_.ComputeTradepxMktpxDiff();
  indep_market_view_.ComputeSqrtSizeTraded();
  indep_market_view_.ComputeTradeImpact();
  computing_lvlsumtdifffsqrtsz_ = true;
}

void TimeDecayedTradeInfoManager::compute_lvlsumtdiffsqrtfsqrtsz() {
  indep_market_view_.ComputeTradepxMktpxDiff();
  indep_market_view_.ComputeSqrtSizeTraded();
  indep_market_view_.ComputeSqrtTradeImpact();
  computing_lvlsumtdiffsqrtfsqrtsz_ = true;
}

void TimeDecayedTradeInfoManager::compute_sumsignsizepx() {
  indep_market_view_.ComputeIntTradeType();
  computing_sumsignsizepx_ = true;
}

void TimeDecayedTradeInfoManager::compute_sumsignszbyl1() {
  indep_market_view_.ComputeIntTradeType();
  computing_sumsignszbyl1_ = true;
}

void TimeDecayedTradeInfoManager::DecayCurrentValues() {
  int num_pages_to_add_ = 0;
  if (watch_.msecs_from_midnight() - last_new_page_msecs_ > page_width_msecs_) {
    num_pages_to_add_ = (int)floor((watch_.msecs_from_midnight() - last_new_page_msecs_) / page_width_msecs_);

    if (num_pages_to_add_ >= (int)decay_vector_.size()) {
      InitializeValues();
    } else {
      if (computing_sumlbtdiffsz_) {
        sumlbtdiffsz_ *= decay_vector_[num_pages_to_add_];
      }
      if (computing_sumlbtdiffsz_hvlbtdiff_) {
        sumlbtdiffsz_hvlbtdiff_ *= decay_vector_[num_pages_to_add_];
      }
      if (computing_sumsz_hvlbtdiff_) {
        sumsz_hvlbtdiff_ *= decay_vector_[num_pages_to_add_];
      }
      if (computing_sumtdiffsz_) {
        sumtdiffsz_ *= decay_vector_[num_pages_to_add_];
      }
      if (computing_lvlsumtdiffsz_) {
        lvlsumtdiffsz_ *= decay_vector_[num_pages_to_add_];
      }
      if (computing_onelvlsumtdiffsz_) {
        onelvlsumtdiffsz_ *= decay_vector_[num_pages_to_add_];
      }
      if (computing_sumtdiffsqrtsz_) {
        sumtdiffsqrtsz_ *= decay_vector_[num_pages_to_add_];
      }
      if (computing_sumsz_) {
        sumsz_ *= decay_vector_[num_pages_to_add_];
      }
      if (computing_sumasz_) {
        sumasz_ *= decay_vector_[num_pages_to_add_];
      }
      if (computing_sumbsz_) {
        sumbsz_ *= decay_vector_[num_pages_to_add_];
      }
      if (computing_sumsqrtsz_) {
        sumsqrtsz_ *= decay_vector_[num_pages_to_add_];
      }
      if (computing_lvlsumsqrtsz_) {
        lvlsumsqrtsz_ *= decay_vector_[num_pages_to_add_];
      }
      if (computing_sumtype_) {
        sumtype_ *= decay_vector_[num_pages_to_add_];
      }
      if (computing_sumcoeffs_) {
        sumcoeffs_ *= decay_vector_[num_pages_to_add_];
      }
      if (computing_sumpx_) {
        sumpx_ *= decay_vector_[num_pages_to_add_];
      }
      if (computing_sumtypesz_) {
        sumtypesz_ *= decay_vector_[num_pages_to_add_];
      }
      if (computing_sumtypesqrtsz_) {
        sumtypesqrtsz_ *= decay_vector_[num_pages_to_add_];
      }
      if (computing_sumpxsz_) {
        sumpxsz_ *= decay_vector_[num_pages_to_add_];
      }
      if (computing_sumpxsqrtsz_) {
        sumpxsqrtsz_ *= decay_vector_[num_pages_to_add_];
      }
      if (computing_sumtdiffnsz_) {
        sumtdiffnsz_ *= decay_vector_[num_pages_to_add_];
      }
      if (computing_sumtdifffsz_) {
        sumtdifffsz_ *= decay_vector_[num_pages_to_add_];
      }
      if (computing_lvlsumtdifffsz_) {
        lvlsumtdifffsz_ *= decay_vector_[num_pages_to_add_];
      }
      if (computing_onelvlsumtdifffsz_) {
        onelvlsumtdifffsz_ *= decay_vector_[num_pages_to_add_];
      }

      if (computing_sumtdifffsqrtsz_) {
        sumtdifffsqrtsz_ *= decay_vector_[num_pages_to_add_];
      }
      if (computing_sumtdiffsqrtfsqrtsz_) {
        sumtdiffsqrtfsqrtsz_ *= decay_vector_[num_pages_to_add_];
      }

      if (computing_lvlsumtdiffsqrtsz_) {
        lvlsumtdiffsqrtsz_ *= decay_vector_[num_pages_to_add_];
      }
      if (computing_lvlsumsz_) {
        lvlsumsz_ *= decay_vector_[num_pages_to_add_];
      }
      if (computing_lvlsumtype_) {
        lvlsumtype_ *= decay_vector_[num_pages_to_add_];
      }
      if (computing_lvlsumcoeffs_) {
        lvlsumcoeffs_ *= decay_vector_[num_pages_to_add_];
      }
      if (computing_onelvlsumcoeffs_) {
        onelvlsumcoeffs_ *= decay_vector_[num_pages_to_add_];
      }
      if (computing_lvlsumpx_) {
        lvlsumpx_ *= decay_vector_[num_pages_to_add_];
      }
      if (computing_onelvlsumpx_) {
        onelvlsumpx_ *= decay_vector_[num_pages_to_add_];
      }
      if (computing_lvlsumtypesz_) {
        lvlsumtypesz_ *= decay_vector_[num_pages_to_add_];
      }
      if (computing_lvlsumtypesqrtsz_) {
        lvlsumtypesqrtsz_ *= decay_vector_[num_pages_to_add_];
      }
      if (computing_lvlsumpxsz_) {
        lvlsumpxsz_ *= decay_vector_[num_pages_to_add_];
      }
      if (computing_lvlsumpxsqrtsz_) {
        lvlsumpxsqrtsz_ *= decay_vector_[num_pages_to_add_];
      }
      if (computing_lvlsumtdiffnsz_) {
        lvlsumtdiffnsz_ *= decay_vector_[num_pages_to_add_];
      }
      if (computing_onelvlsumtdiffnsz_) {
        onelvlsumtdiffnsz_ *= decay_vector_[num_pages_to_add_];
      }
      if (computing_lvlsumtdifffsqrtsz_) {
        lvlsumtdifffsqrtsz_ *= decay_vector_[num_pages_to_add_];
      }
      if (computing_lvlsumtdiffsqrtfsqrtsz_) {
        lvlsumtdiffsqrtfsqrtsz_ *= decay_vector_[num_pages_to_add_];
      }

      if (computing_onelvlsumpxsz_) {
        onelvlsumpxsz_ *= decay_vector_[num_pages_to_add_];
      }
      if (computing_onelvlsumsz_) {
        onelvlsumsz_ *= decay_vector_[num_pages_to_add_];
      }

      if (computing_sumsignsizepx_) {
        sumsignsizepx_ *= decay_vector_[num_pages_to_add_];
      }

      last_new_page_msecs_ += (num_pages_to_add_ * page_width_msecs_);
    }
  }
}

void TimeDecayedTradeInfoManager::SetTimeDecayWeights() {
  const unsigned int kDecayLength =
      100;  ///< here number of samples are not required to be very high and hence the decaylength target is just 20
  const unsigned int kMinPageWidth =
      5;  ///< lower than normal since trades come bunched up and newer ones might be more important
  const unsigned int kMaxPageWidth =
      100;  ///< keeping kMaxPageWidth low makes the number_fadeoffs_ pretty high and hence keeps lots of sample points
  page_width_msecs_ = std::min(kMaxPageWidth, std::max(kMinPageWidth, (trade_history_halflife_msecs_ / kDecayLength)));

  int number_fadeoffs_ = std::max(1, (int)ceil(trade_history_halflife_msecs_ / page_width_msecs_));

  decay_page_factor_ = MathUtils::CalcDecayFactor(number_fadeoffs_);

  decay_vector_.resize(2 * number_fadeoffs_);

  for (auto i = 0u; i < decay_vector_.size(); i++) {
    decay_vector_[i] = pow(decay_page_factor_, (int)i);
  }
}

void TimeDecayedTradeInfoManager::InitializeValues() {
  sumlbtdiffsz_ = 0;
  sumlbtdiffsz_hvlbtdiff_ = 0;
  sumsz_hvlbtdiff_ = 0;
  sumtdiffsz_ = 0;
  sumtdiffsqrtsz_ = 0;
  sumsz_ = 0;
  sumasz_ = 0;
  sumbsz_ = 0;
  sumsqrtsz_ = 0;
  lvlsumsqrtsz_ = 0;
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
  onelvlsumtdiffsz_ = 0;
  lvlsumtdiffsqrtsz_ = 0;
  lvlsumsz_ = 0;
  lvlsumtype_ = 0;
  lvlsumcoeffs_ = 0;
  onelvlsumcoeffs_ = 0;
  lvlsumpx_ = 0;
  onelvlsumpx_ = 0;
  lvlsumtypesz_ = 0;
  lvlsumtypesqrtsz_ = 0;
  lvlsumpxsz_ = 0;
  lvlsumpxsqrtsz_ = 0;
  lvlsumtdiffnsz_ = 0;
  onelvlsumtdiffnsz_ = 0;
  lvlsumtdifffsz_ = 0;
  onelvlsumtdifffsz_ = 0;
  lvlsumtdifffsqrtsz_ = 0;
  lvlsumtdiffsqrtfsqrtsz_ = 0;
  onelvlsumsz_ = 0;
  onelvlsumpxsz_ = 0;
  sumsignsizepx_ = 0;
  last_new_page_msecs_ = watch_.msecs_from_midnight() - watch_.msecs_from_midnight() % page_width_msecs_;
}
}
