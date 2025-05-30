
#pragma once

#include <vector>
#include "dvccode/CDef/debug_logger.hpp"
#include "dvccode/Utils/thread.hpp"
#include "dvccode/Utils/tcp_server_socket.hpp"
#include "dvccode/Utils/CPUAffinity.hpp"
#include "dvccode/Utils/task_handler.hpp"

namespace HFSAT {
namespace Utils {

/// Separate thread that listens for TCP connections on the specified port.
/// Upon connection, it spawns a new ControlThread to continue interacting with the client
// class TaskHandler;
class TaskHandler;
class ConnectionHandler : public Thread {
 public:
  ConnectionHandler(DebugLogger& _dbglogger_,int32_t worker_control_port, bool useAffinity=false);

  ~ConnectionHandler();

  /// inherited from Thread.
  /// start a while loop of accept
  void thread_main();
  void StopControlThreads();
  void RemoveThread(int32_t const& thread_id);
  void DisconnectSocket();
 protected:
 private:
  DebugLogger& dbglogger_;
  TCPServerSocket tcp_server_socket_;  ///< main acceptor socket
  bool use_affinity_;

  // static std::string comboProductMap_;
  std::map<int32_t, TaskHandler*> active_control_threads_;
  std::map<int32_t, bool> active_control_threads_marking_;
};
}
}
