// =====================================================================================
//
//       Filename:  pairs_trading.hpp
//
//    Description:
//
//        Version:  1.0
//        Created:  12/23/2014 02:54:27 PM
//       Revision:  none
//       Compiler:  g++
//
//         Author:  (c) Copyright Two Roads Technological Solutions Pvt Ltd 2011
//
//        Address:  Suite No 162, Evoma, #14, Bhattarhalli,
//                  Old Madras Road, Near Garden City College,
//                  KR Puram, Bangalore 560049, India
//          Phone:  +91 80 4190 3551
//
// =====================================================================================

#pragma once

#include "dvctrade/ExecLogic/base_trading.hpp"
#include "dvctrade/Indicators/offline_returns_lrdb.hpp"
#include "dvctrade/Indicators/offline_returns_rlrdb.hpp"
#include "baseinfra/SmartOrderRouting/mult_base_pnl.hpp"
#include "dvctrade/ExecLogic/instrument_info.hpp"
#include "dvccode/CommonTradeUtils/sample_data_util.hpp"
#include "dvctrade/Indicators/slow_stdev_calculator.hpp"
#include "dvctrade/Indicators/expression_indicator.hpp"

#define FAT_FINGER_FACTOR 5

namespace HFSAT {
class PairsTrading : public virtual BaseTrading,
                     public VolumeRatioListener,
                     public IndicatorListener,
                     public MultBasePNLListener {
 protected:
 public:
  PairsTrading(DebugLogger& _dbglogger_, const Watch& _watch_, const SecurityMarketView& _dep_market_view_,
               SmartOrderManager& _order_manager_, MultBasePNL* _mult_base_pnl_,
               const std::string& _trading_structure_shortcode_, const std::string& _paramfilename_,
               const bool _livetrading_, MulticastSenderSocket* _p_strategy_param_sender_socket_,
               EconomicEventsManager& t_economic_events_manager_, const int t_trading_start_utc_mfm_,
               const int t_trading_end_utc_mfm_, const int _runtime_id_,
               const std::vector<std::string> _this_model_source_shortcode_vec_,
               SecurityNameIndexer& sec_name_indexer_);

  ~PairsTrading() {}
  static std::string StrategyName() {
    return "PairsTrading";
  }  ///< used in trade_init to check which strategy is to be initialized

  void AddProductToTrade(SecurityMarketView* _dep_market_view_, SmartOrderManager* _smart_order_manager_,
                         BaseModelMath* _model_math_, std::string _paramfilename_, int _trading_start_mfm_,
                         int _trading_end_mfm_);

  void SetOrderPlacingLogic(const StrategyType _strategy_type_);

  SecurityNameIndexer& sec_name_indexer_;
  std::string trading_shortcode_;
  // Stuff from BaseTrading, needed to be vector
  std::vector<SecurityMarketView*> dep_market_view_vec_;
  std::vector<SmartOrderManager*> order_manager_vec_;
  std::vector<BaseModelMath*> model_math_vec_;

  std::vector<PositionTradeVarSetMap> prod_position_tradevarset_map_vec_;
  std::vector<ParamSet*> prod_paramset_vec_;  // Not adding regime thing here
  ParamSet common_paramset_;

  std::vector<TradeVars_t> closeout_zeropos_tradevarset_vec_;
  std::vector<TradeVars_t> closeout_long_tradevarset_vec_;
  std::vector<TradeVars_t> closeout_short_tradevarset_vec_;
  std::vector<TradeVars_t> current_global_tradevarset_vec_;
  std::vector<TradeVars_t> current_product_tradevarset_vec_;

  std::vector<L1SizeTrend*> l1_size_indicator_vec_;
  std::vector<L1OrderTrend*> l1_order_indicator_vec_;

  std::vector<InstrumentInfo*> product_vec_;
  InstrumentInfo* current_product_;

  std::vector<OfflineReturnsLRDB*> lrdb_vec_;
  // PricePortfolio* pair_trade_folio_;
  SimpleTrend* pair_trade_folio_;
  SimpleTrend* trend_product_;

  MultBasePNL* mult_base_pnl_;
  StrategyType order_placing_logic_;

  std::vector<double> target_price_vec_;
  std::vector<double> targetbias_numbers_vec_;
  std::vector<double> base_px_vec_;
  std::vector<double> base_px_simpletrend_vec_;

  std::vector<double> px_to_be_placed_at_;
  std::vector<int> volume_adj_max_pos_vec_;
  std::vector<int> int_px_to_be_placed_at_;
  std::vector<int> size_to_be_placed_;
  std::vector<char> order_level_indicator_vec_;
  std::vector<double> stdev_vec_;

  std::vector<std::vector<double> > offline_beta_;
  std::vector<double> position_vec_;

  std::vector<int> sec_id_to_index_;
  std::vector<int> prod_map_pos_increment_;

  std::vector<int> target_position_;
  std::vector<double> l1_bias_vec_;
  std::vector<double> l1_order_bias_vec_;
  std::vector<double> long_positioning_bias_vec_;
  std::vector<double> short_positioning_bias_vec_;

  std::vector<std::string> products_being_traded_;

  std::vector<bool> should_be_getting_flat_vec_;
  std::vector<bool> old_should_be_getting_flat_vec_;
  std::vector<bool> is_ready_vec_;

  int total_products_trading_;
  bool initialized_;
  bool cpu_allocated_;

  // pnls
  // risk aggregation

  std::vector<double> betas_;

  ///( this would be notional risk in case it's set in param)
  /// This would be maximum risk in terms of first contract
  std::vector<double> max_risk_vec_;
  double self_pos_projection_factor_;

 protected:
  double total_pnl_;
  double open_unrealized_pnl_;
  double realized_pnl_;
  double max_loss_;

  int last_max_opentrade_loss_hit_msecs_;

  // Control
  bool getting_flat_;
  bool aggressively_getting_flat_;
  unsigned int base_px_offset_;
  double base_bid_price_, base_ask_price_;
  double best_bid_place_cxl_px_, best_ask_place_cxl_px_;
  int best_int_bid_place_cxl_px_, best_int_ask_place_cxl_px_;
  double trend_portfolio_price_;
  bool print_on_trade_;
  double spread_diff_factor_;
  double index_volume_ratio_;

 protected:
  char GetOrderLevelIndicator(TradeType_t order_side, int int_order_px, int _product_index_);
  void ProcessGetFlat();
  bool ShouldBeGettingFlat();

  void SetComputeTresholds(InstrumentInfo* current_product, ParamSet* t_paramset_,
                           SecurityMarketView* p_dep_market_view_);

 public:
  virtual void SetStartTrading(bool _set_) {
    external_getflat_ = !_set_;
    getflat_due_to_external_getflat_ = !_set_;
  }
  void BuildConstantTradeVarSets(int _product_index_);
  void BuildTradeVarSets(int _product_index_);

  double ModifyTargetBias(double _targetbias_numbers_, int _modelmath_index_);
  double ModifyTargetPx(double _target_price_, int _modelmath_index_);
  double MeanReversionAlpha(int _modelmath_index_);
  bool UpdateTarget(double _target_price_, double _targetbias_numbers_, int _modelmath_index_);
  void TradingLogic(int _product_index_);
  void DatTradingLogic(int _product_index_);
  void PbatTradingLogic(int _product_index_);

  void OnExec(const int t_new_position_, const int _exec_quantity_, const TradeType_t _buysell_, const double _price_,
              const int r_int_price_, const int _security_id_);
  void OnCancelReject(const TradeType_t _buysell_, const double _price_, const int r_int_price_,
                      const int _security_id_);
  void CallPlaceCancelNonBestLevels(int _product_index_);
  void PlaceCancelNonBestLevels(int _product_index_);
  void GetFlatTradingLogic(int _product_index_);
  void NormalGetFlatTradingLogic(int _product_index_);
  void AggressiveGetFlatTradingLogic(int _product_index_);

  void UnusualTradingStatusGetFlatTradingLogic(int _product_index_);

  //
  void OnPositionChange(int t_new_position_, int position_diff_, const unsigned int security_id_);
  void UpdateSecurityWiseRisk(int t_new_position, int old_position, int prod_index);

  void UpdateBetaPosition(const unsigned int sec_id_, int new_position);
  void UpdateProductPosition(int sec_id_, int _new_position_);
  void UpdatePosition(){};
  void UpdateVolumeAdjustedMaxPos(int _product_index_);
  void TradeVarSetLogic(int _product_index_);

  void ProcessTimePeriodUpdate(const int num_pages_to_add_);  // Called from basetrading

  // void OnTimePeriodUpdate ( const int num_pages_to_add_ );

  void OnMarketUpdate(const unsigned int _security_id_, const MarketUpdateInfo& _market_update_info_);
  void OnTradePrint(const unsigned int _security_id_, const TradePrintInfo& _trade_print_info_,
                    const MarketUpdateInfo& _market_update_info_);
  void NonSelfMarketUpdate(int _product_index_);
  void OnMarketStatusChange(const unsigned int _security_id_, const MktStatus_t _new_market_status_);
  void OnStdevUpdate(const unsigned int security_id_, const double& _new_stdev_val_);
  void OnIndicatorUpdate(const unsigned int& _indicator_index_, const double& _new_value_);
  virtual void OnIndicatorUpdate(const unsigned int& indicator_index, const double& new_value_decrease,
                                 const double& new_value_nochange, const double& new_value_increase){};

  void OnVolumeRatioUpdate(const unsigned int _security_index_, const double& r_new_volume_ratio_);

  // PNL Computation
  double total_pnl();
  void UpdatePNL(int _total_pnl_);
  void UpdateOpenUnrealizedPNL(double last_pc1_risk_, double current_pc1_risk_);
  bool IsHittingMaxLoss();
  bool IsHittingOpentradeLoss();

  inline void OnControlUpdate(const ControlMessage& _control_message_, const char* symbol, const int trader_id);
  virtual void GetProductListToGetFlatMultOrder();
  void Initialize();
  void InitializeBetaValues();

  void ShowParams();
  void PrintFullStatus(int _product_index_);
  void LogFullStatus();
  void ReportResults(HFSAT::BulkFileWriter& trades_writer_, bool _conservative_close_ = false);
  void SendMail(std::string _mail_content_, std::string _mail_subject_);
  void get_positions(std::vector<std::string>& _instrument_vec_, std::vector<int>& _positon_vector_);
};
}
