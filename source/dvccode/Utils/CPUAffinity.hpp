/**
   file CPUAffinity.hpp

   \author: (c) Copyright Two Roads Technological Solutions Pvt Ltd 2011
   Address:
   Suite No 162, Evoma, #14, Bhattarhalli,
   Old Madras Road, Near Garden City College,
   KR Puram, Bangalore 560049, India
   +91 80 4190 3551

*/

/* @Info : A CPUManager class to set,get affinity of any process given its pid, default is the calling process pid=0
 * 	   Also has the method to find the totalAvailableCores in the system, forbid the affinity - a varient of
 *get/set affinity
 *         and the availability of a core for a particular process given pid, default is calling process.
 *
 * @NOTES : Sets/Gets the affinity for the threads created from some main process but in that case the arguments for the
 *threads should be the
 *	    unique gettid(), cause threads will share the same PID as that of the main process.
 *
 *	    Please check the TOTALCORES is set to correct value
 *
 * @Date  : October 24, 2011
 *
 * @Remarks on Modifications  :
 *
 */

#ifndef BASE_UTILS_CPU_AFFINITY_H
#define BASE_UTILS_CPU_AFFINITY_H

#include <stdio.h>
#include <stdlib.h>
#include <iostream>
#include <sched.h>
#include <sys/param.h>
#include <sys/user.h>
#include <sys/sysctl.h>
#include <sys/sysinfo.h>
#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include <vector>
#include <string>
#include <time.h>
#include <typeinfo>

#include "dvccode/CDef/file_utils.hpp"

#include "dvccode/Utils/sem_utils.hpp"
#include "dvccode/Utils/PIDTracker.hpp"
#include "dvccode/Utils/sched.hpp"

#define AFFINITY_PROC_LIST_FILENAME "/spare/local/files/affinity/affinity_process_list.txt"
#define AFFINED_PID_PROCS_LIST_FILENAME "/spare/local/files/affinity/affinity_pid_process.txt"

#define INIT_PROCESS_PID 1

static int sem_id_ = semget((key_t)0x37, 1, IPC_CREAT | 0666);

typedef unsigned long long cpu_aff_t;

class CPUManager {
 public:
  static std::map<std::string, int> proc_name_to_core_id_;
  static bool setAffinity(int cpu, int pid = 0);  // set affinity of a process by pid --- default is calling
  static bool forbidAffinity(
      int cpu, int pid = 0);  // forbids a process with pid from being scheduled on a given core -- default is calling
  static cpu_aff_t getAffinity(int pid = 0);  // gets the affinity of a process by pid
  static bool resetAffinity(int pid = 0);     // resets the affinity to run on all cores given a process pid

  static cpu_aff_t getFreeMask();  // gets the freemask, basically affinity of the init process
  static cpu_aff_t getMask(int cpu_core_id);

  static int getTotalAvailableCores();

  static void getInitProcessAssignedCores(std::vector<unsigned int>& init_allocated_cores_);

  static int allocateFirstBestAvailableCore(
      process_type_map& process_and_type_, const std::vector<std::string>& process_names_vec_, int pid = 0,
      std::string const& process_name = "EXTERNAL",
      const bool& send_mail_ = true);  // gets the first available core which none of the processes passed
  static int AffinToInitCores(int pid);
  static bool AffineToSiblingHyperthread(int cpu_id, int pid);
  static int GetCoreForProc(std::string proc);

  // in the argument is using and sets affinity to it
};

inline cpu_aff_t CPUManager::getFreeMask() {
  return CPUManager::getAffinity(INIT_PROCESS_PID);  // get the affinity of the init process
}

inline cpu_aff_t CPUManager::getMask(int cpu_core_id) { return ((cpu_aff_t)1) << cpu_core_id; }

inline int CPUManager::getTotalAvailableCores() { return get_nprocs(); }

inline bool CPUManager::setAffinity(int cpu, int pid) {
  cpu_set_t cpu_set_;

  CPU_ZERO(&cpu_set_);
  CPU_SET(cpu, &cpu_set_);

  int core_allocated_ = sched_setaffinity(pid, sizeof(cpu_set_t), &cpu_set_);

  return core_allocated_ == -1 ? false : true;
}

inline bool CPUManager::forbidAffinity(int cpu, int pid) {
  cpu_set_t cpu_set_;

  CPU_ZERO(&cpu_set_);

  int core_allocated_ = sched_getaffinity(pid, sizeof(cpu_set_t), &cpu_set_);

  if (core_allocated_ == -1) perror("Error Obtaining Current Cpu Affinity");

  CPU_CLR(cpu, &cpu_set_);

  core_allocated_ = sched_setaffinity(pid, sizeof(cpu_set_t), &cpu_set_);

  return core_allocated_ == -1 ? false : true;
}

inline cpu_aff_t CPUManager::getAffinity(int pid) {
  cpu_set_t cpu_set_;

  CPU_ZERO(&cpu_set_);

  int core_allocated_ = sched_getaffinity(pid, sizeof(cpu_set_t), &cpu_set_);

  if (core_allocated_ == -1) {
    std::cerr << typeid(CPUManager).name() << ':' << __func__ << "Error Obtaining Cpu Affinity of The Requested Process"
              << std::endl;
  }

  // not using cpu_set_ to provide abstraction to the calling process
  unsigned long long cpu_affinity_ = 0;

  for (unsigned int core_counter_ = 0; core_counter_ < (unsigned int)CPUManager::getTotalAvailableCores();
       core_counter_++) {
    if (CPU_ISSET(core_counter_, &cpu_set_)) {
      cpu_affinity_ += getMask(core_counter_);  // indicates the cpu-affinity in terms of bit-set as an integer
    }
  }

  return cpu_affinity_;
}

inline void CPUManager::getInitProcessAssignedCores(std::vector<unsigned int>& init_allocated_cores_) {
  init_allocated_cores_.clear();

  cpu_set_t cpu_set_;

  CPU_ZERO(&cpu_set_);

  int core_allocated_ = sched_getaffinity(INIT_PROCESS_PID, sizeof(cpu_set_t), &cpu_set_);

  if (core_allocated_ == -1) {
    std::cerr << typeid(CPUManager).name() << ':' << __func__ << "Error Obtaining Cpu Affinity of The Requested Process"
              << std::endl;
  }

  for (unsigned int core_counter_ = 0; core_counter_ < (unsigned int)CPUManager::getTotalAvailableCores();
       core_counter_++) {
    if (CPU_ISSET(core_counter_, &cpu_set_)) {
      // fill up the list of cores set aside for init
      init_allocated_cores_.push_back(core_counter_);
    }
  }
}

inline int CPUManager::AffinToInitCores(int pid) {
  cpu_set_t cpu_set_;
  CPU_ZERO(&cpu_set_);

  sched_getaffinity(INIT_PROCESS_PID, sizeof(cpu_set_t), &cpu_set_);
  int assigned_core_for_this_pid = sched_setaffinity(pid, sizeof(cpu_set_t), &cpu_set_);

  return assigned_core_for_this_pid;
}

inline bool CPUManager::AffineToSiblingHyperthread(int cpu_id, int pid) {
  AffinityAllocator affinity_allocator;
  affinity_allocator.init();

  int sibling_cpu = affinity_allocator.SiblingHyperThread(cpu_id);

  if (sibling_cpu < 0) return false;
  return setAffinity(sibling_cpu, pid);
}

// Allocates a free dedicated core to the process/thread with PID: pid.
// Returns the core # allocated on success, else returns -1 on failure.
inline int CPUManager::allocateFirstBestAvailableCore(process_type_map& process_and_type_,
                                                      const std::vector<std::string>& process_names_vec_, int pid,
                                                      std::string const& process_name, const bool& send_mail_) {
  if (sem_id_ == -1) {
    return -1;
  }

  HFSAT::SemUtils::wait_and_lock(sem_id_);
  AffinityAllocator affinity_allocator_;
  std::vector<int> pid_list_;
  std::vector<int> pid_list_per_process_name_;
  int core_allocated_ = 0;
  ProcType process_type_ = kEmpty;  // kEmpty is for cores, here just for shake of initialization
  affinity_allocator_.init();
  std::string process_name_for_given_pid_ = "";
  // this identifies the set of cores that have been set aside for init process, default affinity
  std::vector<unsigned int> freemask_affinity_vec_;
  freemask_affinity_vec_.clear();
  std::string file_name_ = HFSAT::FileUtils::AppendHome("Affinity.COUT.ERROR");
  std::ofstream dump_file_(file_name_.c_str(), std::ios_base::app | std::ios_base::out);
  time_t now = time(0);
  struct tm tstruct;
  char buf[80];
  tstruct = *localtime(&now);
  strftime(buf, sizeof(buf), "%Y-%m-%d.%X", &tstruct);
  dump_file_ << buf << std::endl;
  // affinity_allocator_.printAffinity();
  // fetch the cores the init process has been set on
  CPUManager::getInitProcessAssignedCores(freemask_affinity_vec_);

  cpu_set_t cpu_set_;
  cpu_set_t mask_set_;
  CPU_ZERO(&mask_set_);
  for (unsigned process_counter_ = 0; process_counter_ < process_names_vec_.size(); process_counter_++) {
    dump_file_ << "Process name : " << process_names_vec_[process_counter_] << std::endl;
    pid_list_per_process_name_ = PIDTracker::getPIDByName((process_names_vec_[process_counter_]).c_str());
    for (unsigned int pid_counter_ = 0; pid_counter_ < pid_list_per_process_name_.size(); pid_counter_++) {
      if (pid_list_per_process_name_[pid_counter_] == pid) {
        process_name_for_given_pid_ = process_names_vec_[process_counter_].c_str();
        continue;
      }

      if (CPUManager::getAffinity(pid_list_per_process_name_[pid_counter_]) == CPUManager::getFreeMask()) continue;

      CPU_ZERO(&cpu_set_);

      core_allocated_ = sched_getaffinity(pid_list_per_process_name_[pid_counter_], sizeof(cpu_set_t), &cpu_set_);

      // this sets the value for all the cores on which the current process is running
      for (int core_counter_ = 0; core_counter_ < CPUManager::getTotalAvailableCores(); core_counter_++) {
        if (CPU_ISSET(core_counter_, &cpu_set_)) {
          dump_file_ << affinity_allocator_.updateMachInfo(
              core_counter_, process_and_type_[(process_names_vec_[process_counter_]).c_str()]);
        }
      }
      // affinity_allocator_.printAffinity();
      // this assumes that there is only one core attached to one process
      // if (core_allocated_ >= 0)
      //   affinity_allocator_.updateMachInfo(core_allocated_, process_and_type_[(process_names_vec_[ process_counter_
      //   ]).c_str()]);

      // CPU_OR(&mask_set, &mask_set, &cpu_set);
    }
  }
  for (unsigned int init_core_counter_ = 0; init_core_counter_ < freemask_affinity_vec_.size(); init_core_counter_++)
    dump_file_ << affinity_allocator_.updateMachInfo(freemask_affinity_vec_[init_core_counter_], kLowPri);
  if (!process_name_for_given_pid_.empty())
    process_type_ = process_and_type_[process_name_for_given_pid_];
  else
    process_type_ = kLowPri;

  affinity_allocator_.ExcludeCore0();  // Should not affine to core 0
  core_allocated_ = affinity_allocator_.allocate(process_type_);
  if (core_allocated_ < 0) {
    // std::cout << "Unable to allocate core for process. No empty cores found\n";
    dump_file_ << "No empty Cores found\n";
    dump_file_ << affinity_allocator_.printAffinity();
    dump_file_.close();
    if (send_mail_) affinity_allocator_.AlertMail(pid, process_type_);
    HFSAT::SemUtils::signal_and_unlock(sem_id_);

    return -1;
  }
  affinity_allocator_.updateMachInfo(core_allocated_, process_type_);
  // affinity_allocator_.printAffinity();
  if (CPUManager::setAffinity(core_allocated_, pid)) {
    if (core_allocated_ < 0) {
      dump_file_ << affinity_allocator_.printAffinity();
      dump_file_.close();
      if (send_mail_) affinity_allocator_.AlertMail(pid, process_type_);
    } else {
      proc_name_to_core_id_[process_name] = core_allocated_;
    }
    HFSAT::SemUtils::signal_and_unlock(sem_id_);

    std::ofstream afffined_process_list_to_monitor_file_;
    afffined_process_list_to_monitor_file_.open(AFFINED_PID_PROCS_LIST_FILENAME, std::ios::app);  // to be appended

    if (!afffined_process_list_to_monitor_file_.is_open()) {  // not exiting
      std::cerr << " File : " << AFFINED_PID_PROCS_LIST_FILENAME << " does not exist " << std::endl;
    }

    std::ostringstream temp_oss_;
    temp_oss_ << process_name << " " << pid << " " << core_allocated_ << "\n";
    std::string pid_proc_ = temp_oss_.str();

    afffined_process_list_to_monitor_file_.write(pid_proc_.c_str(), pid_proc_.length());

    if (afffined_process_list_to_monitor_file_.bad()) {
      std::cerr << " Write Failed to " << AFFINED_PID_PROCS_LIST_FILENAME << "\n";
    }

    afffined_process_list_to_monitor_file_.close();

    return core_allocated_;
  }

  dump_file_.close();
  HFSAT::SemUtils::signal_and_unlock(sem_id_);

  return -1;
}

#endif
