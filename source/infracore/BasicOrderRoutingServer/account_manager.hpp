/**
   \file BasicOrderRoutingServer/account_manager.hpp

   \author: (c) Copyright Two Roads Technological Solutions Pvt Ltd 2011
   Address:
   Suite 217, Level 2, Prestige Omega,
   No 104, EPIP Zone, Whitefield,
   Bangalore - 560066, India
   +91 80 4060 0717
 */

#ifndef BASE_BASICORDERROUTINGSERVER_ACCOUNTMANAGER_H
#define BASE_BASICORDERROUTINGSERVER_ACCOUNTMANAGER_H

#include "dvccode/CDef/debug_logger.hpp"
#include "infracore/BasicOrderRoutingServer/account_thread.hpp"

namespace HFSAT {
namespace ORS {

/// Holder of AccountThread instances, currently optimized assuming there will only be one AccountThread object
class AccountManager {
 public:
  /// simply returns p_account_thread_ even if NULL
  static AccountThread* GetAccountThread() { return GetUniqueInstance()._GetAccountThread(); }

  /// simply returns p_account_thread_ even if NULL
  static void RemoveAccountThread() { return GetUniqueInstance()._RemoveAccountThread(); }

  /// initializes p_account_thread_ if NULL, and then returns it.
  static AccountThread* GetNewAccountThread(
      DebugLogger& _dbglogger_, OrderManager& _order_manager_, Settings& _settings_,
      HFSAT::Utils::ClientLoggingSegmentInitializer* _client_logging_segment_initializer_ptr_,
      std::string t_output_log_dir_) {
    return GetUniqueInstance()._GetNewAccountThread(_dbglogger_, _order_manager_, _settings_,
                                                    _client_logging_segment_initializer_ptr_, t_output_log_dir_);
  }

 protected:
  static AccountManager& GetUniqueInstance() {
    static AccountManager uniqueinstance_;
    return uniqueinstance_;
  }

  AccountManager() : p_account_thread_(NULL) {}

  AccountThread* _GetAccountThread() { return p_account_thread_; }
  AccountThread* _GetNewAccountThread(
      DebugLogger& _dbglogger_, OrderManager& _order_manager_, Settings& _settings_,
      HFSAT::Utils::ClientLoggingSegmentInitializer* _client_logging_segment_initializer_ptr_,
      std::string t_output_log_dir_) {
    if (p_account_thread_ == NULL) {
      p_account_thread_ = new AccountThread(_dbglogger_, _order_manager_, _settings_,
                                            _client_logging_segment_initializer_ptr_, t_output_log_dir_);
    }
    return p_account_thread_;
  }

  void _RemoveAccountThread() {
    if (p_account_thread_ != NULL) {
      delete p_account_thread_;
      p_account_thread_ = NULL;
    }
  }

 private:
  AccountThread* p_account_thread_;
};
}
}

#endif  // BASE_BASICORDERROUTINGSERVER_ACCOUNTMANAGER_H
