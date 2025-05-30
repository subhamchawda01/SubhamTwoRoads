/**
   \file MarketAdapter/hybrid_security_market_view.hpp

   \author: (c) Copyright Two Roads Technological Solutions Pvt Ltd 2011
   Address:
   Suite No 353, Evoma, #14, Bhattarhalli,
   Old Madras Road, Near Garden City College,
   KR Puram, Bangalore 560049, India
   +91 80 4190 3551
*/
#ifndef BASE_MARKETADAPTER_HYBRID_SECURITY_MARKET_VIEW_H
#define BASE_MARKETADAPTER_HYBRID_SECURITY_MARKET_VIEW_H

#include <vector>
#include <deque>
#include <string>
#include "dvccode/CDef/exchange_symbol_manager.hpp"
#include "dvccode/CDef/debug_logger.hpp"
#include "dvccode/CDef/security_definitions.hpp"

#include "dvccode/CommonDataStructures/security_name_indexer.hpp"
#include "dvccode/CommonDataStructures/fast_price_convertor.hpp"
#include "baseinfra/MarketAdapter/security_market_view.hpp"
#include "dvccode/CommonTradeUtils/watch.hpp"

#include "baseinfra/MarketAdapter/market_defines.hpp"
#include "baseinfra/MarketAdapter/security_market_view_change_listener.hpp"

#include "baseinfra/OrderRouting/prom_order_manager.hpp"

namespace HFSAT {

struct HybridSecurityMarketView : public SecurityMarketView,
                                  public SecurityMarketViewChangeListener,
                                  public SecurityMarketViewOnReadyListener,
                                  public TimePeriodListener {
  double mkt_size_weighted_price_;

  bool flag_switch;
  double s_alpha;
  double s_beta;
  SecurityMarketView* comp1_smv_;
  SecurityMarketView* comp2_smv_;
  std::vector<double> comp1_prices_;
  std::vector<double> comp2_prices_;

  // functions
  HybridSecurityMarketView(DebugLogger& t_dbglogger_, const Watch& t_watch_, SecurityNameIndexer& t_sec_name_indexer_,
                           const std::string& t_shortcode_, const char* t_exchange_symbol_,
                           const unsigned int t_security_id_, const ExchSource_t t_exch_source_,
                           SecurityMarketView* p_smv1_, SecurityMarketView* p_smv2_,
                           const std::string& t_offline_mix_mms_wts_filename_ = DEFAULT_OFFLINEMIXMMS_FILE,
                           const std::string& t_online_mix_price_const_filename_ = DEFAULT_ONLINE_MIX_PRICE_FILE,
                           const std::string& t_online_beta_kalman_const_filename_ = DEFAULT_ONLINE_BETA_KALMAN_FILE);

  ~HybridSecurityMarketView(){};
  inline bool is_ready_complex(int t_num_increments_) const {
#define MIN_L1EVENTS_TO_MAKE_READY_ON 5u
    return (comp1_smv_->is_ready_complex(t_num_increments_));
#undef MIN_L1EVENTS_TO_MAKE_READY_ON
  }

  void subscribe_OnReady(SecurityMarketViewOnReadyListener* t_new_listener_);

  void UpdateMarketUpdateInfo(const MarketUpdateInfo& _market_update_info_);
  void UpdateTradePrintInfo(const TradePrintInfo& _trade_print_info_);

  void ComputeIntPriceLevels() {
    comp1_smv_->ComputeIntPriceLevels();
    comp2_smv_->ComputeIntPriceLevels();
  }

  void ComputeTradepxMktpxDiff() {
    trade_print_info_.computing_tradepx_mktpx_diff_ = true;
    comp1_smv_->ComputeTradepxMktpxDiff();
    comp2_smv_->ComputeTradepxMktpxDiff();
  }
  void ComputeLBTDiff() {
    trade_print_info_.computing_last_book_tdiff_ = true;
    comp1_smv_->ComputeLBTDiff();
    comp2_smv_->ComputeLBTDiff();
  }
  void ComputeIntTradeType() {
    trade_print_info_.computing_int_trade_type_ = true;
    comp1_smv_->ComputeIntTradeType();
    comp2_smv_->ComputeIntTradeType();
  }
  void ComputeTradeImpact() {
    trade_print_info_.computing_trade_impact_ = true;
    comp1_smv_->ComputeTradeImpact();
    comp2_smv_->ComputeTradeImpact();
  }
  // listener interface
  void OnMarketUpdate(const unsigned int _security_id_, const MarketUpdateInfo& cr_market_update_info_);
  void OnTradePrint(const unsigned int _security_id_, const TradePrintInfo& _trade_print_info_,
                    const MarketUpdateInfo& _market_update_info_);

  inline void SMVOnReady() {
    // nothing to do here
  }

  inline void OnTimePeriodUpdate(const int num_pages_to_add) {
    if ((comp1_smv_->is_ready() && comp2_smv_->is_ready()) && !flag_switch &&
        comp1_smv_->market_update_info_.mid_price_ > 0 && comp2_smv_->market_update_info_.mid_price_ > 0) {
      if (comp1_prices_.size() >= 1000) {
        GetRegressionCoeffs();
        flag_switch = true;
      } else {
        comp1_prices_.push_back(comp1_smv_->market_update_info_.mid_price_);
        comp2_prices_.push_back(comp2_smv_->market_update_info_.mid_price_);
        // std::cout<<comp1_smv_->market_update_info_.mid_price_<<" "<<comp2_smv_->market_update_info_.mid_price_<<"\n";
      }
    }
  }

  void GetRegressionCoeffs();
};
}

#endif  // BASE_MARKETADAPTER_HYBRID_SECURITY_MARKET_VIEW_H
