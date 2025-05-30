// =====================================================================================
//
//       Filename:  trade_bar_generator.hpp
//
//    Description:
//
//        Version:  1.0
//        Created:  01/05/2016 12:10:02 PM
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

#pragma once
#include <stdio.h>
#include <string.h>
#include <stdint.h>
#include <stdlib.h>
#include <ctype.h>
#include <vector>
#include <map>
#include <fstream>
#include <iostream>
#include <sstream>
#include "dvccode/CDef/debug_logger.hpp"
#include "dvccode/CommonTradeUtils/watch.hpp"
#include "dvccode/CDef/stored_market_data_common_message_defines.hpp"
#include "dvccode/Utils/tcp_server_manager.hpp"
#include "dvccode/Utils/tcp_server_socket_listener.hpp"
#include "baseinfra/MarketAdapter/security_market_view_change_listener.hpp"
#include "dvccode/CommonDataStructures/security_name_indexer.hpp"
#include "baseinfra/MarketAdapter/security_market_view.hpp"
#include "midterm/MidTerm/watch_update_listener.hpp"
#include "midterm/MidTerm/midterm_constants.hpp"

using namespace std;

struct Output {
  double open_price, close_price, low_price, high_price;
  int trades, volume;
  int start_time;
  int close_time;
  string expiry;
  double bid_px, ask_px;
  string ToString() {
    std::ostringstream t_temp_oss;

    t_temp_oss << start_time << "|";
    t_temp_oss << close_time << "|";
    t_temp_oss << expiry << "|";
    t_temp_oss << open_price << "|";
    t_temp_oss << close_price << "|";
    t_temp_oss << low_price << "|";
    t_temp_oss << high_price << "|";
    t_temp_oss << volume << "|";
    t_temp_oss << trades << "|";
    t_temp_oss << bid_px << "|";
    t_temp_oss << ask_px;

    return t_temp_oss.str();
  }
};

struct BA_struct {
  double Bid_Px, Ask_Px;
  string Expiry;
};

enum class Mode {
  kInvalid = 0,
  kNSELoggerMode,
  kNSEServerMode,
  kNSEOfflineMode,
  kNSEHybridMode,
  kNSESimMode
};

class TradeBarGenerator : public HFSAT::Utils::TCPServerSocketListener,
                          public HFSAT::SecurityMarketViewRawTradesListener,
                          public HFSAT::SecurityMarketViewChangeListener,
                          public WatchUpdateListener {
private:
  Mode operating_mode_;
  HFSAT::DebugLogger &dbglogger_;
  HFSAT::Watch &watch_;
  bool data_only_mode_;
  HFSAT::Utils::TCPServerManager *tcp_server_manager_;
  HFSAT::SecurityNameIndexer &sec_name_indexer_;
  HFSAT::SecurityMarketViewPtrVec &sid_to_smv_ptr_map_;
  char *packet_buffer_;
  map<std::string, BA_struct> shc_2_BA;
  map<string, map<int, Output>> map_price_;
  int bucket_ = -1;
  void AddTrade(NSE_MDS::NSETBTDataCommonStruct input_);
  void FillWithDefaultValues(Output &out_);
  void LogCompletedBucket(int bucket_);
  void UpdatePriceMap(std::string hash_string, int bucket_time_, double price_,
                      double qty_, std::string exp_, double best_bid_px_,
                      double best_ask_px_);
  std::string PrintCompletedBucket(int bucket_);
  bool is_pre_monthly_expiry_week;
  bool is_monthly_expiry_week;
  int last_data_received_time;

  TradeBarGenerator(Mode mode, HFSAT::DebugLogger &dbglogger,
                    HFSAT::Watch &watch, bool data_only_mode = false)
      : operating_mode_(mode), dbglogger_(dbglogger), watch_(watch),
        data_only_mode_(data_only_mode), tcp_server_manager_(NULL),
        sec_name_indexer_(HFSAT::SecurityNameIndexer::GetUniqueInstance()),
        sid_to_smv_ptr_map_(HFSAT::sid_to_security_market_view_map()),
        packet_buffer_(new char[NSE_MIDTERM_DATA_BUFFER_LENGTH]),
        is_pre_monthly_expiry_week(false), is_monthly_expiry_week(false),
        last_data_received_time(0) {
    int nearest_monthly_expiry_ =
        HFSAT::NSESecurityDefinitions::GetExpiryFromShortCode(
            "NSE_BANKNIFTY_C0_A");
    int nearest_weekly_expiry_ =
        HFSAT::NSESecurityDefinitions::GetExpiryFromShortCode(
            "NSE_BANKNIFTY_C0_A_W");
    int next_weekly_expiry_ =
        HFSAT::NSESecurityDefinitions::GetExpiryFromShortCode(
            "NSE_BANKNIFTY_C1_A_W");
    if (nearest_weekly_expiry_ > nearest_monthly_expiry_)
      is_monthly_expiry_week = true;
    else if (next_weekly_expiry_ > nearest_monthly_expiry_)
      is_pre_monthly_expiry_week = true;

    switch (operating_mode_) {
    case Mode::kNSELoggerMode:
    case Mode::kNSEOfflineMode: {
    } break;
    case Mode::kNSEServerMode:
    case Mode::kNSEHybridMode: {
      // Create thread to maintain a watch

      // std::thread (timer_thread, last_data_received_time).detach();
      if (data_only_mode_ == false)
        tcp_server_manager_ = new HFSAT::Utils::TCPServerManager(
            NSE_MIDTERM_DATA_SERVER_PORT, dbglogger, true);
      else {
        // Simulation server only used with data only mode
        int port_ = HFSAT::IsItSimulationServer()
                        ? TEST_SERVER_DATA_PORT
                        : NSE_MIDTERM_DATA_ONLY_SERVER_PORT;
        tcp_server_manager_ =
            new HFSAT::Utils::TCPServerManager(port_, dbglogger, true);
      }
      tcp_server_manager_->run();
    } break;
    default: {
      DBGLOG_CLASS_FUNC_LINE_FATAL
          << "INVALID OPERATING MODE : " << (int32_t)operating_mode_
          << DBGLOG_ENDL_FLUSH;
      DBGLOG_DUMP;
      exit(-1);
    } break;
    }
  }

  TradeBarGenerator(TradeBarGenerator const &disabled_copy_constructor) =
      delete; // don't want a copy constructor

public:
  static TradeBarGenerator &GetUniqueInstance(Mode mode,
                                              HFSAT::DebugLogger &dbglogger,
                                              HFSAT::Watch &watch,
                                              bool data_only_mode = false) {
    static TradeBarGenerator unique_instance(mode, dbglogger, watch,
                                             data_only_mode);
    return unique_instance;
  }

  void CleanUp() {
    if (NULL != tcp_server_manager_) {
      tcp_server_manager_->CleanUp();
    }
  }
  void OnTradePrint(unsigned int const security_id,
                    HFSAT::TradePrintInfo const &trade_print_info,
                    HFSAT::MarketUpdateInfo const &market_update_info) {}

  void OnRawTradePrint(unsigned int const security_id,
                       HFSAT::TradePrintInfo const &trade_print_info,
                       HFSAT::MarketUpdateInfo const &market_update_info);
  void OnMarketUpdate(unsigned int const security_id,
                      HFSAT::MarketUpdateInfo const &market_update_info);

  void OnClientRequest(int32_t client_fd, char *buffer,
                       uint32_t const &length) {}

  int GetLastDataReceivedTime();
};
