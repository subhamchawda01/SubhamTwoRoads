/**
    \file dvccode/Utils/thread.hpp

    \author: (c) Copyright Two Roads Technological Solutions Pvt Ltd 2011
     Address:
         Suite 217, Level 2, Prestige Omega,
         No 104, EPIP Zone, Whitefield,
         Bangalore - 560066, India
         +91 80 4060 0717
 */

#ifndef BASE_UTILS_THREAD_H
#define BASE_UTILS_THREAD_H

#include <pthread.h>
#include <stdio.h>
#include <string.h>
#include <iostream>
#include <sys/syscall.h>
#include <sys/types.h>
#include <fstream>
#include <vector>
#include <sstream>
#include <signal.h>

#include "dvccode/CommonTradeUtils/date_time.hpp"
#include "dvccode/Utils/CPUAffinity.hpp"
#include "dvccode/Utils/allocate_cpu.hpp"
#include "dvccode/CommonDataStructures/perishable_string_tokenizer.hpp"

namespace HFSAT {

/// Class encapsulating pthread functions to start a new thread.
/// After initialization, call run to start the thread specific function, and stop to call pthread_join.
///
/// Override empty function thread_main in subclass
class Thread {
 public:
  Thread() : name_("") {}

  virtual ~Thread() {}

  /// function to be called after construction to start thread
  void run() { pthread_create(&pthread_obj_, NULL, &(static_start_routine), this); }

  /// called by the thread that has created this thread when it has nothing to do
  /// apart from waiting for this thread to exit
  void stop() { pthread_join(pthread_obj_, NULL); }

  void AffinInitCores() {
    int threadId = ((int)(syscall(SYS_gettid)));
    /*int core_alloced_ = */ CPUManager::AffinToInitCores(threadId);  // unused return value commented
  }

  void setAffinityCore(int _core_) {
    int threadId = ((int)(syscall(SYS_gettid)));
    CPUManager::setAffinity(_core_, threadId);

    std::cerr << " Thread Id : " << threadId << " For : " << name_ << "\n";

    cpu_aff_t affinity_ = CPUManager::getAffinity(threadId);

    std::cerr << "AFFINED : PID : " << threadId << " Name : " << name_ << " CORE # ";

    for (int i = 0; i < CPUManager::getTotalAvailableCores(); ++i) {
      if (affinity_ & CPUManager::getMask(i)) {
        std::cerr << i << " ";
      }
    }
  }

  void forbidThreadAffinity(int _core_) {
    int threadId = ((int)(syscall(SYS_gettid)));

    // call CPU affinity manager to forbid this thread from running off this core
    CPUManager::forbidAffinity(_core_, threadId);
  }

  bool allocateToSiblingCPU(int cpu) {
    int threadId = ((int)(syscall(SYS_gettid)));

    bool allocated = CPUManager::AffineToSiblingHyperthread(cpu, threadId);

    if (!allocated) return false;

    cpu_aff_t affinity_ = CPUManager::getAffinity(threadId);

    std::cerr << "AFFINED to Sibling : PID : " << threadId << " Name : " << name_ << " CORE # ";

    for (int i = 0; i < CPUManager::getTotalAvailableCores(); ++i) {
      if (affinity_ & CPUManager::getMask(i)) {
        std::cerr << i << " ";
      }
    }

    return true;
  }

  // It will try to allocate CPU.
  // If it fails to do so, it will raise SIGINT
  void AllocateCPUOrExit() {
    char hostname[128];
    hostname[127] = '\0';
    gethostname(hostname, 127);

    // We do not want to affine at NY servers
    if (std::string(hostname).find("sdv-ny4-srv") == std::string::npos) {
      HFSAT::AllocateCPUUtils::GetUniqueInstance().AllocateCPUOrExit(name_);
    }
  }

  // For debugging purposes only.
  // Check whether expected threads are being allocated cores correctly.
  void setName(const char *_name_) { name_ = _name_; }

 protected:
  /// implement this function in child class. This is sort of the main
  /// point of control for Thread class and derived classes.
  /// Exiting this function menas the thread is closed
  virtual void thread_main() = 0;

 private:
  pthread_t pthread_obj_;

  /// The function supplied as argument to pthread_create must be a static function
  /// and hence the real start of the thread is Thread::thread_main
  static void *static_start_routine(void *p_thread_subclass_object_) {
    reinterpret_cast<Thread *>(p_thread_subclass_object_)->thread_main();
    return NULL;
  }

  std::string name_;  // For debugging purposes ONLY.
};
}

#endif  // BASE_UTILS_THREAD_H
