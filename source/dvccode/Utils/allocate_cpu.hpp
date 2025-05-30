/**
    \file dvccode/Utils/allocate_cpu.hpp

    \author: (c) Copyright Two Roads Technological Solutions Pvt Ltd 2011
     Address:
         Suite 217, Level 2, Prestige Omega,
         No 104, EPIP Zone, Whitefield,
         Bangalore - 560066, India
         +91 80 4060 0717
 */
#pragma once

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
#include "dvccode/CommonDataStructures/perishable_string_tokenizer.hpp"

namespace HFSAT {

class AllocateCPUUtils {
 public:
  static AllocateCPUUtils& GetUniqueInstance();
  static void ResetUniqueInstance();

  /* It will try to allocate CPU. If it fails to do so, it will raise SIGINT */
  void AllocateCPUOrExit(std::string thread_name);

 private:
  AllocateCPUUtils();
  ~AllocateCPUUtils();

  int AllocateCPU(std::string thread_name);

  static AllocateCPUUtils* unique_ptr;
};
}
