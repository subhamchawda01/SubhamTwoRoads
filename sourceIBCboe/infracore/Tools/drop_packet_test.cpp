#include <iostream>
#include <sstream>
#include <unistd.h> // Include for getpid()

#include "dvccode/CDef/defines.hpp"
#include "dvccode/CDef/debug_logger.hpp"
#include "dvccode/ExternalData/simple_live_dispatcher.hpp"
#include "dvccode/TradingInfo/network_account_interface_manager.hpp"
#include "infracore/IBKRMD/ibkr_l1_md_handler.hpp"
#include "dvccode/CDef/cboe_security_definition.hpp"
#include "dvccode/Profiler/cpucycle_profiler.hpp"
#include "dvccode/Profiler/cpucycle_profiler_defines.hpp"
HFSAT::DebugLogger dbglogger_(10240);

void termination_handler(int signum) {
  std::vector<HFSAT::CpucycleProfilerSummaryStruct> cpss_ = HFSAT::CpucycleProfiler::GetUniqueInstance().GetCpucycleSummary();
  for (unsigned int i = 0; i < cpss_.size(); i++) {
    dbglogger_ << "Min: " << cpss_[i].min_ << " "
              << "Max: " << cpss_[i].max_ << " "
              << "Mean: " << cpss_[i].mean_ << " "
              << "50p: " << cpss_[i].fifty_percentile_ << " "
              << "90p: " << cpss_[i].ninety_percentile_ << " "
              << "95p: " << cpss_[i].ninetyfive_percentile_ << "\n";
  }
  dbglogger_.DumpCurrentBuffer();

  // Save Fix Sequences

  signal(signum, SIG_DFL);
  kill(getpid(), signum);

  exit(-1);

  dbglogger_.Close();
  //  tradelogger_.Close ( );
  usleep(5000000);  // Sleep to receive logout confirmations.
  exit(0);
}
class DropPacketTest: public HFSAT::IBKRMD::MarketEventListener {
  private:
  // bool first_data=false;
  public:
  void OnMarketEventDispatch(IBL1UpdateTick* market_event, bool is_timeout) {
    HFSAT::CpucycleProfiler::GetUniqueInstance().End(0);
    HFSAT::CpucycleProfiler::GetUniqueInstance().Start(0);
    return;
  }
};
int main(int argc, char **argv){
  signal(SIGINT, termination_handler);
  signal(SIGTERM, termination_handler);
  if (argc>2 &&( strncmp(argv[2],"useAffinity",11)==0)) {
    std::cout<<"Will affine"<<std::endl;
    HFSAT::AllocateCPUUtils::GetUniqueInstance().AllocateCPUOrExit("drop_packet_test");
  }
  HFSAT::CpucycleProfiler::GetUniqueInstance(3);
  std::ostringstream t_temp_oss_;
  pid_t pid = getpid();
  std::string pathOfLog="/home/dvcinfra/trash/";
  if(argc>1){
    pathOfLog=std::string(argv[1]);
  }
  t_temp_oss_ << pathOfLog <<"dropPacketTest." << pid;
//   HFSAT::Utils::TCPServerManager tcp_server_manager(client_recovery_port, dbglogger);

//   tcp_server_manager.run();
  std::string logfilename_ = t_temp_oss_.str();
  DropPacketTest obj;
  dbglogger_.OpenLogFile(logfilename_.c_str(), std::ofstream::out | std::ofstream::app);
  HFSAT::SimpleLiveDispatcher simple_live_dispatcher;
  HFSAT::IBKRMD::IBKRL1MDHandler::GetUniqueInstance(dbglogger_,simple_live_dispatcher,HFSAT::kRaw,&obj);
  simple_live_dispatcher.RunLive();
}