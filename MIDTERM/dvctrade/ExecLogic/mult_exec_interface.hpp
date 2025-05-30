/*
 =====================================================================================

       Filename:  ExecLogic/mult_exec_interface.hpp

    Description:

        Version:  1.0
        Created:  Monday 30 May 2016 05:06:48  UTC
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

#include "dvccode/CDef/debug_logger.hpp"
#include "dvccode/CommonTradeUtils/new_midnight_listener.hpp"
#include "dvccode/CommonTradeUtils/watch.hpp"
#include "baseinfra/TradeUtils/market_update_manager_listener.hpp"

#include "baseinfra/MarketAdapter/security_market_view_change_listener.hpp"
#include "baseinfra/MarketAdapter/security_market_view.hpp"

#include "dvccode/ORSMessages/control_message_listener.hpp"
#include "dvctrade/InitCommon/global_paramset.hpp"

#include "baseinfra/OrderRouting/order_manager_listeners.hpp"
#include "baseinfra/SmartOrderRouting/smart_order_manager.hpp"
#include "baseinfra/SmartOrderRouting/pnl_listeners.hpp"

#include "dvctrade/RiskManagement/risk_manager.hpp"
#include "dvccode/Utils/bulk_file_writer.hpp"

#include "dvccode/CDef/file_utils.hpp"

#include "baseinfra/OrderRouting/order_manager_listeners.hpp"
#include "baseinfra/SmartOrderRouting/mult_base_pnl.hpp"

namespace HFSAT {

class MultExecInterface : public NewMidnightListener,
                          public PositionChangeListener,
                          public OrderChangeListener,
                          public ExecutionListener,
                          public GlobalPositionChangeListener,
                          public RiskManagerListener,
                          public GlobalPNLChangeListener,
                          public CancelRejectListener,
                          public FokFillRejectListener,
                          public ControlMessageListener,
                          public SecurityMarketViewChangeListener,
                          public MarketDataInterruptedListener,
                          public SecurityMarketViewStatusListener,
                          public ExchangeRejectsListener,
                          public MultBasePNLListener {
 public:
  // base variables needed by all strategy implementations, and in implementing the interface methods
  DebugLogger& dbglogger_;
  const Watch& watch_;
  std::vector<SecurityMarketView*> dep_market_view_vec_;
  std::vector<SmartOrderManager*> order_manager_vec_;
  // std::vector<SecurityMarketView*> dep_market_view_index_vec_;
  // std::vector<SmartOrderManager*> order_manager_index_vec_;
  std::string global_paramfilename_;
  int trading_start_utc_mfm_;
  int trading_end_utc_mfm_;
  const bool livetrading_;
  GlobalParamSet* global_param_set_;
  bool external_freeze_trading_;
  int32_t last_freeze_time_;
  bool freeze_due_to_rejects_;

  // MultBasePNL* mult_base_pnl_ ;

  /**
   *
   * @param _dbglogger_
   * @param _watch_
   * @param _dep_market_view_fut_vec_    : This is the vector of all the products we would like to trqade on
   * @param _dep_market_view_index_vec_  : This is the vector of all the index we would like to observe while trading a
   *portfolio
   * @param _order_manager_fut_vec_      : Order manager vector of all the products we would like to trqade on
   * @param _order_manager_index_vec_    : Order manager vector of all the index we would like to observe while trading
   *a portfolio
   * @param _paramfilename_			   : This is a global param file which contains values like slippage,
   *threshold,
   *sampling time
   * @param _livetrading_
   * @param _trading_start_utc_mfm_	   : Trading start time
   * @param trading_end_utc_mfm_		   : Trading end time
   */
  MultExecInterface(DebugLogger& _dbglogger_, const Watch& _watch_,
                    const std::vector<SecurityMarketView*> _dep_market_view_fut_vec_,
                    const std::vector<SmartOrderManager*> _order_manager_fut_vec_, const std::string& _paramfilename_,
                    const bool _livetrading_, int _trading_start_utc_mfm_, int _trading_end_utc_mfm_)
      : dbglogger_(_dbglogger_),
        watch_(_watch_),
        dep_market_view_vec_(_dep_market_view_fut_vec_),
        order_manager_vec_(_order_manager_fut_vec_),
        global_paramfilename_(_paramfilename_),
        trading_start_utc_mfm_(_trading_start_utc_mfm_),
        trading_end_utc_mfm_(_trading_end_utc_mfm_),
        livetrading_(_livetrading_),
        external_freeze_trading_(false),
        last_freeze_time_(0),
        freeze_due_to_rejects_(false) {
    global_param_set_ = new GlobalParamSet(global_paramfilename_, watch_.YYYYMMDD());
    // mult_base_pnl_->AddListener(this);
    for (auto i = 0u; i < dep_market_view_vec_.size(); i++) {
      order_manager_vec_[i]->AddPositionChangeListener(this);
      order_manager_vec_[i]->AddExecutionListener(this);
      order_manager_vec_[i]->AddCancelRejectListener(this);
      order_manager_vec_[i]->AddFokFillRejectListener(this);
      order_manager_vec_[i]->AddExchangeRejectsListeners(this);
    }
    // LoadGlobalParamSet(_paramfilename_);
  }

  // To make sure memory does not overflow
  virtual ~MultExecInterface() {}

  virtual void SetStartTrading(bool _set_) {}

  // To report results for a portfolio on every trade
  virtual void ReportResults(HFSAT::BulkFileWriter& trades_writer_, bool conservative_close_ = false) = 0;

  // Keeps track of the global position of a portfolio
  virtual int my_global_position() const = 0;

  // Keeps track of the position of outrights portfolio
  // if not implemented in the execlogic, returns 0
  virtual int my_position_for_outright(SecurityMarketView* _dep_market_view_) { return 0; }

  virtual int get_runtime_id_() { return 0; }

  // gets runtime id of exec which inherits this.
  virtual void get_risk(std::vector<std::string>& _instrument_vec_, std::vector<int>& _positon_vector_) {}

  // Get Freeze Due to Exchange Rejects
  void OnGetFreezeDueToExchangeRejects(HFSAT::BaseUtils::FreezeEnforcedReason const& freeze_reason) {
    if (livetrading_) {
      char hostname[128];
      hostname[127] = '\0';
      gethostname(hostname, 127);

      std::ostringstream t_temp_oss;
      t_temp_oss << "Query - " << get_runtime_id_() << " Has Received GetFreeze Due To "
                 << HFSAT::BaseUtils::FreezeEnforcedReasonString(freeze_reason) << " On : " << hostname;
      if (freeze_reason == BaseUtils::FreezeEnforcedReason::kFreezeOnNoResponseFromORS) {
        t_temp_oss << " Query Not Receiving any response from ORS, will not start without manual intervention..";
      }

      HFSAT::Email e;
      e.setSubject(t_temp_oss.str());
      e.addRecepient("nseall@tworoads.co.in");
      e.addSender("nseall@tworoads.co.in");
      if (false == freeze_due_to_rejects_) {
        e.sendMail();
      };

      DBGLOG_TIME_CLASS_FUNC << "Received GetFreeze Due To Rejects.." << DBGLOG_ENDL_FLUSH;
      if (freeze_reason == BaseUtils::FreezeEnforcedReason::kFreezeOnNoResponseFromORS) {
        for (auto itr = order_manager_vec_.begin(); itr < order_manager_vec_.end(); itr++)
          DBGLOG_TIME_CLASS_FUNC << (*itr)->ShowOrders() << DBGLOG_ENDL_FLUSH;
      }

      DBGLOG_DUMP;

      // We'll remain in freeze mode until,
      // 1) Either an unfreeze is given externally
      // 2) We reach timeout
      if (false == freeze_due_to_rejects_) {
        external_freeze_trading_ = true;
        freeze_due_to_rejects_ = true;
        last_freeze_time_ = watch_.msecs_from_midnight();
      }
    }
  }

  void OnResetByManualInterventionOverRejects() {
    char hostname[128];
    hostname[127] = '\0';
    gethostname(hostname, 127);

    std::ostringstream t_temp_oss;
    t_temp_oss << "Query - " << get_runtime_id_() << " Received Auto/External Unfreeze From Rejects On : " << hostname;

    HFSAT::Email e;
    e.setSubject(t_temp_oss.str());
    e.addRecepient("nseall@tworoads.co.in");
    e.addSender("nseall@tworoads.co.in");
    if (true == freeze_due_to_rejects_) {
      e.sendMail();
    };
    freeze_due_to_rejects_ = false;
    external_freeze_trading_ = false;
  };

  /*  void LoadGlobalParamSet(std::string paramfilename_) {
      std::ifstream paramlistfile_;
      paramlistfile_.open(paramfilename_);
      bool paramset_file_list_read_ = false;
      if (paramlistfile_.is_open()) {
        const int kParamfileListBufflerLen = 1024;
        char readlinebuffer_[kParamfileListBufflerLen];
        bzero(readlinebuffer_, kParamfileListBufflerLen);
        while (paramlistfile_.good()) {
          bzero(readlinebuffer_, kParamfileListBufflerLen);
          paramlistfile_.getline(readlinebuffer_, kParamfileListBufflerLen);
          std::string this_line_ = std::string(readlinebuffer_);
          PerishableStringTokenizer st_(readlinebuffer_, kParamfileListBufflerLen);
          const std::vector<const char*>& tokens_ = st_.GetTokens();
          if (tokens_.size() < 1) {
            continue;
          }
          if ((strcmp(tokens_[0], "PARAMVALUE") == 0) && !paramset_file_list_read_) {
            // This is a single paramfile read it normally
            paramlistfile_.close();
            global_param_set_ = GlobalParamSet(paramfilename_, watch_.YYYYMMDD());
            break;
          }
          if (paramlistfile_.is_open()) {
            // may have been closed in case of normal param
            paramlistfile_.close();
          }
        }
      } else {
        std::cerr << "ExecInterface::LoadParamSetVec: can't open paramlistfile_: " << paramfilename_ << " for
    reading\n";
        exit(1);
      }
    }*/
};
}
