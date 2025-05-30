// =====================================================================================
//
//       Filename:  offload_shared_logging_manager.hpp
//
//    Description:
//
//        Version:  1.0
//        Created:  07/29/2014 03:06:31 PM
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

#include <signal.h>

#include "dvccode/CDef/debug_logger.hpp"
#include "dvccode/CDef/shared_logging_defines.hpp"
#include "dvccode/CommonDataStructures/unordered_vector.hpp"
#include "dvccode/Utils/thread.hpp"
#include "dvccode/Utils/offload_logging_thread.hpp"

namespace HFSAT {
namespace Utils {

class LoggingClientAddRemoveListener {
 public:
  virtual ~LoggingClientAddRemoveListener() {}
  virtual void ProcessAddClient(const HFSAT::CDef::ClientMetaData& _writer_metadata_) = 0;
  virtual void ProcessRemoveClient(const int32_t& _assigned_client_id_) = 0;
};

class SharedMemeoryManager {
  class ClientSegmentAllocatorThread : public HFSAT::Thread {
   public:
    bool keep_running_;
    SharedMemeoryManager* callback_handler_;
    int32_t base_segment_id_;
    int32_t assigned_client_id_;
    int32_t client_provided_logging_id_;
    HFSAT::Lock shm_segment_allocation_lock_;

    inline void NotifyAddClient(const HFSAT::CDef::ClientMetaData& _client_metadata_) {
      callback_handler_->AddClient(_client_metadata_);
    }

    ClientSegmentAllocatorThread()
        : keep_running_(false),
          callback_handler_(NULL),
          base_segment_id_(LOGGING_SEGMENTS_START_KEY),
          assigned_client_id_(1),
          client_provided_logging_id_(-1),
          shm_segment_allocation_lock_() {}

   public:
    // callback interface will pass this information via a dead pid detection thread
    inline void ResetBaseSegmentId() {
      shm_segment_allocation_lock_.LockMutex();
      base_segment_id_ = LOGGING_SEGMENTS_START_KEY;
      shm_segment_allocation_lock_.UnlockMutex();
    }

    inline void thread_main() {
      SemUtils::wait_and_lock(callback_handler_->semaphore_lock_id_);

      volatile HFSAT::CDef::ClientMetaData* client_metadata;
      HFSAT::CDef::ClientMetaData temp_copy_of_client_metadata;
      client_metadata = (volatile HFSAT::CDef::ClientMetaData*)shmat(callback_handler_->shm_segment_id_, 0, 0);
      client_metadata->client_pid_ = -1;
      client_metadata->assigned_client_id_ = -2;

      SemUtils::signal_and_unlock(callback_handler_->semaphore_lock_id_);

      assigned_client_id_ = 1;

      while (keep_running_) {
        SemUtils::wait_and_lock(callback_handler_->semaphore_lock_id_);

        if (-1 == client_metadata->assigned_client_id_) {
          // From Server -> Client
          client_metadata->assigned_client_id_ = assigned_client_id_++;

          shm_segment_allocation_lock_.LockMutex();
          client_metadata->assigned_shm_key_ = base_segment_id_++;
          shm_segment_allocation_lock_.UnlockMutex();

          // From Client -> Server
          HFSAT::CDef::ClientIdPid new_client_id_pid;
          new_client_id_pid.client_pid_ = client_metadata->client_pid_;
          new_client_id_pid.assigned_client_id_ = client_metadata->assigned_client_id_;

          callback_handler_->AddPidDataForMonitoring(new_client_id_pid);

          memcpy((void*)&temp_copy_of_client_metadata, (void*)client_metadata, sizeof(HFSAT::CDef::ClientMetaData));
          NotifyAddClient(temp_copy_of_client_metadata);
        }

        SemUtils::signal_and_unlock(callback_handler_->semaphore_lock_id_);
        usleep(100);
      }
    }
  };

  class DetectDeadClientThread : public HFSAT::Thread {
   public:
    bool keep_running_;
    SharedMemeoryManager* callback_handler_;

   private:
    inline static bool isClientPIDAlive(const int32_t& _client_pid_) {
      if (-1 == kill(_client_pid_, 0) && ESRCH == errno) return false;

      return true;
    }

   public:
    inline void NotifyRemoveClient(const int32_t& _assigned_client_id_) {
      callback_handler_->RemoveClient(_assigned_client_id_);
    }

    void thread_main() {
      while (keep_running_) {
        SemUtils::wait_and_lock(callback_handler_->semaphore_lock_id_);
        UnOrderedVec<HFSAT::CDef::ClientIdPid>& all_clients_id_pid_vec = callback_handler_->GetClientsIdPidVec();

        for (uint32_t pid_counter = 0; pid_counter < all_clients_id_pid_vec.size(); pid_counter++) {
          if (!isClientPIDAlive(all_clients_id_pid_vec.at(pid_counter).client_pid_)) {
            NotifyRemoveClient(all_clients_id_pid_vec.at(pid_counter).assigned_client_id_);
            all_clients_id_pid_vec.remove_and_delete(pid_counter);
          }
        }

        // All clients have been removed, time to reset the shm initial key
        if (0 == all_clients_id_pid_vec.size()) {
          callback_handler_->ResetBaseSegmentId();
        }

        SemUtils::signal_and_unlock(callback_handler_->semaphore_lock_id_);
        sleep(1);
      }
    }
  };

  friend class ClientSegmentAllocatorThread;
  friend class DetectDeadClientThread;

 protected:
  int32_t semaphore_lock_id_;
  int32_t shm_segment_id_;
  bool keep_running_;

  UnOrderedVec<HFSAT::CDef::ClientIdPid> pid_client_id_vec_;
  std::vector<LoggingClientAddRemoveListener*> logging_client_add_remove_listener_vec_;

  ClientSegmentAllocatorThread client_segment_allocator_thread_;
  DetectDeadClientThread detect_dead_client_thread_;

 public:
  SharedMemeoryManager()
      : semaphore_lock_id_(-1),
        shm_segment_id_(-1),
        pid_client_id_vec_(),
        logging_client_add_remove_listener_vec_(),
        client_segment_allocator_thread_(),
        detect_dead_client_thread_()

  {
    shm_segment_id_ = shmget(LOGGING_MANAGER_SHM_KEY, sizeof(HFSAT::CDef::ClientMetaData), IPC_CREAT | IPC_EXCL | 0666);

    if (-1 == shm_segment_id_) {
      std::cerr << "ANOTHER INSTANCE OF EXE IS ALREADY RUNNING..." << std::endl;
      exit(-1);
    }

    semaphore_lock_id_ = semget(LOGGING_MANAGER_SEMAPHORE_KEY, 1, IPC_CREAT | IPC_EXCL | 0777);

    if (-1 == semaphore_lock_id_) {
      std::cerr << "ANOTHER INSTANCE OF EXE IS ALREADY RUNNING..." << std::endl;
      exit(-1);
    }

    client_segment_allocator_thread_.callback_handler_ = this;
    client_segment_allocator_thread_.keep_running_ = true;
    client_segment_allocator_thread_.run();

    detect_dead_client_thread_.callback_handler_ = this;
    detect_dead_client_thread_.keep_running_ = true;
    detect_dead_client_thread_.run();
  }

  int32_t& GetSemaphoreKey() { return semaphore_lock_id_; }

  void AddClient(const HFSAT::CDef::ClientMetaData& _new_client_metadata_) {
    for (uint32_t listener_counter = 0; listener_counter < logging_client_add_remove_listener_vec_.size();
         listener_counter++) {
      logging_client_add_remove_listener_vec_[listener_counter]->ProcessAddClient(_new_client_metadata_);
    }
  }

  void AddPidDataForMonitoring(const HFSAT::CDef::ClientIdPid& _client_id_pid_) {
    pid_client_id_vec_.push_back(_client_id_pid_);
  }

  UnOrderedVec<HFSAT::CDef::ClientIdPid>& GetClientsIdPidVec() { return pid_client_id_vec_; }

  void ResetBaseSegmentId() { client_segment_allocator_thread_.ResetBaseSegmentId(); }

  void RemoveClient(const int32_t& _assigned_client_id_) {
    for (uint32_t listener_counter = 0; listener_counter < logging_client_add_remove_listener_vec_.size();
         listener_counter++) {
      logging_client_add_remove_listener_vec_[listener_counter]->ProcessRemoveClient(_assigned_client_id_);
    }
  }

  void SubscribeLoggingClientAddRemoveListener(LoggingClientAddRemoveListener* _logging_client_add_remove_listener_) {
    logging_client_add_remove_listener_vec_.push_back(_logging_client_add_remove_listener_);
  }

  void WaitForShmThread() {
    client_segment_allocator_thread_.stop();
    detect_dead_client_thread_.stop();
  }

  void CleanUp() {
    client_segment_allocator_thread_.keep_running_ = false;
    detect_dead_client_thread_.keep_running_ = false;

    client_segment_allocator_thread_.stop();
    detect_dead_client_thread_.stop();

    if (-1 != (shm_segment_id_ = shmget(LOGGING_MANAGER_SHM_KEY, 0, 0))) {
      shmctl(shm_segment_id_, IPC_RMID, 0);
    }

    if (-1 != (semaphore_lock_id_ = semget(LOGGING_MANAGER_SEMAPHORE_KEY, 1, 0))) {
      semctl(semaphore_lock_id_, IPC_RMID, 0);
    }
  }
};

class OffloadLoggingManager : public LoggingClientAddRemoveListener {
 private:
  HFSAT::DebugLogger& dbglogger_;
  std::map<int32_t, HFSAT::Utils::OffloadLoggerThread*> assigned_client_id_to_offload_logging_thread_map_;
  SharedMemeoryManager* shared_memory_manager_;

 private:
  OffloadLoggingManager(HFSAT::DebugLogger& _dbglogger_)
      : dbglogger_(_dbglogger_),
        assigned_client_id_to_offload_logging_thread_map_(),
        shared_memory_manager_(new SharedMemeoryManager()) {
    shared_memory_manager_->SubscribeLoggingClientAddRemoveListener(this);
  }

  OffloadLoggingManager();
  OffloadLoggingManager(const OffloadLoggingManager& _disabled_copy_constructor_);

 public:
  static OffloadLoggingManager& GetUniqueInstance(HFSAT::DebugLogger& _dbglogger_) {
    static OffloadLoggingManager unique_instance(_dbglogger_);
    return unique_instance;
  }

  ~OffloadLoggingManager() {}

  void ProcessAddClient(const HFSAT::CDef::ClientMetaData& _client_metadata_) {
    if (assigned_client_id_to_offload_logging_thread_map_.end() ==
        assigned_client_id_to_offload_logging_thread_map_.find(_client_metadata_.assigned_client_id_)) {
      HFSAT::Utils::OffloadLoggerThread* new_logger_thread =
          new HFSAT::Utils::OffloadLoggerThread(_client_metadata_, dbglogger_);
      assigned_client_id_to_offload_logging_thread_map_[_client_metadata_.assigned_client_id_] = new_logger_thread;

      new_logger_thread->run();

    } else {
      DBGLOG_CLASS_FUNC_LINE_ERROR << "THERE IS LOGGER THREAD ALREADY RUNNING FOR THE ASSIGNED NEW CLIENT ID : "
                                   << _client_metadata_.assigned_client_id_ << DBGLOG_ENDL_FLUSH;
      DBGLOG_DUMP;
    }
  }

  void ProcessRemoveClient(const int32_t& _assigned_client_id_) {
    if (assigned_client_id_to_offload_logging_thread_map_.end() ==
        assigned_client_id_to_offload_logging_thread_map_.find(_assigned_client_id_)) {
      DBGLOG_CLASS_FUNC_LINE_ERROR << "NO LOGGER THREAD RUNNING WITH THE GIVEN ID : " << _assigned_client_id_
                                   << DBGLOG_ENDL_FLUSH;
      DBGLOG_DUMP;

    } else {
      assigned_client_id_to_offload_logging_thread_map_[_assigned_client_id_]->CleanUp();
      assigned_client_id_to_offload_logging_thread_map_.erase(_assigned_client_id_);
    }
  }

  void CleanUp() {
    SemUtils::wait_and_lock(shared_memory_manager_->GetSemaphoreKey());

    std::vector<HFSAT::Utils::OffloadLoggerThread*> cleanup_threads_vec;

    std::map<int32_t, HFSAT::Utils::OffloadLoggerThread*>::iterator logger_itr;
    std::vector<HFSAT::Utils::OffloadLoggerThread*> temp;

    for (logger_itr = assigned_client_id_to_offload_logging_thread_map_.begin();
         assigned_client_id_to_offload_logging_thread_map_.end() != logger_itr; logger_itr++) {
      if (NULL != logger_itr->second) {
        logger_itr->second->CleanUp();
        logger_itr->second->stop();
        delete (logger_itr->second);
        logger_itr->second = NULL;
      }
    }

    SemUtils::signal_and_unlock(shared_memory_manager_->GetSemaphoreKey());

    if (NULL != shared_memory_manager_) {
      shared_memory_manager_->CleanUp();  // force stop from this point
      delete shared_memory_manager_;
      shared_memory_manager_ = NULL;
    }
  }

  void WaitForLoggersAndCleanUpOnExit() { shared_memory_manager_->WaitForShmThread(); }
};
}
}
