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
#include "dvccode/Utils/CPUAffinity.hpp"
#include "dvccode/CommonDataStructures/perishable_string_tokenizer.hpp"

int main(int argc, const char *argv[]) {
  if (argc < 3) {
    std::cout << "USAGE : \n\t" << argv[0] << " COMMAND NEW_PROC_PID SEND_MAIL=1" << std::endl;
    std::cout << "COMMAND=\"ASSIGN\"\tAssigns CPU affin to proc with PID NEW_PROC_PID while managing cores between all "
                 "process names specified in the affinity_list file"
              << std::endl;
    std::cout << "COMMAND=\"VIEW\"\tDisplays CPU affin for proc with PID NEW_PROC_PID" << std::endl;
    return -1;
  }

  std::vector<std::string> affinity_process_list_vec_;
  bool send_mail_ = true;
  /* std::ifstream affinity_process_list_file_ ;
   affinity_process_list_file_.open( AFFINITY_PROC_LIST_FILENAME );

   if( !affinity_process_list_file_.is_open() ){
     std::cerr << " File : " << AFFINITY_PROC_LIST_FILENAME << " does not exist " << std::endl;
     exit( -1 );
   }

   char line_buffer_[ 1024 ];
   std::string line_read_ ;
   affinity_process_list_vec_.clear( );

   while( affinity_process_list_file_.good() ){

     memset( line_buffer_, 0, sizeof( line_buffer_ ) );

     affinity_process_list_file_.getline( line_buffer_, sizeof( line_buffer_ ) );

     line_read_ = line_buffer_ ;

     if( line_read_.find("#") != std::string::npos )  continue;

     HFSAT::PerishableStringTokenizer st_ ( line_buffer_, sizeof( line_buffer_ ) );
     const std::vector< const char * > & tokens_ = st_.GetTokens ( );

     if( tokens_.size() > 0 )
       affinity_process_list_vec_.push_back( tokens_[0] );
   } */
  if (argc > 3) {
    send_mail_ = (strcmp(argv[argc - 1], "0") == 0) ? false : true;
  }
  process_type_map process_and_type_;
  process_and_type_ = AffinityAllocator::parseProcessListFile(affinity_process_list_vec_);

  if (!strncmp(argv[1], "ASSIGN", strlen("ASSIGN"))) {
    // This is the PID to assign affin for.
    int new_pid_ = atoi(argv[2]);

    // argc - 2 to exclude last argv which is PID & first argv which is COMMAND.
    int core_assigned_ =
        CPUManager::allocateFirstBestAvailableCore(process_and_type_, affinity_process_list_vec_, new_pid_, send_mail_);

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
