/**
    \author: (c) Copyright Two Roads Technological Solutions Pvt Ltd 2011
     Address:
         Suite No 353, Evoma, #14, Bhattarhalli,
  Old Madras Road, Near Garden City College,
  KR Puram, Bangalore 560049, India
  +91 80 4190 3551
*/

/*
honestly extension of indicator classes we already have

two additional components:
a) combination logic is incorporated with in this module itself ( no modelmath )
b) strength of the signal is decided by this module itself, independant of other signals / execution logic
also these are little more researched and can independantly add good/sufficient alpha to strategy

 */
#pragma once


// logger to write down self steps
#include "dvccode/CDef/debug_logger.hpp"

// most likely we will use this instead market_updates / trade_updates to avoid too many calls 
#include "dvccode/CommonTradeUtils/watch.hpp"

// these are just to turn off signal and move to low risk mode, also mkt data api calls
#include "baseinfra/TradeUtils/market_update_manager_listener.hpp"
#include "baseinfra/MarketAdapter/security_market_view_change_listener.hpp"
#include "baseinfra/MarketAdapter/security_market_view.hpp"
#include "baseinfra/MarketAdapter/shortcode_security_market_view_map.hpp"

// we could use of stop loss ? later
//#include "baseinfra/OrderRouting/order_manager_listeners.hpp"
#include "dvctrade/ExecLogic/port_risk_listener.hpp"

namespace HFSAT {
  // arima adjusted stationary portfolio returns

  /***********START_OF_PARAM_CLASS*************************/

  class ParamSet_BaseSignal {
  public:

    // minimal signal paramaters
    std::vector<std::string> instruments_vec_;

    // minimal signal strength parameters
    double place_threshold_;
    double keep_threshold_;
    double increase_threshold_;
    double decrease_threshold_;

    // minimal signal positioning parameters
    std::vector<int> instrument_lots_vec_;
    double max_portfolio_lots_;

    int signal_start_utc_mfm_;
    int signal_end_utc_mfm_;

    ParamSet_BaseSignal(const std::string& _filename_, const int _tradingdate_);
    void LoadParamSet(const std::string& _filename_, const int _tradingdate_);
    void ToString();
  };
  /***********END_OF_PARAM_CLASS*************************/

  /***********START_OF_SIGNAL_CLASS*************************/
  class Signal_BaseSignal : public SecurityMarketViewChangeListener,
			    public MarketDataInterruptedListener,
			    public TimePeriodListener {
  public:
    DebugLogger& dbglogger_;
    const Watch& watch_;

    std::vector <SecurityMarketView*> shortcodes_vec_;

    const ParamSet_BaseSignal& paramset_;
    PortRiskListener* exec_algo_; // execution_algo

    double alpha_;
    // current position vector for this signal
    std::vector<int> signal_risk_vec_;
    std::vector<double> ref_price_vec_;
    int signal_id_;

    bool is_ready_;

  public:
    Signal_BaseSignal(DebugLogger& _dbglogger_, const Watch& _watch_,
		      const ParamSet_BaseSignal& _paramset_);

    ~Signal_BaseSignal() {}

    // market_interface
    void OnMarketUpdate(const unsigned int _security_id_, const MarketUpdateInfo& _market_update_info_) {}
    inline void OnTradePrint(const unsigned int _security_id_, const TradePrintInfo& _trade_print_info_,
			     const MarketUpdateInfo& _market_update_info_) {}
    void OnMarketDataInterrupted(const unsigned int _security_id_, const int msecs_since_last_receive_) {}
    void OnMarketDataResumed(const unsigned int _security_id_) {}

    virtual void OnTimePeriodUpdate(const int num_pages_to_add) = 0;
    virtual void PropagateNewRisk();

    void add_port_risk_listener(PortRiskListener* _port_risk_listener_) {
      exec_algo_ = _port_risk_listener_;
    }

    inline void NotifyPortRiskListener() {
      exec_algo_->OnNewPortRiskFromStrategy(signal_id_, signal_risk_vec_, ref_price_vec_);
    }
  };
  /***********END_OF_SIGNAL_CLASS*************************/
}
