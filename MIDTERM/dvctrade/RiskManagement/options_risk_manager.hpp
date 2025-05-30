/*
  \file RiskManagement/options_risk_manager.hpp

  \author: (c) Copyright Two Roads Technological Solutions Pvt Ltd 2011
  Address:
  Suite No 353, Evoma, #14, Bhattarhalli,
  Old Madras Road, Near Garden City College,
  KR Puram, Bangalore 560049, India
  +91 80 4190 3551
 */

#pragma once

#include <map>
#include <stdio.h>

#include "dvctrade/ExecLogic/base_trading_defines.hpp"
#include "dvccode/Utils/multicast_sender_socket.hpp"
#include "dvccode/CommonTradeUtils/economic_events_manager.hpp"
#include "baseinfra/TradeUtils/price_volatility_listener.hpp"
#include "dvccode/CommonTradeUtils/trading_stage_manager.hpp"
#include "dvccode/CommonTradeUtils/throttle_manager.hpp"
#include "baseinfra/MarketAdapter/shortcode_security_market_view_map.hpp"
#include "baseinfra/OrderRouting/order_manager_listeners.hpp"
#include "baseinfra/SmartOrderRouting/smart_order_manager.hpp"
#include "baseinfra/MarketAdapter/security_market_view_change_listener.hpp"
#include "baseinfra/MarketAdapter/security_market_view.hpp"
#include "dvctrade/Indicators/indicator_list.hpp"
#include "dvccode/CommonTradeUtils/date_time.hpp"
#include "dvctrade/OptionsHelper/option_vars.hpp"
#include "dvccode/Utils/CPUAffinity.hpp"
#include "dvctrade/ModelMath/mult_model_creator.hpp"
#include "dvctrade/OptionsHelper/base_risk_premium_logic.hpp"
#include "baseinfra/SmartOrderRouting/mult_base_pnl.hpp"

namespace HFSAT {

class OptionsRiskManager : public TimePeriodListener,
                           public PositionChangeListener,
                           public ExecutionListener,
                           public OptionsRiskChangeListener,
                           public SecurityMarketViewChangeListener,
                           public MarketDataInterruptedListener,
                           public ControlMessageListener,
                           public ModelMathListener {
 public:
  OptionsRiskManager(DebugLogger& _dbglogger_, const Watch& _watch_, BaseOptionRiskPremium* _options_risk_premium_,
                     std::vector<SecurityMarketView*> _underlying_market_view_,
                     std::vector<SmartOrderManager*> _order_manager_,std::vector<BaseModelMath*> fut_modelmath_vec_,
                     std::map<std::string, std::vector<SmartOrderManager*> > _shc_const_som_map_,
                     std::vector<MultBasePNL*> _mult_base_pnl_vec, MultBasePNL* _total_base_pnl_,
                     int _trading_start_mfm_, int _trading_end_mfm_, bool _livetrading_, int _runtime_id_,
                     SecurityNameIndexer& _sec_name_indexer_);

  ~OptionsRiskManager(){};  ///< destructor not made virtual ... please do so if child classes are made

  const Watch& watch_;
  DebugLogger& dbglogger_;
  SecurityNameIndexer& sec_name_indexer_;

  // this one to check risk and adjust premiums
  BaseOptionRiskPremium* options_risk_premium_;

  // to getflat on delta
  const std::vector<SecurityMarketView*> underlying_market_view_vec_;
  std::vector<SmartOrderManager*> order_manager_vec_;

  // to access future parameters if any and
  // global max loss for underlying
  std::vector<OptionsParamSet*> paramset_vec_;

  // current option trading status variables
  std::vector<std::vector<OptionVars*> > options_data_matrix_;

  // fut model related parameters
  std::vector<BaseModelMath*> fut_modelmath_vec_;
  std::vector<double> hist_stdev_;
  std::vector<TradeVars_t> current_tradevarset_vec_;
  std::vector<TradeVars_t> long_tradevarset_vec_;
  std::vector<TradeVars_t> short_tradevarset_vec_;
  TradeVars_t  closeout_zeropos_tradevarset_;

  // current futures trading status variables
  std::vector<OptionVars*> underlying_data_vec_;

  // base Pnl vector to keep track of risk and delta per underlying
  std::vector<MultBasePNL*> mult_base_pnl_vec;
  MultBasePNL* total_base_pnl_;

  // Keeps track of Pnl Samples (Only doing it per underlying basis)
  std::vector<int> pnl_sampling_timestamps_;
  std::vector<int> pnl_samples_;
  std::vector<std::vector<int> > pnl_samples_vec_;
  unsigned int sample_index_;

  int trading_start_utc_mfm_;
  int trading_end_utc_mfm_;

  int num_underlying_;

  // delta hedge because of futures position, same as -fut_position
  std::vector<double> delta_hedged_;
  // total underlying delta - delta_hedged
  std::vector<double> delta_unhedged_;

  // To keep the value of total risk in trades file consistent in case delta of some underlying changes
  std::vector<double> prev_delta_unhedged_;

  std::vector<double> total_delta_per_underlying_;
  std::vector<double> total_gamma_per_underlying_;
  std::vector<double> total_vega_per_underlying_;
  std::vector<double> total_theta_per_underlying_;

  bool is_max_loss_reached_global_;
  // Band of threshold to get delta hedge
  std::vector<double> upper_threshold_vec_;
  std::vector<double> lower_threshold_vec_;

  std::vector<double> pnl_per_underlying_;
  std::vector<double> min_pnl_per_underlying_;

  // everything included ( all futures and all options )
  double global_max_loss_;
  double total_pnl_;
  double total_pos_;  // This can be beta adjusted position also
  double min_pnl_;

  double interest_rate_;

  std::vector<bool> should_be_getting_hedge_;
  std::vector<bool> should_be_getting_aggressive_hedge_;

  // Control
  bool getting_flat_;
  bool aggressively_getting_flat_;

  bool livetrading_;
  int runtime_id_;

  std::map<unsigned int, std::pair<int, int> > security_id_prod_idx_map_;

  void GetDeltaHedgeLogic(int _product_index_);
  void GetDeltaHedgeLogic1(int _product_index_);
  void GetDeltaHedgeLogic2(int _product_index_);
  void GetDeltaHedgeLogic3(int _product_index_);

  void GetDeltaHedge(int _product_index_, int position_);
  void GetAggressiveDeltaHedge(int _product_index_, int position_);
  // just in case future orders go below best price
  void CallPlaceCancelNonBestLevels(int _product_index_);

 public:
  // processgetflat calls shouldbegettingflat
  void ProcessGetFlat(int _product_index_, int _option_index_);
  void ProcessGetFlat(int _product_index_);
  bool ShouldBeGettingFlat(int _product_index_, int _option_index_, OptionVars* current_exec_vars_);

  // exec logic call this
  void GetFlatTradingLogic(int _product_index_, int _option_index_);

  // get flat trading logic chooses one of this ( private ? )
  void PassiveGetFlatTradingLogic(int _product_index_, int _option_index_);
  void AggressiveGetFlatTradingLogic(int _product_index_, int _option_index_);

  // futures positions
  // double GetHedgedAmount(int _product_index_) { return delta_hedged_[_product_index_]; }
  // total exposure in terms of futures
  // double GetUnhedgedAmount(int _product_index_) { return delta_unhedged_[_product_index_]; }

  // For underlying model
  bool UpdateTarget(double _new_target_, double _new_sum_vars_, int _product_index_);
  bool UpdateTarget(double _prob_decrease_, double _prob_nochange_, double _prob_increase_, int _modelmath_index_ = 0) { return true;}
  void TargetNotReady() {};

  void OnMarketDataInterrupted(const unsigned int _security_id_, const int msecs_since_last_receive_);
  void OnMarketDataResumed(const unsigned int _security_id_);
  void OnMarketUpdate(const unsigned int _security_id_, const MarketUpdateInfo& _market_update_info_);
  void OnTradePrint(const unsigned int _security_id_, const TradePrintInfo& _trade_print_info_,
                    const MarketUpdateInfo& _market_update_info_);

  // adjust vars
  void OnExec(const int _new_position_, const int _exec_quantity_, const TradeType_t _buysell_, const double _price_,
              const int r_int_price_, const int _security_id_);

  // adjust risk
  void OnPositionChange(int t_new_position_, int position_diff_, const unsigned int _security_id_);

  void InitThrottleManager(std::vector<ThrottleManager*>& t_throttle_manager_);

  // risk could change because of onpositionchange ( from som ) or otp from greeks ( delta )
  void OnOptionRiskChange(unsigned int _security_id_, double change_delta_, double change_gamma_, double change_vega_,
                          double change_theta_);

  // calls delta hedge and nnblm
  virtual void OnTimePeriodUpdate(const int num_pages_to_add_);
  inline void OnControlUpdate(const ControlMessage& _control_message_, const char* symbol, const int trader_id);

  // Functions required to load positions in tradeinit
  void LoadPositions(std::string position_file_);
  bool SetPositionOffset(int t_position_offset_, int _product_index_, int _option_index_, double t_exec_price_);

  void SendTradeAndLog(int _int_px_, int _size_, TradeType_t _buysell_, char _level_indicator_, int _product_index_,
                       int _option_index_);
  void ReportResults(HFSAT::BulkFileWriter& trades_writer_, bool _conservative_close_ = false);
  void SendMail(std::string _mail_content_, std::string _mail_subject_);
};
}
