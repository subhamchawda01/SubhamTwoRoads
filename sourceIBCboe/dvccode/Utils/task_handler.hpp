#pragma once

#include <time.h>

#include "dvccode/CDef/debug_logger.hpp"
#include "dvccode/Utils/thread.hpp"
#include "dvccode/Utils/tcp_server_socket.hpp"
#include "dvccode/Utils/connection_handler.hpp"
#include "dvccode/CommonTradeUtils/date_time.hpp"

namespace HFSAT {
namespace Utils {

/// Separate thread that interacts with one control_client,
/// receives control messages and acts accordingly.
// class ConnectionHandler;
class ConnectionHandler;
class TaskHandler : public Thread {
 public:
  TaskHandler(DebugLogger& _dbglogger_, const int _connected_socket_file_descriptor_, HFSAT::Utils::ConnectionHandler*,int32_t thread_id);

  ~TaskHandler() {
    Close();
  }

  /// inherited from Thread.
  /// start a while loop of accept
  void thread_main();

  void StopControlThread();

 protected:
 private:
  DebugLogger& dbglogger_;
  int connected_socket_file_descriptor_;  ///< socket id returned on accept, for this client.
  HFSAT::Utils::ConnectionHandler* connection_handler_;
  /// for reponse to GUI based commands
  int keep_task_handler_threads_running_;

  int32_t thread_id;

  void Close();

};
}
}
