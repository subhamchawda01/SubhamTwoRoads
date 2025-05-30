/*
 * retail_fly_trading_manager.hpp
 *
 *  Created on: Mar 19, 2015
 *      Author: archit
 */

#ifndef EXECLOGIC_RETAIL_FLY_TRADING_MANAGER_HPP_
#define EXECLOGIC_RETAIL_FLY_TRADING_MANAGER_HPP_

#include "baseinfra/BaseUtils/curve_utils.hpp"
#include "baseinfra/TradeUtils/spread_utils.hpp"
#include "dvctrade/ExecLogic/trading_manager.hpp"
#include "dvccode/LiveSources/retail_trade_listener.hpp"
#include "baseinfra/OrderRouting/prom_order_manager.hpp"
#include "baseinfra/SmartOrderRouting/mult_base_pnl.hpp"
#include "baseinfra/VolatileTradingInfo/shortcode_ezone_vec.hpp"

#define FAT_FINGER_FACTOR 5
#define MIN_PRICE_CHANGE 0.015

namespace HFSAT {

class RetailFlyTradingManager : public TradingManager,
                                public MultBasePNLListener,
                                public TimePeriodListener,
                                public ExecutionListener,
                                public GlobalPositionChangeListener,
                                public ControlMessageListener,
                                public MarketDataInterruptedListener,
                                public SecurityMarketViewOnReadyListener,
                                public FPOrderExecutedListener {
 private:
  // all these vectors indices in decreasing order of time to expiry, so index = 0 is longest leg
  std::vector<const SecurityMarketView *> p_smv_vec_;
  std::vector<int> my_position_vec_;
  std::vector<int> fp_position_vec_;
  std::vector<int> bom_position_vec_;
  std::vector<int> my_global_position_vec_;
  std::vector<int> projected_pc1_risk_vec_;
  std::vector<double> target_price_vec_;

  int total_pnl_;

  bool is_ready_;
  std::vector<bool> is_ready_vec_;
  std::vector<bool> is_data_interrupted_vec_;
  int trading_start_utc_mfm_;
  int trading_end_utc_mfm_;
  const int runtime_id_;

  bool livetrading_;
  bool should_be_getting_flat_;
  bool start_not_given_;                  // just to check if start was given once or not in live
  bool getflat_due_to_external_getflat_;  ///< internal flag set to true when we get a message to get flat from a user
  /// message
  bool getflat_due_to_close_;           ///< internal flag set to true when the time of day exceeds endtime of trading
  bool getflat_due_to_max_loss_;        ///< internal flag set to true when total_pnl_ falls below paramset_.max_loss_
  bool getflat_due_to_economic_times_;  ///< internal flag set to true when economic events happening
  bool getflat_due_to_market_data_interrupt_;  ///< internal flag set to true when market data interrupted
  bool getflat_due_to_allowed_economic_event_;
  bool getflat_due_to_freeze_;
  unsigned int last_allowed_event_index_;

  EconomicEventsManager &economic_events_manager_;
  std::vector<EconomicZone_t> ezone_vec_;

  double severity_to_getflat_on_base_;
  double severity_to_getflat_on_;
  int severity_change_end_msecs_;
  double applicable_severity_;
  bool allowed_events_present_;
  HFSAT::CDef::RetailUpdateType getflat_retail_update_type_;

  std::vector<SmartOrderManager *> order_manager_vec_;

  std::vector<double> dv01_vec_;
  std::vector<double> price_factor_vec_;
  std::vector<double> size_factor_vec_;
  std::string fly_secname_string_;
  unsigned int belly_index_;

  MultBasePNL *p_mult_base_pnl_;
  ParamSet common_param_set_;
  std::vector<ParamSet> param_set_vec_;

  int last_retail_update_msecs_;
  std::vector<RetailTradingListener *> retail_offer_listeners_;
  HFSAT::CDef::RetailOffer last_retail_offer_;
  double last_retail_offered_bidprice_;
  double last_retail_offered_askprice_;

  std::vector<int> bidsize_to_show_maxpos_limit_vec_;
  std::vector<int> asksize_to_show_maxpos_limit_vec_;
  std::vector<int> bidsize_to_show_global_maxpos_limit_vec_;
  std::vector<int> asksize_to_show_global_maxpos_limit_vec_;

  double min_min_price_increment_to_use_;
  double belly_min_price_increment_;
  int last_full_logging_msecs_;

  bool last_trade_was_buy_;
  bool last_trade_was_sell_;

  double last_buy_price_;
  double last_sell_price_;

  // variables for moving avg of price
  double moving_avg_spread_;
  int last_new_page_msecs_;
  int page_width_msecs_;

  double decay_page_factor_;
  std::vector<double> decay_vector_;
  double inv_decay_sum_;
  std::vector<double> decay_vector_sums_;

  double last_price_recorded_;
  double current_spread_mkt_price_;

  struct FRAVars {
    FRAVars(double _offered_bid_fra_, double _offered_ask_fra_, double _current_mid_fra_)
        : offered_bid_fra_(_offered_bid_fra_),
          offered_ask_fra_(_offered_ask_fra_),
          current_mid_fra_(_current_mid_fra_) {}
    double offered_bid_fra_;
    double offered_ask_fra_;
    double current_mid_fra_;
  };
  std::vector<FRAVars> fra_sp_vec_;
  std::vector<double> current_pu_vec_;
  std::vector<double> inv_pu_bid_vec_;
  std::vector<double> inv_pu_ask_vec_;
  std::vector<double> inv_pu_mid_vec_;
  std::vector<double> offered_bid_price_vec_;
  std::vector<double> offered_ask_price_vec_;
  double retail_offer_bid_fra_;
  double retail_offer_ask_fra_;
  std::vector<double> size_ratio_vec_;
  std::vector<std::vector<double>> bid_size_ratio_vec_vec_;
  std::vector<std::vector<double>> ask_size_ratio_vec_vec_;
  std::vector<int> reserve_days_vec_;
  std::vector<HFSAT::CDef::RetailOffer> last_retail_offer_bid_vec_;
  std::vector<double> last_retail_offer_bid_price_vec_;
  std::vector<HFSAT::CDef::RetailOffer> last_retail_offer_ask_vec_;
  std::vector<double> last_retail_offer_ask_price_vec_;

  std::map<unsigned int, unsigned int> secid_idx_map_;

  int last_agg_buy_send_msecs_;
  int last_agg_sell_send_msecs_;

  int freeze_start_mfm_;
  int freeze_end_mfm_;

  // TODO:
  void ShowParams() {}

  void PrintFullStatus();
  void LogFullStatus();
  void CheckParamSet();
  void ReconcileParams();

  void SetOfferVars();
  void ComputeSpreadOffer(HFSAT::CDef::RetailOffer &_retail_offer_);

  void ComputeSpreadBidOffer(double &_spread_offer_bid_price_, int &_spread_bidsize_to_offer_,
                             double _spread_bestbid_price_, double _target_price_);
  void ComputeSpreadAskOffer(double &_spread_offer_ask_price_, int &_spread_asksize_to_offer_,
                             double _spread_bestask_price_, double _target_price_);
  void ComputeFlyOffer(HFSAT::CDef::RetailOffer &_retail_offer_);

  void NotifyListeners(const HFSAT::CDef::RetailOffer &_retail_offer_);

  void QuoteAndClose();
  void TradingLogic();
  void ProcessGetFlat();
  void SendTradeAndLog(int _idx_, int _int_px_, int _size_, TradeType_t _buysell_, char _level_indicator_);
  bool RetailShouldbeGettingFlat(HFSAT::CDef::RetailUpdateType &_retail_update_type_);

  void UpdateDv01Ratio();
  void UpdatePC1Risk();
  void AdjustMaxPosLimitOfferSizes();
  void AdjustGlobalMaxPosLimitOfferSizes();

  void BreakFlyTrade(const char *_secname_, double _fly_trd_px_, unsigned int _fly_trd_qty_, TradeType_t _fly_buysell_,
                     std::vector<TradeInfoStruct> &retval_);
  void SetPriceFactVec();
  bool AddPosition(const std::string &_shc_, int _position_offset_);

  void ComputeMovingAverage();
  void SetTimeDecayWeights();

  void ComputeSlideFRA(double _spread_offer_bid_price_, double _spread_offer_ask_price_);
  void ComputeFRAFromInvPU(std::vector<double> &_inv_pu_vec_, unsigned int _index_);
  void CalibrateBidFRA();
  void CalibrateAskFRA();
  void ComputePU();
  void UpdateSizeRatio();
  void FillLegInfo(HFSAT::CDef::RetailOffer &_retail_offer_);
  double GetBidFRA();
  double GetAskFRA();
  int GetIndex(std::vector<double> _price_vector_, double _price_to_compare_);

 public:
  RetailFlyTradingManager(
      DebugLogger &_dbglogger_, const Watch &_watch_, const std::vector<const SecurityMarketView *> &_p_smv_vec_,
      MultBasePNL *_mult_base_pnl_, const std::string &_paramfilename_, const bool _livetrading_,
      EconomicEventsManager &t_economic_events_manager_, const int t_trading_start_utc_mfm_,
      const int t_trading_end_utc_mfm_, const int t_runtime_id_, const std::vector<std::string> &_paramfilename_vec_,
      const std::string &_fly_secname_ =
          "NONAME"  // can be used for custom structure naming, for DI1* retail this will always have NONAME
      );

  ~RetailFlyTradingManager() {}

  void ReportResults(HFSAT::BulkFileWriter &trades_writer_);
  inline const std::vector<const SecurityMarketView *> &GetSmvVec() { return p_smv_vec_; }
  inline void UpdatePNL(int _total_pnl_) { total_pnl_ = _total_pnl_; }
  void OnTimePeriodUpdate(const int num_pages_to_add_);
  void OnExec(const int _new_position_, const int _exec_quantity_, const TradeType_t _buysell_, const double _price_,
              const int r_int_price_, const int _security_id_);
  void OnGlobalPositionChange(const unsigned int _security_id_, int _new_global_position_);

  void FPOrderExecuted(const char *_secname_, double _price_, TradeType_t r_buysell_, int _size_executed_);
  void SMVOnReady();

  void OnMarketDataInterrupted(const unsigned int _security_id_, const int msecs_since_last_receive_);
  void OnMarketDataResumed(const unsigned int _security_id_);

  void OnControlUpdate(const ControlMessage &_control_message_, const char *symbol_, const int trader_id);
  void SetOrderManager(SmartOrderManager *_p_om_, const std::string &_shc_);

  inline bool SubscribeRetailOfferUpdate(RetailTradingListener *_listener_) {
    return VectorUtils::UniqueVectorAdd(retail_offer_listeners_, _listener_);
  }

  inline std::string secname() { return fly_secname_string_; }
  inline static std::string StrategyName() { return "RetailFlyTrading"; }

  inline const std::vector<int> my_position_vec() { return my_position_vec_; }
};

} /* namespace HFSAT */

#endif /* EXECLOGIC_RETAIL_FLY_TRADING_MANAGER_HPP_ */
