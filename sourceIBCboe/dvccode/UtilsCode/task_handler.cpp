#include <iostream>
#include <sstream>
#include <string>
#include "dvccode/CommonDataStructures/perishable_string_tokenizer.hpp"
#include "dvccode/Utils/task_handler.hpp"
#include "dvccode/IBUtils/ComboProductHandler.hpp"
#include "dvccode/IBUtils/Contract.hpp"
#include "dvccode/CommonTradeUtils/date_time.hpp"
#include "dvccode/IBUtils/combo_product_data_request_ibkr.hpp"

namespace HFSAT {
namespace Utils {

TaskHandler::TaskHandler(DebugLogger& _dbglogger_, const int _connected_socket_file_descriptor_, 
                            HFSAT::Utils::ConnectionHandler* _connection_handler_,
                            int32_t tid)
    : dbglogger_(_dbglogger_),
      connected_socket_file_descriptor_(_connected_socket_file_descriptor_),
      connection_handler_(_connection_handler_),
      keep_task_handler_threads_running_(1),
      thread_id(tid){}

void TaskHandler::Close() {
  if (connected_socket_file_descriptor_ != -1) {
    shutdown(connected_socket_file_descriptor_, SHUT_RDWR);
    close(connected_socket_file_descriptor_);
    connected_socket_file_descriptor_ = -1;
  }
  connection_handler_->RemoveThread(thread_id);
}

void TaskHandler::StopControlThread() { __sync_bool_compare_and_swap(&keep_task_handler_threads_running_, 1, 0); }

void TaskHandler::thread_main()  // Short lived thread no affinity needed.
{
#define COMMAND_BUFFER_LEN 1024
  char command_buffer_[COMMAND_BUFFER_LEN];
  char temp_command_buffer_[COMMAND_BUFFER_LEN];

  // select related vars
  fd_set rfd;
  int maxfdplus1 = connected_socket_file_descriptor_ + 1;

  while (keep_task_handler_threads_running_) {
    bzero(command_buffer_, COMMAND_BUFFER_LEN);
    bzero(temp_command_buffer_, COMMAND_BUFFER_LEN);
    FD_ZERO(&rfd);
    FD_SET(maxfdplus1 - 1, &rfd);

    struct timespec timeout_pselect_;
    timeout_pselect_.tv_sec = 2;
    timeout_pselect_.tv_nsec = 0;
    int retval = pselect(maxfdplus1, &rfd, NULL, NULL, &timeout_pselect_, NULL);
    if (retval == 0) {
      continue;
    }
    if (retval == -1) {
      if (dbglogger_.CheckLoggingLevel(ORS_ERROR)) {
        dbglogger_ << " Control Client disconnects .. returning.\n ";
        dbglogger_.CheckToFlushBuffer();
      }
      // Error at pselect (network issue) => close socket and return
      StopControlThread();
      break;
    }

    int read_len_ = read(connected_socket_file_descriptor_, command_buffer_, COMMAND_BUFFER_LEN);

    // Lack of this check can cause some serious CPU hogging, when
    // the other end of the TCP connection is snapped shut by ors_control_exec.
    if (read_len_ <= 0) {
      // Almost always means that the other end has closed the connection.
      // Exit gracefully.

      if (dbglogger_.CheckLoggingLevel(ORS_ERROR)) {
        // dbglogger_ << " Control Client disconnects .. returning.\n ";
        // dbglogger_.CheckToFlushBuffer ( );
      }
      // Read returned no data/error (due to socket close) => close socket and return
      StopControlThread();
      break;
    }

    if (command_buffer_[0] != '\0') {
      dbglogger_.snprintf(COMMAND_BUFFER_LEN, command_buffer_);
      dbglogger_ << " : ThreadID: " << thread_id;
      dbglogger_ << '\n';
      dbglogger_.CheckToFlushBuffer();
    }

    strcpy(temp_command_buffer_, command_buffer_);
    PerishableStringTokenizer st_(command_buffer_, COMMAND_BUFFER_LEN);
    const std::vector<const char*>& tokens_ = st_.GetTokens();

    if ((tokens_.size() >= 2) && (strcmp(tokens_[0], "CONTROLCOMMAND") == 0)) {

      if((strcmp(tokens_[1], "LISTENCOMBOPRODUCTDATA") == 0)){
          /// CONTROLCOMMAND LISTENCOMBOPRODUCTDATA COMBOPRODUCTSTRING COMBOUniqueId
      
          std::string comboProductString=tokens_[2];
          std::string uniqueId=tokens_[3];
          int today = HFSAT::DateTime::GetCurrentIsoDateLocal();

          std::cout << "Inside ListenComboProductData " << std::endl; 
          std::cout << "The combo string is :" << comboProductString << std::endl;
          std::cout << "The uniqueId is :" << uniqueId << std::endl;
          std::cout << "The date is :" << today << std::endl;

          Contract contract_=ComboProductHandler::getContractFromCombinedString(comboProductString,today);
          HFSAT::ComboProductDataRequestIBKR::RequestDataFor(contract_,uniqueId);

      }else{
        std::cerr<<"Not yet supported this control command try using Control Thread instead"<<std::endl;
      }
        
    }else{
      std::cerr<<"Invalid request please check again"<<std::endl;
    }
  }
  // Close socket before exiting thread
  Close();
}
}
}
