/**
    \author: (c) Copyright Two Roads Technological Solutions Pvt Ltd 2011
     Address:
         Suite No 353, Evoma, #14, Bhattarhalli,
  Old Madras Road, Near Garden City College,
  KR Puram, Bangalore 560049, India
  +91 80 4190 3551
*/

#pragma once

#include "dvctrade/ExecLogic/sbe_interface.hpp"
#include "baseinfra/SmartOrderRouting/mult_base_pnl.hpp"

/*
usually execution path depends on current risk,
a) if we measure risk of individual using its own position and take decisions independant
b) if we measure risk of porfolio ( defined by some funtion(position_vec_) and then decisions become dependant
c) to make things easier we restrict for now from using non_self information, that will open up another direction

objectivity of the logic is to bring the risk to 0, 
risk is affected by onExec and onNewOrderFromStrategy ( that is we receive executions from both )
( we can handle both functions in same manner, except we probably can negate some orders )


PARAM_FILE
OFFLINE_SIGNALINFO FILENAME1 FILENAME2 ....
ONLINE_SIGNALINFO MODULE1 MODULE2 ..
SIGNAL_WEIGHTS 0.4 0.5 0.1 / ALGO

TRADING_TIME START_TIME END_TIME

EXECPARAMS_FILE
THRESHOLDS
POSITION_CONSTRAINTS
PNL_CONSTRAINTS
EXECHELPER X

we intend to listen signals suggesting to buy / sell the porfolio from multiple sources
a) this exec process those buy / sell signals (normalized) OnTimePeriod
b) maintains risk per signal (current_pnl, min_pnl, open_pnl, open_position)
c) we are using smv, som, multbasepnl and portexecinterface 
*/

namespace HFSAT {
  class SignalBasedTrading : public SBEInterface {
  public:
// there is only one portfolio used by multiple signals, <shortcodes> <weights>
/*  static void CollectTradingInfo(DebugLogger& _dbglogger_, std::string _model_filename,
				 std::vector<std::string>& _source_shortcode_vec_,
				 std::vector<std::string>& _ors_shortcode_vec_,
				 std::vector<double>& _constituents_weights_);*/


    SignalBasedTrading(DebugLogger& _dbglogger_, const Watch& _watch_,
		       const std::vector<SecurityMarketView*> _dep_market_view_vec_,
		       const std::vector<SmartOrderManager*> _order_manager_vec_,
		       const std::vector<SBEParamSet*> _sbe_paramset_vec_, const bool _livetrading_,
		       MultBasePNL* _mult_base_pnl_, int _trading_start_utc_mfm_, int _trading_end_utc_mfm_);

    ~SignalBasedTrading() {}


    // SBERiskReaderListener
    void OnNewOrderFromStrategy(std::string _instrument_, std::string _order_id_, int _order_lots_, double _ref_px_);
    void OnNewPositionFromStrategy(std::string _instrument_, std::string _strat_id_, int _position_lots_, double _new_ref_px_);

    // SecurityMarketViewChangeListener
    void OnMarketUpdate(const unsigned int _security_id_, const HFSAT::MarketUpdateInfo& _market_update_info_);
  // this update is slightly important than onmarketupdate, ofcourse only if we are doing something different
  // for now tradeprint can be ignored, as a 
    void OnTradePrint(const unsigned int _security_id_, const HFSAT::TradePrintInfo& _trade_print_info_,
		      const HFSAT::MarketUpdateInfo& _market_update_info_){}

    // TimePeriodListener
    void OnTimePeriodUpdate(const int num_pages_to_add) {}
    
    // ControlMessageListener
    void OnControlUpdate(const ControlMessage& _control_message_, const char* symbol,
			 const int trader_id) {}
    
    // ExecutionListener
    void OnExec(const int _new_position_, const int _exec_quantity_, const HFSAT::TradeType_t _buysell_,
		const double _price_, const int r_int_price_, const int _security_id_);

    // MultBasePNLListener
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
    std::vector<int> risk_from_exchange_;
    std::vector<int> risk_from_signal_;
    
    const SecurityNameIndexer & sec_name_indexer_;

    int number_of_contracts_;

  };
}
