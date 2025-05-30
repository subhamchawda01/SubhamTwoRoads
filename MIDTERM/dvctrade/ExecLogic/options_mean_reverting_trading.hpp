/* =====================================================================================

       Filename:  ExecLogic/options_mean_reverting_trading.hpp

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

#include "dvctrade/ExecLogic/nse_exec_logic_utils.hpp"
#include "baseinfra/MarketAdapter/shortcode_security_market_view_map.hpp"

#include "dvctrade/ExecLogic/mult_exec_interface.hpp"
#include "dvctrade/Indicators/indicator_listener.hpp"
#include "dvccode/ExternalData/external_time_listener.hpp"
#include "baseinfra/SmartOrderRouting/mult_base_pnl.hpp"

// support for implied vol signal
// INDICATOR 1 MovingAverageImpliedVol option_shc 30 Midprice
#include "dvctrade/Indicators/moving_average_implied_vol.hpp"
#include "dvctrade/Indicators/moving_avg_price_implied_vol.hpp"

// support for thresholds
#include "dvctrade/Indicators/moving_avg_bidask_spread.hpp"

namespace HFSAT {

/**

*** broader logic ***
* OptionsMeanReverting: simply inherits from MeanRevertingTrading class for future prices
* and implied_vol from moving_avg_implied_vol/other indicator (simple model) and then applies BScholes

*** modules ***
* options_risk_manager (to maintain/apply various risk checks)  // we will work with one function instead for now
* options_object (making sure we only have one option defination) // we will use this class
* options risk premium (threshold defination/computation) // we will work with one function instead for now
* option_vars (controls the execution logic) // we will work with vectors instead

*** control flow ***
* onmarketupdate {updateprices + calltradinglogic}
* ontradeprint {updateprices + updateerror; betascale + calltradinglogic}
* onindicatorupdate {} // we keep ondemand iv calculator
* ontimeperiodupdate {} // nothing
* onexec {} // calltradinglogic

*** contracts to trade ***
 * generally interested in strike_price > fut_price, others have bid_ask spread is high to absorb any fut_prices moves
 * in the money contracts, we need them assuming they might become out of the money / at the money ), couple of cases in
last 3     months. we listen to one ITM contract ( turn on/off basing on cut fut_price < strike )
 * at the money and out of the money are most likely the highest notional traded, usually strike steps are adjusted
using volati    lity, hence it is fine to rely upon constant number of contracts ( 1 ATM + 4 OTM )
 * step of contract can be changed and so we always use CurrentSchema to define consecutive {contracts may be alive from
previou    s Schema, but we noticed that the volume isnt high in strike which doesnt fall in current schema}
 **/

/*
*** modules defination extension ***
options_object:

 */

/*
 * just so we avoid confusion we are not inhering meanrevertingtrading
 * but we are almost doing the same ( except our sources are futures )
 * trading is in options
 */

class OptionsMeanRevertingTrading : public MultExecInterface, public IndicatorListener, public TimePeriodListener {
  MultBasePNL* mult_base_pnl_;

  // using longer 1D vs 2D vector, praying for data locality.
  // TODO : how many times we are going across options Vs futures and then choose
  std::vector<std::string> opt_shortcode_vec_;
  std::vector<std::string> trading_fut_shortcode_vec_;
  std::vector<std::string> nontrading_fut_shortcode_vec_;
  unsigned int num_trading_futures_;
  unsigned int num_nontrading_futures_;
  unsigned int num_futures_;
  unsigned int num_options_per_future_;
  unsigned int num_options_;

  // prod id simply for our sequential references {time to move to maps !?}
  // future prod_id is the index in future_smv_vec_
  // option prod_id is the index in dep_market_view_vec_
  // we make security_id -> {future_prod_id, option_prod_id}
  // for futures security_id we assign (futures_prod_id, -1)
  // for options security_id we assign (futures_prod_id, options_prod_id)
  std::vector<std::pair<int, int> > secid_2_prodid_;

  // mult_exec_interface will have dep_market_view_vec and order_manager_fut_vec_
  // i.e options smv and options som
  // we maintain futures smvs here ( they are our sources )
  std::vector<SecurityMarketView*> future_smv_vec_;

  double global_min_pnl_;
  // same as futures_smv_vec length
  std::vector<double> product_delta_adjusted_position_in_lots_;
  std::vector<bool> fut_level_should_be_getting_flat_;
  std::vector<bool> fut_level_should_be_getting_flat_aggressively_;
  std::vector<double> risk_adjusted_model_future_bid_price_vec_;
  std::vector<double> risk_adjusted_model_future_ask_price_vec_;

  // same as dep_market_view_vec_/order_manager_vec_ length
  // we could between vector of vectors or one long vector
  std::vector<int> product_position_in_lots_;
  std::vector<bool> opt_level_should_be_getting_flat_;
  std::vector<int> seconds_at_last_buy_;
  std::vector<int> seconds_at_last_sell_;
  std::vector<int> bid_int_price_to_place_at_;
  std::vector<int> ask_int_price_to_place_at_;

  // core strategy variables
  std::vector<std::vector<double> > residuals_;
  std::vector<double> residual_sum_;
  std::vector<double> residual_sumsqr_;
  std::vector<double> stdev_residuals_;

  std::vector<std::vector<double> > inst_prices_;
  std::vector<std::vector<double> > port_prices_;
  std::vector<double> inst_betas_;
  std::vector<double> last_inst_prices_;
  std::vector<double> last_port_prices_;

  // variables to update residual and price vectors
  int msecs_at_last_vec_processing_;
  std::vector<std::vector<std::pair<int, double> > > predictor_vec_;
  std::vector<bool> dont_trade_;

  std::vector<OptionObject*> option_obj_vec_;
  std::vector<CommonIndicator*> implied_vol_vec_;
  std::vector<double> option_delta_vec_;

  // these for deducin thresholds so futures length is fine
  // we use delta_adjusted_position

  std::vector<int> inst_unitlots_;
  std::vector<int> inst_maxlots_;
  std::vector<double> inst_base_threshold_;
  std::vector<double> inst_increase_threshold_;
  std::vector<double> inst_decrease_threshold_;
  std::vector<double> inst_return_vol_;

  // risk
  std::vector<CommonIndicator*> bid_ask_spread_vec_;

  // we further keep inst_unitlots_ and max_lots
  std::vector<int> option_unitlots_;
  std::vector<int> option_maxlots_;

  // is ready vec !
  std::vector<bool> is_ready_vec_;

  // iv logic
  int days_to_expiry_;

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

  // futures data for contructing portfolio + target_price
  // options data for placing orders
  static void CollectSourceShortCodes(DebugLogger& _dbglogger_, std::string product_filename,
                                      std::vector<std::string>& source_shortcode_vec,
                                      std::vector<std::string>& ors_source_needed_vec_);

  // if options logic is specified we then add those options
  // we are not going to take risk in future at all for now
  static void CollectTradingShortCodes(DebugLogger& _dbglogger_, std::string filename,
                                       std::vector<std::string>& _trading_product_vec_);
  /**
   *
   * @param _security_id_ id of product for which update has come
   * @param _market_update_info_ struct where we have stored the updated value
   */

  OptionsMeanRevertingTrading(DebugLogger& _dbglogger_, const Watch& _watch_, std::string _products_filename_,
                              const std::vector<SecurityMarketView*> _dep_market_view_fut_vec_,
                              const std::vector<SmartOrderManager*> _order_manager_fut_vec_,
                              const std::string& _paramfilename_, const bool _livetrading_,
                              MultBasePNL* _mult_base_pnl_, int _trading_start_utc_mfm_, int _trading_end_utc_mfm_);

 protected:
  void CollectFuturesOptionsShortCodes(std::string product_filename);
  void Initialize();
  void ReadHistValues();
  void DumpPortfolioConstituents(unsigned int);
  void SetOptionObjects();
  void SetImpliedVolCalculator();
  void ExpandRiskPremiumVarsAcrossFutures();
  void ExpandRiskPremiumVarsAcrossOptions();
  void GetAllFlat();
  void GetFlatTradingLogic(int);
  void UpdatePortPxForId(int _fut_prod_id_, double _px_diff_);

  // we expect this class will inherited to setbestprices differently
  virtual void SetBestPrices(int _fut_idx_, int _opt_idx_ = -1);

 private:
  void TradingLogic(int _fut_idx_ = -1, int _opt_idx_ = -1);
  // we should move these three to multexec
  void PlaceAndCancelOrders(int _fut_idx_ = -1, int _opt_idx_ = -1);
  void PlaceSingleBuyOrder(unsigned int _opt_id_, int int_order_px_);
  void PlaceSingleSellOrder(unsigned int _opt_id_, int int_order_px_);

 public:
  // inherited virtual classes

  // Trader
  virtual void OnControlUpdate(const ControlMessage& _control_message_, const char* symbol, const int trader_id);
  // MultExecInterface
  virtual void OnPositionChange(int t_new_position_, int position_diff_, const unsigned int _security_id_) {}
  virtual void OnOrderChange() {}
  virtual void OnExec(const int _new_position_, const int _exec_quantity_, const TradeType_t _buysell_,
                      const double _price_, const int r_int_price_, const int _security_id_);
  virtual void OnGlobalPositionChange(const unsigned int _security_id_, int _new_global_position_) {}
  virtual void OnRiskManagerUpdate(const unsigned int r_security_id_, double new_risk_) {}
  virtual void OnGlobalPNLChange(double _new_PNL_) {}
  virtual void OnCancelReject(const TradeType_t _buysell_, const double _price_, const int r_int_price_,
                              const int _security_id_) {}
  virtual void OnFokReject(const TradeType_t _buysell_, const double _price_, const int intpx_,
                           const int _size_remaining_) {}
  virtual void OnFokFill(const TradeType_t _buysell_, const double _price_, const int intpx_, const int _size_exec_) {}
  virtual void ReportResults(HFSAT::BulkFileWriter& trades_writer_, bool _conservative_close_);
  virtual int my_global_position() const;

  // SMV
  virtual void OnMarketUpdate(const unsigned int _security_id_, const MarketUpdateInfo& _market_update_info_);
  virtual void OnTradePrint(const unsigned int _security_id_, const TradePrintInfo& _trade_print_info_,
                            const MarketUpdateInfo& _market_update_info_);
  virtual void OnMarketDataInterrupted(const unsigned int _security_id_, const int msecs_since_last_receive_) {}
  virtual void OnMarketDataResumed(const unsigned int _security_id_) {}
  virtual void OnMarketStatusChange(const unsigned int _security_id_, const MktStatus_t _new_market_status_) {}

  // IndicatorListener
  virtual void OnIndicatorUpdate(const unsigned int& _indicator_index_, const double& _new_value_) {}
  virtual void OnIndicatorUpdate(const unsigned int& _indicator_index_, const double& _new_value_decrease_,
                                 const double& _new_value_nochange_, const double& _new_value_increase_) {}

  // TimePeriodListener
  virtual void OnTimePeriodUpdate(const int num_pages_to_add_) override;
  virtual void OnNewMidNight() {}

  // mult_base_pnl
  virtual void UpdatePNL(int _total_pnl_) {}
};
}
