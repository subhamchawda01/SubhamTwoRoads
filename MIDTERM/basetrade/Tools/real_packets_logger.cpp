#include "MDSMessages/combined_mds_messages_shm_logger.hpp"
#include "CDef/debug_logger.hpp"
#include "Utils/common_files_path.hpp"

HFSAT::DebugLogger *dbglogger_ = new HFSAT::DebugLogger(10240, 1);

void termination_handler(int signum) {
  dbglogger_->Close();
  delete dbglogger_;

  exit(0);
}

std::string ConstructFilePath(){
	std::string file_path(HFSAT::FILEPATH::kRealPacketsOrderDirectory);
	std::string file_name(HFSAT::FILEPATH::kRealPacketsOrderFileName);
	return file_path + file_name + "_" + HFSAT::DateTime::GetCurrentIsoDateLocalAsString();
}

int main(int argc, char **argv) {
  // signal handling, Interrupts and seg faults
  signal(SIGINT, termination_handler);
  signal(SIGSEGV, termination_handler);
  signal(SIGPIPE, SIG_IGN);

  std::string mdslog_file_name_ = ConstructFilePath();
  HFSAT::MDSMessages::CombinedMDSMessagesShmLogger combined_mds_messages_shm_logger_(
      *dbglogger_, HFSAT::kComShmConsumer, mdslog_file_name_);
  combined_mds_messages_shm_logger_.RunLiveShmSource();

  return 0;
}
