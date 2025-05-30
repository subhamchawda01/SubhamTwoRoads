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
#include "dvctrade/Indicators/indicator_list.hpp"
#include "dvctrade/ExecLogic/exec_interface.hpp"
#include "dvctrade/ExecLogic/trade_vars.hpp"
#include "dvctrade/ExecLogic/exec_logic_utils.hpp"
#include "dvctrade/ExecLogic/instrument_info.hpp"
#include "dvctrade/ExecLogic/ExecLogicHelper/indicator_helper.hpp"
#include "dvctrade/ExecLogic/ExecLogicHelper/tradevarset_builder.hpp"
#include "dvccode/CommonTradeUtils/date_time.hpp"
#include "dvctrade/ExecLogic/options_exec_vars.hpp"
#include "dvccode/Utils/CPUAffinity.hpp"

namespace HFSAT {

/** @brief Class to delta hedge a underlying
 *
 */

class DeltaHedge : public TimePeriodListener,
                   public PositionChangeListener,
                   public OrderChangeListener,
                   public ExecutionListener,
                   public IndicatorListener {
 public:
  DeltaHedge(DebugLogger& _dbglogger_, const Watch& _watch_, const SecurityMarketView& _underlying_market_view_,
             SmartOrderManager& _order_manager_, OptionsParamSet paramset_, const bool _livetrading_);

  ~DeltaHedge(){};  ///< destructor not made virtual ... please do so if child classes are made

  std::vector<OptionsExecVars*> this_options_data_vec_;
  const SecurityMarketView& underlying_market_view_;
  SmartOrderManager& order_manager_;
  const Watch& watch_;
  DebugLogger& dbglogger_;
  OptionsParamSet paramset_;

  int trading_start_utc_mfm_;
  int trading_end_utc_mfm_;

  double delta_hedged_;
  double delta_unhedged_;

  double total_delta_;
  double total_gamma_;
  double total_vega_;
  double total_theta_;

  double gamma_ATM_;  // Put Code to keep track of this

  // To decide the band of delta to hedge based on gamma
  double upper_threshold_;
  double lower_threshold_;
  double interest_rate_;

  bool should_be_getting_hedge_;
  bool should_be_getting_aggressive_hedge_;

  const unsigned int security_id_;
  int fractional_second_implied_vol_;

  std::vector<MovingAvgPriceImpliedVol*> moving_average_implied_vol_vec_;

  void GetDeltaHedgeLogic();
  void GetDeltaHedge();
  void GetAggressiveDeltaHedge();

 public:
  void AddOptionsToHedge(OptionsExecVars* option_to_hedge_, int _trading_start_mfm_, int _trading_end_mfm_);

  double GetHedgedAmount() { return delta_hedged_; }
  double GetUnhedgedAmount() { return delta_unhedged_; }

  void OnOptionPositionChange(int t_new_position_, int position_diff_, const unsigned int _option_index_);

  void OnIndicatorUpdate(const unsigned int& _indicator_index_, const double& _new_value_);
  inline void OnIndicatorUpdate(const unsigned int& _indicator_index_, const double& new_value_decrease_,
                                const double& new_value_nochange_, const double& new_value_increase_) {
    return;
  }

  void SendTradeAndLog(int _int_px_, int _size_, TradeType_t _buysell_, char _level_indicator_);

  void OnExec(const int _new_position_, const int _exec_quantity_, const TradeType_t _buysell_, const double _price_,
              const int r_int_price_, const int _security_id_) {}

  void OnPositionChange(int t_new_position_, int position_diff_, const unsigned int _security_id_);

  void OnOrderChange(){};

  void GetDeltaHedgeFlat();
  void GetAggressiveDeltaHedgeFlat();

  void CallPlaceCancelNonBestLevels();

  virtual void OnTimePeriodUpdate(const int num_pages_to_add_);
};
}
