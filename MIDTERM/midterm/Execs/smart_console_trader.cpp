// =====================================================================================
//
//       Filename:  smart_console_trader.cpp
//
//    Description:
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

#include "baseinfra/MDSMessages/combined_mds_messages_shm_processor.hpp"

#include "dvccode/CommonDataStructures/security_name_indexer.hpp"
#include "dvccode/CommonDataStructures/perishable_string_tokenizer.hpp"

#include "dvccode/CommonTradeUtils/date_time.hpp"
#include "dvccode/CommonTradeUtils/watch.hpp"
#include "dvccode/TradingInfo/network_account_info_manager.hpp"

#include "dvccode/ExternalData/simple_live_dispatcher.hpp"

#include "baseinfra/MarketAdapter/shortcode_security_market_view_map.hpp"
#include "baseinfra/MarketAdapter/market_defines.hpp"

#include "baseinfra/MarketAdapter/indexed_liffe_price_level_market_view_manager.hpp"
#include "baseinfra/MarketAdapter/indexed_micex_market_view_manager.hpp"
#include "baseinfra/MarketAdapter/indexed_cme_market_view_manager.hpp"
#include "baseinfra/MarketAdapter/indexed_ntp_market_view_manager.hpp"
#include "baseinfra/MarketAdapter/indexed_eobi_price_level_market_view_manager.hpp"
#include "baseinfra/MarketAdapter/bmf_order_level_market_view_manager.hpp"
#include "baseinfra/MarketAdapter/indexed_eobi_market_view_manager.hpp"
#include "baseinfra/MarketAdapter/indexed_ose_price_feed_market_view_manager.hpp"
#include "baseinfra/MarketAdapter/hkex_indexed_market_view_manager.hpp"
#include "baseinfra/MarketAdapter/indexed_tmx_market_view_manager.hpp"
#include "baseinfra/MarketAdapter/indexed_ice_market_view_manager.hpp"
#include "baseinfra/MarketAdapter/indexed_hkomd_price_level_market_view_manager.hpp"
#include "baseinfra/MarketAdapter/indexed_cfe_market_view_manager.hpp"
#include "baseinfra/MarketAdapter/indexed_nse_market_view_manager.hpp"
#include "baseinfra/MarketAdapter/indexed_micex_of_market_view_manager.hpp"
#include "baseinfra/MarketAdapter/indexed_nse_market_view_manager_2.hpp"

#include "baseinfra/MarketAdapter/book_init_utils.hpp"
//#include "dvctrade/RiskManager/risk_notifier.hpp"
#include "dvccode/Utils/runtime_profiler.hpp"
#include "dvccode/Utils/slack_utils.hpp"
#include "dvccode/Utils/common_files_path.hpp"

#define MAX_CONSOLE_ORDER_LIMIT 20000
#define MAX_ORDER_LOT_LIMIT 4000

#define MAX_CONSOLE_EQUITY_ORDER_LIMIT 50000
#define MAX_EQUITY_ORDER_LOT_LIMIT 20000

#define MAX_CONSOLE_NSE_ORDER_LIMIT 500000

#define START_SEQUENCE 90000000
#define DUMMY_QID_FOR_CONSOLE_QUERY 123456
#define DUMMY_START_END_TIMES_FOR_CONSOLE_TRADES -1

class BookManager : public HFSAT::Thread,
                    public HFSAT::SecurityMarketViewChangeListener,
                    public HFSAT::OrderNotFoundListener,
                    public HFSAT::OrderSequencedListener,
                    public HFSAT::OrderConfirmedListener,
                    public HFSAT::OrderConfCxlReplacedListener,
                    public HFSAT::OrderConfCxlReplaceRejectListener,
                    public HFSAT::OrderCxlSeqdListener,
                    public HFSAT::OrderCanceledListener,
                    public HFSAT::OrderExecutedListener,
                    public HFSAT::OrderRejectedListener,
                    public HFSAT::OrderRejectedDueToFundsListener,
                    public HFSAT::OrderInternallyMatchedListener {
 private:
  HFSAT::DebugLogger& dbglogger_;
  HFSAT::Watch& watch_;
  std::string shortcode_;
  HFSAT::SecurityNameIndexer& sec_name_indexer_;
  long long unsigned int traded_volume_;
  HFSAT::MDSMessages::CombinedMDSMessagesShmProcessor combined_mds_messages_shm_processor_;
  HFSAT::MarketUpdateInfo market_update_info_;
  bool has_updated_market_info_ = false;
  int last_seen_sams = 0;
  HFSAT::Lock market_update_info_lock_;
  HFSAT::Lock sams_lock_;
  int saci_;
  int position = 0;
  std::map<int, HFSAT::BaseOrder> confirmed_orders_;
  HFSAT::CDef::LogBuffer* log_buffer_;
  HFSAT::Utils::ClientLoggingSegmentInitializer* client_logging_segment_initializer_;
  std::map<int, int> saos_to_exec_size_;

 public:
  BookManager(HFSAT::DebugLogger& dbglogger, HFSAT::Watch& watch, std::string const& shortcode, int _saci_,
              HFSAT::Utils::ClientLoggingSegmentInitializer* _client_logging_segment_initializer_)
      : dbglogger_(dbglogger),
        watch_(watch),
        shortcode_(shortcode),
        sec_name_indexer_(HFSAT::SecurityNameIndexer::GetUniqueInstance()),
        traded_volume_(0),
        combined_mds_messages_shm_processor_(dbglogger_, sec_name_indexer_, HFSAT::kComShmConsumer),
        market_update_info_(),
        has_updated_market_info_(false),
        last_seen_sams(0),
        market_update_info_lock_(),
        sams_lock_(),
        saci_(_saci_),
        position(0),
        confirmed_orders_(),
        log_buffer_(new HFSAT::CDef::LogBuffer()),
        client_logging_segment_initializer_(_client_logging_segment_initializer_) {
    char const* exchange_symbol = HFSAT::ExchangeSymbolManager::GetExchSymbol(shortcode_);
    int len = strlen(exchange_symbol);
    memcpy((void*)log_buffer_->buffer_data_.query_trade_.security_name_, (void*)exchange_symbol, std::min(40, len));
    log_buffer_->buffer_data_.query_trade_.security_name_[std::min(40, len)] = '\0';
  }

  void OnMarketUpdate(const unsigned int _security_id_, const HFSAT::MarketUpdateInfo& _market_update_info_) {
    market_update_info_lock_.LockMutex();
    market_update_info_ = _market_update_info_;
    has_updated_market_info_ = true;
    market_update_info_lock_.UnlockMutex();
  }

  // this function prints best bid and ask levels from last update(if it exists).
  void PrintBestLevels() {
    market_update_info_lock_.LockMutex();
    if (has_updated_market_info_) {
      printf("\n");
      if (market_update_info_.bidlevels_.size() > 0) {
        printf("%2d %5d %3d %11.7f", market_update_info_.bestbid_int_price_, market_update_info_.bestbid_size_,
               market_update_info_.bestbid_ordercount_, market_update_info_.bestbid_price_);
      }

      printf(" X ");

      if (market_update_info_.asklevels_.size() > 0) {
        printf("%11.7f %3d %5d %2d", market_update_info_.bestask_price_, market_update_info_.bestask_ordercount_,
               market_update_info_.bestask_size_, market_update_info_.bestask_int_price_);
      }
      printf("\n");
    } else {
      printf(
          "Did not receive any market update after console trader started. Please refer oebu for current market "
          "data.\n");
    }
    market_update_info_lock_.UnlockMutex();
  }

  // prints all confirmed orders in current run
  void PrintConfirmedOrders() {
    if (confirmed_orders_.empty()) {
      std::cout << "No confirmed orders in this run of console trader." << std::endl;
    } else {
      std::cout << "List of confirmed orders in this run of console trader: " << std::endl;
      for (auto& it : confirmed_orders_) {
        HFSAT::BaseOrder base_order = it.second;
        std::cout << "SAOS: " << it.first << " Price: " << base_order.price_ << " Size: " << base_order.size_remaining_
                  << " Side: " << base_order.buysell_ << " Int_price: " << base_order.int_price_ << std::endl;
      }
      std::cout << std::endl;
    }
  }

  inline void OnTradePrint(const unsigned int _security_id_, const HFSAT::TradePrintInfo& _trade_print_info_,
                           const HFSAT::MarketUpdateInfo& _market_update_info_) {
    traded_volume_ += _trade_print_info_.size_traded_;
  }

  // to check if a particular packet received on from ors feed belongs to this run of smart_console_trader. Also checks
  // for duplicate packets (as we listen on from two sources).
  bool isLatestVaildSams(const int32_t _server_assigned_message_sequence, int saci_received) {
    bool ret_val = false;
    sams_lock_.LockMutex();
    if (saci_received == saci_) {
      if (_server_assigned_message_sequence == 0)
        ret_val = true;
      else if (_server_assigned_message_sequence > last_seen_sams) {
        last_seen_sams = _server_assigned_message_sequence;
        ret_val = true;
      }
    }
    sams_lock_.UnlockMutex();
    return ret_val;
  }

  void OrderNotFound(const int t_server_assigned_client_id_, const int _client_assigned_order_sequence_,
                     const int _server_assigned_order_sequence_, const unsigned int _security_id_,
                     const HFSAT::TradeType_t r_buysell_, const int r_int_price_,
                     const int32_t server_assigned_message_sequence, const uint64_t exchange_order_id,
                     const HFSAT::ttime_t time_set_by_server) {
    if (isLatestVaildSams(server_assigned_message_sequence, t_server_assigned_client_id_)) {
      dbglogger_ << "Order not found: "
                 << "shortcode: " << shortcode_ << " "
                 << "SACI: " << t_server_assigned_client_id_ << " "
                 << "CAOS: " << _client_assigned_order_sequence_ << " "
                 << "SAOS: " << _server_assigned_order_sequence_ << " "
                 << "security_id: " << _security_id_ << " "
                 << "buysell: " << r_buysell_ << " "
                 << "Intprint: " << r_int_price_ << " "
                 << "SAMS: " << server_assigned_message_sequence << "\n";
    }
  }

  void OrderSequenced(const int t_server_assigned_client_id_, const int _client_assigned_order_sequence_,
                      const int _server_assigned_order_sequence_, const unsigned int _security_id_,
                      const double _price_, const HFSAT::TradeType_t r_buysell_, const int _size_remaining_,
                      const int _size_executed_, const int _client_position_, const int _global_position_,
                      const int r_int_price_, const int32_t server_assigned_message_sequence,
                      const uint64_t exchange_order_id, const HFSAT::ttime_t time_set_by_server) {
    if (isLatestVaildSams(server_assigned_message_sequence, t_server_assigned_client_id_)) {
      dbglogger_ << "Order Sequenced: "
                 << "shortcode: " << shortcode_ << " "
                 << "SACI: " << t_server_assigned_client_id_ << " "
                 << "CAOS: " << _client_assigned_order_sequence_ << " "
                 << "SAOS: " << _server_assigned_order_sequence_ << " "
                 << "security_id: " << _security_id_ << " "
                 << "price: " << _price_ << " "
                 << "buysell: " << r_buysell_ << " "
                 << "SR: " << _size_remaining_ << " "
                 << "SE: " << _size_executed_ << " "
                 << "CPos: " << _client_position_ << " "
                 << "GPos: " << _global_position_ << " "
                 << "Intprice: " << r_int_price_ << " "
                 << "SAMS: " << server_assigned_message_sequence << "\n";
      dbglogger_.DumpCurrentBuffer();
    }
  }

  void OrderConfirmed(const int t_server_assigned_client_id_, const int _client_assigned_order_sequence_,
                      const int _server_assigned_order_sequence_, const unsigned int _security_id_,
                      const double _price_, const HFSAT::TradeType_t r_buysell_, const int _size_remaining_,
                      const int _size_executed_, const int _client_position_, const int _global_position_,
                      const int r_int_price_, const int32_t server_assigned_message_sequence,
                      const uint64_t exchange_order_id, const HFSAT::ttime_t time_set_by_server) {
    if (isLatestVaildSams(server_assigned_message_sequence, t_server_assigned_client_id_)) {
      saos_to_exec_size_[_server_assigned_order_sequence_] = 0;

      dbglogger_ << "Order Confirmed: "
                 << "shortcode: " << shortcode_ << " "
                 << "SACI: " << t_server_assigned_client_id_ << " "
                 << "CAOS: " << _client_assigned_order_sequence_ << " "
                 << "SAOS: " << _server_assigned_order_sequence_ << " "
                 << "security_id: " << _security_id_ << " "
                 << "price: " << _price_ << " "
                 << "buysell: " << r_buysell_ << " "
                 << "SR: " << _size_remaining_ << " "
                 << "SE: " << _size_executed_ << " "
                 << "CPos: " << _client_position_ << " "
                 << "GPos: " << _global_position_ << " "
                 << "Intprice: " << r_int_price_ << " "
                 << "SAMS: " << server_assigned_message_sequence << "\n";

      HFSAT::BaseOrder new_order;
      new_order.buysell_ = r_buysell_;
      new_order.int_price_ = r_int_price_;
      new_order.price_ = _price_;
      new_order.server_assigned_order_sequence_ = _server_assigned_order_sequence_;
      new_order.client_assigned_order_sequence_ = _client_assigned_order_sequence_;
      new_order.size_remaining_ = _size_remaining_;
      new_order.size_executed_ = _size_executed_;
      confirmed_orders_[_server_assigned_order_sequence_] = new_order;

      dbglogger_.DumpCurrentBuffer();
    }
  }

  void OrderORSConfirmed(const int t_server_assigned_client_id_, const int _client_assigned_order_sequence_,
                         const int _server_assigned_order_sequence_, const unsigned int _security_id_,
                         const double _price_, const HFSAT::TradeType_t r_buysell_, const int _size_remaining_,
                         const int _size_executed_, const int r_int_price_,
                         const int32_t server_assigned_message_sequence, const uint64_t exchange_order_id,
                         const HFSAT::ttime_t time_set_by_server) {
    if (isLatestVaildSams(server_assigned_message_sequence, t_server_assigned_client_id_)) {
      saos_to_exec_size_[_server_assigned_order_sequence_] = 0;
      dbglogger_ << "Order ORS Confirmed: "
                 << "shortcode: " << shortcode_ << " "
                 << "SACI: " << t_server_assigned_client_id_ << " "
                 << "CAOS: " << _client_assigned_order_sequence_ << " "
                 << "SAOS: " << _server_assigned_order_sequence_ << " "
                 << "security_id: " << _security_id_ << " "
                 << "price: " << _price_ << " "
                 << "buysell: " << r_buysell_ << " "
                 << "SR: " << _size_remaining_ << " "
                 << "SE: " << _size_executed_ << " "
                 << "Intprice: " << r_int_price_ << " "
                 << "SAMS: " << server_assigned_message_sequence << "\n";
      dbglogger_.DumpCurrentBuffer();
    }
  }

  void OrderConfCxlReplaceRejected(const int t_server_assigned_client_id_, const int _client_assigned_order_sequence_,
                                   const int _server_assigned_order_sequence_, const unsigned int _security_id_,
                                   const double _price_, const HFSAT::TradeType_t r_buysell_,
                                   const int _size_remaining_, const int _client_position_, const int _global_position_,
                                   const int r_int_price_, const int32_t _rejection_reason_,
                                   const int32_t server_assigned_message_sequence, const uint64_t exchange_order_id,
                                   const HFSAT::ttime_t time_set_by_server) {
    if (isLatestVaildSams(server_assigned_message_sequence, t_server_assigned_client_id_)) {
      dbglogger_ << "OrderConfCxlReplaceRejected: "
                 << "shortcode: " << shortcode_ << " "
                 << "SACI: " << t_server_assigned_client_id_ << " "
                 << "SAOS: " << _server_assigned_order_sequence_ << " "
                 << "CAOS: " << _client_assigned_order_sequence_ << " "
                 << "security_id: " << _security_id_ << " "
                 << "price: " << _price_ << " "
                 << "buysell: " << r_buysell_ << " "
                 << "SR: " << _size_remaining_ << " "
                 << "CPos: " << _client_position_ << " "
                 << "GPos: " << _global_position_ << " "
                 << "Intprice: " << r_int_price_ << " "
                 << "SAMS: " << server_assigned_message_sequence << "\n";
      dbglogger_.DumpCurrentBuffer();
    }
  }

  void OrderConfCxlReplaced(const int t_server_assigned_client_id_, const int _client_assigned_order_sequence_,
                            const int _server_assigned_order_sequence_, const unsigned int _security_id_,
                            const double _price_, const HFSAT::TradeType_t r_buysell_, const int _size_remaining_,
                            const int _size_executed_, const int _client_position_, const int _global_position_,
                            const int r_int_price_, const int32_t server_assigned_message_sequence,
                            const uint64_t exchange_order_id, const HFSAT::ttime_t time_set_by_server) {
    if (isLatestVaildSams(server_assigned_message_sequence, t_server_assigned_client_id_)) {
      dbglogger_ << "OrderConfCxlReplaced: "
                 << "shortcode: " << shortcode_ << " "
                 << "SACI: " << t_server_assigned_client_id_ << " "
                 << "CAOS: " << _client_assigned_order_sequence_ << " "
                 << "SAOS: " << _server_assigned_order_sequence_ << " "
                 << "security_id: " << _security_id_ << " "
                 << "price: " << _price_ << " "
                 << "buysell: " << r_buysell_ << " "
                 << "SR: " << _size_remaining_ << " "
                 << "SE: " << _size_executed_ << " "
                 << "Intprice: " << r_int_price_ << " "
                 << "SAMS: " << server_assigned_message_sequence << "\n";
      dbglogger_.DumpCurrentBuffer();
    }
  }

  void OrderCxlSequenced(const int t_server_assigned_client_id_, const int _client_assigned_order_sequence_,
                         const int _server_assigned_order_sequence_, const unsigned int _security_id_,
                         const double _price_, const HFSAT::TradeType_t r_buysell_, const int _size_remaining_,
                         const int _client_position_, const int _global_position_, const int r_int_price_,
                         const int32_t server_assigned_message_sequence, const uint64_t exchange_order_id,
                         const HFSAT::ttime_t time_set_by_server) {
    if (isLatestVaildSams(server_assigned_message_sequence, t_server_assigned_client_id_)) {
      dbglogger_ << "OrderCxlSequenced: "
                 << "shortcode: " << shortcode_ << " "
                 << "SACI: " << t_server_assigned_client_id_ << " "
                 << "CAOS: " << _client_assigned_order_sequence_ << " "
                 << "SAOS: " << _server_assigned_order_sequence_ << " "
                 << "security_id: " << _security_id_ << " "
                 << "price: " << _price_ << " "
                 << "buysell: " << r_buysell_ << " "
                 << "SR: " << _size_remaining_ << " "
                 << "Intprice: " << r_int_price_ << " "
                 << "SAMS: " << server_assigned_message_sequence << "\n";
      dbglogger_.DumpCurrentBuffer();
    }
  }

  void OrderCanceled(const int t_server_assigned_client_id_, const int _client_assigned_order_sequence_,
                     const int _server_assigned_order_sequence_, const unsigned int _security_id_, const double _price_,
                     const HFSAT::TradeType_t r_buysell_, const int _size_remaining_, const int _client_position_,
                     const int _global_position_, const int r_int_price_,
                     const int32_t server_assigned_message_sequence, const uint64_t exchange_order_id,
                     const HFSAT::ttime_t time_set_by_server) {
    if (isLatestVaildSams(server_assigned_message_sequence, t_server_assigned_client_id_)) {
      dbglogger_ << "OrderCanceled: "
                 << "shortcode: " << shortcode_ << " "
                 << "SACI: " << t_server_assigned_client_id_ << " "
                 << "CAOS: " << _client_assigned_order_sequence_ << " "
                 << "SAOS: " << _server_assigned_order_sequence_ << " "
                 << "security_id: " << _security_id_ << " "
                 << "price: " << _price_ << " "
                 << "buysell: " << r_buysell_ << " "
                 << "SR: " << _size_remaining_ << " "
                 << "Intprice: " << r_int_price_ << " "
                 << "SAMS: " << server_assigned_message_sequence << "\n";
      confirmed_orders_.erase(_server_assigned_order_sequence_);
      dbglogger_.DumpCurrentBuffer();
    }
  }

  void OrderCancelRejected(const int t_server_assigned_client_id_, const int _client_assigned_order_sequence_,
                           const int _server_assigned_order_sequence_, const unsigned int _security_id_,
                           const double _price_, const HFSAT::TradeType_t t_buysell_, const int _size_remaining_,
                           const int _rejection_reason_, const int t_client_position_, const int t_global_position_,
                           const int r_int_price_, const uint64_t exchange_order_id,
                           const HFSAT::ttime_t time_set_by_server) {
    if (t_server_assigned_client_id_ == saci_) {
      dbglogger_ << "OrderCancelRejected: "
                 << "shortcode: " << shortcode_ << " "
                 << "SACI: " << t_server_assigned_client_id_ << " "
                 << "CAOS: " << _client_assigned_order_sequence_ << " "
                 << "SAOS: " << _server_assigned_order_sequence_ << " "
                 << "security_id: " << _security_id_ << " "
                 << "price: " << _price_ << " "
                 << "SR: " << _size_remaining_ << " "
                 << "Intprice: " << r_int_price_ << "\n";
      dbglogger_.DumpCurrentBuffer();
    }
  }

  void OrderExecuted(const int t_server_assigned_client_id_, const int _client_assigned_order_sequence_,
                     const int _server_assigned_order_sequence_, const unsigned int _security_id_, const double _price_,
                     const HFSAT::TradeType_t r_buysell_, const int _size_remaining_, const int _size_executed_,
                     const int _client_position_, const int _global_position_, const int r_int_price_,
                     const int32_t server_assigned_message_sequence, const uint64_t exchange_order_id,
                     const HFSAT::ttime_t time_set_by_server) {
    if (isLatestVaildSams(server_assigned_message_sequence, t_server_assigned_client_id_)) {
      int size_now_executed = _size_executed_;
      if (saos_to_exec_size_.find(_server_assigned_order_sequence_) != saos_to_exec_size_.end()) {
        size_now_executed -= saos_to_exec_size_[_server_assigned_order_sequence_];
      }
      saos_to_exec_size_[_server_assigned_order_sequence_] = _size_executed_;

      dbglogger_ << "OrderExecuted: "
                 << "shortcode: " << shortcode_ << " "
                 << "SACI: " << t_server_assigned_client_id_ << " "
                 << "CAOS: " << _client_assigned_order_sequence_ << " "
                 << "SAOS: " << _server_assigned_order_sequence_ << " "
                 << "security_id: " << _security_id_ << " "
                 << "price: " << _price_ << " "
                 << "buysell: " << r_buysell_ << " "
                 << "SR: " << _size_remaining_ << " "
                 << "SE: " << _size_executed_ << " "
                 << "Intprice: " << r_int_price_ << " "
                 << "SAMS: " << server_assigned_message_sequence << "\n";
      if (r_buysell_ == HFSAT::kTradeTypeBuy)
        position += size_now_executed;
      else
        position -= size_now_executed;

      log_buffer_->content_type_ = HFSAT::CDef::QueryTrade;

      log_buffer_->buffer_data_.query_trade_.watch_tv_sec_ = watch_.tv().tv_sec;
      log_buffer_->buffer_data_.query_trade_.watch_tv_usec_ = watch_.tv().tv_usec;
      log_buffer_->buffer_data_.query_trade_.trade_size_ = size_now_executed;
      log_buffer_->buffer_data_.query_trade_.trade_price_ = _price_;
      log_buffer_->buffer_data_.query_trade_.new_position_ = position;
      log_buffer_->buffer_data_.query_trade_.open_unrealized_pnl_ = 0;
      log_buffer_->buffer_data_.query_trade_.total_pnl_ = 0;
      log_buffer_->buffer_data_.query_trade_.bestbid_size_ = 0;
      log_buffer_->buffer_data_.query_trade_.bestbid_price_ = 0;
      log_buffer_->buffer_data_.query_trade_.bestask_price_ = 0;
      log_buffer_->buffer_data_.query_trade_.bestask_size_ = 0;
      log_buffer_->buffer_data_.query_trade_.mult_risk_ = 0;
      log_buffer_->buffer_data_.query_trade_.mult_base_pnl_ = 0;
      if (position == 0)
        log_buffer_->buffer_data_.query_trade_.open_or_flat_ = 'F';
      else
        log_buffer_->buffer_data_.query_trade_.open_or_flat_ = 'O';
      log_buffer_->buffer_data_.query_trade_.trade_type_ = GetTradeTypeChar(r_buysell_);

      client_logging_segment_initializer_->Log(log_buffer_);

      if (_size_remaining_ == 0) confirmed_orders_.erase(_server_assigned_order_sequence_);
      dbglogger_.DumpCurrentBuffer();
    }
  }

  void OrderInternallyMatched(const int t_server_assigned_client_id_, const int _client_assigned_order_sequence_,
                              const int _server_assigned_order_sequence_, const unsigned int _security_id_,
                              const double _price_, const HFSAT::TradeType_t r_buysell_, const int _size_remaining_,
                              const int _size_executed_, const int _client_position_, const int _global_position_,
                              const int r_int_price_, const int32_t server_assigned_message_sequence,
                              const uint64_t exchange_order_id, const HFSAT::ttime_t time_set_by_server) {
    if (isLatestVaildSams(server_assigned_message_sequence, t_server_assigned_client_id_)) {
      int size_now_executed = _size_executed_;
      if (saos_to_exec_size_.find(_server_assigned_order_sequence_) != saos_to_exec_size_.end()) {
        size_now_executed -= saos_to_exec_size_[_server_assigned_order_sequence_];
      }
      saos_to_exec_size_[_server_assigned_order_sequence_] = _size_executed_;

      dbglogger_ << "OrderInternallyMatched: "
                 << "shortcode: " << shortcode_ << " "
                 << "SACI: " << t_server_assigned_client_id_ << " "
                 << "CAOS: " << _client_assigned_order_sequence_ << " "
                 << "SAOS: " << _server_assigned_order_sequence_ << " "
                 << "security_id: " << _security_id_ << " "
                 << "price: " << _price_ << " "
                 << "buysell: " << r_buysell_ << " "
                 << "SR: " << _size_remaining_ << " "
                 << "SE: " << _size_executed_ << " "
                 << "Intprice: " << r_int_price_ << " "
                 << "SAMS: " << server_assigned_message_sequence << "\n";
      dbglogger_.DumpCurrentBuffer();

      if (r_buysell_ == HFSAT::kTradeTypeBuy)
        position += size_now_executed;
      else
        position -= size_now_executed;

      log_buffer_->content_type_ = HFSAT::CDef::QueryTrade;

      log_buffer_->buffer_data_.query_trade_.watch_tv_sec_ = watch_.tv().tv_sec;
      log_buffer_->buffer_data_.query_trade_.watch_tv_usec_ = watch_.tv().tv_usec;
      log_buffer_->buffer_data_.query_trade_.trade_size_ = size_now_executed;
      log_buffer_->buffer_data_.query_trade_.trade_price_ = _price_;
      log_buffer_->buffer_data_.query_trade_.new_position_ = position;
      log_buffer_->buffer_data_.query_trade_.open_unrealized_pnl_ = 0;
      log_buffer_->buffer_data_.query_trade_.total_pnl_ = 0;
      log_buffer_->buffer_data_.query_trade_.bestbid_size_ = 0;
      log_buffer_->buffer_data_.query_trade_.bestbid_price_ = 0;
      log_buffer_->buffer_data_.query_trade_.bestask_price_ = 0;
      log_buffer_->buffer_data_.query_trade_.bestask_size_ = 0;
      log_buffer_->buffer_data_.query_trade_.mult_risk_ = 0;
      log_buffer_->buffer_data_.query_trade_.mult_base_pnl_ = 0;
      if (position == 0)
        log_buffer_->buffer_data_.query_trade_.open_or_flat_ = 'F';
      else
        log_buffer_->buffer_data_.query_trade_.open_or_flat_ = 'O';
      log_buffer_->buffer_data_.query_trade_.trade_type_ = GetTradeTypeChar(r_buysell_);

      client_logging_segment_initializer_->Log(log_buffer_);

      if (_size_remaining_ == 0) confirmed_orders_.erase(_server_assigned_order_sequence_);
      dbglogger_.DumpCurrentBuffer();
    }
  }

  void OrderRejected(const int t_server_assigned_client_id_, const int _client_assigned_order_sequence_,
                     const unsigned int _security_id_, const double _price_, const HFSAT::TradeType_t r_buysell_,
                     const int _size_remaining_, const int _rejection_reason_, const int r_int_price_,
                     const uint64_t exchange_order_id, const HFSAT::ttime_t time_set_by_server) {
    if (t_server_assigned_client_id_ == saci_) {
      dbglogger_ << "OrderRejected: "
                 << "shortcode: " << shortcode_ << " "
                 << "SACI: " << t_server_assigned_client_id_ << " "
                 << "CAOS: " << _client_assigned_order_sequence_ << " "
                 << "security_id: " << _security_id_ << " "
                 << "price: " << _price_ << " "
                 << "buysell: " << r_buysell_ << " "
                 << "rejected_reason: " << _rejection_reason_ << " "
                 << "SR: " << _size_remaining_ << " "
                 << "Intprice: " << r_int_price_ << "\n";
      dbglogger_.DumpCurrentBuffer();
    }
  }

  void OrderRejectedDueToFunds(const int t_server_assigned_client_id_, const int _client_assigned_order_sequence_,
                               const unsigned int _security_id_, const double _price_,
                               const HFSAT::TradeType_t r_buysell_, const int _size_remaining_,
                               const int _rejection_reason_, const int r_int_price_, const uint64_t exchange_order_id,
                               const HFSAT::ttime_t time_set_by_server) {
    if (t_server_assigned_client_id_ == saci_) {
      dbglogger_ << "OrderRejectedDueToFunds: "
                 << "shortcode: " << shortcode_ << " "
                 << "SACI: " << t_server_assigned_client_id_ << " "
                 << "CAOS: " << _client_assigned_order_sequence_ << " "
                 << "security_id: " << _security_id_ << " "
                 << "price: " << _price_ << " "
                 << "buysell: " << r_buysell_ << " "
                 << "SR: " << _size_remaining_ << " "
                 << "Intprice: " << r_int_price_ << "\n";
      dbglogger_.DumpCurrentBuffer();
    }
  }

  void CleanUp() { combined_mds_messages_shm_processor_.CleanUp(); }

  // Nothing to be done with console
  void WakeUpifRejectedDueToFunds() {}

  void thread_main() {
    int32_t tradingdate = HFSAT::DateTime::GetCurrentIsoDateLocal();
    HFSAT::ExchangeSymbolManager::SetUniqueInstance(tradingdate);
    char const* exchange_symbol = HFSAT::ExchangeSymbolManager::GetExchSymbol(shortcode_);

    sec_name_indexer_.AddString(exchange_symbol, shortcode_);
    uint32_t sec_id = sec_name_indexer_.GetIdFromSecname(exchange_symbol);

    HFSAT::ExchSource_t this_exch_source = HFSAT::SecurityDefinitions::GetContractExchSource(shortcode_, tradingdate);
    HFSAT::SecurityMarketViewPtrVec& sid_to_smv_ptr_map = HFSAT::sid_to_security_market_view_map();
    HFSAT::ShortcodeSecurityMarketViewMap& shortcode__smv_map =
       HFSAT::ShortcodeSecurityMarketViewMap::GetUniqueInstance();
    std::cout<<"Exchange Source " << ExchSourceStringForm(this_exch_source) <<std::endl;
    HFSAT::SecurityMarketView* p_smv = new HFSAT::SecurityMarketView(dbglogger_, watch_, sec_name_indexer_, shortcode_,
                                                                     exchange_symbol, sec_id, this_exch_source, true,
                                                                     "INVALID", "INVALID", "INVALID");
    p_smv->SetL1OnlyFlag(false);
    sid_to_smv_ptr_map.push_back(p_smv);
    shortcode__smv_map.AddEntry(shortcode_, p_smv);
    p_smv->InitializeSMVForIndexedBook();
    bool use_self_book_ = false;
    HFSAT::IndexedNSEMarketViewManager2 indexed_nse_market_view_manager_(dbglogger_, watch_, sec_name_indexer_,
                                                                        sid_to_smv_ptr_map, use_self_book_);
    if (this_exch_source == HFSAT::kExchSourceHONGKONG) {
      this_exch_source = HFSAT::kExchSourceHKOMDPF;
    }
    switch (this_exch_source) {
      case HFSAT::kExchSourceNSE: {
        combined_mds_messages_shm_processor_.AddDataSourceForProcessing(
            HFSAT::MDS_MSG::NSE, (void*)((HFSAT::OrderGlobalListenerNSE*)&(indexed_nse_market_view_manager_)), &watch_);
      } break;

      default:
        fprintf(stderr, "Not implemented for exchange %d", this_exch_source);
        exit(1);
    }

    combined_mds_messages_shm_processor_.AddORSreplySourceForProcessing(HFSAT::MDS_MSG::ORS_REPLY, &watch_,
                                                                        sid_to_smv_ptr_map);

    combined_mds_messages_shm_processor_.GetORSReplyProcessor()->AddOrderNotFoundListener(sec_id, this);
    combined_mds_messages_shm_processor_.GetORSReplyProcessor()->AddOrderSequencedListener(sec_id, this);
    combined_mds_messages_shm_processor_.GetORSReplyProcessor()->AddOrderConfirmedListener(sec_id, this);
    combined_mds_messages_shm_processor_.GetORSReplyProcessor()->AddOrderConfCxlReplacedListener(sec_id, this);
    combined_mds_messages_shm_processor_.GetORSReplyProcessor()->AddOrderConfCxlReplaceRejectListener(sec_id, this);
    combined_mds_messages_shm_processor_.GetORSReplyProcessor()->AddOrderCancelSequencedListener(sec_id, this);
    combined_mds_messages_shm_processor_.GetORSReplyProcessor()->AddOrderCanceledListener(sec_id, this);
    combined_mds_messages_shm_processor_.GetORSReplyProcessor()->AddOrderExecutedListener(sec_id, this);
    combined_mds_messages_shm_processor_.GetORSReplyProcessor()->AddOrderRejectedListener(sec_id, this);
    combined_mds_messages_shm_processor_.GetORSReplyProcessor()->AddOrderRejectedDueToFundsListener(sec_id, this);
    combined_mds_messages_shm_processor_.GetORSReplyProcessor()->AddOrderInternallyMatchedListener(sec_id, this);

    p_smv->subscribe_price_type(this, HFSAT::kPriceTypeMktSizeWPrice);
    p_smv->subscribe_L2(this);

    combined_mds_messages_shm_processor_.RunLiveShmSource();
  }
};

class ORSReplyManager : public HFSAT::Thread {
 private:
  HFSAT::DebugLogger& dbglogger_;
  HFSAT::Watch& watch_;
  HFSAT::SecurityNameIndexer& sec_name_indexer_;
  HFSAT::SimpleLiveDispatcher simple_live_dispatcher_;
  BookManager* bookManager_;

 public:
  ORSReplyManager(HFSAT::DebugLogger& dbglogger, HFSAT::Watch& watch, std::string const& shortcode,
                  BookManager* _bookManager_)
      : dbglogger_(dbglogger),
        watch_(watch),
        sec_name_indexer_(HFSAT::SecurityNameIndexer::GetUniqueInstance()),
        simple_live_dispatcher_(),
        bookManager_(_bookManager_) {
    HFSAT::NetworkAccountInfoManager network_account_info_manager;

    int32_t tradingdate = HFSAT::DateTime::GetCurrentIsoDateLocal();
    HFSAT::ExchangeSymbolManager::SetUniqueInstance(tradingdate);

    char const* exchange_symbol = HFSAT::ExchangeSymbolManager::GetExchSymbol(shortcode);
    sec_name_indexer_.AddString(exchange_symbol, shortcode);
    uint32_t sec_id = sec_name_indexer_.GetIdFromSecname(exchange_symbol);

    HFSAT::ExchSource_t this_exch_source = HFSAT::SecurityDefinitions::GetContractExchSource(shortcode, tradingdate);

    HFSAT::DataInfo data_info = network_account_info_manager.GetDepDataInfo(this_exch_source, shortcode);
    std::cout << "Listening for ORS on " << data_info.bcast_ip_ << ":" << data_info.bcast_port_ << std::endl;
    HFSAT::ORSMessageLiveSource* new_ors_message_livesource = new HFSAT::ORSMessageLiveSource(
        dbglogger_, sec_name_indexer_, data_info.bcast_ip_, data_info.bcast_port_, MULTICAST);
    new_ors_message_livesource->SetExternalTimeListener(&watch_);
    simple_live_dispatcher_.AddSimpleExternalDataLiveListenerSocket(
        new_ors_message_livesource, new_ors_message_livesource->socket_file_descriptor());

    new_ors_message_livesource->AddOrderNotFoundListener(sec_id, bookManager_);
    new_ors_message_livesource->AddOrderSequencedListener(sec_id, bookManager_);
    new_ors_message_livesource->AddOrderConfirmedListener(sec_id, bookManager_);
    new_ors_message_livesource->AddOrderConfCxlReplacedListener(sec_id, bookManager_);
    new_ors_message_livesource->AddOrderConfCxlReplaceRejectListener(sec_id, bookManager_);
    new_ors_message_livesource->AddOrderSequencedListener(sec_id, bookManager_);
    new_ors_message_livesource->AddOrderCancelSequencedListener(sec_id, bookManager_);
    new_ors_message_livesource->AddOrderCanceledListener(sec_id, bookManager_);
    new_ors_message_livesource->AddOrderExecutedListener(sec_id, bookManager_);
    new_ors_message_livesource->AddOrderRejectedListener(sec_id, bookManager_);
    new_ors_message_livesource->AddOrderRejectedDueToFundsListener(sec_id, bookManager_);
    new_ors_message_livesource->AddOrderInternallyMatchedListener(sec_id, bookManager_);
  }

  void CleanUp() { simple_live_dispatcher_.CleanUp(); }

  void thread_main() { simple_live_dispatcher_.RunLive(); }
};

uint32_t mts = 0;
int yyyymmdd = 0;
time_t m_time_t;
int total_order_count = 0;
int total_order_size = 0;
HFSAT::ExchSource_t exchange_source = HFSAT::kExchSourceCME;

HFSAT::Watch* watch_;

void print_usage(const char* prog_name) {
  printf("This is the Console Trader program \n");
  printf("Usage:%s [max trading size]\n", prog_name);
}

static struct option options_[] = {{"help", no_argument, 0, 'h'}, {0, 0, 0, 0}};

char getAction() {
  char value;
  std::cout << std::endl
            << "1) Enter Order" << std::endl
            << "2) Cancel Order" << std::endl
            << "3) Modify Order" << std::endl
            << "4) Enter FOK Order" << std::endl
            << "5) Enter Immediate or Cancel Order" << std::endl
            << "6) Exit" << std::endl
            << "Action: ";
  std::cin >> value;

  return value;
}

std::string getSymbol() {
  std::string symbol;
  std::cout << std::endl
            << "Symbol: ";
  std::cin >> symbol;
  return symbol;
}

inline bool IsNumber(const std::string& str) {
  for (std::string::const_iterator iter = str.begin(); iter != str.end(); iter++) {
    if (!std::isdigit(*iter)) {
      return false;
    }
  }
  return true;
}

std::string getTradeTag() {
  std::string tags;
  std::string inp_tags;
  std::vector<std::string> tags_vector;
  std::ifstream common_tags_file(HFSAT::FILEPATH::kConsoleTraderCommonTagsFile);
  if (common_tags_file.is_open()) {
    std::string line;
    tags_vector.push_back("Invalid_Tag");  // Keep First tag as invalid for providing this option in future
    while (getline(common_tags_file, line)) {
      if (line == "") continue;
      tags_vector.push_back(line);
    }
    common_tags_file.close();
  } else {
    // TODO: File is missing
  }

  std::cout << std::endl
            << "Query Tags:\n";
  for (unsigned int i = 1; i < tags_vector.size(); i++) {
    std::cout << i << ") " << tags_vector[i] << "\n";
  }
  std::cin >> inp_tags;
  if (IsNumber(inp_tags)) {
    unsigned int tag_selected = std::stoi(inp_tags);
    if ((tag_selected > 0) && (tag_selected < tags_vector.size())) {
      tags = tags_vector[tag_selected];
    }
  } else {
    tags = inp_tags;
  }
  return tags;
}
double getPrice() {
  double px;
  std::cout << std::endl
            << "Price: ";
  std::cin >> px;
  return px;
}

int getSize() {
  int size;
  std::cout << std::endl
            << "Size: ";
  std::cin >> size;
  if (mts <= 0) {
    std::cout << std::endl
              << "Max Trading Size: ";
    std::cin >> mts;
  }
  return size;
}

HFSAT::TradeType_t getSide() {
  char value;
  std::cout << std::endl
            << "1) Buy" << std::endl
            << "2) Sell" << std::endl;
  std::cin >> value;
  switch (value) {
    case '1':
      return HFSAT::kTradeTypeBuy;

    case '2':
      return HFSAT::kTradeTypeSell;

    default:
      throw std::exception();
  }
}

int getSeq() {
  static int seqno = START_SEQUENCE;
  return seqno++;
}

int getServerSeq() {
  int value;
  std::cout << std::endl
            << "OrderSeq: " << std::endl;
  std::cin >> value;
  return value;
}

void SendOrder(BookManager& book_manager, HFSAT::BaseTrader* tr, const char* inst,
               HFSAT::FastPriceConvertor* fast_px_converter_, bool _fok_, bool ioc) {
  book_manager.PrintBestLevels();

  try {
    HFSAT::BaseOrder bord;
    bord.price_ = getPrice();
    bord.int_price_ = fast_px_converter_->GetFastIntPx(bord.price_);
    bord.price_ = fast_px_converter_->GetDoublePx(bord.int_price_);

    uint32_t max_equity_order=500000;

    if(exchange_source == HFSAT::kExchSourceNSE_FO){
      max_equity_order=10000000;
    }

    uint32_t max_size = max_equity_order/bord.price_;
    std::cout << "Max Size value:: " << max_size << std::endl; 
    
    uint32_t size_ = getSize();
    if (!mts || size_>max_size) {
      std::cerr << "Wrong max-trading-size and size...Try again." << mts << " " << size_ << std::endl;
      mts = 0;
      exit(-1);
    }
    
    if (((int)size_) <= 0 || ((int)(mts)) <= 0) {
      std::cerr << " Size Requested : " << (int)size_ << " Max Order Size : " << (int)mts << " Fails Sanity Checks \n";
      return;
    }

    bord.buysell_ = getSide();
    bord.security_name_ = inst;
    bord.size_requested_ = mts;
    bord.size_disclosed_ = mts;
    bord.is_fok_ = _fok_;
    bord.is_ioc_ = ioc;

    //max_equity_order = size_*bord.price_;
 
    while (size_ > 0) {
      if (size_ < mts) {
        bord.size_requested_ = size_;
        bord.size_disclosed_ = size_;
        size_ = 0;
      } else
        size_ -= mts;

      if (total_order_count == MAX_ORDER_LOT_LIMIT) {
      //if (total_order_count == max_size) {
        std::cerr << " Send Order Fails On : Exceeded Max Order Count, Total Active Order Lots : " << total_order_count
                  << " Total Order Size : " << total_order_size << "\n";
        break;
      }

      if ((int)total_order_size + bord.size_requested_ > (int)max_equity_order) {
        std::cerr << " Send Order Fails On : Exceeded Max Order Size Placed, Total Order Placed : " << total_order_size
                  << " Allowed : " << (MAX_CONSOLE_ORDER_LIMIT - total_order_size)
                  << " Requested : " << bord.size_requested_ << "\n";
        break;
      }

      bord.client_assigned_order_sequence_ = getSeq();
      std::cout << "Order Sent: " << bord.security_name_ << " " << bord.int_price_ << " " << bord.size_requested_
                << std::endl;

      total_order_count++;
      total_order_size += bord.size_requested_;

      struct timeval tv;
      gettimeofday(&tv, NULL);

      HFSAT::ttime_t temp_(tv);

      watch_->OnTimeReceived(temp_);

      tr->SendTrade(bord);
      usleep(100000);  // 1 sec sleep to avoid throttle reject
    }

  } catch (std::exception& e) {
    std::cout << "SendOrder fails: " << e.what();
  }
}

void CancelOrder(HFSAT::BaseTrader* tr, const char* inst) {
  HFSAT::BaseOrder bord;
  bord.server_assigned_order_sequence_ = getServerSeq();

  if (0 == bord.server_assigned_order_sequence_) {
    std::cerr << "Invalid SAOS : " << bord.server_assigned_order_sequence_ << "\n";
    exit(-1);
  }

  bord.security_name_ = inst;  // not really needed in the ORS but livetrader segfaults without it
  struct timeval tv;
  gettimeofday(&tv, NULL);

  HFSAT::ttime_t temp_(tv);

  watch_->OnTimeReceived(temp_);

  tr->Cancel(bord);
}

void ModifyOrder(BookManager& book_manager, HFSAT::BaseTrader* tr, HFSAT::FastPriceConvertor* fast_px_converter_,
                 const char* inst) {
  try {
    HFSAT::BaseOrder bord;
    bord.server_assigned_order_sequence_ = getServerSeq();
    bord.security_name_ = inst;  // not really needed in the ORS but livetrader segfaults without it
    bord.price_ = getPrice();

    uint32_t max_equity_order=500000;

    if(exchange_source == HFSAT::kExchSourceNSE_FO){
      max_equity_order=10000000;
    }


    uint32_t max_size = max_equity_order/bord.price_;
    std::cout << "Max Size value:: " << max_size << std::endl;

    int new_size_requested_ = getSize();

    max_equity_order = new_size_requested_ * bord.price_;

    if ((int)new_size_requested_ > (int)max_size) {
      std::cerr << "Wrong size...Try again." << new_size_requested_ << std::endl;
      mts = 0;
      exit(-1);
    }

    bord.size_disclosed_ = new_size_requested_;
    bord.int_price_ = fast_px_converter_->GetFastIntPx(bord.price_);
    struct timeval tv;
    gettimeofday(&tv, NULL);

    HFSAT::ttime_t temp_(tv);

    watch_->OnTimeReceived(temp_);
    tr->Modify(bord, bord.price(), bord.int_price(), new_size_requested_);

  } catch (std::exception& e) {
    std::cout << "ModifyOrder fails: " << e.what();
  }
}

void ChangeLogTradingDateIfRequired(int& log_trading_date) {
  time_t tvsec_ = time(NULL);
  if (HFSAT::TradingLocationUtils::GetTradingLocationFromHostname() == HFSAT::kTLocSYD) {
    std::cout << "SYD : Current : tvsec_ : " << tvsec_ << "\n";
    tvsec_ += (3600 * 4);  // Adding 4 hrs
    std::cout << "SYD : After adding 4hrs " << tvsec_ << "\n";
    log_trading_date = HFSAT::DateTime::Get_local_YYYYMMDD_from_time_t(tvsec_);
  } else if (HFSAT::TradingLocationUtils::GetTradingLocationFromHostname() == HFSAT::kTLocCHI) {
    std::cout << "CHI : Current : tvsec_ : " << tvsec_ << "\n";
    tvsec_ += (3600 * 2);  // Adding 2 hrs
    std::cout << "CHI : After adding 2hrs " << tvsec_ << "\n";
    log_trading_date = HFSAT::DateTime::Get_local_YYYYMMDD_from_time_t(tvsec_);
  }
}

// notifies trade tags to risk monitor client and ny11 pnl server
/*void NotifyTradeTag(HFSAT::DebugLogger& dbglogger, int saci, int query_id, std::string tags) {
  // create a query tag with dummy start and end times.
  // We do not desire console queries to determine end times of a tag in risk monitor
  // console queries are very short lived
  HFSAT::QueryTag query_tag(saci, query_id, tags, DUMMY_START_END_TIMES_FOR_CONSOLE_TRADES,
                            DUMMY_START_END_TIMES_FOR_CONSOLE_TRADES);
  if (!HFSAT::RiskNotifier::InformMappingViaTcp(query_tag)) {
    // TCP failed => risk monitor slave not running => Write to file (it can load later)
    dbglogger << "Couldn't notifying risk client of triplet : " << query_id << ", " << saci << ", " << tags
              << " Dumping to file.\n";
    dbglogger.DumpCurrentBuffer();
    HFSAT::RiskNotifier::DumpSACITagMapping(query_tag);
  } else {
    dbglogger << "Successfully notified risk monitor client of  " << query_id << ", " << saci << ", " << tags << "\n";
    dbglogger.DumpCurrentBuffer();
  }

  if (!HFSAT::RiskNotifier::InformMappingViaTcpToNy11(query_tag)) {
    std::stringstream ss;
    ss << "Couldn't notifying NY11 of triplet : " << query_id << ", " << saci << ", " << tags << std::endl;
    dbglogger << ss.str();
    dbglogger.DumpCurrentBuffer();
    HFSAT::SlackManager slack_pnl(TAG_PNL);
    slack_pnl.sendNotification(ss.str());
  } else {
    dbglogger << "Successfully notified NY11 of triplet " << query_id << ", " << saci << ", " << tags << "\n";
    dbglogger.DumpCurrentBuffer();
  }
  std::ostringstream temp_oss;

  int log_trading_date = yyyymmdd;
  ChangeLogTradingDateIfRequired(log_trading_date);
  temp_oss
      << PATH_TO_TAG_MAP_FILE << "qid_saci_tag." << m_time_t << "."
      << log_trading_date;  // We use time instead of query id here, we need unique number to avoid file contention.
                            // Name of file containing the saci tag mapping queryid wise
  std::ofstream saci_tag_map_file;
  saci_tag_map_file.open(temp_oss.str().c_str(),
                         std::ofstream::out | std::ofstream::app);  // Open the file in append mode as a
                                                                    // strategy might have multiple
                                                                    // BaseLiveTraders
  saci_tag_map_file << query_id << ", " << saci << ", " << tags << std::endl;
  saci_tag_map_file.close();
}
*/

int main(int argc, char** argv) {
  int c;
  int hflag = 0;

  int saci;

  if (argc > 1) mts = atoi(argv[1]);
  while (1) {
    int option_index = 0;
    c = getopt_long(argc, argv, "", options_, &option_index);
    if (c == -1) break;

    switch (c) {
      case 'h':
        hflag = 1;
        break;

      case '?':
        if (optopt != 'h') {
          fprintf(stderr, "Option %c requires an argument .. will exit \n", optopt);
          print_usage(argv[0]);
          exit(-1);
        }
        break;

      default:
        fprintf(stderr, "Weird option specified .. no handling yet \n");
        break;
    }
  }

  if (hflag) {
    print_usage(argv[0]);
    exit(-1);
  }

  /// get the instrument and corresponding exchange
  std::string shortcode = getSymbol();
  /// get the tag info corresponding to this trade, used by risk monitor and pnl calculations
  std::string tags = "";//getTradeTag();
  HFSAT::Lock book_ors_reply_lock;
  /// get current date
  time(&m_time_t);
  struct tm m_tm;
  localtime_r(&m_time_t, &m_tm);
  yyyymmdd = (1900 + m_tm.tm_year) * 10000 + (1 + m_tm.tm_mon) * 100 + m_tm.tm_mday;
  // Make Sure This Is Done Above All Other Classe's Initialization, Needed For ASX TICK Changes
  time_t current_time;
  time(&current_time);
  HFSAT::SecurityDefinitions::ResetASXMpiIfNeeded(yyyymmdd, current_time);
  if (std::string::npos != shortcode.find("NSE_")) {
    HFSAT::SecurityDefinitions::GetUniqueInstance(yyyymmdd).LoadNSESecurityDefinitions();

    if (std::string::npos != shortcode.find("INR_") || std::string::npos != shortcode.find("_GIND")) {
      exchange_source = HFSAT::kExchSourceNSE_CD;
    } else {
      if (std::string::npos != shortcode.find("_FUT") || HFSAT::NSESecurityDefinitions::IsOption(shortcode)) {
        exchange_source = HFSAT::kExchSourceNSE_FO;
      } else {
        exchange_source = HFSAT::kExchSourceNSE_FO;
      }
    }

  } else {
    exchange_source = HFSAT::SecurityDefinitions::GetContractExchSource(shortcode, yyyymmdd);
  }
  HFSAT::NetworkAccountInfoManager network_account_info_manager_;
  if (exchange_source == HFSAT::kExchSourceEUREX) {
    HFSAT::DataInfo m_dinfo =
        network_account_info_manager_.GetMarginControlDataInfo(HFSAT::kExchSourceEUREX, "NTAPROD4");
    HFSAT::Utils::ETIAlgoTagging::GetUniqueInstance("EUREX")
        .SetORSControlNetworkConfiguration("127.0.0.1", m_dinfo.bcast_port_);
  }
  if (exchange_source == HFSAT::kExchSourceICE) {
    HFSAT::DataInfo m_dinfo = network_account_info_manager_.GetMarginControlDataInfo(HFSAT::kExchSourceICE, "MSICE1");
    HFSAT::Utils::ETIAlgoTagging::GetUniqueInstance("ICE")
        .SetORSControlNetworkConfiguration("127.0.0.1", m_dinfo.bcast_port_);
  }
  double min_price_increment_ = HFSAT::SecurityDefinitions::GetContractMinPriceIncrement(shortcode, yyyymmdd);

  HFSAT::CpucycleProfiler::SetUniqueInstance(10);
  HFSAT::ExchangeSymbolManager::SetUniqueInstance(yyyymmdd);
  const char* exch_symbol_ = HFSAT::ExchangeSymbolManager::GetExchSymbol(shortcode);

  HFSAT::DebugLogger dbglogger_(1024);
  {
    std::ostringstream t_temp_oss_;
    t_temp_oss_ << "/spare/local/logs/alllogs/console_trader_log." << exch_symbol_ << "."
                << HFSAT::DateTime::GetCurrentIsoDateLocal() << "." << m_time_t;
    std::string logfilename_ = t_temp_oss_.str();
    std::cout << "Printing all logs into file: " << logfilename_ << std::endl;
    dbglogger_.OpenLogFile(logfilename_.c_str(), std::ofstream::out);
  }

  int tradingdate_ = HFSAT::DateTime::GetCurrentIsoDateLocal();
  HFSAT::FastPriceConvertor* fast_px_converter_ = new HFSAT::FastPriceConvertor(min_price_increment_);
  watch_ = new HFSAT::Watch(dbglogger_, tradingdate_);

  /// create trader class
  std::cout<<"EXCHNAGE SORUCE " << exchange_source<<std::endl;
  HFSAT::BaseTrader* trader = new HFSAT::BaseLiveTrader(
      exchange_source, network_account_info_manager_.GetDepTradeAccount(exchange_source, "NSE_SBIN_FUT0"),
      network_account_info_manager_.GetDepTradeHostIp(exchange_source, "NSE_SBIN_FUT0"),
      network_account_info_manager_.GetDepTradeHostPort(exchange_source, "NSE_SBIN_FUT0"), *watch_, dbglogger_);

  saci = trader->GetClientId();
  if (exchange_source == HFSAT::kExchSourceEUREX) {
    HFSAT::Utils::ETIAlgoTagging::GetUniqueInstance("EUREX")
        .TagAlgo(trader->GetClientId(), "CONSOLE", "CONSOLE", "EUREX");
  }
  if (exchange_source == HFSAT::kExchSourceICE) {
    HFSAT::Utils::ETIAlgoTagging::GetUniqueInstance("ICE").TagAlgo(trader->GetClientId(), "CONSOLE", "CONSOLE", "ICE");
  }
  std::stringstream offload_logging_filename;
  offload_logging_filename << "trades." << yyyymmdd << "." << m_time_t << '\0';
  std::string offload_logging_filedir = "/spare/local/logs/tradelogs/";
  HFSAT::Utils::ClientLoggingSegmentInitializer client_logging_segment_initializer(
      dbglogger_, m_time_t, offload_logging_filedir, offload_logging_filename.str());

  std::cout << "logging trades in: " << offload_logging_filedir << offload_logging_filename.str() << std::endl;
  
  BookManager book_manager(dbglogger_, *watch_, shortcode, saci, &client_logging_segment_initializer);
  book_manager.run();


  ORSReplyManager ors_reply_manager(dbglogger_, *watch_, shortcode, &book_manager);
  ors_reply_manager.run();
  std::cout << " Loading Console... " << std::endl;

  usleep(3000000);  // 1 sec sleep to complete all print statements in initialization of threads

  // notify the tag mapping to risk monitor client on localhost and pnl setup on ny11
  //NotifyTradeTag(dbglogger_, trader->GetClientId(), DUMMY_QID_FOR_CONSOLE_QUERY, tags);

//  HFSAT::RuntimeProfiler::GetUniqueInstance(HFSAT::ProfilerType::kTRADEINIT).DisableTradeinitProfiling();
//  HFSAT::ProfilerTimeInfo time_info{1, 0, 0, 0, 1, 0};
//  HFSAT::RuntimeProfiler::GetUniqueInstance(HFSAT::ProfilerType::kCONSOLETRADER).Start(time_info);

  while (true) {
    char action = getAction();
    if (action == '1') {
      SendOrder(book_manager, trader, exch_symbol_, fast_px_converter_, false, false);
    } else if (action == '4') {
      SendOrder(book_manager, trader, exch_symbol_, fast_px_converter_, true, false);
    } else if (action == '2') {
      book_manager.PrintConfirmedOrders();
      CancelOrder(trader, exch_symbol_);
    } else if (action == '5') {
      SendOrder(book_manager, trader, exch_symbol_, fast_px_converter_, false, true);
    } else if (action == '3') {
      book_manager.PrintConfirmedOrders();
      ModifyOrder(book_manager, trader, fast_px_converter_, exch_symbol_);
    } else {
      book_manager.CleanUp();
      ors_reply_manager.CleanUp();
      book_manager.stop();
      ors_reply_manager.stop();

      exit(0);
    }
  }
}
