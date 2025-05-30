#include <getopt.h>
#include "infracore/Tools/exanic_reader.hpp"
#include "dvccode/CDef/stored_market_data_common_message_defines.hpp"
#include "dvccode/CDef/defines.hpp"
#include "dvccode/CDef/refdata_locator.hpp"
#include "dvccode/Profiler/cpucycle_profiler.hpp"
#include "dvccode/Utils/allocate_cpu.hpp"

void signalHandler(int signum) {
  std::cout << " \n";
  std::cout << "Interruption signal (" << signum << ") received!\n";
  std::string summary = HFSAT::CpucycleProfiler::GetUniqueInstance().GetCpucycleSummaryString();
  std::cout << "CPU Cycle Profiler Stats: \n" << summary << std::endl;
  signal(signum, SIG_DFL);
  kill(getpid(), signum);
  exit(0);
}

HFSAT::FastMdConsumerMode_t ExtractMode(std::string mode) {
  if (mode == "Logger") {
    return HFSAT::kLogger;
  } else if (mode == "Writer") {
    return HFSAT::kProShm;
  } else if (mode == "Hybrid") {
    return HFSAT::kRaw;
  } else if (mode == "STRAT") {
    return HFSAT::kLiveConsumer;
  }
  return HFSAT::kModeMax;
}


void PrintUsage(const char* prg_name) {
  printf(" This is the exanic Data Daemon exec \n");
  printf(" Usage:%s <interface> <port>\n", prg_name);
}

int main(int argc, char* argv[]) {
  HFSAT::AllocateCPUUtils::GetUniqueInstance().AllocateCPUOrExit("Exanic_daemon");
  // Assigning signal handler to quit application by typing Ctrl-4
  if (argc < 2) {
    std::cerr << " Usage: " << argv[0] << "<interface> <port>" << std::endl;
    exit(0);
  }
  signal(SIGQUIT, signalHandler);
  std::cout << "Enter" << std::endl;
  signal(SIGINT, signalHandler);
  HFSAT::CpucycleProfiler::SetUniqueInstance(10);
//HFSAT::CpucycleProfiler::GetUniqueInstance().SetTag(1, "FPGA -> NSEFpga::ReadData ");
  std::string mode = "Logger";
  std::string interface = argv[1];
  std::string port_ = argv[2];

  
  NSE_Exanic::ExanicReader &exanic_reader = NSE_Exanic::ExanicReader::GetUniqueInstance(interface, port_); 
  if (!exanic_reader.Initialize(ExtractMode(mode))) {
    std::cerr << "Intialise failed Exiting..." << std::endl;
    return -1;
  }

  std::cout<<"Running Updates: "<<std::endl;
  exanic_reader.RunLive();
  return 0;
}
