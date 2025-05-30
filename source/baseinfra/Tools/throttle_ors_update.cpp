// =====================================================================================
//
//       Filename:  throttle_ors_update.cpp
//
//    Description: Throttle Update from the ORS
//
//        Version:  1.0
//        Created:  10/29/2015 08:36:37 AM
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

#include <iostream>
#include <stdio.h>
#include <stdlib.h>
#include <signal.h>
#include <getopt.h>
#include <fstream>
#include <time.h>
#include <unistd.h>
#include <sstream>
#include <map>
#include <cstring>

#include "baseinfra/MDSMessages/mds_message_listener.hpp"
#include "dvccode/CDef/ors_defines.hpp"
#include "dvccode/CDef/ors_messages.hpp"
#include "dvccode/TradingInfo/network_account_info_manager.hpp"
#include "baseinfra/BaseTrader/base_live_trader.hpp"
#include "baseinfra/BaseTrader/query_tag_info.hpp"
#include "dvccode/Utils/eti_algo_tagging.hpp"
#include "dvccode/CommonDataStructures/fast_price_convertor.hpp"
#include "dvccode/CommonDataStructures/security_name_indexer.hpp"
#include "dvccode/Utils/hybrid_sec.hpp"

#include "baseinfra/MarketAdapter/security_market_view.hpp"

#include "dvccode/Utils/rdtsc_timer.hpp"
#include "dvccode/Utils/thread.hpp"

#include "dvccode/CDef/defines.hpp"
#include "dvccode/CDef/security_definitions.hpp"
#include "dvccode/CDef/file_utils.hpp"
#include "dvccode/CDef/trading_location_manager.hpp"
#include "dvccode/CDef/exchange_symbol_manager.hpp"
#include "dvccode/CDef/debug_logger.hpp"
#include "dvccode/CDef/stored_market_data_common_message_defines.hpp"
#include "dvccode/CDef/random_channel.hpp"
#include "dvccode/CDef/mds_messages.hpp"
#include "dvccode/CDef/shared_logging_defines.hpp"
#include "dvccode/Utils/lock.hpp"
#include "dvccode/Utils/client_logging_segment_initializer.hpp"

#include "baseinfra/MDSMessages/combined_mds_messages_multi_shm_processor.hpp"

#include "dvccode/CommonDataStructures/security_name_indexer.hpp"
#include "dvccode/CommonDataStructures/perishable_string_tokenizer.hpp"

#include "dvccode/CommonTradeUtils/date_time.hpp"
#include "dvccode/CommonTradeUtils/watch.hpp"
#include "dvccode/TradingInfo/network_account_info_manager.hpp"

#include "dvccode/ExternalData/simple_live_dispatcher.hpp"

#include "baseinfra/MarketAdapter/shortcode_security_market_view_map.hpp"
#include "baseinfra/MarketAdapter/market_defines.hpp"

#include "baseinfra/MarketAdapter/book_init_utils.hpp"
#include "dvccode/Utils/runtime_profiler.hpp"
#include "dvccode/Utils/slack_utils.hpp"
#include "dvccode/Utils/common_files_path.hpp"

uint32_t mts = 0;
int yyyymmdd = 0;
time_t m_time_t;
int total_order_count = 0;
int total_order_size = 0;
HFSAT::ExchSource_t exchange_source = HFSAT::kExchSourceCME;

HFSAT::Watch* watch_;
void UpdateThrottle(HFSAT::BaseTrader* tr,int throttle_limit,int ioc_throttle_limit) {

  try {
    std::cout << "Throttle_limit: " << throttle_limit <<" IOC " << ioc_throttle_limit << std::endl;
    tr->UpdateThrottle(throttle_limit, ioc_throttle_limit);
    usleep(100000);
  } catch (std::exception& e) {
    std::cout << "SendOrder fails: " << e.what();
  }
}

int main(int argc, char** argv) {
  int throttle_limit, ioc_throttle_limit;
  std::string exchange_;
  if (argc == 4){
    exchange_ = argv[1];
    throttle_limit = stoi(argv[2]);
    ioc_throttle_limit = stoi(argv[3]);
  }
  else{
    printf("Usage:%s EXCH[NSE_FO] THROTTLE[300] IOC_THROTTLE[50]\n", argv[0]);
    exit(-1);
  } 

  std::string shortcode;
  if ( exchange_ =="NSE_FO"){
     shortcode="NSE_SBIN_FUT0";
  }else {
     shortcode="NSE_SBIN";
  }

  /// get current date
  time(&m_time_t);
  struct tm m_tm;
  localtime_r(&m_time_t, &m_tm);
  yyyymmdd = (1900 + m_tm.tm_year) * 10000 + (1 + m_tm.tm_mon) * 100 + m_tm.tm_mday;

  HFSAT::SecurityDefinitions::GetUniqueInstance(yyyymmdd).LoadNSESecurityDefinitions();
  if (std::string::npos != shortcode.find("_FUT") || HFSAT::NSESecurityDefinitions::IsOption(shortcode)) {
        exchange_source = HFSAT::kExchSourceNSE_FO;
  } else {
        exchange_source = HFSAT::kExchSourceNSE_EQ;
  }
  std::cout<< ExchSourceStringForm(exchange_source) <<std::endl;
  HFSAT::NetworkAccountInfoManager network_account_info_manager_;
  HFSAT::ExchangeSymbolManager::SetUniqueInstance(yyyymmdd);
  const char* exch_symbol_ = HFSAT::ExchangeSymbolManager::GetExchSymbol(shortcode);
  std::cout << "Using Exchange Symbol : " << exch_symbol_ << "\n";

  HFSAT::DebugLogger dbglogger_(1024);
  std::ostringstream t_temp_oss_;
  t_temp_oss_ << "/spare/local/logs/alllogs/console_trader_log." << exch_symbol_ << "."
              << HFSAT::DateTime::GetCurrentIsoDateLocal() << "." << m_time_t;
  std::string logfilename_ = t_temp_oss_.str();
  std::cout << "Printing all logs into file: " << logfilename_ << std::endl;
  dbglogger_.OpenLogFile(logfilename_.c_str(), std::ofstream::out);

  HFSAT::BaseTrader* trader = new HFSAT::BaseLiveTrader(
      exchange_source, network_account_info_manager_.GetDepTradeAccount(exchange_source, shortcode),
      network_account_info_manager_.GetDepTradeHostIp(exchange_source, shortcode),
      network_account_info_manager_.GetDepTradeHostPort(exchange_source, shortcode), *watch_, dbglogger_);

  usleep(3000000);  // 1 sec sleep to complete all print statements in initialization of threads

  UpdateThrottle(trader, throttle_limit, ioc_throttle_limit);
}
