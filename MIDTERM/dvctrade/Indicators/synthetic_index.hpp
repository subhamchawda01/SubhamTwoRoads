/**
    \file Indicators/synthetic_index.hpp

    \author: (c) Copyright Two Roads Technological Solutions Pvt Ltd 2011
     Address:
         Suite No 353, Evoma, #14, Bhattarhalli,
         Old Madras Road, Near Garden City College,
         KR Puram, Bangalore 560049, India
         +91 80 4190 3551
 */

#pragma once

#include "dvctrade/Indicators/common_indicator.hpp"
#include "baseinfra/MarketAdapter/security_market_view.hpp"
#include "dvctrade/FuturesUtils/SpotFutureIndicatorUtils.hpp"

namespace HFSAT {

class SyntheticIndex : public CommonIndicator {
 protected:
  // variables
  SecurityMarketView* dep_market_view_;
  std::vector<SecurityMarketView*> indep_market_view_vec_;
  SecurityNameIndexer& sec_name_indexer_;
  PriceType_t price_type_;

  // computational variables

  double index_divisor_;
  double index_multiplier_;
  double synthetic_index_;
  int tradingdate_;

  double current_dep_price_;
  double current_indep_price_;

  double base_index_value_;
  double base_mkt_capital_;

  int indicator_start_mfm_;

  std::vector<double> last_constituent_price_;
  std::vector<double> constituent_theoretical_volume_;
  std::vector<double> constituent_price_from_file_;
  std::vector<double> constituent_weight_;

  std::vector<bool> is_illiquid_prod_;

  bool dep_interrupted_;
  bool indep_interrupted_;
  std::vector<bool> indep_interrupted_vec_;
  std::vector<bool> is_ready_vec_;
  bool dep_ready_;

  int use_fut_;  // Used in NSE to specify whether to use Future for creating synthetic index

  int index_type_;
  // functions
 public:
  static void CollectShortCodes(std::vector<std::string>& _shortcodes_affecting_this_indicator_,
                                std::vector<std::string>& _ors_source_needed_vec_,
                                const std::vector<const char*>& _tokens_);

  static void CollectConstituents(std::vector<std::string>& _shortcodes_affecting_this_indicator_,
                                  std::string shortcode_, int use_fut_ = 0);
  static void CollectBovespaConstituents(std::vector<std::string>& shortcodes_affecting_this_indicator,
                                         std::string shortcode);
  static void CollectNSEIndicesConstituents(std::vector<std::string>& shortcodes_affecting_this_indicator,
                                            std::string shortcode, int use_fut_ = 0);
  static SyntheticIndex* GetUniqueInstance(DebugLogger& _dbglogger_, const Watch& _watch_,
                                           const std::vector<const char*>& _tokens_, PriceType_t _basepx_pxtype_);

  static SyntheticIndex* GetUniqueInstance(DebugLogger& _dbglogger_, const Watch& _watch_,
                                           SecurityMarketView* _dep_market_view_, PriceType_t _price_type_,
                                           int _use_fut_ = 0);

 protected:
  SyntheticIndex(DebugLogger& _dbglogger_, const Watch& _watch_, const std::string& concise_indicator_description_,
                 SecurityMarketView* _dep_market_view_, PriceType_t _price_type_, int _use_fut_ = 0);

 public:
  ~SyntheticIndex() {}

  // listener interface
  void OnMarketUpdate(const unsigned int _security_id_, const MarketUpdateInfo& _market_update_info_);
  inline void OnTradePrint(const unsigned int _security_id_, const TradePrintInfo& _trade_print_info_,
                           const MarketUpdateInfo& _market_update_info_) {
    OnMarketUpdate(_security_id_, _market_update_info_);
  }

  void ComputeBovespaIndex(const unsigned int sec_id, const double current_price);
  void ComputeNSEIndices(const unsigned int sec_id, const double current_price);

  inline void OnPortfolioPriceChange(double _new_price_) {}
  inline void OnPortfolioPriceReset(double t_new_price_, double t_old_price_, unsigned int is_data_interrupted_){};

  // functions
  /// Used in ModelCreator to see which shortcodes are core
  bool GetReadinessRequired(const std::string& r_dep_shortcode_, const std::vector<const char*>& tokens_) const {
    std::vector<std::string> core_shortcodes_;
    GetCoreShortcodes(r_dep_shortcode_, core_shortcodes_);
    // here tokes 3 and 4 are important
    // if ( ( tokens_.size() > 3u ) &&
    // 	   ( VectorUtils::LinearSearchValue ( core_shortcodes_, std::string(tokens_[3]) ) ) )
    // 	{ return true ; }
    if ((tokens_.size() > 4u) && (VectorUtils::LinearSearchValue(core_shortcodes_, std::string(tokens_[4])))) {
      return true;
    }
    return false;
  }

  static std::string VarName() { return "SyntheticIndex"; }

  void WhyNotReady();

  void OnMarketDataInterrupted(const unsigned int _security_id_, const int msecs_since_last_receive_);
  void OnMarketDataResumed(const unsigned int _security_id_);
  void LoadBovespaConstituents();
  void LoadNSEConstituents();

 protected:
  void InitializeValues();
};
}
