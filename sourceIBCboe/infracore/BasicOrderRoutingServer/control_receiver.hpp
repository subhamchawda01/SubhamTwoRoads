/**
    \file BasicOrderRoutingServer/control_receiver.hpp

    \author: (c) Copyright Two Roads Technological Solutions Pvt Ltd 2011
     Address:
         Suite 217, Level 2, Prestige Omega,
         No 104, EPIP Zone, Whitefield,
         Bangalore - 560066, India
         +91 80 4060 0717
 */

#ifndef BASE_BASICORDERROUTINGSERVER_CONTROLRECEIVER_H
#define BASE_BASICORDERROUTINGSERVER_CONTROLRECEIVER_H

#include <vector>
// #include <shared_mutex>
#include "dvccode/CDef/debug_logger.hpp"
#include "dvccode/Utils/thread.hpp"
#include "dvccode/Utils/tcp_server_socket.hpp"
#include "dvccode/Utils/CPUAffinity.hpp"
#include "dvccode/Utils/client_logging_segment_initializer.hpp"
#include "dvccode/Utils/lock.hpp"
#include "infracore/BasicOrderRoutingServer/control_thread.hpp"
#include "dvccode/Utils/settings.hpp"
namespace HFSAT {
namespace ORS {

/// Separate thread that listens for TCP connections on the specified port.
/// Upon connection, it spawns a new ControlThread to continue interacting with the client
/// Used for one shot communications eg login, logout, margin management etc
class ControlReceiver : public Thread {
 public:
  ControlReceiver(DebugLogger& _dbglogger_, Settings& _settings_,
                  HFSAT::Utils::ClientLoggingSegmentInitializer* _client_logging_segment_initializer_,
                  std::string t_output_log_dir_, bool &_cancel_live_order_flag_);

  ~ControlReceiver();

  /// inherited from Thread.
  /// start a while loop of accept
  void thread_main();
  void StopControlThreads();
  void LoadMarginFile();
  void LoadComboProductsMapFile();
  inline static void AddEntryForComboProduct(const std::string& comboStringProduct, const std::string uniqueId);
  static std::string getUniqueIdForComboSymbol(const std::vector<std::pair<std::string,int32_t>> comboProducts);
  void addMaxPosAndOrdSzForComboProductCBOE(const std::string &shortcodeOfDay,const int max_pos,const int max_ord_sz);
  static std::vector<std::pair<std::string,int32_t>> getComboSymbolForUniqueId(const std::string uniqueId);

  void RemoveThread(int32_t const& thread_id);

 protected:
 private:
  inline static std::string convertToComboUniqueId(const int32_t uniqueId, const std::string prefix);
  static std::string getThePrefixForComboProductId(const std::string firstShortCode);
  DebugLogger& dbglogger_;
  TCPServerSocket tcp_server_socket_;  ///< main acceptor socket
  Settings& settings_;
  bool &cancel_live_order_flag_;
  bool use_affinity_;
  bool* is_addts_thread_running_;
  std::map<std::string, int> sec_to_max_pos_map_;
  std::map<std::string, int> sec_to_max_ord_sz_map_;

  HFSAT::Utils::ClientLoggingSegmentInitializer* client_logging_segment_initializer_;

  /// only stored since Engine class or anyone else down the road might need to know where to log
  std::string output_log_dir_;
  // static std::string comboProductMap_;
  std::map<int32_t, ControlThread*> active_control_threads_;
  std::map<int32_t, bool> active_control_threads_marking_;
  static std::unordered_map<std::string,std::string> combinedSymbToUniqueId;
  static std::unordered_map<std::string,std::string> uniqueIdToCombinedSymb;
  static HFSAT::Lock mapMutexForCombinedSymb;
};
}
}
#endif  //  BASE_BASICORDERROUTINGSERVER_CONTROLRECEIVER_H
