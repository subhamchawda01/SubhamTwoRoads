// =====================================================================================
//
//       Filename:  mid_term_order_manager.hpp
//
//    Description:
//
//        Version:  1.0
//        Cre  01/07/2016 10:31:11 AM
//       Revision:  none
//       Compiler:  g++
//
//         Author:  (c) Copyright Two Roads Technological Solutions Pvt Ltd 2011
//
//        Address:  Suite No 162, Evoma, #14, Bhattarhalli,
//                  Old Madras Road, Near Garden City College,
//                  KR Puram, Bangalore 560049, India
//          Phone:  +91 80 4190 3551
//
// =====================================================================================

#pragma once
#include "base_order_manager.hpp"
namespace MIDTERM {

// BaseOrderManager with a tcp to listen to all strategies
class MidTermOrderManager : public BaseOrderManager {
private:
  HFSAT::Utils::TCPServerManager *tcp_server_manager_;

private:
  MidTermOrderManager(Mode mode, HFSAT::DebugLogger &dbglogger,
                      HFSAT::Watch &watch, BaseTrader &trader,
                      BaseAlgoManager &algo_manager)
      : BaseOrderManager(mode, dbglogger, watch, trader, algo_manager),
        tcp_server_manager_(NULL) {
    // Runs the data and order server in the required more
    if (Mode::kNSEServerMode == mode || Mode::kNSEHybridMode == mode) {
      dbglogger_ << "Running the TCPServer Manager...\n";
      tcp_server_manager_ = new HFSAT::Utils::TCPServerManager(
          NSE_MIDTERM_ORDER_SERVER_PORT, dbglogger);
      tcp_server_manager_->run();
      tcp_server_manager_->SubscribeForUpdates(this);
    }
    // -------------------------
  }

  MidTermOrderManager(MidTermOrderManager const &disabled_copy_constructor) =
      delete;

public:
  static MidTermOrderManager &GetUniqueInstance(Mode mode,
                                                HFSAT::DebugLogger &dbglogger,
                                                HFSAT::Watch &watch,
                                                BaseTrader &trader,
                                                BaseAlgoManager &algo_manager) {
    static MidTermOrderManager unique_instance(mode, dbglogger, watch, trader,
                                               algo_manager);
    return unique_instance;
  }

  void CleanUp() {
    if (NULL != tcp_server_manager_) {
      tcp_server_manager_->CleanUp();
    }
  }

  void Sleep(int n) { sleep(n); }
  void MarketLock() { mkt_lock_.LockMutex(); }
  void MarketUnLock() { mkt_lock_.UnlockMutex(); }
};
}
