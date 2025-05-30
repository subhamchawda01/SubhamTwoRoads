/**
    \file Indicators/time_decayed_trade_info_manager.hpp

    \author: (c) Copyright Two Roads Technological Solutions Pvt Ltd 2011
     Address:
         Suite No 353, Evoma, #14, Bhattarhalli,
         Old Madras Road, Near Garden City College,
         KR Puram, Bangalore 560049, India
         +91 80 4190 3551
*/
#pragma once

#include <map>
#include "baseinfra/MarketAdapter/security_market_view.hpp"

namespace HFSAT {

/// Class that takes a decay factor, or rather a
/// _fractional_seconds_ time to fadeoff
/// and encapsulates the logic to compute trade based variables
/// like time decayed sum of tradepx - mktpx
/// Better than writing the logic in that indicator when
/// subcomputations of two required indicators is common
/// single repository for all the logic
/// multiple calls to the same variable
class TimeDecayedTradeInfoManager : public SecurityMarketViewChangeListener,
                                    public TimePeriodListener {
 protected:
  DebugLogger& dbglogger_;
  const Watch& watch_;

  SecurityMarketView& indep_market_view_;
  int trade_history_halflife_msecs_;

  /// internal variables for detecting that a new decay zone has passed
  /// As in we break the time to fadeoff into pages, where items in each page are decayed similarly
  int last_new_page_msecs_;
  int page_width_msecs_;
  /// internal variables for decay computation
  double decay_page_factor_;
  std::vector<double> decay_vector_;
  // double inv_decay_sum_ ;

  // the following are used only for bid and ask decaying
  int last_bid_int_price_;
  int last_ask_int_price_;

  double min_price_increment_;

  // Set of booleans showing whether the corresponding variable needs to be computed or not
  bool computing_sumlbtdiffsz_;
  bool computing_sumlbtdiffsz_hvlbtdiff_;
  bool computing_sumsz_hvlbtdiff_;
  bool computing_sumtdiffsz_;
  bool computing_sumtdiffsqrtsz_;
  bool computing_sumsz_;
  bool computing_sumasz_;
  bool computing_sumbsz_;
  bool computing_sumsqrtsz_;
  bool computing_lvlsumsqrtsz_;
  bool computing_sumtype_;
  bool computing_sumcoeffs_;
  bool computing_sumpx_;
  bool computing_sumtypesz_;
  bool computing_sumtypesqrtsz_;
  bool computing_sumpxsz_;
  bool computing_sumpxsqrtsz_;
  bool computing_sumtdiffnsz_;
  bool computing_sumtdifffsz_;
  bool computing_lvlsumtdifffsz_;
  bool computing_onelvlsumtdifffsz_;
  bool computing_lvlsumtdifffszvol_;
  bool computing_sumtdifffsqrtsz_;
  bool computing_sumtdiffsqrtfsqrtsz_;

  bool computing_lvlsumtdiffsz_;
  bool computing_onelvlsumtdiffsz_;
  bool computing_lvlsumtdiffsqrtsz_;
  bool computing_lvlsumsz_;
  bool computing_onelvlsumsz_;
  bool computing_lvlsumtype_;
  bool computing_lvlsumcoeffs_;
  bool computing_onelvlsumcoeffs_;
  bool computing_lvlsumpx_;
  bool computing_onelvlsumpx_;
  bool computing_lvlsumtypesz_;
  bool computing_lvlsumtypesqrtsz_;
  bool computing_lvlsumpxsz_;
  bool computing_onelvlsumpxsz_;
  bool computing_lvlsumpxsqrtsz_;
  bool computing_lvlsumtdiffnsz_;
  bool computing_onelvlsumtdiffnsz_;
  bool computing_lvlsumtdifffsqrtsz_;
  bool computing_lvlsumtdiffsqrtfsqrtsz_;
  bool computing_lvlsumtdiffszvol_;
  bool computing_lvlsumtdiffnszvol_;

  bool computing_sumsignsizepx_;
  bool computing_sumsignszbyl1_;

 public:
  double sumlbtdiffsz_;            ///< Sum ( last_book_tdiff_ * size_traded_ )
  double sumlbtdiffsz_hvlbtdiff_;  ///< Sum ( last_book_tdiff_ * size_traded_ ) for cases when lbtdiff has a high value
  double sumsz_hvlbtdiff_;         ///< Sum ( size_traded_ ) for cases when lbtdiff has a high value
  double sumtdiffsz_;              ///< Sum ( tradepx_mktpx_diff_ * size_traded_ )
  double sumtdiffsqrtsz_;          ///< Sum ( tradepx_mktpx_diff_ * sqrt_size_traded_ )
  double sumsz_;                   ///< Sum ( size_traded_ )
  double sumasz_;                  ///< Sum ( size_traded_ / agg_buy )
  double sumbsz_;                  ///< Sum ( size_traded_ / agg_sell )
  double sumsqrtsz_;               ///< Sum ( sqrt_size_traded_ )
  double lvlsumsqrtsz_;            ///< Sum ( sqrt_size_traded_ )
  double sumtype_;                 ///< Sum ( int_trade_type_ )
  double sumcoeffs_;               ///< Sum ( 1 ) ... used primarily in averaging
  double sumpx_;                   ///< Sum ( trade_price_ )
  double sumtypesz_;               ///< Sum ( int_trade_type_ * size_traded_ )
  double sumtypesqrtsz_;           ///< Sum ( int_trade_type_ * sqrt_size_traded_ )
  double sumpxsz_;                 ///< Sum ( trade_price_ * size_traded_ )
  double sumpxsqrtsz_;             ///< Sum ( trade_price_ * sqrt_size_traded_ )
  double sumtdiffnsz_;     ///< Sum ( tradepx_mktpx_diff_ * trade_impact_ ) for trade_impact_ refer TradePrintInfo
  double sumtdifffsz_;     ///< Sum ( tradepx_mktpx_diff_ * trade_impact_ * size_traded_ )
  double lvlsumtdifffsz_;  ///< Sum ( tradepx_mktpx_diff_ * trade_impact_ * size_traded_), same as sumtdifffsz_ except
  /// the value is decayed by 1/2.0 when the mid_price changes
  double onelvlsumtdifffsz_;  ///< Sum ( tradepx_mktpx_diff_ * trade_impact_ * size_traded_), same as sumtdifffsz_
  /// except the value is set to 0 when the mid_price changes
  double sumtdifffsqrtsz_;      ///< Sum ( tradepx_mktpx_diff_ * trade_impact_ * sqrt_size_traded_ )
  double sumtdiffsqrtfsqrtsz_;  ///< Sum ( tradepx_mktpx_diff_ * sqrt_trade_impact_ * sqrt_size_traded_ )

  // set of variables that are decayed sharply when the best level changes
  double lvlsumtdiffsz_;  ///< Sum ( tradepx_mktpx_diff_ * size_traded_ ), same as sumtdiffsz_ except the value is
  /// decayed by 1/2.0 when the mid_price changes
  double onelvlsumtdiffsz_;  ///< Sum ( tradepx_mktpx_diff_ * size_traded_ ), same as sumtdiffsz_ except the value is
  /// set to 0, all history forgotten when mid_price changes
  double lvlsumtdiffsqrtsz_;  ///< Sum ( tradepx_mktpx_diff_ * sqrt_size_traded_ )
  double lvlsumsz_;           ///< Sum ( size_traded_ )
  double lvlsumtype_;         ///< Sum ( int_trade_type_ )
  double lvlsumcoeffs_;       ///< Sum ( 1 ) ... used primarily in averaging
  double onelvlsumcoeffs_;    ///< Sum ( 1 ) ... used primarily in averaging
  double lvlsumpx_;           ///< Sum ( trade_price_ )
  double onelvlsumpx_;        ///< Sum ( trade_price_ )
  double lvlsumtypesz_;       ///< Sum ( int_trade_type_ * size_traded_ )
  double lvlsumtypesqrtsz_;   ///< Sum ( int_trade_type_ * sqrt_size_traded_ )
  double lvlsumpxsz_;         ///< Sum ( trade_price_ * size_traded_ )
  double lvlsumpxsqrtsz_;     ///< Sum ( trade_price_ * sqrt_size_traded_ )
  double lvlsumtdiffnsz_;     ///< Sum ( tradepx_mktpx_diff_ * trade_impact_ ), same as sumtdiffnsz_ except the value is
  /// decayed by 1/2.0 when the mid_price changes
  double onelvlsumtdiffnsz_;  ///< Sum ( tradepx_mktpx_diff_ * trade_impact_ ), same as sumtdiffnsz_ except the value is
  /// set to 0 when the mid_price changes
  double lvlsumtdifffsqrtsz_;      ///< Sum ( tradepx_mktpx_diff_ * trade_impact_ * sqrt_size_traded_ )
  double lvlsumtdiffsqrtfsqrtsz_;  ///< Sum ( tradepx_mktpx_diff_ * sqrt_trade_impact_ * sqrt_size_traded_ )
  double onelvlsumsz_;
  double onelvlsumpxsz_;
  double onelvlmaxsumsz_;

  double sumsignsizepx_;  /// sum ( trade_type_ * traded_size * price )

 protected:
  TimeDecayedTradeInfoManager(DebugLogger& _dbglogger_, const Watch& _watch_, SecurityMarketView& _indep_market_view_,
                              double _fractional_seconds_);

 public:
  static TimeDecayedTradeInfoManager* GetUniqueInstance(DebugLogger& _dbglogger_, const Watch& _watch_,
                                                        SecurityMarketView& _indep_market_view_,
                                                        double _fractional_seconds_);

  void compute_sumlbtdiffsz();
  void compute_sumlbtdiffsz_hvlbtdiff();
  void compute_sumsz_hvlbtdiff();
  void compute_sumtdiffsz();
  void compute_sumtdiffsqrtsz();
  void compute_sumsz();
  void compute_sumasz();
  void compute_sumbsz();
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
  void compute_lvlsumtdifffsz();
  void compute_onelvlsumtdifffsz();
  void compute_lvlsumtdifffszvol();
  void compute_sumtdifffsqrtsz();
  void compute_sumtdiffsqrtfsqrtsz();
  void compute_lvlsumtdiffsz();
  void compute_onelvlsumtdiffsz();
  void compute_lvlsumtdiffsqrtsz();
  void compute_lvlsumsz();
  void compute_lvlsumsqrtsz();
  void compute_lvlsumtype();
  void compute_lvlsumcoeffs();
  void compute_onelvlsumcoeffs();
  void compute_lvlsumpx();
  void compute_onelvlsumpx();
  void compute_lvlsumtypesz();
  void compute_lvlsumtypesqrtsz();
  void compute_lvlsumpxsz();
  void compute_lvlsumpxsqrtsz();
  void compute_lvlsumtdiffnsz();
  void compute_onelvlsumtdiffnsz();
  void compute_lvlsumtdifffsqrtsz();
  void compute_lvlsumtdiffsqrtfsqrtsz();
  void compute_lvlsumtdiffszvol();
  void compute_lvlsumtdiffnszvol();
  void compute_onelvlsumsz();
  void compute_onelvlsumpxsz();
  void compute_sumsignsizepx();
  void compute_sumsignszbyl1();
  void InitializeValues();

  void OnTimePeriodUpdate(const int num_pages_to_add_);
  void OnMarketUpdate(const unsigned int _security_id_, const MarketUpdateInfo& _market_update_info_);
  void OnTradePrint(const unsigned int _security_id_, const TradePrintInfo& _trade_print_info_,
                    const MarketUpdateInfo& _market_update_info_);

  // functions
  static std::string VarName() { return "TimeDecayedTradeInfoManager"; }

 protected:
  void SetTimeDecayWeights();
  void DecayCurrentValues();
  void AdjustLevelVars();
};
}
