/**
    \file Indicators/event_decayed_trade_info_manager.hpp

    \author: (c) Copyright Two Roads Technological Solutions Pvt Ltd 2011
     Address:
         Suite No 353, Evoma, #14, Bhattarhalli,
         Old Madras Road, Near Garden City College,
         KR Puram, Bangalore 560049, India
         +91 80 4190 3551
*/
#ifndef BASE_INDICATORS_EVENT_DECAYED_TRADE_INFO_MANAGER_H
#define BASE_INDICATORS_EVENT_DECAYED_TRADE_INFO_MANAGER_H

#include <map>
#include "baseinfra/MarketAdapter/security_market_view.hpp"

namespace HFSAT {

class EventDecayedTradeInfoManager : public SecurityMarketViewChangeListener {
 protected:
  DebugLogger& dbglogger_;
  const Watch& watch_;

  SecurityMarketView& indep_market_view_;
  unsigned int trade_history_halflife_mktevents_;

  double decay_page_factor_;

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
  // set of variables that are decayed sharply when the best level changes
  bool computing_lvlsumtdiffsz_;
  bool computing_lvlsumtdifffsz_;
  bool computing_lvlsumtdiffnsz_;

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
  double lvlsumtdifffsz_;
  double lvlsumtdiffnsz_;

 protected:
  EventDecayedTradeInfoManager(DebugLogger& _dbglogger_, const Watch& _watch_, SecurityMarketView& _indep_market_view_,
                               unsigned int _num_events_halflife_);

 public:
  static EventDecayedTradeInfoManager* GetUniqueInstance(DebugLogger& _dbglogger_, const Watch& _watch_,
                                                         SecurityMarketView& _indep_market_view_,
                                                         unsigned int _num_events_halflife_);

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
  void compute_lvlsumtdifffsz();
  void compute_lvlsumtdiffnsz();

  void InitializeValues();

  // listener interface
  void OnMarketUpdate(const unsigned int _security_id_, const MarketUpdateInfo& _market_update_info_);
  void OnTradePrint(const unsigned int _security_id_, const TradePrintInfo& _trade_print_info_,
                    const MarketUpdateInfo& _market_update_info_);

  // functions
  static std::string VarName() { return "EventDecayedTradeInfoManager"; }

 protected:
  void DecayCurrentValues();
};
}

#endif  // BASE_INDICATORS_EVENT_DECAYED_TRADE_INFO_MANAGER_H
