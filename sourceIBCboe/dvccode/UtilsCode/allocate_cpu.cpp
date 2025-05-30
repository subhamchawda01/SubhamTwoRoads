/**
    \file dvccode/Utils/allocate_cpu.cpp

    \author: (c) Copyright Two Roads Technological Solutions Pvt Ltd 2011
     Address:
         Suite 217, Level 2, Prestige Omega,
         No 104, EPIP Zone, Whitefield,
         Bangalore - 560066, India
         +91 80 4060 0717
 */

#include "dvccode/Utils/allocate_cpu.hpp"

namespace HFSAT {
AllocateCPUUtils* AllocateCPUUtils::unique_ptr = nullptr;

AllocateCPUUtils::AllocateCPUUtils() {}
AllocateCPUUtils::~AllocateCPUUtils() {}

AllocateCPUUtils& AllocateCPUUtils::GetUniqueInstance() {
  if (unique_ptr == nullptr) {
    unique_ptr = new AllocateCPUUtils();
  }
  return *(unique_ptr);
}

void AllocateCPUUtils::ResetUniqueInstance() {
  if (unique_ptr != nullptr) {
    delete unique_ptr;
  }
}

void AllocateCPUUtils::AllocateCPUOrExit(std::string thread_name) {
  int core_num_allocd = AllocateCPU(thread_name);
  if (core_num_allocd < 0) {
    std::cout << "Failed to allocate core for Thread : " << thread_name << ". Raising SIGINT!" << std::endl;
    //raise(SIGINT);
  }
}

int AllocateCPUUtils::AllocateCPU(std::string thread_name) {
  std::vector<std::string> affinity_process_list_vec_;
  process_type_map process_and_type_;
  process_and_type_ = AffinityAllocator::parseProcessListFile(affinity_process_list_vec_);

  int threadId = ((int)(syscall(SYS_gettid)));
  int core_alloced_ = CPUManager::allocateFirstBestAvailableCore(process_and_type_, affinity_process_list_vec_,
                                                                 threadId, thread_name, true);

  std::cout << "Thread : " << thread_name << " Thread ID : " << threadId << " CORE # " << core_alloced_ << std::endl;

  return core_alloced_;
}
}
