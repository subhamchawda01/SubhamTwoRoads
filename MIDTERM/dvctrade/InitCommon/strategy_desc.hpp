/**
   \file InitCommon/strategy_desc.hpp

   \author: (c) Copyright Two Roads Technological Solutions Pvt Ltd 2011
   Address:
   Suite No 353, Evoma, #14, Bhattarhalli,
   Old Madras Road, Near Garden City College,
   KR Puram, Bangalore 560049, India
   +91 80 4190 3551
 */

#pragma once

#include <iostream>
#include <vector>

#include "baseinfra/MarketAdapter/security_market_view.hpp"
#include "dvctrade/ExecLogic/LFI_trading_manager.hpp"
#include "dvctrade/ExecLogic/curve_trading_manager.hpp"
#include "dvctrade/ExecLogic/di1_trading_manager.hpp"
#include "dvctrade/ExecLogic/equity_trading_2.hpp"
#include "dvctrade/ExecLogic/exec_interface.hpp"
#include "dvctrade/ExecLogic/minrisk_trading_manager.hpp"
#include "dvctrade/ExecLogic/mult_exec_interface.hpp"
#include "dvctrade/ExecLogic/options_exec_interface.hpp"
#include "dvctrade/ExecLogic/pairs_trading.hpp"
#include "dvctrade/ExecLogic/portfolio_trading_interface.hpp"
#include "dvctrade/ExecLogic/structured_trading_manager.hpp"
#include "dvctrade/ExecLogic/trading_manager.hpp"

namespace HFSAT {

/** @brief POD struct containing information in the line that specifies how to load up the strategy
 *
 * POD so that we can make a vector of it
 */

struct PortfolioStrategy {
  std::string strategy_name_;         // like NSE_PORT
  std::string strategy_type_;         // like MeanRevertingTrading
  std::string global_paramfilename_;  // All the global parameters mentioned like thresholds
  std::string prod_filename_;         // This file has got all the products that we want to trade on
  ttime_t trading_start_ttime_t_;
  ttime_t trading_end_ttime_t_;
  int trading_start_utc_mfm_;
  int trading_end_utc_mfm_;
  int runtime_id_;
  MultExecInterface* exec_;
  // other one
  PortfolioTradingInterface* port_interface_;

  PortfolioStrategy()
      : strategy_name_("invalid"),
        strategy_type_("invalid"),
        global_paramfilename_(""),
        prod_filename_(""),
        trading_start_ttime_t_(ttime_t(time_t(0), 0)),
        trading_end_ttime_t_(ttime_t(time_t(0), 0)),
        trading_start_utc_mfm_(8 * 60 * 60 * 1000),
        trading_end_utc_mfm_(8 * 60 * 60 * 1000),
        runtime_id_(0),
        exec_(nullptr),
        port_interface_(nullptr) {}

  const PortfolioStrategy& operator=(const PortfolioStrategy& p) {
    strategy_name_ = p.strategy_name_;
    strategy_type_ = p.strategy_type_;
    global_paramfilename_ = p.global_paramfilename_;
    prod_filename_ = p.prod_filename_;
    trading_start_ttime_t_ = p.trading_start_ttime_t_;
    trading_end_ttime_t_ = p.trading_end_ttime_t_;
    trading_start_utc_mfm_ = p.trading_start_utc_mfm_;
    trading_end_utc_mfm_ = p.trading_end_utc_mfm_;
    runtime_id_ = p.runtime_id_;
    // exec_ = p.exec_;
    return *this;
  }
};

struct StructuredStrategy {
  int runtime_id_;
  std::string traded_ezone_;
  ttime_t trading_start_ttime_t_;
  ttime_t trading_end_ttime_t_;
  int trading_start_utc_mfm_;
  int trading_end_utc_mfm_;
  std::string common_paramfilename_;
  std::vector<SecurityMarketView*> p_dep_market_view_vec_;
  ExchSource_t exch_traded_on_;
  std::map<std::string, std::vector<BaseTrader*> > shortcode_to_base_trader_vec_;
  std::map<std::string, std::vector<ExecInterface*> > shortcode_to_exec_vec_;
  std::map<std::string, std::vector<ttime_t> > shortcode_to_trading_start_ttime_t_vec_;
  std::map<std::string, std::vector<ttime_t> > shortcode_to_trading_end_ttime_t_vec_;
  std::map<std::string, std::vector<int> > shortcode_to_trading_start_mfm_vec_;
  std::map<std::string, std::vector<int> > shortcode_to_trading_end_mfm_vec_;

  StructuredTradingManager* trading_manager_;
  CurveTradingManager* curve_trading_manager_;
  MinRiskTradingManager* min_risk_trading_manager_;
  TradingManager* common_trading_manager_;  //< this can be used for all execlogics
  EquityTrading2* equity_trading_;
  PairsTrading* pairs_trading_;

  std::string trading_structure_;
  std::string strategy_name_;
  std::string strategy_string_;
  std::string sub_strategy_name_;

  std::vector<std::string> shortcodes_;
  std::map<std::string, std::vector<std::string> > shortcode_to_modelfile_vec_;
  std::map<std::string, std::vector<std::string> > shortcode_to_paramfile_vec_;

  StructuredStrategy()
      : runtime_id_(0),
        traded_ezone_(""),
        trading_start_ttime_t_(ttime_t(time_t(0), 0)),
        trading_end_ttime_t_(ttime_t(time_t(0), 0)),
        trading_start_utc_mfm_(8 * 60 * 60 * 1000),
        trading_end_utc_mfm_(8 * 60 * 60 * 1000),
        common_paramfilename_("invalid"),
        p_dep_market_view_vec_(),
        exch_traded_on_(kExchSourceInvalid),
        shortcode_to_base_trader_vec_(),
        shortcode_to_exec_vec_(),
        trading_manager_(nullptr),
        curve_trading_manager_(nullptr),
        min_risk_trading_manager_(nullptr),
        common_trading_manager_(nullptr),
        equity_trading_(nullptr),
        pairs_trading_(nullptr),
        trading_structure_("invalid"),
        strategy_name_("invalid"),
        strategy_string_("invalid"),
        sub_strategy_name_("invalid"),
        shortcodes_(),
        shortcode_to_modelfile_vec_(),
        shortcode_to_paramfile_vec_() {}

  const StructuredStrategy& operator=(const StructuredStrategy& p) {
    runtime_id_ = p.runtime_id_;
    traded_ezone_ = p.traded_ezone_;
    trading_start_ttime_t_ = p.trading_start_ttime_t_;
    trading_end_ttime_t_ = p.trading_end_ttime_t_;
    trading_start_utc_mfm_ = p.trading_start_utc_mfm_;
    trading_end_utc_mfm_ = p.trading_end_utc_mfm_;
    common_paramfilename_ = p.common_paramfilename_;
    p_dep_market_view_vec_ = p.p_dep_market_view_vec_;
    exch_traded_on_ = p.exch_traded_on_;
    shortcode_to_base_trader_vec_ = p.shortcode_to_base_trader_vec_;
    trading_manager_ = p.trading_manager_;
    strategy_name_ = p.strategy_name_;
    strategy_string_ = p.strategy_string_;
    shortcodes_ = p.shortcodes_;
    shortcode_to_modelfile_vec_ = p.shortcode_to_modelfile_vec_;
    shortcode_to_paramfile_vec_ = p.shortcode_to_paramfile_vec_;
    return *this;
  }
};

struct StructuredStrategyLine {
  StructuredStrategy structured_strategy_;

  StructuredStrategyLine() : structured_strategy_() {}
};

struct StrategyLine {
  // inputs
  int runtime_id_;  ///< used for control screen, <dep_exchange_symbol_, runtime_id_> is used to send messages to the
  /// strategy from the control_screen

  // these variables are to be set during strategy initialization after trading date is known
  ttime_t trading_start_ttime_t_;
  ttime_t trading_end_ttime_t_;
  int trading_start_utc_mfm_;
  int trading_end_utc_mfm_;

  SecurityMarketView* p_dep_market_view_;
  ExchSource_t exch_traded_on_;
  BaseTrader* p_base_trader_;
  ExecInterface*
      exec_;  ///< (not set from the file) this is computed after Trading engines are created for the strategy.

  std::string strategy_full_line_;  ///< the full text based on which this strategy line has been created

  // inputs
  std::string dep_shortcode_;  ///< the shortcode of the dependant
  std::string strategy_name_;  ///< name like "PriceBasedTrading"
  std::string modelfilename_;  ///< the model, which will govenr how the signal is calculated
  std::string paramfilename_;  ///< the parameters for the trading strategy "PriceBasedTrading"
  std::string traded_ezone_;
  TradingManager* trading_manager_;  ///< super abstract framework for managing interation of multiple exec_logics
  ExecInterface* pair_exec_;

  StrategyLine()
      : runtime_id_(0),
        trading_start_ttime_t_(ttime_t(time_t(0), 0)),
        trading_end_ttime_t_(ttime_t(time_t(0), 0)),
        trading_start_utc_mfm_(8 * 60 * 60 * 1000),
        trading_end_utc_mfm_(8 * 60 * 60 * 1000),
        p_dep_market_view_(nullptr),
        exch_traded_on_(kExchSourceInvalid),
        p_base_trader_(nullptr),
        exec_(nullptr),
        strategy_full_line_(""),
        dep_shortcode_("invalid"),
        strategy_name_("invalid"),
        modelfilename_("invalid"),
        paramfilename_("invalid"),
        traded_ezone_(""),
        trading_manager_(nullptr),
        pair_exec_(nullptr) {}
};

class StrategyDesc {
 public:
  std::string strategy_desc_filename_;
  bool is_structured_trading_strategy_;
  bool is_combined_retail_trading_;
  bool is_portfolio_trading_strategy_;
  std::vector<StrategyLine> strategy_vec_;
  std::vector<StructuredStrategy> structured_strategy_vec_;
  std::vector<PortfolioStrategy> portfolio_strategy_vec_;
  std::vector<std::string> spread_modelfile_vec_;
  std::vector<std::string> spread_paramfile_vec_;
  std::vector<std::vector<std::string> > spread_shortcode_vec_;
  std::string simconfig_filename_;  // This is multiplexed to be sim-config-filename in SIM and strategy-name in LIVE.
  StrategyDesc(DebugLogger& t_dbglogger_, const std::string& _strategy_desc_filename_, const int tradingdate_);
  ttime_t GetMinStartTime();
  ttime_t GetMaxEndTime();
  bool AllDependantsSame();
  static std::string GetRollParam(std::string paramfile_, const int tradingdate_);
  inline bool IsStructuredStrategy() { return is_structured_trading_strategy_; };
  inline bool IsCombinedRetailStrategy() { return is_combined_retail_trading_; };
  inline bool IsPortfolioTradingStrategy() { return is_portfolio_trading_strategy_; };
  void LoadStructuredStrategy(DebugLogger& t_dbglogger_, std::string structured_strategy_filename_,
                              StructuredStrategy& structured_strategy_, const int tradingdate_);

 protected:
  void GetTimeAndMFMFromString(int _tradingdate_, ttime_t& _time_, int& _mfm_, const char* _tz_hhmm_str_, bool is_end_);
};
}
