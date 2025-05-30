// =====================================================================================
//
//       Filename:  mid_term_data_server.cpp
//
//    Description:
//
//        Version:  1.0
//        Created:  01/07/2016 05:19:39 AM
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

#include <signal.h>
#include <cstdlib>
#include <iostream>
#include <getopt.h>
#include "midterm/MidTerm/trade_bar_generator.hpp"
#include "dvccode/Utils/thread.hpp"
#include "dvccode/CDef/mds_shm_interface_defines.hpp"
#include "dvccode/CommonTradeUtils/date_time.hpp"
#include "midterm/MidTerm/Timer_thread.hpp"
#include "dvccode/CommonDataStructures/security_name_indexer.hpp"
#include "dvccode/CDef/exchange_symbol_manager.hpp"
#include "baseinfra/MDSMessages/combined_mds_messages_shm_processor.hpp"
#include "baseinfra/MarketAdapter/indexed_nse_market_view_manager.hpp"
#include "baseinfra/MarketAdapter/indexed_nse_market_view_manager_2.hpp"
#include "dvccode/Utils/shortcode_request_helper.hpp"

HFSAT::ShortcodeRequestHelper *global_shc_request_helper = nullptr;

class DataServerManager {
private:
  HFSAT::DebugLogger &dbglogger_;
  TradeBarGenerator &trade_bar_generator_;
  HFSAT::TCPClientSocket *tcp_client_socket_;

public:
  DataServerManager(Mode operating_mode, HFSAT::DebugLogger &dbglogger,
                    HFSAT::Watch &watch)
      : dbglogger_(dbglogger),
        trade_bar_generator_(TradeBarGenerator::GetUniqueInstance(
            operating_mode, dbglogger, watch, true)),
        tcp_client_socket_(new HFSAT::TCPClientSocket()) {
    // Runs tcp wait thread for ORS replies
    if (operating_mode == Mode::kNSELoggerMode)
      return;
  }

  TradeBarGenerator *GetDataProcessingObject() { return &trade_bar_generator_; }

  // cleanup stuff
  void CleanUp() { trade_bar_generator_.CleanUp(); }
};

DataServerManager *GLOBAL_PTR_FOR_TERMINATION;
void termination_handler(int32_t signal_handler) {
  if (NULL != GLOBAL_PTR_FOR_TERMINATION) {
    GLOBAL_PTR_FOR_TERMINATION->CleanUp();
  }

  if (global_shc_request_helper != nullptr) {
    global_shc_request_helper->RemoveAllShortcodesToListen();
    global_shc_request_helper = nullptr;
  }
}

void print_usage_and_exit(char const *prog_name) {
  std::cout << "THIS IS THE MID_TERM DATA SERVER EXEC" << std::endl;
  std::cout << "USAGE : <" << prog_name
            << "> --products_file <products_file_complete_pathname> "
               "--data_process_mode <LOGGER/SERVER/SERVERLOG/OFFLINE> --qid "
               "<qid>"
            << std::endl;
  exit(-1);
}

static struct option data_options[] = {
    {"help", no_argument, 0, 'h'},
    {"products_file", required_argument, 0, 'a'},
    {"data_process_mode", required_argument, 0, 'b'},
    {"qid", required_argument, 0, 'c'},
    {0, 0, 0, 0}};

int main(int argc, char *argv[]) {
  signal(SIGPIPE, SIG_IGN);
  // TODO Handle Later
  //  signal(SIGINT, termination_handler);
  //  signal(SIGSEGV, termination_handler);

  // to filter unnecessary products
  std::string products_of_interest_file = "INVALID";
  std::string operating_string_data_mode = "INVALID";

  int qid = -1;

  int32_t option_return_value = 0;
  int32_t help_flag = 0;

  while (1) {
    int32_t option_index = 0;
    option_return_value =
        getopt_long(argc, argv, "", data_options, &option_index);
    if (-1 == option_return_value)
      break;

    switch (option_return_value) {
    case 'h':
      help_flag = 1;
      break;
    case 'a':
      products_of_interest_file = optarg;
      break;
    case 'b':
      operating_string_data_mode = optarg;
      break;
    case 'c':
      qid = atoi(optarg);
      break;
    case '?':
      if ('a' == optopt || 'b' == optopt) {
        std::cerr << "Option : " << optopt << " Requires An Argument"
                  << std::endl;
        print_usage_and_exit(argv[0]);
      }
      break;
    default: {
      std::cerr << "Unknown Option Specified, No Handling Yet For : " << optopt
                << std::endl;
      print_usage_and_exit(argv[0]);
    } break;
    }
  }

  if (1 == help_flag)
    print_usage_and_exit(argv[0]);

  if (std::string("INVALID") == products_of_interest_file ||
      std::string("INVALID") == operating_string_data_mode)
    print_usage_and_exit(argv[0]);

  if (false == HFSAT::FileUtils::ExistsAndReadable(products_of_interest_file)) {
    std::cerr << "Products File Given In Input Either Doesn't Exist Or It's "
                 "Not Readable"
              << std::endl;
    exit(-1);
  }

  Mode operating_mode = Mode::kInvalid;

  if (std::string("SERVER") == operating_string_data_mode)
    operating_mode = Mode::kNSEServerMode;
  else if (std::string("OFFLINE") == operating_string_data_mode)
    operating_mode = Mode::kNSEOfflineMode;
  else if (std::string("SERVERLOG") == operating_string_data_mode)
    operating_mode = Mode::kNSEHybridMode;
  else
    operating_mode = Mode::kNSELoggerMode;

  // Read file and add products of interest
  std::ifstream products_file_stream;
  products_file_stream.open(products_of_interest_file);

  if (!products_file_stream.is_open()) {
    std::cerr << "Unable To Read File ! : " << products_of_interest_file
              << std::endl;
    exit(-1);
  }

  int32_t trading_date = HFSAT::DateTime::GetCurrentIsoDateLocal();

      HFSAT::Utils::NSEDailyTokenSymbolHandler::GetUniqueInstance(trading_date);
      HFSAT::Utils::NSERefDataLoader::GetUniqueInstance(trading_date);

  // Set Exchagne Symbol Manager
  HFSAT::ExchangeSymbolManager::SetUniqueInstance(trading_date);
  HFSAT::SecurityDefinitions::GetUniqueInstance(trading_date)
      .LoadNSESecurityDefinitions();

  HFSAT::SecurityNameIndexer &sec_name_indexer =
      HFSAT::SecurityNameIndexer::GetUniqueInstance();

  HFSAT::ShortcodeRequestHelper shc_request_helper(qid);
  global_shc_request_helper = &shc_request_helper;

  std::vector<std::string> shc_list;

  std::string line;
  while (getline(products_file_stream, line)) {
    if (line.substr(0, 1) == "#" || line.empty()) {
      continue;
    }

    std::vector<std::string> tokens_;
    HFSAT::PerishableStringTokenizer::StringSplit(line, '\t', tokens_);
    // Get all option shortcodes for the ticker
    std::vector<std::string> temp_shcs_ = HFSAT::NSESecurityDefinitions::
        GetAllOptionShortcodesForUnderlyingGeneric(tokens_[0]);
    // Add FUT0 and FUT1 by default
    std::string fut0_shc_ = "NSE_" + tokens_[0] + "_FUT0";
    std::string fut1_shc_ = "NSE_" + tokens_[0] + "_FUT1";
	bool is_shortcode_fut0_present = HFSAT::SecurityDefinitions::CheckIfContractSpecExists(
										fut0_shc_, HFSAT::DateTime::GetCurrentIsoDateLocal());
	bool is_shortcode_fut1_present = HFSAT::SecurityDefinitions::CheckIfContractSpecExists(
             							fut1_shc_, HFSAT::DateTime::GetCurrentIsoDateLocal());
	if ( true == is_shortcode_fut0_present ) {
		shc_list.push_back(fut0_shc_);
	}
	if ( true == is_shortcode_fut1_present) {
		shc_list.push_back(fut1_shc_);
	}

    // Filter
    if (tokens_.size() > 1) {
      double last_close_ =
          HFSAT::NSESecurityDefinitions::GetLastClose(fut0_shc_);
      double moneyness_ = atof(tokens_[1].c_str());
      int expiries_to_look_till = 0;
      if (tokens_.size() > 2) {
        expiries_to_look_till = atoi(tokens_[2].c_str());
      }

      for (auto shc_ : temp_shcs_) {
        std::vector<std::string> temp_tokens_;
        HFSAT::PerishableStringTokenizer::StringSplit(shc_, '_', temp_tokens_);
        // Filter out contracts ( default 0 or specified from params )
        // Note : This works for all option shortocdes eg -> NSE_ABC_C0_A
        int expiry_num =
            atoi(temp_tokens_[2].substr(temp_tokens_[2].size() - 1, 1).c_str());
        if (expiry_num > expiries_to_look_till) {
          continue;
        }

        if (!HFSAT::NSESecurityDefinitions::IsFuture(shc_)) {
          double strike_ =
              HFSAT::NSESecurityDefinitions::GetStrikePriceFromShortCodeGeneric(
                  shc_);
          if (std::abs(strike_ / last_close_ - 1) > moneyness_) {
            continue;
          }
          shc_list.push_back(shc_);
        }
      }
    }
  }

  for (auto shc_ : shc_list) {
    sec_name_indexer.AddString(
        HFSAT::ExchangeSymbolManager::GetExchSymbol(shc_), shc_);
  }

  products_file_stream.close(); // END OF FILE PART

  shc_request_helper.AddShortcodeListToListen(shc_list);

  HFSAT::DebugLogger dbglogger(10240, 1);
  std::ostringstream t_temp_oss;
  t_temp_oss << "/spare/local/logs/alllogs/mid_term_data_server_"
             << trading_date << ".log";
  dbglogger.OpenLogFile(t_temp_oss.str().c_str(), std::ofstream::app);

  // Log Testing, Breaks Here if something is wrong rather than at a later point
  dbglogger << "Opened LogFile In Append Mode \n";
  dbglogger.DumpCurrentBuffer();
  HFSAT::Watch watch(dbglogger, trading_date);

  HFSAT::SecurityMarketViewPtrVec &sid_to_smv_ptr_map =
      HFSAT::sid_to_security_market_view_map();

  DataServerManager data_server_manager(operating_mode, dbglogger, watch);

  for (uint32_t sec_counter = 0; sec_counter < sec_name_indexer.NumSecurityId();
       sec_counter++) {
    HFSAT::SecurityMarketView *this_new_smv = new HFSAT::SecurityMarketView(
        dbglogger, watch, sec_name_indexer,
        sec_name_indexer.GetShortcodeFromId(sec_counter), sec_name_indexer.GetSecurityNameFromId(sec_counter),
        sec_counter, HFSAT::SecurityDefinitions::GetContractExchSource(
        sec_name_indexer.GetShortcodeFromId(sec_counter), trading_date), true, "INVALID", "INVALID", "INVALID");
    this_new_smv->SetL1OnlyFlag(false);
    this_new_smv->InitializeSMVForIndexedBook();
    this_new_smv->subscribe_rawtradeprints(
        data_server_manager.GetDataProcessingObject());
    this_new_smv->subscribe_L1_Only(
        data_server_manager.GetDataProcessingObject());
    sid_to_smv_ptr_map.push_back(this_new_smv);

    std::cout << "USING SYMBOL : "
              << sec_name_indexer.GetSecurityNameFromId(sec_counter) << '\t'
              << sec_name_indexer.GetShortcodeFromId(sec_counter) << '\t'
              << std::endl;
  }
  bool use_self_book_ = false;
  HFSAT::IndexedNSEMarketViewManager2 indexed_nse_market_view_manager(
      dbglogger, watch, sec_name_indexer, sid_to_smv_ptr_map, use_self_book_);
  HFSAT::MDSMessages::CombinedMDSMessagesShmProcessor
      combined_mds_messages_shm_processor(dbglogger, sec_name_indexer,
                                          HFSAT::kComShmConsumer);

  combined_mds_messages_shm_processor.AddDataSourceForProcessing(
      HFSAT::MDS_MSG::NSE, (void *)((HFSAT::OrderGlobalListenerNSE *)&(
                               indexed_nse_market_view_manager)),
      &watch);

  std::cout << "Started Both Servers" << std::endl;
  Timer_thread::GetUniqueInstance(
      (WatchUpdateListener *)data_server_manager.GetDataProcessingObject());

  // Loop
  combined_mds_messages_shm_processor.RunLiveShmSource();

  return EXIT_SUCCESS;
} // ----------  end of function main  ----------
