// =====================================================================================
//
//       Filename:  offload_logging_file_manager.hpp
//
//    Description:
//
//        Version:  1.0
//        Created:  08/01/2014 12:40:01 PM
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

#include <iostream>
#include <fstream>
#include <map>
#include "dvccode/CDef/debug_logger.hpp"
#include "dvccode/Utils/lock.hpp"
#include "dvccode/CDef/email_utils.hpp"
#include "dvccode/Utils/send_alert.hpp"

namespace HFSAT {
namespace Utils {

class OffloadLoggingFileManager {
 private:
  HFSAT::DebugLogger& dbglogger_;
  std::map<std::string, std::ofstream*> unique_logging_key_to_file_ptr_;
  HFSAT::Lock atomic_file_write_lock_;

  OffloadLoggingFileManager(HFSAT::DebugLogger& _dbglogger_)
      : dbglogger_(_dbglogger_),
        unique_logging_key_to_file_ptr_(),
        atomic_file_write_lock_()

  {}

  OffloadLoggingFileManager(const OffloadLoggingFileManager& _disabled_copy_constructor_);

  void SendEmail(std::string _email_body_) {
    // also send an alert
    char hostname[128];
    hostname[127] = '\0';
    gethostname(hostname, 127);
    std::string alert_message = "ALERT : LOGGING SERVER FAILED TO OPEN FILE FOR WRITING " + std::string(hostname);
    HFSAT::SendAlert::sendAlert(alert_message);

    HFSAT::Email e;

    e.setSubject("LOGGING SERVER FAILED TO OPEN FILE FOR WRITING");
    e.addRecepient("ravi.parikh@tworoads.co.in, nseall@tworoads.co.in");
    e.addSender("ravi.parikh@tworoads.co.in");
    e.content_stream << "host_machine: " << hostname << "<br/>";
    e.content_stream << "ISSUE : " << _email_body_ << "<br/>";
    e.sendMail();
  }

 public:
  static OffloadLoggingFileManager& GetUniqueInstance(HFSAT::DebugLogger& _dbglogger_) {
    static OffloadLoggingFileManager unique_instance_(_dbglogger_);
    return unique_instance_;
  }

  ~OffloadLoggingFileManager() { CleanUp(); }

  void CleanUp() {  // THIS IS CLEANUP ALL

    for (auto& itr : unique_logging_key_to_file_ptr_) {
      if (NULL != itr.second) {
        if (itr.second->is_open()) {
          itr.second->flush();
          itr.second->close();
        }

        delete itr.second;
      }
    }

    unique_logging_key_to_file_ptr_.clear();
  }

  void CloseFile(const std::string& _unique_log_id_) {
    if (unique_logging_key_to_file_ptr_.end() != unique_logging_key_to_file_ptr_.find(_unique_log_id_)) {
      if (NULL != unique_logging_key_to_file_ptr_[_unique_log_id_] &&
          true == unique_logging_key_to_file_ptr_[_unique_log_id_]->is_open()) {
        unique_logging_key_to_file_ptr_[_unique_log_id_]->flush();
        unique_logging_key_to_file_ptr_[_unique_log_id_]->close();
        delete unique_logging_key_to_file_ptr_[_unique_log_id_];
      }

      unique_logging_key_to_file_ptr_.erase(unique_logging_key_to_file_ptr_.find(_unique_log_id_));
    }
  }

  void FlushDataAndCleanUp(const std::string& _unique_log_id_) {
    atomic_file_write_lock_.LockMutex();

    if (unique_logging_key_to_file_ptr_.find(_unique_log_id_) != unique_logging_key_to_file_ptr_.end()) {
      unique_logging_key_to_file_ptr_[_unique_log_id_]->flush();
      unique_logging_key_to_file_ptr_[_unique_log_id_]->close();
    }

    if (NULL != unique_logging_key_to_file_ptr_[_unique_log_id_]) {
      delete unique_logging_key_to_file_ptr_[_unique_log_id_];
      unique_logging_key_to_file_ptr_[_unique_log_id_] = NULL;
      unique_logging_key_to_file_ptr_.erase(_unique_log_id_);
    }

    atomic_file_write_lock_.UnlockMutex();
  }

  void CommitDataToFile(const std::string& _unique_log_id_, const char* _log_data_,
                        HFSAT::CDef::BufferContentType buffer_type) {
    atomic_file_write_lock_.LockMutex();

    if (unique_logging_key_to_file_ptr_.find(_unique_log_id_) == unique_logging_key_to_file_ptr_.end()) {
      std::ofstream* new_file_handler =
          new std::ofstream(_unique_log_id_.c_str(), std::ofstream::app | std::ofstream::ate);
      unique_logging_key_to_file_ptr_[_unique_log_id_] = new_file_handler;

      if (!new_file_handler->is_open()) {
        SendEmail("CAN'T OPEN FILE, WILL MISS LOGS UNLESS RESOLVED : " + std::string(_unique_log_id_));
      }
    }

    *(unique_logging_key_to_file_ptr_[_unique_log_id_]) << _log_data_;

    // We don't want the trailing newline character in case of unstructured text data,
    // But we need it for ORSTrade and QueryTrade
    if (buffer_type != HFSAT::CDef::UnstructuredText) {
      *(unique_logging_key_to_file_ptr_[_unique_log_id_]) << "\n";
    }

    unique_logging_key_to_file_ptr_[_unique_log_id_]->flush();

    atomic_file_write_lock_.UnlockMutex();
  }
};
}
}
