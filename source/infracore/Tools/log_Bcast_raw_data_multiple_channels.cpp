// =====================================================================================
// 
//       Filename:  log_Bcast_raw_data_multiple_channels.cpp
// 
//    Description:  log multiple streams OI data
// 
//        Version:  1.0
//        Created:  06/08/2023 07:22:28 AM
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


#include "dvccode/CDef/online_debug_logger.hpp"
#include "infracore/NSEMD/nse_md_report_handler.hpp"
typedef std::map<std::string, std::string> KEY_VAL_MAP;

HFSAT::OnlineDebugLogger *online_dbglogger_ = new HFSAT::OnlineDebugLogger(1024 * 4 * 1024, 1109, 256 * 1024);
HFSAT::DebugLogger &dbglogger_ = *(online_dbglogger_);  //( 4*1024*1024, 256*1024 ); // making logging more efficient,



HFSAT::ClockSource &clock_source_ = HFSAT::ClockSource::GetUniqueInstance();



int main(int argc, char *argv[]) {
  if (argc < 4) {
    printf("USAGE: %s <channel_file> <interface> <OF of>\nSize of TimeVal: %ld\t Int: %ld\n", argv[0], sizeof(timeval),
           sizeof(uint32_t));
    exit(0);
  }

std::string CHANNEL_DETAILS_FILE = argv[1];

std::string INDEX= argv[3];
std::string interface = argv[2];
//char fname[200];
//sprintf(fname, "/spare/local/MDSlogs/RawData/Bcast/%s_%s.raw", argv[4], currentDateTime().c_str());
//std::cerr << "File name: " << fname << std::endl;

KEY_VAL_MAP shc_to_channel_info_;
//std::set<std::string> included_channels_;

HFSAT::SimpleLiveDispatcher* simple_live_dispatcher = new HFSAT::SimpleLiveDispatcher();

HFSAT::NSEMDReportHandler nse_md_report_handler (dbglogger_, *simple_live_dispatcher, CHANNEL_DETAILS_FILE, interface);
std::cout<<"nse_md_report_handler constructed"<<std::endl;
//sock_sub.run();
//LoadChannelToSubscribe(CHANNEL_DETAILS_FILE,INDEX);
char hostname[128];
hostname[127] = '\0';
gethostname(hostname, 127);

nse_md_report_handler.run_logger_thread();
simple_live_dispatcher->RunLive();

if (std::string(hostname).find("sdv-ny4-srv") == std::string::npos) {
    HFSAT::AllocateCPUUtils::GetUniqueInstance().AllocateCPUOrExit("log_Bcast_raw_data_multiple_channels");
}
return 0;
}
