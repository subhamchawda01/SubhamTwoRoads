/**
    \author: (c) Copyright Two Roads Technological Solutions Pvt Ltd 2011
     Address:
         Suite No 353, Evoma, #14, Bhattarhalli,
  Old Madras Road, Near Garden City College,
  KR Puram, Bangalore 560049, India
  +91 80 4190 3551
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

// usermsg and params
#include "dvccode/ORSMessages/control_message_listener.hpp"
#include "dvctrade/InitCommon/sbe_paramset.hpp"

// market data api
#include "baseinfra/TradeUtils/market_update_manager_listener.hpp"
#include "baseinfra/MarketAdapter/security_market_view_change_listener.hpp"
#include "baseinfra/MarketAdapter/security_market_view.hpp"

// ors and pnl
#include "baseinfra/OrderRouting/order_manager_listeners.hpp"
#include "baseinfra/SmartOrderRouting/smart_order_manager.hpp"
#include "baseinfra/SmartOrderRouting/pnl_listeners.hpp"
#include "baseinfra/SmartOrderRouting/mult_base_pnl.hpp"

// signals risk api
#include "dvctrade/ExecLogic/sbe_risk_reader_listener.hpp"

namespace HFSAT {
  // read or receive orders  // onmarketupdate // sychronization and other steps // start/stop // onexec
  class SBEInterface : public SBERiskReaderListener,
		       public SecurityMarketViewChangeListener,
		       public TimePeriodListener,
		       public ControlMessageListener,
		       public ExecutionListener,
		       public MultBasePNLListener {
  public:
    DebugLogger& dbglogger_;
    const Watch& watch_;

    std::vector<SecurityMarketView*> dep_market_view_vec_;
    std::vector<SmartOrderManager*> order_manager_vec_;

    std::vector<SBEParamSet*> paramset_vec_;

    int trading_start_utc_hhmm_;
    int trading_end_utc_hhmm_;

    const bool livetrading_;


    SBEInterface(DebugLogger& _dbglogger_, const Watch& _watch_,
		 const std::vector<SecurityMarketView*> _dep_market_view_vec_,
		 const std::vector<SmartOrderManager*> _order_manager_vec_,
		 const std::vector<SBEParamSet*> _sbe_paramset_vec_,
		 const bool _livetrading_,
		 int _trading_start_utc_mfm_, int _trading_end_utc_mfm_)
      : dbglogger_(_dbglogger_),
	watch_(_watch_),
	dep_market_view_vec_(_dep_market_view_vec_),
	order_manager_vec_(_order_manager_vec_),
        paramset_vec_(_sbe_paramset_vec_),
	trading_start_utc_hhmm_(_trading_start_utc_mfm_),
	trading_end_utc_hhmm_(_trading_end_utc_mfm_),
	livetrading_(_livetrading_) {
      for (auto i = 0u; i < dep_market_view_vec_.size(); i++) {
	order_manager_vec_[i]->AddExecutionListener(this);
	//orderrejected ?
      }
    }

    virtual ~SBEInterface() {}
    virtual void ReportResults(BulkFileWriter& trades_writer_, bool conservative_close_ = false) {}

  
  // collect trading info using various static functions
  // this is expecting param file in following format
  // params_filename_list contains list of params { child_param_filename }, one correponding to each intrument
    static void CollectParamSetVec(std::vector<SBEParamSet*>& _paramset_vec_, const std::string& _paramfile_listname_) {
      std::ifstream t_paramlist_stream_;
      t_paramlist_stream_.open(_paramfile_listname_.c_str(), std::ifstream::in);
      if (t_paramlist_stream_.is_open()) {
	const int kParamFileLineBufferLen = 1024;
	char readline_buffer_[kParamFileLineBufferLen];
	bzero(readline_buffer_, kParamFileLineBufferLen);
      
	while (t_paramlist_stream_.good()) {
	  bzero(readline_buffer_, kParamFileLineBufferLen);
	  t_paramlist_stream_.getline(readline_buffer_, kParamFileLineBufferLen);
	
	  std::string t_param_filename_(readline_buffer_);
	  boost::trim(t_param_filename_);
	  if (t_param_filename_ == "") continue;
	  _paramset_vec_.push_back(new SBEParamSet(t_param_filename_));
	}
	t_paramlist_stream_.close();
      }
    }
  };
}
