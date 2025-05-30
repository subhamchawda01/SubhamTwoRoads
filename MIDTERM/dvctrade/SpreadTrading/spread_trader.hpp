#ifndef MT_SPRD_TRADER
#define MT_SPRD_TRADER

#include <deque>

#include <boost/archive/tmpdir.hpp>
#include <boost/archive/text_iarchive.hpp>
#include <boost/archive/text_oarchive.hpp>
#include <boost/serialization/base_object.hpp>
#include <boost/serialization/utility.hpp>
#include <boost/serialization/list.hpp>
#include <boost/serialization/vector.hpp>
#include <boost/serialization/deque.hpp>
#include <boost/serialization/assume_abstract.hpp>

#include <sqlite3.h>

#include "dvccode/CDef/debug_logger.hpp"
#include "baseinfra/EventDispatcher/minute_bar_events_dataloader_and_dispatcher.hpp"
// support for HFT data infrastructure
#include "baseinfra/MarketAdapter/security_market_view_change_listener.hpp"
#include "dvccode/CommonTradeUtils/watch.hpp"
#include "dvctrade/InitCommon/nse_spread_logic_param.hpp"
#include "dvctrade/SpreadTrading/spread_exec_logic.hpp"
#include "dvctrade/SpreadTrading/record.hpp"
#include "dvctrade/SpreadTrading/kalman_regression.hpp"


#define DEF_DB "/spare/local/tradeinfo/NSE_Files/midterm_db"

namespace MT_SPRD {
class SpreadTrader : public hftrap::eventdispatcher::MinuteBarEventsListener,
                     public HFSAT::SecurityMarketViewChangeListener,
                     public HFSAT::SecurityMarketViewRawTradesListener {
 private:
  // allow external boost class to access private and protected members of this class
  // to create restore point and load data on reboot
  friend class boost::serialization::access;

 public:
  SpreadTrader(ParamSet* t_param_, ParamSet* t_param_next_month_, HFSAT::DebugLogger& t_dbglogger_,
               SpreadExecLogic* t_sprd_exec_fm_ = NULL, SpreadExecLogic* t_sprd_exec_nm_ = NULL,
               bool t_pass_leg_first = false, bool t_use_adjusted_ = false, time_t t_trading_start_time = 0,
               bool t_live = false, bool t_ban = false);
  virtual ~SpreadTrader(){};

  SpreadExecLogic* GetFrontMonthExec();
  ParamSet GetParams();
  ParamSet GetParams2();
  void UpdateLotSizes(int inst1_fut0, int inst1_fut1, int inst2_fut0, int inst2_fut1);
  void SetUnitTradeSize(int t_uts1, int t_uts2);
  void UpdateFlat(bool is_flat);

  std::string GetFilename(std::string);
  bool IsRelevantInstrument(int);
  void OnBarUpdate(int inst_id_, uint64_t time_, bool is_front_month_, int expiry_date_, double price_,
                   bool more_data_in_bar_);  // inst_id is taken from param
  void OnAllEventsConsumed();

  // SMV listener functions
  void OnMarketUpdate(const unsigned int _security_id_, const HFSAT::MarketUpdateInfo& _market_update_info_) {}
  void OnTradePrint(const unsigned int _security_id_, const HFSAT::TradePrintInfo& _trade_print_info_,
                    const HFSAT::MarketUpdateInfo& _market_update_info_);
  void OnRawTradePrint(const unsigned int _security_id_, const HFSAT::TradePrintInfo& _trade_print_info_,
                       const HFSAT::MarketUpdateInfo& _market_update_info_);

  // function to set HFT specific parameters on a daily basis -- not stored persistently
  void SetHFTClasses(HFSAT::Watch* t_watch_, unsigned int t_inst1_fm_sid_, unsigned int t_inst1_nm_sid_,
                     unsigned int t_inst2_fm_sid_, unsigned int t_inst2_nm_sid_, int t_exp_date_);
  // function to set Spread Exec classes if needed
  void SetExecClasses(SpreadExecLogic* t_sprd_fm_, SpreadExecLogic* t_sprd_nm_) {
    spread_exec_front_month_ = t_sprd_fm_;
    spread_exec_next_month_ = t_sprd_nm_;
    spread_exec_ = t_sprd_fm_;
  }

  // function to update realized positions at EOD from spread_exec - called from main class
  void GetRealizedPositions() {
    if (is_pass_leg_first_)
      spread_exec_->GetOMPositions(om_position_1_, om_position_2_);
    else
      spread_exec_->GetOMPositions(om_position_2_, om_position_1_);
  }
  void SetRealizedPositions() {
    if (is_pass_leg_first_)
      spread_exec_->SetOMPositions(om_position_1_, om_position_2_);
    else
      spread_exec_->SetOMPositions(om_position_2_, om_position_1_);
  }

  void GetSpreadExecVars() {
    spread_exec_->GetAllVariables(sw_des_pos_pass_leg_, sw_des_pos_agg_leg_, sw_tgt_sprd_, sw_sprd_alpha_,
                                  sw_sprd_beta_, sw_pos_pass_leg_, sw_pos_agg_leg_, sw_hedge_ratio_);
  }

  void SetSpreadExecVars() {
    spread_exec_->SetAllVariables(sw_des_pos_pass_leg_, sw_des_pos_agg_leg_, sw_tgt_sprd_, sw_sprd_alpha_,
                                  sw_sprd_beta_, sw_pos_pass_leg_, sw_pos_agg_leg_, sw_hedge_ratio_);
  }

  void SetOvernightPosition(int pos1_, int pos2_) {
    if (is_pass_leg_first_)
      spread_exec_->SetOMPositions(pos1_, pos2_);  // passive_pos first
    else
      spread_exec_->SetOMPositions(pos2_, pos1_);  // passive pos first
  }

  void SaveKalmanState();
  void LoadKalmanState(std::vector<double>& x_, std::vector<double>& P_, std::vector<double>& Q_,
                       std::vector<double>& R_);
  void InitializeKalman(std::vector<double>& x_, std::vector<double>& P_, std::vector<double>& Q_,
                        std::vector<double>& R_);
  void PrintTimeRecord(time_record_t*);
  void PrintOneTrade(spread_trade_record_t*);

  // load adf and hlife maps in historical or live modes
  void LoadStatMaps(int t_start_yyyymmdd, int t_end_yyyymmdd);

  // load earnings date for setting getflat if needed
  void LoadEarningsMaps();

  // check current date for Earnings sensitivity and get flat accordingly
  void SetCurrentDayEarningsGetflat(int t_currdate_yyyymmdd_);

  void OnCorporateAction(double, double);

 private:
  // functions for supporting HFT data callbacks
  HFSAT::Watch* watch_;
  unsigned int inst_1_front_month_sid_;
  unsigned int inst_1_next_month_sid_;
  unsigned int inst_2_front_month_sid_;
  unsigned int inst_2_next_month_sid_;
  int expiry_date_yyyymmdd_;

  // SpreadExecLogic handle for execution and other vars needed for integration with
  // SpreadExecLogic
  SpreadExecLogic* spread_exec_front_month_;
  SpreadExecLogic* spread_exec_next_month_;
  SpreadExecLogic* spread_exec_;  // execlogic representing active traded execlogic at any given time
  bool is_pass_leg_first_;

  // position and pnl reconciliation variables
  int unit_size_position_;
  double current_drawdown_;
  double highest_nav_so_far_;
  double max_drawdown_;

  // file for dumping logs
  std::string filename_;

  // spread and sma(spread) values
  std::vector<double> spread_values_;
  std::vector<uint64_t> spread_value_snaptimes_;
  double current_spread_;

  // used to compute variance of spread around its mean - to implement zscore thresholding
  std::deque<double> traded_spread_difference_;
  double sum_x_;
  double sum_x2_;
  double diff_stdev_;
  bool stdev_ready_;

  // prices history of first expiry - used in spread mode 1; asset mode 0
  std::deque<double> hist_prices_1_;
  std::deque<double> hist_prices_2_;
  double sum_prices_1_;
  double sum_prices_2_;
  double sum_prod_prices_;
  double sum_prices1_sqr_;
  double sum_prices2_sqr_;

  // logpx history of first expiry - used in spread mode 1; asset mode 1
  std::deque<double> hist_logpx_1_;
  std::deque<double> hist_logpx_2_;
  double sum_logpx_1_;
  double sum_logpx_2_;
  double sum_prod_logpx_;
  double sum_logpx2_sqr_;

  // beta and intercept for portfolio relation
  double beta_;
  double intercept_;

  // variables to enforce trade cooloff  & stoploss/stopgain cooloff
  char last_opentrade_type_;
  char last_exec_type_;
  uint64_t last_exec_entry_time_;
  bool last_trade_stopped_out_;

  // data filters - to avoid issues of stale data on one instrument
  // second suffix is for front month/next month future - used only on rolls
  uint64_t last_mkt_trade_time_1_0_;
  uint64_t last_mkt_trade_time_1_1_;
  uint64_t last_mkt_trade_time_2_0_;
  uint64_t last_mkt_trade_time_2_1_;
  double last_close_px_1_0_;
  double last_close_px_1_1_;
  double last_close_px_2_0_;
  double last_close_px_2_1_;
  uint64_t last_bardata_time_;

  bool seen_today_px_1;
  bool seen_today_px_2;

  // trade statistics  - for analysis
  int num_trades_;
  std::vector<double> trade_returns_;
  double annualized_returns_;
  double initial_nav_;
  double annualized_stdev_;

  // rollover related variables
  bool is_expiry_day_;
  bool has_rolled_over_;
  bool roll_trade_done_;
  uint64_t last_expiry_check_tm_;

  // check to prevent initialization errors
  bool ready_to_trade_;

  // variable to time the first trade
  time_t trading_start_time;

  // Param instance
  ParamSet* param_;
  ParamSet* param_next_month_;

  // main trade logic functions
  void TradeLogic();
  void CheckAndPlaceOrder(int t_pos_desired_);
  void UpdateMTM();
  void HandleStop(bool is_stoploss_);
  void RollOver();

  // maintaining trade records
  spread_trade_record_t* open_trade_;
  std::vector<spread_trade_record_t*> trade_history_;

  // maintaining time records
  std::vector<time_record_t*> time_history_;

  // actual realized positions - from spread_exec
  int om_position_1_;
  int om_position_2_;

  // overnight save-worthy variables from spread_exec
  int sw_des_pos_pass_leg_;
  int sw_des_pos_agg_leg_;
  double sw_tgt_sprd_;
  double sw_sprd_alpha_;
  double sw_sprd_beta_;
  int sw_pos_pass_leg_;
  int sw_pos_agg_leg_;
  double sw_hedge_ratio_;

  bool is_banned;

  // utility functions
  void CheckAndUpdateSpreadVector(double px1_, double px2_);
  void FillAndPopulateTradeRecords(int t_pos_desired_);
  void DumpTradeStats();
  void DumpTimeStats();
  void AdjustTimeOnNewDay(uint64_t time_);

  // spread computation
  double ComputeCurrentAvgSpread();
  double ComputeCurrentInstantaneousSpread(double px1, double px2);

  // debug functions
  void DumpLRData();
  HFSAT::DebugLogger& dbglogger_;

  // check if trade is disallowed because of cooloff or getflat considerations
  bool IsTradeAllowed(int t_pos_);

  // to compute pnl volatility
  double ComputePnlVol();

  // Kalman classes
  Kalman::KalmanReg* kalman_;
  std::vector<double> kalman_x;
  std::vector<double> kalman_P;
  std::vector<double> kalman_Q;
  std::vector<double> kalman_R;
  std::vector<double> kalman_Y;
  std::vector<double> kalman_H;

  bool use_adjusted_data_;
  bool live;

  // maps to store adf and hlife values and trailing returns
  // adf and hlife stats are not serialized - maps don't support serialization
  std::map<int, double> date_to_hlife_;
  std::map<int, double> date_to_adf_;
  double curr_day_hlife_;
  double curr_day_adf_;
  double trailing_ret_;

  // Earnings dates are indexed ( to account for weekends, holidays etc ).
  // index is per alldates table of database.
  // indexes are used to automatically getflat
  // None of these need to be serialized since they are always called
  // externally when process starts.
  std::vector<int> earnings_dates_indices_;
  std::map<int, int> all_dates_indices_;
  int current_day_comparison_index_;

  // zscore value at last getflat
  double zscore_ema_;
  double zscore_at_last_getflat_;
  void CheckAndSetZScoreGetflat(double t_curr_zscore_);

  // templatized so as I think boost will call serialize over all the data types as they would
  // need to store metadata
  template <class Archive>
  void serialize(Archive& ar, const unsigned int /*  file_version */) {
    ar& unit_size_position_& current_drawdown_& highest_nav_so_far_& max_drawdown_;
    ar& filename_;
    ar& spread_values_;
    ar& spread_value_snaptimes_;
    ar& current_spread_;
    ar& traded_spread_difference_;
    ar& sum_x_& sum_x2_& diff_stdev_& stdev_ready_;
    ar& hist_prices_1_;
    ar& hist_prices_2_;
    ar& sum_prices_1_& sum_prices_2_& sum_prod_prices_& sum_prices2_sqr_;
    ar& hist_logpx_1_;
    ar& hist_logpx_2_;
    ar& sum_logpx_1_& sum_logpx_2_& sum_prod_logpx_& sum_logpx2_sqr_;
    ar& beta_& intercept_;
    ar& last_opentrade_type_& last_exec_type_& last_exec_entry_time_& last_trade_stopped_out_;
    ar& last_mkt_trade_time_1_0_& last_mkt_trade_time_1_1_& last_mkt_trade_time_2_0_& last_mkt_trade_time_2_1_;
    ar& last_close_px_1_0_& last_close_px_1_1_& last_close_px_2_0_& last_close_px_2_1_& last_bardata_time_;
    ar& num_trades_;
    ar& trade_returns_;
    ar& annualized_returns_& initial_nav_& annualized_stdev_;
    ar& is_expiry_day_& has_rolled_over_& last_expiry_check_tm_;
    ar& ready_to_trade_;
    ar& om_position_1_& om_position_2_;
    ar& param_& param_next_month_;
    ar& open_trade_;
    ar& trade_history_;
    ar& time_history_;  // TODO_DBG - This one is giving segfault into the boost library

    // spread_exec variables
    ar& sw_des_pos_pass_leg_;
    ar& sw_des_pos_agg_leg_;
    ar& sw_tgt_sprd_;
    ar& sw_sprd_alpha_;
    ar& sw_sprd_beta_;
    ar& sw_pos_pass_leg_;
    ar& sw_pos_agg_leg_;
    ar& sw_hedge_ratio_;

    // save kalman params for next run
    ar& kalman_x;
    ar& kalman_P;
    ar& kalman_Q;
    ar& kalman_R;
    ar& kalman_Y;
    ar& kalman_H;

    ar& use_adjusted_data_;
    
    ar& curr_day_hlife_ & curr_day_adf_;
    ar& trailing_ret_;
    ar& zscore_ema_& zscore_at_last_getflat_;
  }

 public:
  void ReInitializeFromStaleState(ParamSet* param) { param_ = param; }
};
}
#endif
