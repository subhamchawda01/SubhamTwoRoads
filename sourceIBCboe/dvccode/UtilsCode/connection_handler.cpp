#include "dvccode/Utils/connection_handler.hpp"
#include "dvccode/CommonDataStructures/perishable_string_tokenizer.hpp"
#include "dvccode/Utils/get_todays_date_utc.hpp"
#include <sstream>
#include <fstream>

namespace HFSAT {
namespace Utils {

ConnectionHandler::ConnectionHandler(DebugLogger& _dbglogger_,int32_t worker_control_port, bool useAffinity)
    : dbglogger_(_dbglogger_),
      tcp_server_socket_(worker_control_port),
      use_affinity_(useAffinity){}

ConnectionHandler::~ConnectionHandler() {
  tcp_server_socket_.Close();

  // Cleanup
  for (unsigned int i = 0; i < active_control_threads_.size(); i++) {
    if (active_control_threads_[i] != NULL) {
      delete active_control_threads_[i];
      active_control_threads_[i] = NULL;
    }
  }

  active_control_threads_.empty();
  active_control_threads_marking_.empty();
}

void ConnectionHandler::DisconnectSocket() {
  tcp_server_socket_.Close();
  std::cout<<"Socket Disconnected"<<std::endl;
}
// Should be thread-safe
void ConnectionHandler::RemoveThread(int32_t const& thread_id) { active_control_threads_marking_[thread_id] = false; }

void ConnectionHandler::thread_main() {
  int32_t thread_id = 0;

  if (use_affinity_) {
    setName("ConnectionHandler");
    // AllocateCPUOrExit ();
  }
  // std::cout<<"Control Re ceiver thread started\n";

  while (int connected_socket_file_descriptor_ = tcp_server_socket_.Accept()) {
    if (connected_socket_file_descriptor_ < 0) {
      break;
    }

    for (auto& itr : active_control_threads_marking_) {
      if (false == itr.second) {
        if (NULL != active_control_threads_[itr.first]) {
          std::cout << " Deleting : " << itr.first << " " << std::endl;
          active_control_threads_[itr.first]->StopControlThread();
          std::cout << " Waiting to stop task handler thread..." << std::endl;
          active_control_threads_[itr.first]->stop();
          std::cout << " Stopped task handler thread for..." << itr.first << std::endl;
          // Doesn't make any difference anymore here, Will prevent further stop calls
          // TODO : Check Why should a stopped thread wait call matter
          itr.second = true;
          //          delete active_control_threads_[ itr.first ] ;
          //          active_control_threads_[ itr.first ] = NULL ;
        }
      }
    }

    // std::cout<<"Connection Accepted\n";
    TaskHandler* _new_control_thread_ =
        new TaskHandler(dbglogger_, connected_socket_file_descriptor_, this, thread_id);

    active_control_threads_[thread_id] = _new_control_thread_;
    active_control_threads_marking_[thread_id] = true;

    _new_control_thread_->run();  // starts the thread

    thread_id++;
  }
}

void ConnectionHandler::StopControlThreads() {
  for (unsigned int i = 0; i < active_control_threads_.size(); i++) {
    active_control_threads_[i]->StopControlThread();
  }
}
}
}
