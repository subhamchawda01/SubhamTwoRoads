/**
    \author: (c) Copyright Two Roads Technological Solutions Pvt Ltd 2011
     Address:
         Suite No 353, Evoma, #14, Bhattarhalli,
         Old Madras Road, Near Garden City College,
         KR Puram, Bangalore 560049, India
         +91 80 4190 3551
**/

#pragma once

#include "dvctrade/ExecLogic/portfolio_trading_interface.hpp"

namespace HFSAT {
  class BasePortTrading : public PortfolioTradingInterface {

  public:
    BasePortTrading(DebugLogger& _dbglogger_, const Watch& _watch_,
		    const std::vector<SecurityMarketView*> _dep_market_view_vec_,
		    const std::vector<SmartOrderManager*> _order_manager_vec_,
		    const bool _livetrading_, MultBasePNL* _mult_base_pnl_, // this module can be efficient->sum(base_pnl(i))
		    int _trading_start_utc_mfm_, int _trading_end_utc_mfm_);

    ~BasePortTrading() {}


    // PortRiskListener
    void OnNewOrderFromStrategy(std::string _instrument_, std::string _strat_id_, int _order_lots_, double _ref_px_) {}
    void OnNewPositionFromStrategy(std::string _instrument_, std::string _strat_id_, int _position_lots_, double _new_ref_px_) {}
    void OnNewPortRiskFromStrategy(int _strat_id_, std::vector<int>& _signal_risk_vec_, std::vector<double>& _ref_px_);

    // SecurityMarketViewChangeListener
    // we need to modify our orders as best prices change
    void OnMarketUpdate(const unsigned int _security_id_, const HFSAT::MarketUpdateInfo& _market_update_info_);
    // bid_price / ask_price / trade_price (?)
    void OnTradePrint(const unsigned int _security_id_, const HFSAT::TradePrintInfo& _trade_print_info_,
		      const HFSAT::MarketUpdateInfo& _market_update_info_){}
   
    // ControlMessageListener: need to choose specifics ( start / stop / show-risk / reset-maxloss / ignore-events )
    void OnControlUpdate(const ControlMessage& _control_message_, const char* symbol,
			 const int trader_id) {}
    
    // ExecutionListener: compute risk / pnl / feed to tradinglogic
    void OnExec(const int _new_position_, const int _exec_quantity_, const HFSAT::TradeType_t _buysell_,
		const double _price_, const int r_int_price_, const int _security_id_);

    // MultBasePNLListener : we should avoid subscribing, we fetch only when required ( saves calls )
    void UpdatePNL(int _total_pnl_) {}

    //@ core logic functions
    // deals only with best level orders
    // works based on risk_from_strategy
    virtual void L1TradingLogic(int _security_id_);
    // deals with non best level orders ( readies itself for executions, think about msgs though )
    virtual void L2TradingLogic(int _security_id_) {}
    // moves to freeze state, might still receive orders from strategy
    virtual void CancelAndFreeze(int _security_id_) {}
    // works based on risk_from_exchange
    virtual void GetFlat(int _security_id_);

    // final output
    void ReportResults(HFSAT::BulkFileWriter& trades_writer_, bool _conservative_close_ = true);

    MultBasePNL* mult_base_pnl_;

  protected:
    // position to operate upon = (position_from_signal_ - position_from_exchange_)
    // there is no reason to seperate per strategy given our decisions are going to be based on overall numbers
    // these ids correspond to security_id
    std::vector<int> risk_from_exchange_;
    std::vector<int> risk_from_signal_;

    // difference between these two should be coming from ( execution_slippage )
    std::vector<double> signal_tick_pnl_;
    std::vector<double> real_tick_pnl_;

    // we could look into maintaing basing on strategy_id ( but for what ? )
    // if we independantly evalute S1 and S2 performance that should be suffice
    // combining S1 + S2 should only make things better ( more than sum pnl )

    const SecurityNameIndexer & sec_name_indexer_;
  };
}
