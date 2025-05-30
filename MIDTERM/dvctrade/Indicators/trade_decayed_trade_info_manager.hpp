/**
    \file Indicators/trade_decayed_trade_info_manager.hpp

    \author: (c) Copyright Two Roads Technological Solutions Pvt Ltd 2011
     Address:
         Suite No 353, Evoma, #14, Bhattarhalli,
         Old Madras Road, Near Garden City College,
         KR Puram, Bangalore 560049, India
         +91 80 4190 3551
 */
#ifndef BASE_INDICATORS_TRADE_DECAYED_TRADE_INFO_MANAGER_H
#define BASE_INDICATORS_TRADE_DECAYED_TRADE_INFO_MANAGER_H

#include <map>
#include "baseinfra/MarketAdapter/security_market_view.hpp"

namespace HFSAT {

/// Indicator subcomputation Class that takes a decay factor, or rather a
/// _num_trades_halflife_ ( i.e. number of trades to fadeoff ),
/// and encapsulates the logic to compute trade based variables
/// like trade decayed sum of tradepx - mktpx.
/// Better than writing the logic in that indicator when
/// subcomputations of two required indicators is common.
/// single repository for all the logic.
/// multiple calls to the same variable.
class TradeDecayedTradeInfoManager : public SecurityMarketViewChangeListener {
 protected:
  DebugLogger& dbglogger_;
  const Watch& watch_;

  SecurityMarketView& indep_market_view_;
  const unsigned int trade_history_halflife_trades_;

  const double decay_page_factor_;

  // the following are used only for bid and ask decaying
  int last_bid_int_price_;
  int last_ask_int_price_;

  bool computing_sumtdiffsz_;
  bool computing_sumtdiffsqrtsz_;
  bool computing_sumsz_;
  bool computing_sumsqrtsz_;
  bool computing_sumtype_;
  bool computing_sumcoeffs_;
  bool computing_sumpx_;
  bool computing_sumtypesz_;
  bool computing_sumtypesqrtsz_;
  bool computing_sumpxsz_;
  bool computing_sumpxsqrtsz_;
  bool computing_sumtdiffnsz_;
  bool computing_sumtdifffsz_;
  bool computing_sumtdifffsqrtsz_;
  bool computing_sumtdiffsqrtfsqrtsz_;

  bool computing_lvlsumtdiffsz_;
  bool computing_lvlsumtdiffnsz_;
  bool computing_lvlsumtdifffsz_;

 public:
  double sumtdiffsz_;           ///< Sum ( tradepx_mktpx_diff_ * size_traded_ )
  double sumtdiffsqrtsz_;       ///< Sum ( tradepx_mktpx_diff_ * sqrt_size_traded_ )
  double sumsz_;                ///< Sum ( size_traded_ )
  double sumsqrtsz_;            ///< Sum ( sqrt_size_traded_ )
  double sumtype_;              ///< Sum ( int_trade_type_ )
  double sumcoeffs_;            ///< Sum ( 1 ) ... used primarily in averaging
  double sumpx_;                ///< Sum ( trade_price_ )
  double sumtypesz_;            ///< Sum ( int_trade_type_ * size_traded_ )
  double sumtypesqrtsz_;        ///< Sum ( int_trade_type_ * sqrt_size_traded_ )
  double sumpxsz_;              ///< Sum ( trade_price_ * size_traded_ )
  double sumpxsqrtsz_;          ///< Sum ( trade_price_ * sqrt_size_traded_ )
  double sumtdiffnsz_;          ///< Sum ( tradepx_mktpx_diff_ * trade_impact_ ) for trade_impact_ refer TradePrintInfo
  double sumtdifffsz_;          ///< Sum ( tradepx_mktpx_diff_ * trade_impact_ * size_traded_ )
  double sumtdifffsqrtsz_;      ///< Sum ( tradepx_mktpx_diff_ * trade_impact_ * sqrt_size_traded_ )
  double sumtdiffsqrtfsqrtsz_;  ///< Sum ( tradepx_mktpx_diff_ * sqrt_trade_impact_ * sqrt_size_traded_ )

  // set of variables that are decayed sharply when the best level changes
  double lvlsumtdiffsz_;  ///< Sum ( tradepx_mktpx_diff_ * size_traded_ ), same as sumtdiffsz_ except the value is
  /// decayed by 1/2.0 when the mid_price changes

  double lvlsumtdiffnsz_;  ///< Sum ( tradepx_mktpx_diff_ * trade_impact_ ), same as sumtdiffnsz_ except the value is
  /// decayed by 1/2.0 when the mid_price changes

  double lvlsumtdifffsz_;  ///< Sum ( tradepx_mktpx_diff_ * trade_impact_ * size_traded_), same as sumtdifffsz_ except
                           /// the value is decayed by 1/2.0 when the mid_price changes

 protected:
  TradeDecayedTradeInfoManager(DebugLogger& _dbglogger_, const Watch& _watch_, SecurityMarketView& _indep_market_view_,
                               unsigned int _num_trades_halflife_);

 public:
  static TradeDecayedTradeInfoManager* GetUniqueInstance(DebugLogger& _dbglogger_, const Watch& _watch_,
                                                         SecurityMarketView& _indep_market_view_,
                                                         unsigned int _num_trades_halflife_);

  void compute_sumtdiffsz();
  void compute_sumtdiffsqrtsz();
  void compute_sumsz();
  void compute_sumsqrtsz();
  void compute_sumtype();
  void compute_sumcoeffs();
  void compute_sumpx();
  void compute_sumtypesz();
  void compute_sumtypesqrtsz();
  void compute_sumpxsz();
  void compute_sumpxsqrtsz();
  void compute_sumtdiffnsz();
  void compute_sumtdifffsz();
  void compute_sumtdifffsqrtsz();
  void compute_sumtdiffsqrtfsqrtsz();
  void compute_lvlsumtdiffsz();
  void compute_lvlsumtdiffnsz();
  void compute_lvlsumtdifffsz();

  inline void OnMarketUpdate(const unsigned int _security_id_, const MarketUpdateInfo& _market_update_info_) {
    if ((last_bid_int_price_ != _market_update_info_.bestbid_int_price_) ||
        (last_ask_int_price_ != _market_update_info_.bestask_int_price_)) {
      AdjustLevelVars();
      last_bid_int_price_ = _market_update_info_.bestbid_int_price_;
      last_ask_int_price_ = _market_update_info_.bestask_int_price_;
    }
  }

  inline void OnTradePrint(const unsigned int _security_id_, const TradePrintInfo& cr_trade_print_info_,
                           const MarketUpdateInfo& cr_market_update_info_) {
    DecayCurrentValues();

    if ((last_bid_int_price_ != cr_market_update_info_.bestbid_int_price_) ||
        (last_ask_int_price_ != cr_market_update_info_.bestask_int_price_)) {
      AdjustLevelVars();
      last_bid_int_price_ = cr_market_update_info_.bestbid_int_price_;
      last_ask_int_price_ = cr_market_update_info_.bestask_int_price_;
    }

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
      lvlsumtdiffsz_ += cr_trade_print_info_.tradepx_mktpx_diff_ * cr_trade_print_info_.size_traded_;
    }

    if (computing_lvlsumtdifffsz_) {
      lvlsumtdifffsz_ += (cr_trade_print_info_.tradepx_mktpx_diff_ * cr_trade_print_info_.trade_impact_ *
                          cr_trade_print_info_.size_traded_);
    }

    if (computing_lvlsumtdiffnsz_) {
      lvlsumtdiffnsz_ += (cr_trade_print_info_.tradepx_mktpx_diff_ * cr_trade_print_info_.trade_impact_);
    }
  }

  void InitializeValues();

  // functions
  static std::string VarName() { return "TradeDecayedTradeInfoManager"; }

 protected:
  inline void DecayCurrentValues() {
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

    if (computing_lvlsumtdiffnsz_) {
      lvlsumtdiffnsz_ *= decay_page_factor_;
    }

    if (computing_lvlsumtdifffsz_) {
      lvlsumtdifffsz_ *= decay_page_factor_;
    }
  }

  inline void AdjustLevelVars() {
    if (computing_lvlsumtdiffsz_) {
      // if the level as changed then half the magnitude of prev recorded value
      lvlsumtdiffsz_ /= 2.0;
    }

    if (computing_lvlsumtdiffnsz_) {
      lvlsumtdiffnsz_ /= 2.0;
    }

    if (computing_lvlsumtdifffsz_) {
      lvlsumtdifffsz_ /= 2.0;
    }
  }
};
}

#endif  // BASE_INDICATORS_TRADE_DECAYED_TRADE_INFO_MANAGER_H
