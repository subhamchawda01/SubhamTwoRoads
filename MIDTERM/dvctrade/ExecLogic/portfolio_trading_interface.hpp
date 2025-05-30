/**
    \author: (c) Copyright Two Roads Technological Solutions Pvt Ltd 2011
     Address:
         Suite No 353, Evoma, #14, Bhattarhalli,
         Old Madras Road, Near Garden City College,
         KR Puram, Bangalore 560049, India
         +91 80 4190 3551
**/


/*
we are looking to generalize portfolio trading

TICKDATA
|      |
|      |
|      |
|      MODULE_S1/S2/S3.. -- prepares the frequency of the data : computes signal : compute and propagatees risk
|      |
|      |
|      |
MODULE_E -- takes care of execution

we are looking to keep one execution_module_paramset
and signal modules will operate on their own signal_module_paramset

 */

#pragma once

// boost trim
#include <boost/algorithm/string/trim.hpp>

// log and other
#include "dvccode/CDef/file_utils.hpp"
#include "dvccode/CDef/debug_logger.hpp"
#include "dvccode/Utils/bulk_file_writer.hpp"

// watch and other
#include "dvccode/CommonTradeUtils/watch.hpp"
#include "dvccode/CommonTradeUtils/new_midnight_listener.hpp"

// market data api
#include "baseinfra/TradeUtils/market_update_manager_listener.hpp"
#include "baseinfra/MarketAdapter/security_market_view_change_listener.hpp"
#include "baseinfra/MarketAdapter/security_market_view.hpp"

// ors and pnl
#include "baseinfra/OrderRouting/order_manager_listeners.hpp"
#include "baseinfra/SmartOrderRouting/smart_order_manager.hpp"
#include "baseinfra/SmartOrderRouting/pnl_listeners.hpp"
#include "baseinfra/SmartOrderRouting/mult_base_pnl.hpp"


// usermsg
#include "dvccode/ORSMessages/control_message_listener.hpp"

// these are execution control params only
// include signal param in signal module
#include "dvctrade/InitCommon/port_exec_paramset.hpp"

// signals risk api
// mention the risk in which your exec can understand
// virtual onportriskupdate()
#include "dvctrade/ExecLogic/port_risk_listener.hpp"



namespace HFSAT {
  class PortfolioTradingInterface : public SecurityMarketViewChangeListener,
				    public ExecutionListener,
				    public MultBasePNLListener,
				    public ControlMessageListener,
				    public PortRiskListener {
  public:
    DebugLogger& dbglogger_;
    const Watch& watch_;

    std::vector<SecurityMarketView*> dep_market_view_vec_;
    std::vector<SmartOrderManager*> order_manager_vec_;

    int trading_start_utc_hhmm_;
    int trading_end_utc_hhmm_;

    const bool live_trading_;
    static std::vector<PortExecParamSet*> paramset_vec_;

    PortfolioTradingInterface(DebugLogger& _dbglogger_, const Watch& _watch_,
			      const std::vector<SecurityMarketView*> _dep_market_view_vec_,
			      const std::vector<SmartOrderManager*> _order_manager_vec_,
			      const bool _livetrading_,
			      int _trading_start_utc_mfm_, int _trading_end_utc_mfm_)
      : dbglogger_(_dbglogger_),
	watch_(_watch_),
	dep_market_view_vec_(_dep_market_view_vec_),
	order_manager_vec_(_order_manager_vec_),
      trading_start_utc_hhmm_(_trading_start_utc_mfm_),
      trading_end_utc_hhmm_(_trading_end_utc_mfm_),
      live_trading_(_livetrading_) {
      for (auto i = 0u; i < dep_market_view_vec_.size(); i++) {
	order_manager_vec_[i]->AddExecutionListener(this);
      }
    }

    virtual ~PortfolioTradingInterface() {}

    // inherited from initlogic 
    virtual void ReportResults(BulkFileWriter& trades_writer_, bool conservative_close_ = false) {}

    // to tell initlogic what it needs to prepare itself
    static void CollectPortExecParamSetVec(std::vector<PortExecParamSet*>& _paramset_vec_,
					   const std::string& _paramfile_listname_);
  };	    
}

