/* =====================================================================================

       Filename:  ExecLogic/index_futures_mean_reverting_trading.hpp

    Description:

        Version:  1.0
        Created:  Monday 30 May 2016 05:05:08  UTC
       Revision:  none
       Compiler:  g++

         Author:  (c) Copyright Two Roads Technological Solutions Pvt Ltd 2011

        Address:  Suite No 162, Evoma, #14, Bhattarhalli,
                  Old Madras Road, Near Garden City College,
                  KR Puram, Bangalore 560049, India
          Phone:  +91 80 4190 3551

 =====================================================================================
 */
#pragma once

#ifndef BASE_EXECLOGIC_INDEX_FUTURES_MEAN_REVERSION_TRADING_H
#define BASE_EXECLOGIC_INDEX_FUTURES_MEAN_REVERSION_TRADING_H

#include "dvctrade/ExecLogic/exec_logic_utils.hpp"
#include "dvctrade/ExecLogic/mult_exec_interface.hpp"
#include "dvctrade/Indicators/indicator_listener.hpp"
#include "dvccode/ExternalData/external_time_listener.hpp"
#include "baseinfra/SmartOrderRouting/mult_base_pnl.hpp"
#include "dvctrade/Indicators/simple_returns.hpp"
#include "dvctrade/Indicators/bid_ask_to_pay_notional_dynamic_sd.hpp"
#include "dvctrade/Indicators/diff_trsizeavg_tpx_basepx.hpp"
#include "dvccode/CommonTradeUtils/economic_events_manager.hpp"
#include "dvctrade/ExecLogic/base_trading_defines.hpp"
#include "baseinfra/BaseUtils/curve_utils.hpp"

// support for extracting out earnings date and setting don't trade
#include "sqlite3.h"
#define DEF_MIDTERM_DB "/spare/local/tradeinfo/NSE_Files/midterm_db"
#define FAT_FINGER_FACTOR 5

namespace HFSAT {

/**
 * IndexFuturesMeanRevertingTrading class inherits Indicator Listener because we will be having returns of indicators
 * we are inheriting TimePeriodListener beacuse we will stratgey will update logic every 5 sec or so
 * MultBasePNLListener because we need the pnl of every product
 */
class IndexFuturesMeanRevertingTrading : public MultExecInterface, public IndicatorListener, public TimePeriodListener {
  std::vector<int> product_position_in_lots_;
  std::vector<std::string> shortcode_vec_;
  std::vector<std::string> secname_vec_;

  // products that are not to be traded sid -> bool
  std::vector<bool> dont_trade_;

  // variables to store prices for buy/sell orders
  std::vector<int> bid_int_price_to_place_at_;
  std::vector<int> ask_int_price_to_place_at_;
  std::vector<int> bid_int_price_to_keep_at_;
  std::vector<int> ask_int_price_to_keep_at_;

  int num_total_sources_;
  int num_total_products_;
  SecurityMarketView* first_dep_smv_;

  int top_products_;
  int total_pnl_;
  MultBasePNL* mult_base_pnl_;

  double global_min_pnl_;

  bool is_affined_;                 // to avoid affining more than once
  int last_affine_attempt_msecs_;   // to avoid too frequent affining attempts
  int first_affine_attempt_msecs_;  // to keep track of our first affine attempt

  EconomicEventsManager& economic_events_manager_;
  std::vector<EconomicZone_t> ezone_vec_;

  std::vector<SecurityMarketView*> all_market_view_vec_;

  std::vector<bool> should_be_getting_flat_;
  bool getflat_due_to_economic_times_;
  bool getflat_due_to_allowed_economic_event_;
  bool allowed_events_present_;
  int severity_to_getflat_on_;
  int severity_to_getflat_on_base_;
  int severity_change_end_msecs_;
  int applicable_severity_;
  unsigned int last_allowed_event_index_;
  std::vector<int> pnl_sampling_timestamps_;
  std::vector<int> pnl_samples_;
  int next_open_pnl_log_timestamp_;
  unsigned int sample_index_;
  unsigned int runtime_id_;

  // vector storing predictor weights : <sid of predictor > -> vector of < predicted_sid, pred_wt > pairs
  std::vector<std::vector<std::pair<int, double>>> predictor_vec_;

  // residuals for computing bands around fair px
  std::vector<std::vector<double>> residuals_;
  std::vector<double> residual_sum_;
  std::vector<double> residual_sumsqr_;
  std::vector<double> stdev_residuals_;
  std::vector<double> support_lvl_buffer_;

  std::vector<std::vector<double>> inst_prices_;
  std::vector<std::vector<double>> port_prices_;
  std::vector<double> inst_betas_;
  std::vector<int> online_price_update_count_;

  std::vector<double> last_inst_prices_;
  std::vector<double> last_port_prices_;

  // variables to update residual and price vectors
  int msecs_at_last_vec_processing_;

  // maintaining timestamps of execution for cooloff purposes
  std::vector<int> seconds_at_last_buy_;
  std::vector<int> seconds_at_last_sell_;

  // values of book indicators
  std::vector<double> book_indicator_values_;
  // values of trade indicators
  std::vector<double> trade_indicator_values_;

  // move parameters to inst specific values to account for varying lot notional
  // volatility etc
  std::vector<int> inst_unitlots_;
  std::vector<int> inst_maxlots_;
  std::vector<int> inst_worstcaselots_;
  std::vector<double> inst_base_threshold_;
  std::vector<double> inst_increase_threshold_;
  std::vector<double> inst_decrease_threshold_;
  std::vector<double> inst_return_vol_;
  std::vector<std::vector<double>> buy_thresholds_;
  std::vector<std::vector<double>> sell_thresholds_;
  std::vector<double> current_dv01_vec_;

  std::vector<double> spread_pos_vec_;
  std::vector<double> unhedged_pos_vec_;
  std::vector<int> pos_vec_tstamps_;
  bool record_hedged_unhedged_risks_;

  std::vector<int> prev_ask_order_lvl_;
  std::vector<std::vector<int>> asklvls_tstamp_;
  std::vector<std::vector<int>> asklvls_from_best_;

  std::vector<int> prev_bid_order_lvl_;
  std::vector<std::vector<int>> bidlvls_tstamp_;
  std::vector<std::vector<int>> bidlvls_from_best_;

  bool is_ready_;
  bool all_smv_ready_;

 private:
  /**
   * Make all returns indicators and listen to them
   */
  void AddIndicatorListener();

  void Initialize();

  void ComputeCurrentSeverity();

  void ProcessAllowedEco();

  void UpdatePrices();

  bool CheckStratMaxLoss();

  void BuildThresholds();

  void ResetUnitSizes();

  void SetBuySellThresholds();

  void PrintHedgedUnhedgedDetails();

  void PrintOrderStats();

  int GetHighestAskIntPrice(int security_id_);

  int GetHighestBidIntPrice(int security_id_);

 public:
  /**
   * This is the constructor
   * @param _dbglogger_
   * @param _watch_
   * @param _dep_market_view_fut_vec_
   * @param _order_manager_fut_vec_
   * @param _paramfilename_
   * @param _livetrading_
   */

  static inline bool compare_pairs(const std::pair<int, double>& lhs, const std::pair<int, double>& rhs) {
    return lhs.second > rhs.second;
  }

  IndexFuturesMeanRevertingTrading(DebugLogger& _dbglogger_, const Watch& _watch_,
                                   const std::vector<SecurityMarketView*> _all_market_view_fut_vec_,
                                   const std::vector<SecurityMarketView*> _dep_market_view_fut_vec_,
                                   const std::vector<SmartOrderManager*> _order_manager_fut_vec_,
                                   const std::string& _paramfilename_, const bool _livetrading_,
                                   MultBasePNL* _mult_base_pnl_, EconomicEventsManager& t_economic_events_manager_,
                                   int _trading_start_utc_mfm_, int _trading_end_utc_mfm_, unsigned int _runtime_id_);

  /**
   * This function is to be called in tradeinit to get list of shortcodes for which we need data
   * @param product_filename : name of the file which will be there in the strategy
   * @param source_shortcode_vec : vector where we will store list of shortcodes
   * @param ors_source_needed_vec_
   */

  static void CollectORSShortCodes(DebugLogger& _dbglogger_, std::string product_filename,
                                   std::vector<std::string>& source_shortcode_vec,
                                   std::vector<std::string>& ors_source_needed_vec_);

  /**
   * This function is to be called in tradeinit to get the list of product where we would be trading
   * @param _dbglogger_
   * @param filename : Name of the file where list of shortcodes to be traded are specified
   * @param _trading_product_vec_ : vector where we will store the list
   * @param ors_source_needed_vec_
   */
  static void CollectTradingShortCodes(DebugLogger& _dbglogger_, std::string filename,
                                       std::vector<std::string>& _trading_product_vec_);

  /**
   * This needs to be implemented as this class is inheriting IndicatorListener
   * @param _indicator_index_ index for which indicator has updated
   * @param _new_value_ value of the indicator
   */
  virtual void OnIndicatorUpdate(const unsigned int& _indicator_index_, const double& _new_value_);

  /**
   * This needs to be implemented as this class is inheriting TimePeriodListener
   * @param num_pages_to_add
   */
  virtual void OnTimePeriodUpdate(const int num_pages_to_add_) override;

  /**
   *
   * @param _security_id_ id of product for which update has come
   * @param _market_update_info_ struct where we have stored the updated value
   */
  virtual void OnMarketUpdate(const unsigned int _security_id_, const MarketUpdateInfo& _market_update_info_);

  /**
   *
   * @param _security_id_ id of product for which data has stopped coming
   * @param msecs_since_last_receive_ milliseconds since last update received
   */
  virtual void OnMarketDataInterrupted(const unsigned int _security_id_, const int msecs_since_last_receive_) {
    // lets keep empty for the time being.
  }

  /**
   *
   * @param _security_id_ id of product for which market data is resumed
   */
  virtual void OnMarketDataResumed(const unsigned int _security_id_) {
    // lets keep it empty for the time being.
  }

  /**
   * specify the trading logic here, that could be called in OnMarketUpdate or OnTimePeriodUpdate or OnTradePrint
   */
  void TradingLogic();

  /** Check if SMV is ready for all products **/
  bool CheckSMVOnReadyAllProducts();

  /**
   *
   * @param _security_id_ id of security for which trade has come
   * @param _trade_print_info_ trade info in this struct
   * @param _market_update_info_ market data info n this struct
   */
  virtual void OnTradePrint(const unsigned int _security_id_, const TradePrintInfo& _trade_print_info_,
                            const MarketUpdateInfo& _market_update_info_);

  /**
   *
   * @param t_new_position_ new position of security
   * @param position_diff_ position difference wrt earlier position
   * @param _security_id_ id of security
   */
  virtual void OnPositionChange(int t_new_position_, int position_diff_, const unsigned int _security_id_) {}

  /**
   *This is called from ControlMessageManager
   * @param _control_message_
   * @param symbol
   * @param trader_id
   */

  virtual void OnCancelReject(const TradeType_t _buysell_, const double _price_, const int r_int_price_,
                              const int _security_id_) {}
  //  virtual void OnRejectDueToFunds(const TradeType_t _buysell_) {}
  //  virtual void OnWakeUpifRejectDueToFunds() {}
  virtual void OnFokReject(const TradeType_t _buysell_, const double _price_, const int intpx_,
                           const int _size_remaining_) {}
  virtual void OnFokFill(const TradeType_t _buysell_, const double _price_, const int intpx_, const int _size_exec_) {}
  virtual void OnOrderChange() {}
  virtual void OnGlobalPositionChange(const unsigned int _security_id_, int _new_global_position_) {}
  //  virtual void OnGlobalOrderExec(const unsigned int _security_id_, const TradeType_t _buysell_, const int _size_,
  //  const double _trade_px_) {}
  //  virtual void OnGlobalOrderChange(const unsigned int _security_id_, const TradeType_t _buysell_, const int
  //  _int_price_) {}
  virtual void OnGlobalPNLChange(double _new_PNL_) {}
  virtual void OnNewMidNight() {}
  // virtual void OnMarketStatusChange(const unsigned int _security_id_, const MktStatus_t _new_market_status_) {}
  virtual void OnMarketStatusChange(const unsigned int _security_id_, const MktStatus_t _new_market_status_) {}
  virtual void OnIndicatorUpdate(const unsigned int& _indicator_index_, const double& _new_value_decrease_,
                                 const double& _new_value_nochange_, const double& _new_value_increase_) {}
  virtual void OnRiskManagerUpdate(const unsigned int r_security_id_, double new_risk_) {}

  virtual void OnControlUpdate(const ControlMessage& _control_message_, const char* symbol,
                               const int trader_id);  // the user messages from the trader

  /**
   * reports trading date pnl volume and other statistics ...
   * @param trades_writer_
   * @param _conservative_close_
   */
  virtual void ReportResults(HFSAT::BulkFileWriter& trades_writer_, bool _conservative_close_);

  /**
   * Specifies the Get Flat Logic here
   * @param _product_index_ index of product in which we want to get flat
   */

  void GetFlatTradingLogic(int _product_index_);

  /**
   * This function is called whenever we get execution
   * @param _new_position_ our new position
   * @param _exec_quantity_
   * @param _buysell_
   * @param _price_
   * @param r_int_price_
   * @param _security_id_
   */

  virtual void OnExec(const int _new_position_, const int _exec_quantity_, const TradeType_t _buysell_,
                      const double _price_, const int r_int_price_, const int _security_id_);

  /**
   *
   * @param _total_pnl_
   */
  virtual void UpdatePNL(int _total_pnl_);

  /**
   * Needs to be defined because we are inheriting MultExecInterface
   * @return our global position
   */
  virtual int my_global_position() const;
  virtual double my_portfolio_risk();

  int my_position_for_outright(SecurityMarketView* _dep_market_view_);

  /**
   *
   * @param _instrument_vec_
   * @param _positon_vector_
   */
  virtual void get_positions(std::vector<std::string>& _instrument_vec_, std::vector<int>& _positon_vector_) {}

  /**
   *
   * @param _instrument_vec_
   * @param _positon_vector_
   */
  virtual void get_risk(std::vector<std::string>& _instrument_vec_, std::vector<int>& _positon_vector_) {
    // we will add this functionality later, lets keep this empty for now
  }

  // debug function
  void DumpPortfolioConstituents(int sec_id);

  // to poupulate historical price and rolling regression error data from file
  void ReadHistValues();

  // sets best bid/ask prices to place at
  void SetBestPrices(int sec_id);

  void AllocateCPU();

  // update port prices effected by a change
  void UpdatePortPxForId(int _security_id_, double px_diff_);

  // places/cancels orders based on prices set in TradingLogic
  void PlaceAndCancelOrders();

  // places/modifies orders on bid/ask side
  void PlaceSingleBuyOrder(int index, int int_order_px_, int int_keep_px_);
  void PlaceSingleSellOrder(int index, int int_order_px_, int int_keep_px_);

  // getflat a product with product id = product, this is upon calling of user message
  void GetAllFlat();

  // utility functions for abnormal volume getflat
  void InitializeImpliedTriggerValues();

  inline int get_runtime_id_() { return runtime_id_; }
};
}
#endif  // BASE_EXECLOGIC_INDEX_FUTURES_MEAN_REVERSION_TRADING_H
