// =====================================================================================
//
//       Filename:  bse_tbt_raw_md_handler.hpp
//
//    Description:
//
//        Version:  1.0
//        Created:  09/16/2015 03:44:05 PM
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

//#define BSE_RAW_MD_CHANNELS "bse-tbt-mcast_test.txt" 
#define BSE_RAW_MD_CHANNELS "bse-tbt-mcast.txt"
#define BSE_RAW_MD_FO_ONLY_CHANNELS "bse-tbt-fo-mcast.txt"
#define BSE_RAW_MD_CD_ONLY_CHANNELS "bse-tbt-cd-mcast.txt"
#define BSE_RAW_MD_EQ_ONLY_CHANNELS "bse-tbt-eq-mcast.txt"
#define MAX_BSE_DATA_BUFFER 65536

#define BSE_RECOVERY_START_CONF "bse_recovery_start_flag.txt"

#include <iostream>
#include <fstream>
#include <ctime>
#include <cstdlib>

//#include "infracore/BSEMD/bse_tbt_templates.hpp"
#include "infracore/BSEMD/bse_tbt_data_decoder.hpp"
#include "infracore/BSEMD/bse_tbt_data_processor.hpp"

//#include "infracore/NSEMD/nse_tbt_templates.hpp"
#include "dvccode/Utils/bse_daily_token_symbol_handler.hpp"
#include "dvccode/CommonTradeUtils/global_sim_data_manager.hpp"
#include "dvccode/CDef/security_definitions.hpp"
#include "dvccode/Utils/bse_refdata_loader.hpp"

#include "dvccode/TradingInfo/network_account_interface_manager.hpp"
#include "dvccode/ExternalData/simple_live_dispatcher.hpp"
#include "dvccode/Utils/multicast_receiver_socket.hpp"
#include "dvccode/CommonDataStructures/perishable_string_tokenizer.hpp"
#include "dvccode/Utils/udp_direct_muxer.hpp"
#include "dvccode/Utils/tcp_direct_client_zocket_with_logging.hpp"
#include "dvccode/CommonTradeUtils/trade_time_manager.hpp"

namespace HFSAT {
namespace BSEMD {

struct SocketMetaData {
  char segment_type;
  bool is_trade_exec_fd;
  bool is_spot_index_fd;
  bool is_oi_index_fd;

  SocketMetaData() {
    segment_type = 'I';
    is_trade_exec_fd = false;
    is_spot_index_fd = false;
    is_oi_index_fd = false;
  }
};

class BSETBTRawMDHandler : public HFSAT::SimpleExternalDataLiveListener {
 private:
  HFSAT::DebugLogger& dbglogger_;
  HFSAT::Utils::BSEDailyTokenSymbolHandler& bse_daily_token_symbol_handler_;
  HFSAT::Utils::BSERefDataLoader& bse_refdata_loader_;

  int32_t trading_date_;
  HFSAT::Utils::UDPDirectMuxer* udp_direct_muxer_ptr_;
  HFSAT::Utils::UDPDirectMultipleZocket* udp_direct_multiple_zockets_ptr_;

  HFSAT::SimpleLiveDispatcher& simple_live_dispatcher_;
  char data_buffer[MAX_BSE_DATA_BUFFER];

  // This will allow us to pick sockets from which we need to read data based on socket fd
  HFSAT::MulticastReceiverSocket* socket_fd_to_multicast_receiver_sockets_[BSE_MAX_FD_SUPPORTED];
  int32_t socket_fd_to_last_seen_seq_[BSE_MAX_FD_SUPPORTED];
  std::unordered_map<int32_t, uint32_t> mkt_seg_id_to_last_seen_seq_;

  HFSAT::BSEMD::BSETBTDataProcessor& bse_tbt_data_processor_;
  // Data Decoder
  BSETBTDataDecoder& bse_tbt_data_decoder_;

  // Trade Time Manager
  HFSAT::SecurityNameIndexer &sec_name_indexer_;
  TradeTimeManager& trade_time_manager_;

  // Segment Identifier
  SocketMetaData socket_fd_to_meta_data_[BSE_MAX_FD_SUPPORTED];

  HFSAT::FastMdConsumerMode_t daemon_mode_;

  bool is_local_data_;
  bool recovery_socket_closed_;
  int mkt_data_drops_count;
  int mkt_data_drops_occurance;
  char hostname[128];

  std::vector<int> recovery_socket_file_descriptor_;

  void CreateSocketsUDPDirect(HFSAT::FastMdConsumerMode_t daemon_mode, std::string exch,
                              std::string channels_file_name = "DEFAULT") {
    std::ostringstream host_bse_tbt_filename;
    host_bse_tbt_filename << PROD_CONFIGS_DIR << hostname << "_" << BSE_RAW_MD_CHANNELS;
    std::string bse_tbt_filename = host_bse_tbt_filename.str();

    std::cout << "CreateSocketsUDPDirect " << channels_file_name << std::endl;

    // override
    if ("DEFAULT" != channels_file_name) {
      bse_tbt_filename = channels_file_name;
    }

    // Recovery Manager Specific Handling
    if (HFSAT::kRecoveryHost == daemon_mode) {
      if ("BSE_FO" == exch) {
        host_bse_tbt_filename.str("");
        host_bse_tbt_filename.clear();
        host_bse_tbt_filename << PROD_CONFIGS_DIR << hostname << "_" << BSE_RAW_MD_FO_ONLY_CHANNELS;
        bse_tbt_filename = host_bse_tbt_filename.str();
      } else if ("BSE_EQ" == exch) {
        host_bse_tbt_filename.str("");
        host_bse_tbt_filename.clear();
        host_bse_tbt_filename << PROD_CONFIGS_DIR << hostname << "_" << BSE_RAW_MD_EQ_ONLY_CHANNELS;
        bse_tbt_filename = host_bse_tbt_filename.str();
      } else if ("BSE_CD" == exch) {
        host_bse_tbt_filename.str("");
        host_bse_tbt_filename.clear();
        host_bse_tbt_filename << PROD_CONFIGS_DIR << hostname << "_" << BSE_RAW_MD_CD_ONLY_CHANNELS;
        bse_tbt_filename = host_bse_tbt_filename.str();
      }
    }

    std::ifstream bse_tbt_channels_file;
    bse_tbt_channels_file.open(bse_tbt_filename.c_str());

    if (!bse_tbt_channels_file.is_open()) {
      DBGLOG_CLASS_FUNC_LINE_ERROR << "Failed To Load The TBT Multicast File : " << bse_tbt_filename
                                   << DBGLOG_ENDL_NOFLUSH;
      DBGLOG_DUMP;
      exit(-1);
    }

#undef MAX_LINE_SIZE
#define MAX_LINE_SIZE 1024

    char buffer[1024];

    while (bse_tbt_channels_file.good()) {
      bse_tbt_channels_file.getline(buffer, MAX_LINE_SIZE);
      std::string line_buffer = buffer;

      // Comments
      if (line_buffer.find("#") != std::string::npos) continue;

      HFSAT::PerishableStringTokenizer pst(buffer, MAX_LINE_SIZE);
      std::vector<char const*> const& tokens = pst.GetTokens();

      // We expect to read StreamId, StreamIP, StreamPort
      if (tokens.size() < 3) continue;

      char seg_type = BSE_EQ_SEGMENT_MARKING;
      bool is_trade_exec_fd = false;
      bool is_spot_idx_fd = false;
      bool is_oi_data_fd = false;

      if(tokens.size() == 4){
        if (( bse_tbt_data_processor_.initial_recovery_not_finished_ == false ) && ( atoi(tokens[3]) == 1 )) {
          recovery_socket_closed_ = true;
          continue;
        }
//	is_trade_exec_fd = ((atoi(tokens[3]) == 1 ) ? true : false);
      }
      // 239.60.* Are EQ Channels
      // 239.70.* FO
      // 239.80.* CD
      // 239.55.* TradeExecRange
      if (std::string::npos != std::string(tokens[1]).find(BSE_EQ_CHANNELS_ADDRESS_PATTERN) && (tokens.size() == 4) && ( atoi(tokens[3]) == 1 )) {
        DBGLOG_CLASS_FUNC_LINE_INFO << "CHANNEL : " << tokens[1] << " X " << tokens[2] << " ADDED WITH EQ RECOVERY MARKING "
                                    << DBGLOG_ENDL_NOFLUSH;
        seg_type = BSE_EQ_SEGMENT_MARKING;
        is_trade_exec_fd = true;

      } else if (std::string::npos != std::string(tokens[1]).find(BSE_EQ_CHANNELS_ADDRESS_PATTERN)) {
        DBGLOG_CLASS_FUNC_LINE_INFO << "CHANNEL : " << tokens[1] << " X " << tokens[2] << " ADDED WITH EQ MARKING "
                                    << DBGLOG_ENDL_NOFLUSH;
        seg_type = BSE_EQ_SEGMENT_MARKING;

      } else if (std::string::npos != std::string(tokens[1]).find(BSE_FO_CHANNELS_ADDRESS_PATTERN)) {
        DBGLOG_CLASS_FUNC_LINE_INFO << "CHANNEL : " << tokens[1] << " X " << tokens[2] << " ADDED WITH FO MARKING "
                                    << DBGLOG_ENDL_NOFLUSH;
        seg_type = BSE_FO_SEGMENT_MARKING;

      } else if (std::string::npos != std::string(tokens[1]).find(BSE_CD_CHANNELS_ADDRESS_PATTERN)) {
        DBGLOG_CLASS_FUNC_LINE_INFO << "CHANNEL : " << tokens[1] << " X " << tokens[2] << " ADDED WITH CD MARKING "
                                    << DBGLOG_ENDL_NOFLUSH;
        seg_type = BSE_CD_SEGMENT_MARKING;

      /*} else if (std::string::npos != std::string(tokens[1]).find(BSE_TER_CHANNELS_ADDRESS_PATTERN)) {
        DBGLOG_CLASS_FUNC_LINE_INFO << "CHANNEL : " << tokens[1] << " X " << tokens[2] << " ADDED WITH TER MARKING "
                                    << DBGLOG_ENDL_NOFLUSH;

        if (atoi(tokens[2]) == BSE_TER_FO_PORT) {
          seg_type = BSE_FO_SEGMENT_MARKING;
        } else if (atoi(tokens[2]) == BSE_TER_CD_PORT) {
          seg_type = BSE_CD_SEGMENT_MARKING;
        }

        is_trade_exec_fd = true;
*/
      }else if (std::string::npos != std::string(tokens[1]).find(BSE_SPOT_CHANNELS_ADDRESS_PATTERN)) {
        DBGLOG_CLASS_FUNC_LINE_INFO << "CHANNEL : " << tokens[1] << " X " << tokens[2] << " ADDED WITH SPOT INDEX "
                                    << DBGLOG_ENDL_NOFLUSH;
        seg_type = BSE_IX_SEGMENT_MARKING;
        is_spot_idx_fd = true;

      }else if (std::string::npos != std::string(tokens[1]).find(BSE_OI_CHANNELS_ADDRESS_PATTERN)) {
        DBGLOG_CLASS_FUNC_LINE_INFO << "CHANNEL : " << tokens[1] << " X " << tokens[2] << " ADDED WITH OI MARKING"
                                    << DBGLOG_ENDL_NOFLUSH;
        seg_type = BSE_IX_SEGMENT_MARKING;
        is_oi_data_fd = true;

      } 
      else {
        DBGLOG_CLASS_FUNC_LINE_FATAL << "SEEMS LIKE CHANNEL PATTERN ASSUMPTIONS AREN'T HOLDING NOW, PLEASE VALIDATE, "
                                        "UNEXPECTED CHANNEL ADDRESS : "
                                     << tokens[1] << DBGLOG_ENDL_NOFLUSH;
        DBGLOG_DUMP;
        exit(-1);
      }

      //OI support not present yet, assuming false for now and not taking any args
      if("BSE_NSE" == exch){
        (udp_direct_muxer_ptr_->GetUDPZocketForInterface(HFSAT::NetworkAccountInterfaceManager::instance().GetInterface(HFSAT::kExchSourceBSE, HFSAT::k_MktDataRaw)))->CreateSocketAndAddToMuxer(tokens[1], atoi(tokens[2]), this, seg_type,
                                                               is_trade_exec_fd,is_spot_idx_fd,is_oi_data_fd);
      }else{
        udp_direct_multiple_zockets_ptr_->CreateSocketAndAddToMuxer(tokens[1], atoi(tokens[2]), this, seg_type,is_trade_exec_fd,is_spot_idx_fd,is_oi_data_fd);
      }

      DBGLOG_CLASS_FUNC_LINE_INFO << "CREATING SOCKET : " << tokens[1] << " " << tokens[2] << " Interface : "
                                  << HFSAT::NetworkAccountInterfaceManager::instance().GetInterface(
                                         HFSAT::kExchSourceBSE, HFSAT::k_MktDataRaw)
                                  << DBGLOG_ENDL_NOFLUSH;
      DBGLOG_DUMP;
    }

    bse_tbt_channels_file.close();
    DBGLOG_DUMP;

#undef MAX_LINE_SIZE
  }

  void CreateSockets(HFSAT::FastMdConsumerMode_t daemon_mode, std::string exch) {
    std::ostringstream host_bse_tbt_filename;
    host_bse_tbt_filename << PROD_CONFIGS_DIR << hostname << "_" << BSE_RAW_MD_CHANNELS;
    std::string bse_tbt_filename = host_bse_tbt_filename.str();
    // Recovery Manager Specific Handling
    if ((HFSAT::kRecoveryHost == daemon_mode) || (HFSAT::kBardataRecoveryHost == daemon_mode) || (HFSAT::kBardataLogger == daemon_mode)) {
      if ("BSE_FO" == exch) {
        host_bse_tbt_filename.str("");
        host_bse_tbt_filename.clear();
        host_bse_tbt_filename << PROD_CONFIGS_DIR << hostname << "_" << BSE_RAW_MD_FO_ONLY_CHANNELS;
        bse_tbt_filename = host_bse_tbt_filename.str();
      } else if ("BSE_EQ" == exch) {
        host_bse_tbt_filename.str("");
        host_bse_tbt_filename.clear();
        host_bse_tbt_filename << PROD_CONFIGS_DIR << hostname << "_" << BSE_RAW_MD_EQ_ONLY_CHANNELS;
        bse_tbt_filename = host_bse_tbt_filename.str();
      } else if ("BSE_CD" == exch) {
        host_bse_tbt_filename.str("");
        host_bse_tbt_filename.clear();
        host_bse_tbt_filename << PROD_CONFIGS_DIR << hostname << "_" << BSE_RAW_MD_CD_ONLY_CHANNELS;
        bse_tbt_filename = host_bse_tbt_filename.str();
      }
    }

    std::cout << "CreateSockets: bse_tbt_filename: " << bse_tbt_filename << std::endl;
    std::ifstream bse_tbt_channels_file;
    bse_tbt_channels_file.open(bse_tbt_filename.c_str());

    if (!bse_tbt_channels_file.is_open()) {
      DBGLOG_CLASS_FUNC_LINE_ERROR << "Failed To Load The TBT Multicast File : " << bse_tbt_filename
                                   << DBGLOG_ENDL_NOFLUSH;
      DBGLOG_DUMP;
      exit(-1);
    }

#undef MAX_LINE_SIZE
#define MAX_LINE_SIZE 1024

    char buffer[1024];

    while (bse_tbt_channels_file.good()) {
      bse_tbt_channels_file.getline(buffer, MAX_LINE_SIZE);
      std::string line_buffer = buffer;

      // Comments
      if (line_buffer.find("#") != std::string::npos) continue;
      //bool is_spot_idx_fd = false;

      HFSAT::PerishableStringTokenizer pst(buffer, MAX_LINE_SIZE);
      std::vector<char const*> const& tokens = pst.GetTokens();

      // We expect to read StreamId, StreamIP, StreamPort
      if (tokens.size() < 3) continue;

      if(tokens.size() == 4){
        if (( bse_tbt_data_processor_.initial_recovery_not_finished_ == false ) && ( atoi(tokens[3]) == 1 )) {
          recovery_socket_closed_ = true;
          continue;
        }
      }

      // Create a non-blocking socket on each stream
      // Last Argument Being True Is Very Important For Address Specific Bind
      HFSAT::MulticastReceiverSocket* new_multicast_receiver_socket = new HFSAT::MulticastReceiverSocket(
          tokens[1], atoi(tokens[2]),
          HFSAT::NetworkAccountInterfaceManager::instance().GetInterface(HFSAT::kExchSourceBSE, HFSAT::k_MktDataRaw));
      DBGLOG_CLASS_FUNC_LINE_INFO << "CREATING SOCKET : " << tokens[1] << " " << tokens[2] << " Interface : "
                                  << HFSAT::NetworkAccountInterfaceManager::instance().GetInterface(
                                         HFSAT::kExchSourceBSE, HFSAT::k_MktDataRaw)
                                  << DBGLOG_ENDL_NOFLUSH;
      DBGLOG_DUMP;
      new_multicast_receiver_socket->SetNonBlocking();
      //          new_multicast_receiver_socket -> setBufferSize ( 65536 ) ;  //64k allocated to bufferd socket for
      //          queue up events

      if (new_multicast_receiver_socket->socket_file_descriptor() >= BSE_MAX_FD_SUPPORTED) {
        DBGLOG_CLASS_FUNC_LINE_FATAL
            << "SOMETHING HAS GONE WRONG IT SEEMS, DIDN'T EXPECT TO SEE A SOCKET FD WITH DESCRIPTOR : "
            << new_multicast_receiver_socket->socket_file_descriptor()
            << " MAX SUPPOERTED RANGE IS UPTO : " << BSE_MAX_FD_SUPPORTED << DBGLOG_ENDL_NOFLUSH;
        DBGLOG_DUMP;
        exit(-1);
      }

      socket_fd_to_multicast_receiver_sockets_[new_multicast_receiver_socket->socket_file_descriptor()] =
          new_multicast_receiver_socket;

      DBGLOG_CLASS_FUNC_LINE_INFO << "SOCKET FD :" << new_multicast_receiver_socket->socket_file_descriptor()
                                  << " SETUP ON : " << tokens[0] << DBGLOG_ENDL_NOFLUSH;
      DBGLOG_DUMP;

      // 237.0.* Are EQ Channels
      // 238.0.* FO
      // 236.0.* CD
      if (std::string::npos != std::string(tokens[1]).find(BSE_EQ_CHANNELS_ADDRESS_PATTERN) && (tokens.size() == 4) && ( atoi(tokens[3]) == 1 )) {
        DBGLOG_CLASS_FUNC_LINE_INFO << "CHANNEL : " << tokens[1] << " X " << tokens[2] << " ADDED WITH EQ RECOVERY MARKING FD : "
                                    << new_multicast_receiver_socket->socket_file_descriptor() << DBGLOG_ENDL_NOFLUSH;
        socket_fd_to_meta_data_[new_multicast_receiver_socket->socket_file_descriptor()].segment_type =
            BSE_EQ_SEGMENT_MARKING;
        recovery_socket_file_descriptor_.push_back(new_multicast_receiver_socket->socket_file_descriptor());

      } else if (std::string::npos != std::string(tokens[1]).find(BSE_EQ_CHANNELS_ADDRESS_PATTERN)) {
        DBGLOG_CLASS_FUNC_LINE_INFO << "CHANNEL : " << tokens[1] << " X " << tokens[2] << " ADDED WITH EQ MARKING "
                                    << DBGLOG_ENDL_NOFLUSH;
        socket_fd_to_meta_data_[new_multicast_receiver_socket->socket_file_descriptor()].segment_type =
            BSE_EQ_SEGMENT_MARKING;

      } else if (std::string::npos != std::string(tokens[1]).find(BSE_FO_CHANNELS_ADDRESS_PATTERN)) {
        DBGLOG_CLASS_FUNC_LINE_INFO << "CHANNEL : " << tokens[1] << " X " << tokens[2] << " ADDED WITH FO MARKING "
                                    << DBGLOG_ENDL_NOFLUSH;
        socket_fd_to_meta_data_[new_multicast_receiver_socket->socket_file_descriptor()].segment_type =
            BSE_FO_SEGMENT_MARKING;

      } else if (std::string::npos != std::string(tokens[1]).find(BSE_CD_CHANNELS_ADDRESS_PATTERN)) {
        DBGLOG_CLASS_FUNC_LINE_INFO << "CHANNEL : " << tokens[1] << " X " << tokens[2] << " ADDED WITH CD MARKING "
                                    << DBGLOG_ENDL_NOFLUSH;
        socket_fd_to_meta_data_[new_multicast_receiver_socket->socket_file_descriptor()].segment_type =
            BSE_CD_SEGMENT_MARKING;

      /*} else if (std::string::npos != std::string(tokens[1]).find(BSE_TER_CHANNELS_ADDRESS_PATTERN)) {
        DBGLOG_CLASS_FUNC_LINE_INFO << "CHANNEL : " << tokens[1] << " X " << tokens[2] << " ADDED WITH TER MARKING "
                                    << DBGLOG_ENDL_NOFLUSH;

        if (atoi(tokens[2]) == BSE_TER_FO_PORT) {
          socket_fd_to_meta_data_[new_multicast_receiver_socket->socket_file_descriptor()].segment_type =
              BSE_FO_SEGMENT_MARKING;
        } else if (atoi(tokens[2]) == BSE_TER_CD_PORT) {
          socket_fd_to_meta_data_[new_multicast_receiver_socket->socket_file_descriptor()].segment_type =
              BSE_CD_SEGMENT_MARKING;
        }

        socket_fd_to_meta_data_[new_multicast_receiver_socket->socket_file_descriptor()].is_trade_exec_fd = true;
*/
      }else if (std::string::npos != std::string(tokens[1]).find(BSE_SPOT_CHANNELS_ADDRESS_PATTERN)) {
        DBGLOG_CLASS_FUNC_LINE_INFO << "CHANNEL : " << tokens[1] << " X " << tokens[2] << " ADDED WITH SPOT INDEX "
                                    << DBGLOG_ENDL_NOFLUSH;
        socket_fd_to_meta_data_[new_multicast_receiver_socket->socket_file_descriptor()].segment_type = 
            BSE_IX_SEGMENT_MARKING;
        socket_fd_to_meta_data_[new_multicast_receiver_socket->socket_file_descriptor()].is_spot_index_fd = true;                            
        //seg_type = BSE_SP_SEGMENT_MARKING;
        //is_spot_idx_fd = true;

      }else if (std::string::npos != std::string(tokens[1]).find(BSE_OI_CHANNELS_ADDRESS_PATTERN)) {
        DBGLOG_CLASS_FUNC_LINE_INFO << "CHANNEL : " << tokens[1] << " X " << tokens[2] << " ADDED WITH OI INDEX "
                                    << DBGLOG_ENDL_NOFLUSH;
        socket_fd_to_meta_data_[new_multicast_receiver_socket->socket_file_descriptor()].segment_type =
            BSE_IX_SEGMENT_MARKING;
        socket_fd_to_meta_data_[new_multicast_receiver_socket->socket_file_descriptor()].is_oi_index_fd = true;
        //is_spot_idx_fd = true;
      } 
      else {
        DBGLOG_CLASS_FUNC_LINE_FATAL << "SEEMS LIKE CHANNEL PATTERN ASSUMPTIONS AREN'T HOLDING NOW, PLEASE VALIDATE, "
                                        "UNEXPECTED CHANNEL ADDRESS : "
                                     << tokens[1] << DBGLOG_ENDL_NOFLUSH;
        DBGLOG_DUMP;
        exit(-1);
      }

      simple_live_dispatcher_.AddSimpleExternalDataLiveListenerSocket(
          this, new_multicast_receiver_socket->socket_file_descriptor(), true);
    }

    bse_tbt_channels_file.close();
    DBGLOG_DUMP;

#undef MAX_LINE_SIZE
  }

  BSETBTRawMDHandler(HFSAT::DebugLogger& dbglogger, HFSAT::SimpleLiveDispatcher& simple_live_dispatcher,
                     HFSAT::CombinedControlMessageLiveSource* p_ccm_livesource, HFSAT::FastMdConsumerMode_t daemon_mode,
                     std::string exch, bool using_udp_direct = false,
                     HFSAT::BSEMD::MarketEventListener* market_event_listener = nullptr,
                     bool recover_data = false, std::string channels_file_name = "DEFAULT")
      : dbglogger_(dbglogger),
        bse_daily_token_symbol_handler_(HFSAT::Utils::BSEDailyTokenSymbolHandler::GetUniqueInstance(
            HFSAT::GlobalSimDataManager::GetUniqueInstance(dbglogger_).GetTradingDate())),
        bse_refdata_loader_(HFSAT::Utils::BSERefDataLoader::GetUniqueInstance(
            HFSAT::GlobalSimDataManager::GetUniqueInstance(dbglogger_).GetTradingDate())),

        trading_date_(HFSAT::DateTime::GetCurrentIsoDateLocal()),
	udp_direct_muxer_ptr_(nullptr),
        udp_direct_multiple_zockets_ptr_(nullptr),
        simple_live_dispatcher_(simple_live_dispatcher),
        data_buffer(),
        socket_fd_to_multicast_receiver_sockets_(),
        mkt_seg_id_to_last_seen_seq_(),
        bse_tbt_data_processor_(HFSAT::BSEMD::BSETBTDataProcessor::GetUniqueInstance(dbglogger, daemon_mode, nullptr,
                                                                                     market_event_listener)),
        bse_tbt_data_decoder_(BSETBTDataDecoder::GetUniqueInstance(dbglogger, daemon_mode, p_ccm_livesource)),
        sec_name_indexer_(HFSAT::SecurityNameIndexer::GetUniqueInstance()),
        trade_time_manager_(TradeTimeManager::GetUniqueInstance(
            sec_name_indexer_, HFSAT::GlobalSimDataManager::GetUniqueInstance(dbglogger_).GetTradingDate())),
        daemon_mode_(daemon_mode),
        recovery_socket_closed_(false) {

    for (uint32_t fd_counter = 0; fd_counter < BSE_MAX_FD_SUPPORTED; fd_counter++) {
      socket_fd_to_meta_data_[fd_counter] = SocketMetaData();
    }

    for (int32_t ctr = 0; ctr < BSE_MAX_FD_SUPPORTED; ctr++) {
      socket_fd_to_multicast_receiver_sockets_[ctr] = NULL;
      socket_fd_to_last_seen_seq_[ctr] = -1;
    }

    is_local_data_ = TradingLocationUtils::GetTradingLocationExch(kExchSourceBSE) ==
                     TradingLocationUtils::GetTradingLocationFromHostname();
    mkt_data_drops_count = mkt_data_drops_occurance = 0;
    hostname[127] = '\0';
    gethostname(hostname, 127);

    if("BSE_NSE" == exch ){
      udp_direct_muxer_ptr_ = &(HFSAT::Utils::UDPDirectMuxer::GetUniqueInstance());
      udp_direct_muxer_ptr_->InitializeUDPZocketOverInterface(HFSAT::NetworkAccountInterfaceManager::instance().GetInterface(HFSAT::kExchSourceBSE, HFSAT::k_MktDataRaw));
    }else{
      std::cout << "CREATING BSE NORMAL UDP M ZOCKET : " << exch << std::endl;
      udp_direct_multiple_zockets_ptr_ = &(HFSAT::Utils::UDPDirectMultipleZocket::GetUniqueInstance());
    }

   std::map<int32_t, BSE_UDP_MDS::BSERefData> eq_bse_ref_data;

   timeval logger_start_time_;
   gettimeofday(&logger_start_time_, NULL);
   int curr_hhmm = DateTime::GetUTCHHMMFromTime(logger_start_time_.tv_sec);

/*
   std::ostringstream bse_recovery_config_filename;
   bse_recovery_config_filename << PROD_CONFIGS_DIR << hostname << "_" << BSE_RECOVERY_START_CONF;
   std::string bse_recovery_filename = bse_recovery_config_filename.str();


   std::ifstream bse_recovery_flag_file;
   bse_recovery_flag_file.open(bse_recovery_filename.c_str());

#undef MAX_LINE_SIZE
#define MAX_LINE_SIZE 1024

   std::cout << "BSE_RECOVERY_START_CONF: " << bse_recovery_filename << std::endl;
   bool start_recovery = false;
   if (!bse_recovery_flag_file.is_open()) {
     start_recovery = true;
     std::cout << "file not found start_recovery: " << start_recovery << std::endl;
   } else {
     while (bse_recovery_flag_file.good()) {
       char buffer[1024];
       bse_recovery_flag_file.getline(buffer, MAX_LINE_SIZE);
       std::string line_buffer = buffer;

       // Comments
       if (line_buffer.find("#") != std::string::npos) continue;

       HFSAT::PerishableStringTokenizer pst(buffer, MAX_LINE_SIZE);
       std::vector<char const*> const& tokens = pst.GetTokens();

       // We expect to read StreamId, StreamIP, StreamPort
       if (tokens.size() != 1) continue;
       
       start_recovery = std::strcmp(tokens[0],"Y") ? false : true;
       std::cout << "file start_recovery: " << start_recovery << std::endl;
     }
   }
*/

   int start_time = trade_time_manager_.GetStartTimeForSegment("BSE_EQ");
   int start_hhmm = (start_time / 3600) * 100 + (start_time % 3600) / 60;
   std::cout << "Recovery Check: CurrTime: " << curr_hhmm << " StartTime: " << start_hhmm << " recover_data: " << recover_data << std::endl;
   if (curr_hhmm < start_hhmm || !recover_data ) {
     bse_tbt_data_processor_.initial_recovery_not_finished_ = false;
     std::cout << "Not Going for Recovery" << std::endl;
   }
   else {
     std::cout << "Going for Recovery" << std::endl;
   }

   for (auto& itr : bse_refdata_loader_.GetBSERefData(BSE_EQ_SEGMENT_MARKING)) { 
     eq_bse_ref_data[itr.first] = itr.second;
     
     std::ostringstream internal_symbol_str;
     internal_symbol_str << "BSE" << "_" << (itr.second).symbol;
     std::string exchange_symbol = HFSAT::BSESecurityDefinitions::ConvertDataSourceNametoExchSymbol(internal_symbol_str.str());

     
     int64_t token_ = bse_daily_token_symbol_handler_.GetTokenFromInternalSymbol(internal_symbol_str.str().c_str(), BSE_EQ_SEGMENT_MARKING);
     
     if(token_ > 0){
      // If current time is less than the start time for this sec_id, then don't go for its recovery

       if (bse_tbt_data_processor_.initial_recovery_not_finished_) {
         bse_tbt_data_processor_.security_in_recovery[token_] = true;
       } else {
         bse_tbt_data_processor_.security_in_recovery[token_] = false;
       }
       bse_tbt_data_processor_.instr_summry_rcvd_[token_] = false;
       bse_tbt_data_processor_.target_msg_seq_num_[token_] = 0;
       int32_t product_id = HFSAT::BSESecurityDefinitions::GetProductIdFromExchangeId(token_);
       if(bse_tbt_data_processor_.mkt_seg_id_to_seq_num_.find(product_id) == bse_tbt_data_processor_.mkt_seg_id_to_seq_num_.end()){
         bse_tbt_data_processor_.mkt_seg_id_to_seq_num_[product_id] = 0;
         bse_tbt_data_processor_.curr_sec_id_[product_id] = -1;
       }
     }else{
       std::cout << "INVALID TOKEN FOR PROD: " << internal_symbol_str.str() << std::endl;
     }
   }
   
    if (true == using_udp_direct) {
      CreateSocketsUDPDirect(daemon_mode, exch, channels_file_name);
    } else {
      CreateSockets(daemon_mode, exch);
    }
  }

 public:
  static BSETBTRawMDHandler& GetUniqueInstance(HFSAT::DebugLogger& dbglogger,
                                               HFSAT::SimpleLiveDispatcher& simple_live_dispatcher,
                                               HFSAT::CombinedControlMessageLiveSource* p_ccm_livesource,
                                               HFSAT::FastMdConsumerMode_t daemon_mode, std::string exch = "BSE",
                                               bool using_udp_direct = false,
                                               HFSAT::BSEMD::MarketEventListener* market_event_listener = nullptr,
                                               bool recover_data = false, std::string channels_file_name = "DEFAULT") {
    static BSETBTRawMDHandler unique_instance(dbglogger, simple_live_dispatcher, p_ccm_livesource, daemon_mode, exch,
                                              using_udp_direct, market_event_listener, recover_data, channels_file_name);
    return unique_instance;
  }

  void SetBardataPeriod(uint32_t bardata_period) {
    bse_tbt_data_processor_.SetBardataPeriod(bardata_period);
    bse_tbt_data_decoder_.SetBardataPeriod(bardata_period);
  }

/*
  bool ProcessRequest(char* in_buffer, int32_t length, char* out_buffer) {
    char segment = in_buffer[0];

    if (BSE_EQ_SEGMENT_MARKING != segment && BSE_FO_SEGMENT_MARKING != segment && BSE_CD_SEGMENT_MARKING != segment) {
      return false;
    }

    int32_t token = *((int32_t*)((char*)in_buffer + 1));

    if (false == bse_tbt_data_processor_.IsTokenAvailableForRecovery(token, segment)) {
      return false;
    }
    return bse_tbt_data_processor_.FetchProcessedLiveOrdersMap(token, out_buffer, MAX_RECOVERY_PACKET_SIZE, segment);
  }
*/

  bool ProcessBardataRequest(char* in_buffer, int32_t length, char* out_buffer) {
    char segment = in_buffer[0];

    if (BSE_EQ_SEGMENT_MARKING != segment && BSE_FO_SEGMENT_MARKING != segment && BSE_CD_SEGMENT_MARKING != segment && BSE_IX_SEGMENT_MARKING != segment) {
      return false;
    }

    in_buffer += BARDATA_RECOVERY_REQUEST_SEGMENT_LENGTH;
    std::string shortcode_((in_buffer),BARDATA_RECOVERY_REQUEST_SYMBOL_LENGTH);

    in_buffer += BARDATA_RECOVERY_REQUEST_SYMBOL_LENGTH;
    int32_t start_hhmm_ = *((int32_t*)((char*)in_buffer));

    in_buffer += BARDATA_RECOVERY_REQUEST_START_TIME_LENGTH;
    int32_t end_hhmm_ = *((int32_t*)((char*)in_buffer));

    std::cout << "ProcessBardataRequest: " << shortcode_ << " start_hhmm_: " << start_hhmm_ << " end_hhmm_: " << end_hhmm_ << std::endl;
    return bse_tbt_data_processor_.FetchProcessedBardataMap(shortcode_, start_hhmm_, end_hhmm_, out_buffer, MAX_RECOVERY_PACKET_SIZE);
  }


int total_mkt_drops(){ return mkt_data_drops_count; }
int total_mkt_drops_occurance(){ return mkt_data_drops_occurance;}

void printHexString(const char *c, int len) {
  int i;
  unsigned char buff[17];
  unsigned char *pc = (unsigned char *)c;

  if (len == 0) {
    printf("  ZERO LENGTH\n");
    return;
  }
  if (len < 0) {
    printf("  NEGATIVE LENGTH: %i\n", len);
    return;
  }

  // Process every byte in the data.
  for (i = 0; i < len; i++) {
    // Multiple of 16 means new line (with line offset).

    if ((i % 16) == 0) {
      // Just don't print ASCII for the zeroth line.
      if (i != 0) printf("  %s\n", buff);

      // Output the offset.
      printf("  %04x ", i);
    }

    // Now the hex code for the specific character.
    printf(" %02x", pc[i]);

    // And store a printable ASCII character for later.
    if ((pc[i] < 0x20) || (pc[i] > 0x7e))
      buff[i % 16] = '.';
    else
      buff[i % 16] = pc[i];
    buff[(i % 16) + 1] = '\0';
  }

  // Pad out last line if not exactly 16 characters.
  while ((i % 16) != 0) {
    printf("   ");
    i++;
  }

  // And print the final ASCII bit.
  printf("  %s\n", buff);

  printf("\n");
  fflush(stdout);
}

  inline void ProcessAllEvents(int32_t socket_fd) {
    // DBGLOG_CLASS_FUNC_LINE_FATAL << "process all events called..." << DBGLOG_ENDL_NOFLUSH;
    // DBGLOG_DUMP;

    int32_t msg_length = socket_fd_to_multicast_receiver_sockets_[socket_fd]->ReadN(MAX_BSE_DATA_BUFFER, data_buffer);
    char* msg_ptr = data_buffer;
    SocketMetaData& meta = socket_fd_to_meta_data_[socket_fd];
    char segment_marking = meta.segment_type;

    if(msg_length <= 0) return;
    if (meta.is_spot_index_fd) {
      // int32_t msg_type = 0;
      // memcpy((void*)&msg_type, (void*)msg_ptr, sizeof(int32_t));
      // int32_t swapped_msg_type = HFSAT::BSEMD::swapEndian32(msg_type);

      // std::cout << " Msg Type : " <<msg_type << " Swapped Msg Type: " << swapped_msg_type << std::endl;
      //std::cout << "START bse_tbt_data_decoder_.DecodeSpotIndexValue" << std::endl;
      bse_tbt_data_decoder_.DecodeSpotIndexValue((const unsigned char*)msg_ptr, segment_marking);
      //std::cout << "END bse_tbt_data_decoder_.DecodeSpotIndexValue" << std::endl;
      return;
    }
    else if (meta.is_oi_index_fd) {
      // int32_t msg_type = 0;
      // memcpy((void*)&msg_type, (void*)msg_ptr, sizeof(int32_t));
      // int32_t swapped_msg_type = HFSAT::BSEMD::swapEndian32(msg_type);

      // std::cout << " Msg Type : " <<msg_type << " Swapped Msg Type: " << swapped_msg_type << std::endl;
      //std::cout << "START bse_tbt_data_decoder_.DecodeOIIndexValue" << std::endl;
      bse_tbt_data_decoder_.DecodeOIIndexValue((const unsigned char*)msg_ptr, segment_marking);
      //std::cout << "END bse_tbt_data_decoder_.DecodeOIIndexValue" << std::endl;
      return;
    }
    // Decoding, keep on decoding all packets until buffer is consumed 
    
    uint16_t body_len_ = *(uint16_t*)(msg_ptr + 0);   
    uint16_t template_id_ = *(uint16_t*)(msg_ptr + 2);
    if (template_id_ != 13002) {
      //std::cout << "template_id_ :" << template_id_ << std::endl;
      return;
    }

    // uint32_t msg_seq_no_not_used = *(uint32_t*)(msg_ptr + 4); //appl_seq_num_
    uint32_t msg_seq_no = *(uint32_t*)(msg_ptr + 8); //appl_seq_num_
    int32_t mkt_seg_id_ = *(int32_t*)(msg_ptr + 12);
/*
    int16_t partition_id_ = (int16_t)(*(uint8_t*)(msg_ptr + 16));
    int16_t completion_indicator_ = (int16_t)(*(uint8_t*)(msg_ptr + 17));
    int16_t AppSeqResetIndicator_ = (int16_t)(*(uint8_t*)(msg_ptr + 18));
    uint64_t TransactTime_ = *(uint64_t*)(msg_ptr + 24);
*/
    bse_tbt_data_processor_.current_mkt_seg_id_ = mkt_seg_id_;
/*
	      << "body_len_: " << body_len_ << "\n"
	      << "template_id_: " << template_id_ << "\n"
	      << "msg_seq_no_not_used: " << msg_seq_no_not_used << "\n"
	      << " PACKET HEADER msg_seq_no: " << msg_seq_no << " " 
	      << "mkt_seg_id_: " << mkt_seg_id_ << " " 
	      << "partition_id_: " << partition_id_ << " "
	      << " = > " << HFSAT::Utils::BSEDailyTokenSymbolHandler::GetUniqueInstance(-1).GetInternalSymbolFromToken(mkt_seg_id_, BSE_EQ_SEGMENT_MARKING) << "\n" 
	      << "completion_indicator_: " << completion_indicator_ << "\n"
	      << "AppSeqResetIndicator_: " << AppSeqResetIndicator_ << "\n"
	      << "TransactTime_: " << TransactTime_ << "\n"
              << "mkt_seg_id_: " << mkt_seg_id_ << "\n" 
    	      << std::endl;
*/  
    //printHexString(msg_ptr, body_len_);


      if(mkt_seg_id_to_last_seen_seq_[socket_fd] <= 0){ // First Packet Seen From The Stream

        DBGLOG_CLASS_FUNC_LINE_INFO << "STARTED RECEIVING DATA FOR SEGMENT ID  : " << mkt_seg_id_ 
                                    << " STARTING SEQUENCE IS : " << msg_seq_no << " FD: " << socket_fd << DBGLOG_ENDL_NOFLUSH;
        //Set the sequence to what's received
        mkt_seg_id_to_last_seen_seq_[socket_fd] = msg_seq_no;

      } else if (mkt_seg_id_to_last_seen_seq_[socket_fd] >= msg_seq_no) {
        if (0 != msg_seq_no) {  // MsgSeq 0 Corrosponds To Heartbeat

          DBGLOG_CLASS_FUNC_LINE_ERROR << "DUPLICATE PACKETS FOR SEGMENT ID : " << mkt_seg_id_
                                       << " RECEVIED MSGSEQ IS : " << msg_seq_no << " EXPECTED SEQUENCE NUMBER WAS : "
                                       << (1 + mkt_seg_id_to_last_seen_seq_[socket_fd]) << " FD: " << socket_fd << DBGLOG_ENDL_NOFLUSH; 
          DBGLOG_CLASS_FUNC_LINE_ERROR << "READ SIZE FROM SOCKET : " << msg_length << DBGLOG_ENDL_NOFLUSH; 

        }
        return;  // Nothing to do

      } else if ((1 + mkt_seg_id_to_last_seen_seq_[socket_fd]) == msg_seq_no) {

        // Set the sequence to what's received
	mkt_seg_id_to_last_seen_seq_[socket_fd] = msg_seq_no;

      } else {
        struct timeval drop_time;
        gettimeofday(&drop_time,NULL);
        // Log that we have dropped packets and move on
        mkt_data_drops_occurance++;
        mkt_data_drops_count += msg_seq_no -  (1 + mkt_seg_id_to_last_seen_seq_[socket_fd]);
        
        DBGLOG_CLASS_FUNC_LINE_ERROR << "DROPPED PACKETS FOR SEGMENT ID : " << mkt_seg_id_
                                     << " RECEVIED MSGSEQ IS : " << msg_seq_no << " EXPECTED SEQUENCE NUMBER WAS : "
                                     << (1 + mkt_seg_id_to_last_seen_seq_[socket_fd])
                                     << " DROP OF : " << ( msg_seq_no -  (1 + mkt_seg_id_to_last_seen_seq_[socket_fd]) )
                                     << " FD: " << socket_fd
                                     << " JUMPING SEQ AND CONTINUING... " << drop_time.tv_sec << "." << drop_time.tv_usec <<DBGLOG_ENDL_NOFLUSH;
        // Set the sequence to what's received
        mkt_seg_id_to_last_seen_seq_[socket_fd] = msg_seq_no;
      }

    
    msg_length -=body_len_;
    msg_ptr += body_len_;
    body_len_ = 0;

    while (msg_length > 0) {
      body_len_ = *(uint16_t *)(msg_ptr + 0);     // Read unsigned int of size 2 bytes at offset 0 for body length
      template_id_ = *(uint16_t *)(msg_ptr + 2);  // Read unsigned int of size 2 bytes at offset 2 for template id

      //      HFSAT::CpucycleProfiler::GetUniqueInstance().End(6);
      //      HFSAT::CpucycleProfiler::GetUniqueInstance().Start(7);

      if ((HFSAT::kBardataLogger == daemon_mode_) || (HFSAT::kBardataRecoveryHost == daemon_mode_)) {
        bse_tbt_data_decoder_.DecodeBardataEvents(msg_ptr, msg_seq_no, segment_marking);
      }
      else {
        bool close_recovery_socket = bse_tbt_data_decoder_.DecodeEvents(msg_ptr, msg_seq_no, segment_marking);
      
        if ( recovery_socket_closed_ == false ) {
          if ( close_recovery_socket == true ) {
            for(auto& it : recovery_socket_file_descriptor_) {
              socket_fd_to_multicast_receiver_sockets_[it]->Close();
              DBGLOG_CLASS_FUNC_LINE_INFO << "Closed Recovery Socket FD: " << it << DBGLOG_ENDL_NOFLUSH;
            }
            recovery_socket_closed_ = true;
            DBGLOG_CLASS_FUNC_LINE_INFO << "RecoverySockeClosed: " << recovery_socket_closed_ << " CloseRecoverySocket: " << close_recovery_socket << DBGLOG_ENDL_NOFLUSH;
          }
        } 
      }
      
      //      HFSAT::CpucycleProfiler::GetUniqueInstance().End(7);

      // a hack to quick return
      //if (msg_length < 50) return;

      msg_length -= body_len_;
      msg_ptr += body_len_;
/*
      if (msg_length > 0) {  // want to know if BSE ever sends multiple packets
        DBGLOG_CLASS_FUNC_LINE_INFO << " MSG LENGTH : " << msg_length << DBGLOG_ENDL_NOFLUSH;
      }
*/
    }
  }

  inline void ProcessEventsFromUDPDirectRead(char const* msg_ptr, int32_t msg_length, char seg_type,
                                             bool is_trade_exec_fd, bool is_spot_idx_fd, bool is_oi_data_fd, uint32_t& udp_msg_seq_no) {

//    std::cout << " BSE Process All Events :" << msg_length << std::endl;
    // DBGLOG_CLASS_FUNC_LINE_FATAL << "ProcessEventsFromUDPDirectRead called..." << DBGLOG_ENDL_NOFLUSH;
    // DBGLOG_DUMP;
    char segment_marking = seg_type;
/*
    if (is_trade_exec_fd) {
      bse_tbt_data_decoder_.DecodeTradeExecutionRange((const unsigned char*)msg_ptr, segment_marking);
      return;
    }
*/
    if (is_spot_idx_fd) {
      bse_tbt_data_decoder_.DecodeSpotIndexValue((const unsigned char*)msg_ptr, segment_marking);
      return;
    }

    else if (is_oi_data_fd) {
      bse_tbt_data_decoder_.DecodeOIIndexValue((const unsigned char*)msg_ptr, segment_marking);
      return;
    }

    if(msg_length <= 0) return;
    uint16_t body_len_ = *(uint16_t*)(msg_ptr + 0);   
    uint16_t template_id_ = *(uint16_t*)(msg_ptr + 2);
    if (template_id_ != 13002) {
      return; 
    }

    // uint32_t msg_seq_no_not_used = *(uint32_t*)(msg_ptr + 4); //appl_seq_num_
    uint32_t msg_seq_no = *(uint32_t*)(msg_ptr + 8); //appl_seq_num_
    int32_t mkt_seg_id_ = *(int32_t*)(msg_ptr + 12);
    bse_tbt_data_processor_.current_mkt_seg_id_ = mkt_seg_id_;
    // int16_t partition_id_ = (int16_t)(*(uint8_t*)(msg_ptr + 16));
    // int16_t completion_indicator_ = (int16_t)(*(uint8_t*)(msg_ptr + 17));
    // int16_t AppSeqResetIndicator_ = (int16_t)(*(uint8_t*)(msg_ptr + 18));
    // uint64_t TransactTime_ = *(uint64_t*)(msg_ptr + 24);
/*
    std::cout << "BSERawMDHandler::ProcessEventsFromUDPDirectRead: \n"
              << "body_len_: " << body_len_ << "\n"
              << "template_id_: " << template_id_ << "\n"
              << "msg_seq_no_not_used: " << msg_seq_no_not_used << "\n"
              << "msg_seq_no: " << msg_seq_no << " " 
              << "mkt_seg_id_: " << mkt_seg_id_ 
              << " = > " << HFSAT::Utils::BSEDailyTokenSymbolHandler::GetUniqueInstance(-1).GetInternalSymbolFromToken(mkt_seg_id_, BSE_EQ_SEGMENT_MARKING) << "\n" 
              << "partition_id_: " << partition_id_ << "\n"
              << "completion_indicator_: " << completion_indicator_ << "\n"
              << "AppSeqResetIndicator_: " << AppSeqResetIndicator_ << "\n"
              << "TransactTime_: " << TransactTime_ << "\n"
              << std::endl;
  
    printHexString(msg_ptr, body_len_);
*/


      //if(mkt_seg_id_to_last_seen_seq_.find(mkt_seg_id_) == mkt_seg_id_to_last_seen_seq_.end()){ // First Packet Seen From The Stream
      if(mkt_seg_id_to_last_seen_seq_[mkt_seg_id_] <= 0){ // First Packet Seen From The Stream

        DBGLOG_CLASS_FUNC_LINE_INFO << "STARTED RECEIVING BSE DATA FROM SOCKET : " 
                                    << " STARTING SEQUENCE IS : " << msg_seq_no << DBGLOG_ENDL_NOFLUSH;
        // Set the sequence to what's received
        mkt_seg_id_to_last_seen_seq_[mkt_seg_id_] = msg_seq_no;

      } else if (mkt_seg_id_to_last_seen_seq_[mkt_seg_id_] >= msg_seq_no) {
        if (0 != msg_seq_no) {  // MsgSeq 0 Corrosponds To Heartbeat
/*
          DBGLOG_CLASS_FUNC_LINE_ERROR << "DUPLICATE PACKETS FOR SEGMENT ID : " << mkt_seg_id_
                                       << " RECEVIED MSGSEQ IS : " << msg_seq_no << " EXPECTED SEQUENCE NUMBER WAS : "
                                       << (1 + mkt_seg_id_to_last_seen_seq_[mkt_seg_id_]) << DBGLOG_ENDL_NOFLUSH; 
          DBGLOG_CLASS_FUNC_LINE_ERROR << "READ SIZE FROM SOCKET : " << msg_length << DBGLOG_ENDL_NOFLUSH; 
*/
        }
        return;  // Nothing to do

      } else if ((1 + mkt_seg_id_to_last_seen_seq_[mkt_seg_id_]) == msg_seq_no) {

        // Set the sequence to what's received
	mkt_seg_id_to_last_seen_seq_[mkt_seg_id_] = msg_seq_no;

      } else {
        struct timeval drop_time;
        gettimeofday(&drop_time,NULL);

        // Log that we have dropped packets and move on
/*      
        DBGLOG_CLASS_FUNC_LINE_ERROR << "DROPPED PACKETS FOR SEGMENT ID : " << mkt_seg_id_
                                     << " RECEVIED MSGSEQ IS : " << msg_seq_no << " EXPECTED SEQUENCE NUMBER WAS : "
                                     << (1 + mkt_seg_id_to_last_seen_seq_[mkt_seg_id_])
                                     << " DROP OF : " << ( msg_seq_no -  (1 + mkt_seg_id_to_last_seen_seq_[mkt_seg_id_]) )
                                     << " JUMPING SEQ AND CONTINUING... " << drop_time.tv_sec << "." << drop_time.tv_usec <<DBGLOG_ENDL_NOFLUSH;
*/
        //        DBGLOG_DUMP;
        // Set the sequence to what's received
        mkt_seg_id_to_last_seen_seq_[mkt_seg_id_] = msg_seq_no;
      }

    msg_length -=body_len_;
    msg_ptr += body_len_;

    body_len_ = 0;
    while (msg_length > 0) {
      body_len_ = *(uint16_t *)(msg_ptr + 0);     // Read unsigned int of size 2 bytes at offset 0 for body length
      template_id_ = *(uint16_t *)(msg_ptr + 2);  // Read unsigned int of size 2 bytes at offset 2 for template id

      //std::cout << "While: body_len, template_id :: " << body_len_ << " , " << template_id_ << std::endl;
      //      HFSAT::CpucycleProfiler::GetUniqueInstance().End(6);
      //      HFSAT::CpucycleProfiler::GetUniqueInstance().Start(7);
//      int32_t processed_length = 
      bool close_recovery_socket = bse_tbt_data_decoder_.DecodeEvents(msg_ptr, msg_seq_no, segment_marking);

      if ( recovery_socket_closed_ == false ) {
        if ( close_recovery_socket == true ) {
          (udp_direct_muxer_ptr_->GetUDPZocketForInterface(HFSAT::NetworkAccountInterfaceManager::instance().GetInterface(HFSAT::kExchSourceBSE, HFSAT::k_MktDataRaw)))->CloseRecoverySocket();
          recovery_socket_closed_ = true;
          DBGLOG_CLASS_FUNC_LINE_INFO << "RecoverySockeClosed: " << recovery_socket_closed_ << " CloseRecoverySocket: " << close_recovery_socket << DBGLOG_ENDL_NOFLUSH;
        }
      }

      //      HFSAT::CpucycleProfiler::GetUniqueInstance().End(7);

      // a hack to quick return
      //if (msg_length < 50) return;

      msg_length -= body_len_;
      msg_ptr += body_len_;
/*
      if (msg_length > 0) {  // want to know if BSE ever sends multiple packets
        DBGLOG_CLASS_FUNC_LINE_INFO << " MSG LENGTH : " << msg_length << DBGLOG_ENDL_NOFLUSH;
      }
*/
    }
  }
};
}
}
