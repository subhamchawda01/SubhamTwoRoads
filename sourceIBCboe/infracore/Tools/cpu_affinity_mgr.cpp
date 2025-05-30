/**
   \file Tools/cpu_affinity_mgr.cpp

   \author: (c) Copyright Two Roads Technological Solutions Pvt Ltd 2011
   Address:
   Suite No 162, Evoma, #14, Bhattarhalli,
   Old Madras Road, Near Garden City College,
   KR Puram, Bangalore 560049, India
   +91 80 4190 3551

*/
#include <iostream>
#include <fstream>
#include "dvccode/CommonTradeUtils/date_time.hpp"
#include "dvccode/Utils/CPUAffinity.hpp"
#include "dvccode/CommonDataStructures/perishable_string_tokenizer.hpp"

int main(int argc, const char *argv[]) {
  if (argc < 4) {
    std::cout << "USAGE : \n\t" << argv[0] << " COMMAND NEW_PROC_PID NAME_OF_PROCESS <SEND_MAIL=1>" << std::endl;
    std::cout << "COMMAND=\"ASSIGN\"\tAssigns CPU affin to proc with PID NEW_PROC_PID while managing cores between all "
                 "process names specified in the affinity_list file"
              << std::endl;
    std::cout << "COMMAND=\"VIEW\"\tDisplays CPU affin for proc with PID NEW_PROC_PID" << std::endl;
    return -1;
  }

  std::vector<std::string> affinity_process_list_vec_;

  std::string name_of_process = "EXTERNAL";
  if (argc > 3) {
    name_of_process = argv[3];
  }

  bool send_mail_ = true;
  if (argc > 4) {
    send_mail_ = (strcmp(argv[4], "0") == 0) ? false : true;
  }

  process_type_map process_and_type_;
  process_and_type_ = AffinityAllocator::parseProcessListFile(affinity_process_list_vec_);

  if (!strncmp(argv[1], "ASSIGN", strlen("ASSIGN"))) {
    // This is the PID to assign affin for.
    int new_pid_ = atoi(argv[2]);

    // argc - 2 to exclude last argv which is PID & first argv which is COMMAND.
    int core_assigned_ = CPUManager::allocateFirstBestAvailableCore(process_and_type_, affinity_process_list_vec_,
                                                                    new_pid_, name_of_process, send_mail_);

    std::cout << "ASSIGN : PID : " << new_pid_ << " CORE # " << core_assigned_ << std::endl;
  } else if (!strncmp(argv[1], "VIEW", strlen("VIEW"))) {
    // This is the PID to view affin for.
    int new_pid_ = atoi(argv[2]);

    // argc - 2 to exclude last argv which is PID & first argv which is COMMAND.
    cpu_aff_t affinity_ = CPUManager::getAffinity(new_pid_);

    std::cout << "VIEW : PID : " << new_pid_ << " CORE # ";

    for (int i = 0; i < CPUManager::getTotalAvailableCores(); ++i) {
      if (affinity_ & CPUManager::getMask(i)) {
        std::cout << i << " ";
      }
    }
    std::cout << std::endl;
  } else {
    std::cout << "COMMAND : " << argv[1] << " not recognized." << std::endl;
  }

  return 0;
}
