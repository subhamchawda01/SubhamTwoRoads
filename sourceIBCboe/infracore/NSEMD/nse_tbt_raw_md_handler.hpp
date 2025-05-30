// =====================================================================================
//
//       Filename:  nse_tbt_raw_md_handler.hpp
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

#define NSE_RAW_MD_CHANNELS "nse-tbt-mcast.txt"
#define NSE_RAW_MD_BARDATA_CHANNELS "nse-tbt-fo-bardata-mcast.txt"
#define NSE_RAW_MD_FO_ONLY_CHANNELS "nse-tbt-fo-mcast.txt"
#define NSE_RAW_MD_CD_ONLY_CHANNELS "nse-tbt-cd-mcast.txt"
#define NSE_RAW_MD_EQ_ONLY_CHANNELS "nse-tbt-eq-mcast.txt"
#define MAX_NSE_DATA_BUFFER 65536
#define MS_BCAST_INDICES_TRANS_CODE 7207
#define BCAST_TICKER_AND_MKT_INDEX 7202

#include <iostream>
#include <fstream>
#include <ctime>
#include <cstdlib>

#include "infracore/NSEMD/nse_tbt_templates.hpp"
#include "infracore/NSEMD/nse_tbt_data_decoder.hpp"
#include "infracore/NSEMD/nse_tbt_data_processor.hpp"
#include "dvccode/Utils/rdtsc_timer.hpp"

#include "dvccode/ExternalData/simple_live_dispatcher.hpp"
#include "dvccode/Utils/multicast_receiver_socket.hpp"
#include "dvccode/CommonDataStructures/perishable_string_tokenizer.hpp"
#include "dvccode/Utils/udp_direct_muxer.hpp"
#include "dvccode/Utils/tcp_direct_client_zocket_with_logging.hpp"
#include "dvccode/Utils/exanic_rx_buffer.hpp"
#include "dvccode/CDef/online_debug_logger.hpp"

namespace HFSAT {
namespace NSEMD {

struct SocketMetaData {
  char segment_type;
  bool is_trade_exec_fd, is_spot_index_fd;

  SocketMetaData() {
    segment_type = 'I';
    is_trade_exec_fd = false;
    is_spot_index_fd = false;
  }
};

class NSETBTRawMDHandler : public HFSAT::SimpleExternalDataLiveListener {
 private:
  HFSAT::DebugLogger& dbglogger_;
  int32_t trading_date_;
  HFSAT::Utils::UDPDirectMuxer* udp_direct_muxer_ptr_;
  HFSAT::Utils::UDPDirectMultipleZocket* udp_direct_multiple_zockets_ptr_;
  HFSAT::Utils::ExanicMultipleRx* exanic_multiple_zockets_ptr_;
  HFSAT::SimpleLiveDispatcher& simple_live_dispatcher_;
  char data_buffer[MAX_NSE_DATA_BUFFER];
  HFSAT::NSEMD::NSETBTDataProcessor& nse_tbt_data_processor_;
  bool using_exanic_direct;

  // This will allow us to pick sockets from which we need to read data based on socket fd
  HFSAT::MulticastReceiverSocket* socket_fd_to_multicast_receiver_sockets_[NSE_MAX_FD_SUPPORTED];
  int64_t socket_fd_to_last_seen_seq_[NSE_MAX_FD_SUPPORTED];

  // Data Decoder
  NSETBTDataDecoder& nse_tbt_data_decoder_;

  // Segment Identifier
  SocketMetaData socket_fd_to_meta_data_[NSE_MAX_FD_SUPPORTED];

  HFSAT::FastMdConsumerMode_t daemon_mode_;

  bool is_local_data_;
  char hostname[128];

  void CreateSocketsUDPDirect(HFSAT::FastMdConsumerMode_t daemon_mode, std::string exch,
                              std::string channels_file_name = "DEFAULT") {
    std::ostringstream host_nse_tbt_filename;
    host_nse_tbt_filename << PROD_CONFIGS_DIR << hostname << "_" << NSE_RAW_MD_CHANNELS;
    std::string nse_tbt_filename = host_nse_tbt_filename.str();

    // override
    if ("DEFAULT" != channels_file_name) {
      nse_tbt_filename = channels_file_name;
    }

    // Recovery Manager Specific Handling
    if (HFSAT::kRecoveryHost == daemon_mode) {
      if ("NSE_FO" == exch) {
        host_nse_tbt_filename.str("");
        host_nse_tbt_filename.clear();
        host_nse_tbt_filename << PROD_CONFIGS_DIR << hostname << "_" << NSE_RAW_MD_FO_ONLY_CHANNELS;
        nse_tbt_filename = host_nse_tbt_filename.str();
      } else if ("NSE_EQ" == exch) {
        host_nse_tbt_filename.str("");
        host_nse_tbt_filename.clear();
        host_nse_tbt_filename << PROD_CONFIGS_DIR << hostname << "_" << NSE_RAW_MD_EQ_ONLY_CHANNELS;
        nse_tbt_filename = host_nse_tbt_filename.str();
      } else if ("NSE_CD" == exch) {
        host_nse_tbt_filename.str("");
        host_nse_tbt_filename.clear();
        host_nse_tbt_filename << PROD_CONFIGS_DIR << hostname << "_" << NSE_RAW_MD_CD_ONLY_CHANNELS;
        nse_tbt_filename = host_nse_tbt_filename.str();
      }
    }
    if ((HFSAT::kBardataLogger == daemon_mode_) || (HFSAT::kBardataRecoveryHost == daemon_mode_)){
      host_nse_tbt_filename.str("");
      host_nse_tbt_filename.clear();
      host_nse_tbt_filename << PROD_CONFIGS_DIR << hostname << "_" << NSE_RAW_MD_BARDATA_CHANNELS;
      nse_tbt_filename = host_nse_tbt_filename.str();
    }

    std::ifstream nse_tbt_channels_file;
    nse_tbt_channels_file.open(nse_tbt_filename.c_str());

    if (!nse_tbt_channels_file.is_open()) {
      DBGLOG_CLASS_FUNC_LINE_ERROR << "Failed To Load The TBT Multicast File : " << nse_tbt_filename
                                   << DBGLOG_ENDL_NOFLUSH;
      DBGLOG_DUMP;
      exit(-1);
    }

    DBGLOG_CLASS_FUNC_LINE_INFO << "CHANNEL FILE : " << nse_tbt_filename << DBGLOG_ENDL_NOFLUSH;

#undef MAX_LINE_SIZE
#define MAX_LINE_SIZE 1024

    char buffer[1024];

    while (nse_tbt_channels_file.good()) {
      nse_tbt_channels_file.getline(buffer, MAX_LINE_SIZE);
      std::string line_buffer = buffer;

      // Comments
      if (line_buffer.find("#") != std::string::npos) continue;

      HFSAT::PerishableStringTokenizer pst(buffer, MAX_LINE_SIZE);
      std::vector<char const*> const& tokens = pst.GetTokens();

      // We expect to read StreamId, StreamIP, StreamPort
      if (tokens.size() != 3) continue;

      char seg_type = NSE_EQ_SEGMENT_MARKING;
      bool is_trade_exec_fd = false;
      bool is_oi_data_fd = false;
      bool is_spot_idx_fd = false;

      // 239.60.* Are EQ Channels
      // 239.70.* FO
      // 239.80.* CD
      // 239.55.* TradeExecRange
      if (std::string::npos != std::string(tokens[1]).find(NSE_EQ_CHANNELS_ADDRESS_PATTERN)) {
        DBGLOG_CLASS_FUNC_LINE_INFO << "CHANNEL : " << tokens[1] << " X " << tokens[2] << " ADDED WITH EQ MARKING "
                                    << DBGLOG_ENDL_NOFLUSH;
        seg_type = NSE_EQ_SEGMENT_MARKING;

      } else if (std::string::npos != std::string(tokens[1]).find(NSE_FO_CHANNELS_ADDRESS_PATTERN)) {
        DBGLOG_CLASS_FUNC_LINE_INFO << "CHANNEL : " << tokens[1] << " X " << tokens[2] << " ADDED WITH FO MARKING "
                                    << DBGLOG_ENDL_NOFLUSH;
        seg_type = NSE_FO_SEGMENT_MARKING;

      } else if (std::string::npos != std::string(tokens[1]).find(NSE_CD_CHANNELS_ADDRESS_PATTERN)) {
        DBGLOG_CLASS_FUNC_LINE_INFO << "CHANNEL : " << tokens[1] << " X " << tokens[2] << " ADDED WITH CD MARKING "
                                    << DBGLOG_ENDL_NOFLUSH;
        seg_type = NSE_CD_SEGMENT_MARKING;
      } else if (std::string::npos != std::string(tokens[1]).find(NSE_SPOT_CHANNELS_ADDRESS_PATTERN)) {
        DBGLOG_CLASS_FUNC_LINE_INFO << "CHANNEL : " << tokens[1] << " X " << tokens[2] << " ADDED WITH SPOT INDEX "
                                    << DBGLOG_ENDL_NOFLUSH;
        seg_type = NSE_IX_SEGMENT_MARKING;
        is_spot_idx_fd = true;

      } else if (std::string::npos != std::string(tokens[1]).find(NSE_TER_CHANNELS_ADDRESS_PATTERN)) {
        DBGLOG_CLASS_FUNC_LINE_INFO << "CHANNEL : " << tokens[1] << " X " << tokens[2] << " ADDED WITH TER MARKING "
                                    << DBGLOG_ENDL_NOFLUSH;

        if (atoi(tokens[2]) == NSE_TER_FO_PORT) {
          seg_type = NSE_FO_SEGMENT_MARKING;
        } else if (atoi(tokens[2]) == NSE_TER_CD_PORT) {
          seg_type = NSE_CD_SEGMENT_MARKING;
        }

        is_trade_exec_fd = true;

      } else if (std::string::npos !=std::string(tokens[1]).find(NSE_OI_CHANNELS_ADDRESS_PATTERN)){
	  DBGLOG_CLASS_FUNC_LINE_INFO << "CHANNEL : " << tokens[1] << " X " << tokens[2] << " ADDED WITH OI MARKING "
                                    << DBGLOG_ENDL_NOFLUSH;
          seg_type = NSE_FO_SEGMENT_MARKING;
          is_oi_data_fd = true;
      } else {
        DBGLOG_CLASS_FUNC_LINE_FATAL << "SEEMS LIKE CHANNEL PATTERN ASSUMPTIONS AREN'T HOLDING NOW, PLEASE VALIDATE, "
                                        "UNEXPECTED CHANNEL ADDRESS : "
                                     << tokens[1] << DBGLOG_ENDL_NOFLUSH;
        DBGLOG_DUMP;
        exit(-1);
      }


      if("BSE_NSE" == exch){
        (udp_direct_muxer_ptr_->GetUDPZocketForInterface(HFSAT::NetworkAccountInterfaceManager::instance().GetInterface(HFSAT::kExchSourceNSE, HFSAT::k_MktDataRaw)))->CreateSocketAndAddToMuxer(tokens[1], atoi(tokens[2]), this, seg_type,
                                                             is_trade_exec_fd, is_spot_idx_fd, is_oi_data_fd);
      }else{

        udp_direct_multiple_zockets_ptr_->CreateSocketAndAddToMuxer(tokens[1], atoi(tokens[2]), this, seg_type,
                                                             is_trade_exec_fd, is_spot_idx_fd, is_oi_data_fd);
      }

      DBGLOG_CLASS_FUNC_LINE_INFO << "CREATING SOCKET : " << tokens[1] << " " << tokens[2] << " Interface : "
                                  << HFSAT::NetworkAccountInterfaceManager::instance().GetInterface(
                                         HFSAT::kExchSourceNSE, HFSAT::k_MktDataRaw)
                                  << DBGLOG_ENDL_NOFLUSH;
      DBGLOG_DUMP;
    }

    nse_tbt_channels_file.close();
    DBGLOG_DUMP;

#undef MAX_LINE_SIZE
  }

  void CreateSocketsExanic(HFSAT::FastMdConsumerMode_t daemon_mode, std::string exch,
                           std::string channels_file_name = "DEFAULT") {
    std::ostringstream host_nse_tbt_filename;
    host_nse_tbt_filename << PROD_CONFIGS_DIR << hostname << "_" << NSE_RAW_MD_CHANNELS;
    std::string nse_tbt_filename = host_nse_tbt_filename.str();

    // override
    if ("DEFAULT" != channels_file_name) {
      nse_tbt_filename = channels_file_name;
    }

    // Recovery Manager Specific Handling
    if (HFSAT::kRecoveryHost == daemon_mode) {
      if ("NSE_FO" == exch) {
        host_nse_tbt_filename.str("");
        host_nse_tbt_filename.clear();
        host_nse_tbt_filename << PROD_CONFIGS_DIR << hostname << "_" << NSE_RAW_MD_FO_ONLY_CHANNELS;
        nse_tbt_filename = host_nse_tbt_filename.str();
      } else if ("NSE_EQ" == exch) {
        host_nse_tbt_filename.str("");
        host_nse_tbt_filename.clear();
        host_nse_tbt_filename << PROD_CONFIGS_DIR << hostname << "_" << NSE_RAW_MD_EQ_ONLY_CHANNELS;
        nse_tbt_filename = host_nse_tbt_filename.str();
      } else if ("NSE_CD" == exch) {
        host_nse_tbt_filename.str("");
        host_nse_tbt_filename.clear();
        host_nse_tbt_filename << PROD_CONFIGS_DIR << hostname << "_" << NSE_RAW_MD_CD_ONLY_CHANNELS;
        nse_tbt_filename = host_nse_tbt_filename.str();
      }
    }
    if ((HFSAT::kBardataLogger == daemon_mode_) || (HFSAT::kBardataRecoveryHost == daemon_mode_)){
      host_nse_tbt_filename.str("");
      host_nse_tbt_filename.clear();
      host_nse_tbt_filename << PROD_CONFIGS_DIR << hostname << "_" << NSE_RAW_MD_BARDATA_CHANNELS;
      nse_tbt_filename = host_nse_tbt_filename.str();
    }

    std::ifstream nse_tbt_channels_file;
    nse_tbt_channels_file.open(nse_tbt_filename.c_str());

    if (!nse_tbt_channels_file.is_open()) {
      DBGLOG_CLASS_FUNC_LINE_ERROR << "Failed To Load The TBT Multicast File : " << nse_tbt_filename
                                   << DBGLOG_ENDL_NOFLUSH;
      DBGLOG_DUMP;
      exit(-1);
    }

    DBGLOG_CLASS_FUNC_LINE_INFO << "CHANNEL FILE : " << nse_tbt_filename << DBGLOG_ENDL_NOFLUSH;

#undef MAX_LINE_SIZE
#define MAX_LINE_SIZE 1024

    char buffer[1024];

    while (nse_tbt_channels_file.good()) {
      nse_tbt_channels_file.getline(buffer, MAX_LINE_SIZE);
      std::string line_buffer = buffer;

      // Comments
      if (line_buffer.find("#") != std::string::npos) continue;

      HFSAT::PerishableStringTokenizer pst(buffer, MAX_LINE_SIZE);
      std::vector<char const*> const& tokens = pst.GetTokens();

      // We expect to read StreamId, StreamIP, StreamPort
      if (tokens.size() != 3) continue;

      char seg_type = NSE_EQ_SEGMENT_MARKING;
      bool is_trade_exec_fd = false;
      bool is_oi_data_fd = false;
      bool is_spot_idx_fd = false;

      HFSAT::MulticastReceiverSocket* new_multicast_receiver_socket = new HFSAT::MulticastReceiverSocket(
          tokens[1], atoi(tokens[2]),
          HFSAT::NetworkAccountInterfaceManager::instance().GetInterface(HFSAT::kExchSourceNSE, HFSAT::k_MktDataRawExanic));
      DBGLOG_CLASS_FUNC_LINE_INFO << "CREATING SOCKET : " << tokens[1] << " " << tokens[2] << " Interface : "
                                  << HFSAT::NetworkAccountInterfaceManager::instance().GetInterface(
                                         HFSAT::kExchSourceNSE, HFSAT::k_MktDataRawExanic)
                                  << DBGLOG_ENDL_NOFLUSH;
      DBGLOG_DUMP;
      new_multicast_receiver_socket->SetNonBlocking();
      if (new_multicast_receiver_socket->socket_file_descriptor() >= NSE_MAX_FD_SUPPORTED) {
        DBGLOG_CLASS_FUNC_LINE_FATAL
            << "SOMETHING HAS GONE WRONG IT SEEMS, DIDN'T EXPECT TO SEE A SOCKET FD WITH DESCRIPTOR : "
            << new_multicast_receiver_socket->socket_file_descriptor()
            << " MAX SUPPOERTED RANGE IS UPTO : " << NSE_MAX_FD_SUPPORTED << DBGLOG_ENDL_NOFLUSH;
        DBGLOG_DUMP;
        exit(-1);
      }
      socket_fd_to_multicast_receiver_sockets_[new_multicast_receiver_socket->socket_file_descriptor()] =
          new_multicast_receiver_socket;

      DBGLOG_CLASS_FUNC_LINE_INFO << "SOCKET FD :" << new_multicast_receiver_socket->socket_file_descriptor()
                                  << " SETUP ON : " << tokens[0] << DBGLOG_ENDL_NOFLUSH;
      DBGLOG_DUMP;

      // 239.60.* Are EQ Channels
      // 239.70.* FO
      // 239.80.* CD
      // 239.55.* TradeExecRange
      if (std::string::npos != std::string(tokens[1]).find(NSE_EQ_CHANNELS_ADDRESS_PATTERN)) {
        DBGLOG_CLASS_FUNC_LINE_INFO << "CHANNEL : " << tokens[1] << " X " << tokens[2] << " ADDED WITH EQ MARKING "
                                    << DBGLOG_ENDL_NOFLUSH;
        seg_type = NSE_EQ_SEGMENT_MARKING;

      } else if (std::string::npos != std::string(tokens[1]).find(NSE_FO_CHANNELS_ADDRESS_PATTERN)) {
        DBGLOG_CLASS_FUNC_LINE_INFO << "CHANNEL : " << tokens[1] << " X " << tokens[2] << " ADDED WITH FO MARKING "
                                    << DBGLOG_ENDL_NOFLUSH;
        seg_type = NSE_FO_SEGMENT_MARKING;

      } else if (std::string::npos != std::string(tokens[1]).find(NSE_CD_CHANNELS_ADDRESS_PATTERN)) {
        DBGLOG_CLASS_FUNC_LINE_INFO << "CHANNEL : " << tokens[1] << " X " << tokens[2] << " ADDED WITH CD MARKING "
                                    << DBGLOG_ENDL_NOFLUSH;
        seg_type = NSE_CD_SEGMENT_MARKING;

      } else if (std::string::npos != std::string(tokens[1]).find(NSE_SPOT_CHANNELS_ADDRESS_PATTERN)) {
        DBGLOG_CLASS_FUNC_LINE_INFO << "CHANNEL : " << tokens[1] << " X " << tokens[2] << " ADDED WITH SPOT INDEX "
                                    << DBGLOG_ENDL_NOFLUSH;
        seg_type = NSE_IX_SEGMENT_MARKING;
        is_spot_idx_fd = true;

      } else if (std::string::npos != std::string(tokens[1]).find(NSE_TER_CHANNELS_ADDRESS_PATTERN)) {
        DBGLOG_CLASS_FUNC_LINE_INFO << "CHANNEL : " << tokens[1] << " X " << tokens[2] << " ADDED WITH TER MARKING "
                                    << DBGLOG_ENDL_NOFLUSH;

        if (atoi(tokens[2]) == NSE_TER_FO_PORT) {
          seg_type = NSE_FO_SEGMENT_MARKING;
        } else if (atoi(tokens[2]) == NSE_TER_CD_PORT) {
          seg_type = NSE_CD_SEGMENT_MARKING;
        }

        is_trade_exec_fd = true;

      } else if (std::string::npos !=std::string(tokens[1]).find(NSE_OI_CHANNELS_ADDRESS_PATTERN)){
          DBGLOG_CLASS_FUNC_LINE_INFO << "CHANNEL : " << tokens[1] << " X " << tokens[2] << " ADDED WITH OI MARKING "
                                    << DBGLOG_ENDL_NOFLUSH;
          seg_type = NSE_FO_SEGMENT_MARKING;
          is_oi_data_fd = true;
      } else {
        DBGLOG_CLASS_FUNC_LINE_FATAL << "SEEMS LIKE CHANNEL PATTERN ASSUMPTIONS AREN'T HOLDING NOW, PLEASE VALIDATE, "
                                        "UNEXPECTED CHANNEL ADDRESS : "
                                     << tokens[1] << DBGLOG_ENDL_NOFLUSH;
        DBGLOG_DUMP;
        exit(-1);
      }
      exanic_multiple_zockets_ptr_->CreateSocketAndAddToMuxer(tokens[1], atoi(tokens[2]), this, seg_type, is_trade_exec_fd,is_spot_idx_fd, is_oi_data_fd);

      DBGLOG_CLASS_FUNC_LINE_INFO << "CREATING SOCKET : " << tokens[1] << " " << tokens[2] << " Interface : "
                                  << HFSAT::NetworkAccountInterfaceManager::instance().GetInterface(
                                         HFSAT::kExchSourceNSE, HFSAT::k_MktDataRawExanic)
                                  << DBGLOG_ENDL_NOFLUSH;
      DBGLOG_DUMP;
    }

    nse_tbt_channels_file.close();
    DBGLOG_DUMP;

#undef MAX_LINE_SIZE
  }

  void CreateSockets(HFSAT::FastMdConsumerMode_t daemon_mode, std::string exch) {
    std::ostringstream host_nse_tbt_filename;
    host_nse_tbt_filename << PROD_CONFIGS_DIR << hostname << "_" << NSE_RAW_MD_CHANNELS;
    std::string nse_tbt_filename = host_nse_tbt_filename.str();

    // Recovery Manager Specific Handling
    if ((HFSAT::kRecoveryHost == daemon_mode)) {
      if ("NSE_FO" == exch) {
        host_nse_tbt_filename.str("");
        host_nse_tbt_filename.clear();
        host_nse_tbt_filename << PROD_CONFIGS_DIR << hostname << "_" << NSE_RAW_MD_FO_ONLY_CHANNELS;
        nse_tbt_filename = host_nse_tbt_filename.str();
      } else if ("NSE_EQ" == exch) {
        host_nse_tbt_filename.str("");
        host_nse_tbt_filename.clear();
        host_nse_tbt_filename << PROD_CONFIGS_DIR << hostname << "_" << NSE_RAW_MD_EQ_ONLY_CHANNELS;
        nse_tbt_filename = host_nse_tbt_filename.str();
      } else if ("NSE_CD" == exch) {
        host_nse_tbt_filename.str("");
        host_nse_tbt_filename.clear();
        host_nse_tbt_filename << PROD_CONFIGS_DIR << hostname << "_" << NSE_RAW_MD_CD_ONLY_CHANNELS;
        nse_tbt_filename = host_nse_tbt_filename.str();
      }
    }

    if ((HFSAT::kBardataLogger == daemon_mode_) || (HFSAT::kBardataRecoveryHost == daemon_mode_)){
      host_nse_tbt_filename.str("");
      host_nse_tbt_filename.clear();
      host_nse_tbt_filename << PROD_CONFIGS_DIR << hostname << "_" << NSE_RAW_MD_BARDATA_CHANNELS;
      nse_tbt_filename = host_nse_tbt_filename.str();
    }
    
    std::ifstream nse_tbt_channels_file;
    nse_tbt_channels_file.open(nse_tbt_filename.c_str());

    if (!nse_tbt_channels_file.is_open()) {
      DBGLOG_CLASS_FUNC_LINE_ERROR << "Failed To Load The TBT Multicast File : " << nse_tbt_filename
                                   << DBGLOG_ENDL_NOFLUSH;
      DBGLOG_DUMP;
      exit(-1);
    }

    DBGLOG_CLASS_FUNC_LINE_INFO << "CHANNEL FILE : " << nse_tbt_filename << DBGLOG_ENDL_NOFLUSH;

#undef MAX_LINE_SIZE
#define MAX_LINE_SIZE 1024

    char buffer[1024];

    while (nse_tbt_channels_file.good()) {
      nse_tbt_channels_file.getline(buffer, MAX_LINE_SIZE);
      std::string line_buffer = buffer;

      // Comments
      if (line_buffer.find("#") != std::string::npos) continue;

      HFSAT::PerishableStringTokenizer pst(buffer, MAX_LINE_SIZE);
      std::vector<char const*> const& tokens = pst.GetTokens();

      // We expect to read StreamId, StreamIP, StreamPort
      if (tokens.size() != 3) continue;

      // Create a non-blocking socket on each stream
      // Last Argument Being True Is Very Important For Address Specific Bind
      HFSAT::MulticastReceiverSocket* new_multicast_receiver_socket = new HFSAT::MulticastReceiverSocket(
          tokens[1], atoi(tokens[2]),
          HFSAT::NetworkAccountInterfaceManager::instance().GetInterface(HFSAT::kExchSourceNSE, HFSAT::k_MktDataRaw));
      DBGLOG_CLASS_FUNC_LINE_INFO << "CREATING SOCKET : " << tokens[1] << " " << tokens[2] << " Interface : "
                                  << HFSAT::NetworkAccountInterfaceManager::instance().GetInterface(
                                         HFSAT::kExchSourceNSE, HFSAT::k_MktDataRaw)
                                  << DBGLOG_ENDL_NOFLUSH;
      DBGLOG_DUMP;
      new_multicast_receiver_socket->SetNonBlocking();
      //          new_multicast_receiver_socket -> setBufferSize ( 65536 ) ;  //64k allocated to bufferd socket for
      //          queue up events

      if (new_multicast_receiver_socket->socket_file_descriptor() >= NSE_MAX_FD_SUPPORTED) {
        DBGLOG_CLASS_FUNC_LINE_FATAL
            << "SOMETHING HAS GONE WRONG IT SEEMS, DIDN'T EXPECT TO SEE A SOCKET FD WITH DESCRIPTOR : "
            << new_multicast_receiver_socket->socket_file_descriptor()
            << " MAX SUPPOERTED RANGE IS UPTO : " << NSE_MAX_FD_SUPPORTED << DBGLOG_ENDL_NOFLUSH;
        DBGLOG_DUMP;
        exit(-1);
      }

      socket_fd_to_multicast_receiver_sockets_[new_multicast_receiver_socket->socket_file_descriptor()] =
          new_multicast_receiver_socket;

      DBGLOG_CLASS_FUNC_LINE_INFO << "SOCKET FD :" << new_multicast_receiver_socket->socket_file_descriptor()
                                  << " SETUP ON : " << tokens[0] << DBGLOG_ENDL_NOFLUSH;
      DBGLOG_DUMP;

      // 239.60.* Are EQ Channels
      // 239.70.* FO
      // 239.80.* CD
      // 239.55.* TradeExecRange
      if (std::string::npos != std::string(tokens[1]).find(NSE_EQ_CHANNELS_ADDRESS_PATTERN)) {
        DBGLOG_CLASS_FUNC_LINE_INFO << "CHANNEL : " << tokens[1] << " X " << tokens[2] << " ADDED WITH EQ MARKING "
                                    << DBGLOG_ENDL_NOFLUSH;
        socket_fd_to_meta_data_[new_multicast_receiver_socket->socket_file_descriptor()].segment_type =
            NSE_EQ_SEGMENT_MARKING;

      } else if (std::string::npos != std::string(tokens[1]).find(NSE_FO_CHANNELS_ADDRESS_PATTERN)) {
        DBGLOG_CLASS_FUNC_LINE_INFO << "CHANNEL : " << tokens[1] << " X " << tokens[2] << " ADDED WITH FO MARKING "
                                    << DBGLOG_ENDL_NOFLUSH;
        socket_fd_to_meta_data_[new_multicast_receiver_socket->socket_file_descriptor()].segment_type =
            NSE_FO_SEGMENT_MARKING;

      } else if (std::string::npos != std::string(tokens[1]).find(NSE_CD_CHANNELS_ADDRESS_PATTERN)) {
        DBGLOG_CLASS_FUNC_LINE_INFO << "CHANNEL : " << tokens[1] << " X " << tokens[2] << " ADDED WITH CD MARKING "
                                    << DBGLOG_ENDL_NOFLUSH;
        socket_fd_to_meta_data_[new_multicast_receiver_socket->socket_file_descriptor()].segment_type =
            NSE_CD_SEGMENT_MARKING;

      } else if (std::string::npos != std::string(tokens[1]).find(NSE_SPOT_CHANNELS_ADDRESS_PATTERN)) {
        DBGLOG_CLASS_FUNC_LINE_INFO << "CHANNEL : " << tokens[1] << " X " << tokens[2] << " ADDED WITH SPOT INDEX "
                                    << DBGLOG_ENDL_NOFLUSH;
        socket_fd_to_meta_data_[new_multicast_receiver_socket->socket_file_descriptor()].segment_type =
            NSE_IX_SEGMENT_MARKING;
        socket_fd_to_meta_data_[new_multicast_receiver_socket->socket_file_descriptor()].is_spot_index_fd = true;

      } else if (std::string::npos != std::string(tokens[1]).find(NSE_TER_CHANNELS_ADDRESS_PATTERN)) {
        DBGLOG_CLASS_FUNC_LINE_INFO << "CHANNEL : " << tokens[1] << " X " << tokens[2] << " ADDED WITH TER MARKING "
                                    << DBGLOG_ENDL_NOFLUSH;

        if (atoi(tokens[2]) == NSE_TER_FO_PORT) {
          socket_fd_to_meta_data_[new_multicast_receiver_socket->socket_file_descriptor()].segment_type =
              NSE_FO_SEGMENT_MARKING;
        } else if (atoi(tokens[2]) == NSE_TER_CD_PORT) {
          socket_fd_to_meta_data_[new_multicast_receiver_socket->socket_file_descriptor()].segment_type =
              NSE_CD_SEGMENT_MARKING;
        }

        socket_fd_to_meta_data_[new_multicast_receiver_socket->socket_file_descriptor()].is_trade_exec_fd = true;

      } else {
        DBGLOG_CLASS_FUNC_LINE_FATAL << "SEEMS LIKE CHANNEL PATTERN ASSUMPTIONS AREN'T HOLDING NOW, PLEASE VALIDATE, "
                                        "UNEXPECTED CHANNEL ADDRESS : "
                                     << tokens[1] << DBGLOG_ENDL_NOFLUSH;
        DBGLOG_DUMP;
        exit(-1);
      }

      simple_live_dispatcher_.AddSimpleExternalDataLiveListenerSocket(
          this, new_multicast_receiver_socket->socket_file_descriptor(), true);
    }

    nse_tbt_channels_file.close();
    DBGLOG_DUMP;

#undef MAX_LINE_SIZE
  }

  NSETBTRawMDHandler(HFSAT::DebugLogger& dbglogger, HFSAT::SimpleLiveDispatcher& simple_live_dispatcher,
                     HFSAT::CombinedControlMessageLiveSource* p_ccm_livesource, HFSAT::FastMdConsumerMode_t daemon_mode,
                     std::string exch, bool using_udp_direct = false,
                     HFSAT::NSEMD::MarketEventListener* market_event_listener = nullptr,
                     std::string channels_file_name = "DEFAULT", bool using_exanic_direct_ = false)
      : dbglogger_(dbglogger),
        trading_date_(HFSAT::DateTime::GetCurrentIsoDateLocal()),
        udp_direct_muxer_ptr_(nullptr),
        udp_direct_multiple_zockets_ptr_(nullptr),
        exanic_multiple_zockets_ptr_(nullptr),
        simple_live_dispatcher_(simple_live_dispatcher),
        data_buffer(),
        nse_tbt_data_processor_(HFSAT::NSEMD::NSETBTDataProcessor::GetUniqueInstance(dbglogger, daemon_mode, nullptr,
                                                                                     market_event_listener,(exch == "BSE_NSE"))),
        socket_fd_to_multicast_receiver_sockets_(),
        nse_tbt_data_decoder_(NSETBTDataDecoder::GetUniqueInstance(dbglogger, daemon_mode, p_ccm_livesource)),
        daemon_mode_(daemon_mode) {
    for (uint32_t fd_counter = 0; fd_counter < NSE_MAX_FD_SUPPORTED; fd_counter++) {
      socket_fd_to_meta_data_[fd_counter] = SocketMetaData();
    }

    for (int32_t ctr = 0; ctr < NSE_MAX_FD_SUPPORTED; ctr++) {
      socket_fd_to_multicast_receiver_sockets_[ctr] = NULL;
      socket_fd_to_last_seen_seq_[ctr] = -1;
    }

    is_local_data_ = TradingLocationUtils::GetTradingLocationExch(kExchSourceNSE) ==
                     TradingLocationUtils::GetTradingLocationFromHostname();
    hostname[127] = '\0';
    gethostname(hostname, 127);
    
    // using_exanic_direct 1 priority, using_udp_direct 2nd priority
    using_exanic_direct = using_exanic_direct_;

    //using multi exchange mode
    if("BSE_NSE" == exch){
      udp_direct_muxer_ptr_ = &(HFSAT::Utils::UDPDirectMuxer::GetUniqueInstance());
      udp_direct_muxer_ptr_->InitializeUDPZocketOverInterface(HFSAT::NetworkAccountInterfaceManager::instance().GetInterface(HFSAT::kExchSourceNSE, HFSAT::k_MktDataRaw));
    }else{
      if(true == using_exanic_direct){
        exanic_multiple_zockets_ptr_ = &(HFSAT::Utils::ExanicMultipleRx::GetUniqueInstance());
      }else{
        std::cout << "CREATING NSE NORMAL UDP M ZOCKET : " << std::endl;
        udp_direct_multiple_zockets_ptr_ = &(HFSAT::Utils::UDPDirectMultipleZocket::GetUniqueInstance());
      }
    }

    if ( true == using_exanic_direct){
      CreateSocketsExanic(daemon_mode, "NSE", channels_file_name);
    }else {
      if (true == using_udp_direct) {
          CreateSocketsUDPDirect(daemon_mode, exch, channels_file_name);
      } else {
          CreateSockets(daemon_mode, exch);
      }
    }
  }

 public:
  static NSETBTRawMDHandler& GetUniqueInstance(HFSAT::DebugLogger& dbglogger,
                                               HFSAT::SimpleLiveDispatcher& simple_live_dispatcher,
                                               HFSAT::CombinedControlMessageLiveSource* p_ccm_livesource,
                                               HFSAT::FastMdConsumerMode_t daemon_mode, std::string exch = "NSE",
                                               bool using_udp_direct = false,
                                               HFSAT::NSEMD::MarketEventListener* market_event_listener = nullptr,
                                               std::string channels_file_name = "DEFAULT", bool using_exanic_direct_ = false) {
    static NSETBTRawMDHandler unique_instance(dbglogger, simple_live_dispatcher, p_ccm_livesource, daemon_mode, exch,
                                              using_udp_direct, market_event_listener, channels_file_name, using_exanic_direct_);
    return unique_instance;
  }

  void SetBardataPeriod(uint32_t bardata_period) {
    nse_tbt_data_processor_.SetBardataPeriod(bardata_period);
    nse_tbt_data_decoder_.SetBardataPeriod(bardata_period);
  }

  bool ProcessRequest(char* in_buffer, int32_t length, char* out_buffer) {
    char segment = in_buffer[0];

    if (NSE_EQ_SEGMENT_MARKING != segment && NSE_FO_SEGMENT_MARKING != segment && NSE_CD_SEGMENT_MARKING != segment) {
      return false;
    }

    int32_t token = *((int32_t*)((char*)in_buffer + 1));

    if (false == nse_tbt_data_processor_.IsTokenAvailableForRecovery(token, segment)) {
      return false;
    }
    return nse_tbt_data_processor_.FetchProcessedLiveOrdersMap(token, out_buffer, MAX_RECOVERY_PACKET_SIZE, segment);
  }

  bool ProcessBardataRequest(char* in_buffer, int32_t length, char* out_buffer) {
    char segment = in_buffer[0];

    if (NSE_EQ_SEGMENT_MARKING != segment && NSE_FO_SEGMENT_MARKING != segment && NSE_CD_SEGMENT_MARKING != segment && NSE_IX_SEGMENT_MARKING != segment) {
      return false;
    }

    in_buffer += BARDATA_RECOVERY_REQUEST_SEGMENT_LENGTH;
    std::string shortcode_((in_buffer),BARDATA_RECOVERY_REQUEST_SYMBOL_LENGTH);
    
    in_buffer += BARDATA_RECOVERY_REQUEST_SYMBOL_LENGTH;
    int32_t start_hhmm_ = *((int32_t*)((char*)in_buffer)); 

    in_buffer += BARDATA_RECOVERY_REQUEST_START_TIME_LENGTH;
    int32_t end_hhmm_ = *((int32_t*)((char*)in_buffer));

    std::cout << "ProcessBardataRequest: " << shortcode_ << " start_hhmm_: " << start_hhmm_ << " end_hhmm_: " << end_hhmm_ << std::endl;
    return nse_tbt_data_processor_.FetchProcessedBardataMap(shortcode_, start_hhmm_, end_hhmm_, out_buffer, MAX_RECOVERY_PACKET_SIZE);
  }

  inline void ProcessAllEventsExanic(char* msg_ptr, ssize_t msg_length, int fd, char segment_marking, bool is_trade_range, bool is_spot) {
    msg_ptr = msg_ptr + 42;
    msg_length -= 42;
    while (msg_length > 0) {
      uint32_t msg_seq_no = *((uint32_t*)((char*)(msg_ptr + NSE_TBT_DATA_HEADER_SEQUENCE_NUMBER_OFFSET))); // version 6.3 int to unsigned int for seqnumber FO

      if (is_trade_range) {
        nse_tbt_data_decoder_.DecodeTradeExecutionRange((const unsigned char*)msg_ptr, segment_marking);
        return;
      }
      else if (is_spot) {
        nse_tbt_data_decoder_.DecodeSpotIndexValue((const unsigned char*)msg_ptr, segment_marking);
        return;
      }

      //      std::cout << "Message Seq Number " << msg_seq_no << " " << fd << " " << segment_marking << std::endl;
      if (msg_length != 42 && msg_length != 49 && msg_length != 22) {
        // 	Ignore packets
        //      		DBGLOG_CLASS_FUNC_LINE_INFO << "Irregular Msg : " << msg_length
        //                                    << " STARTING SEQUENCE IS : " << msg_seq_no << DBGLOG_ENDL_NOFLUSH;
        return;
      }
      //      check drops
      int socket_fd = fd;
      if (-1 == socket_fd_to_last_seen_seq_[socket_fd]) {  // First Packet Seen From
                                                           // The Stream

        DBGLOG_CLASS_FUNC_LINE_INFO << "STARTED RECEIVING DATA FROM SOCKET : " << socket_fd << " SOCKET : " << socket_fd
                                    << " STARTING SEQUENCE IS : " << msg_seq_no << DBGLOG_ENDL_NOFLUSH;

        // Set the sequence to what's received
        socket_fd_to_last_seen_seq_[socket_fd] = msg_seq_no;
      } else if (socket_fd_to_last_seen_seq_[socket_fd] >= msg_seq_no) {  // Duplicate / Already Seen

        if (0 != msg_seq_no) {  // MsgSeq 0 Corrosponds To Heartbeat

          DBGLOG_CLASS_FUNC_LINE_ERROR << "DUPLICATE PACKETS FROM SOCKET : " << socket_fd
                                       << " RECEVIED MSGSEQ IS : " << msg_seq_no << " EXPECTED SEQUENCE NUMBER WAS : "
                                       << (1 + socket_fd_to_last_seen_seq_[socket_fd]) << DBGLOG_ENDL_NOFLUSH;
          DBGLOG_CLASS_FUNC_LINE_ERROR << "READ SIZE FROM SOCKET : " << msg_length << DBGLOG_ENDL_NOFLUSH;
          DBGLOG_DUMP;
        }

        return;  // Nothing to do

      } else if ((1 + socket_fd_to_last_seen_seq_[socket_fd]) == msg_seq_no) {  // Nothing Missed

        // Set the sequence to what's received
        socket_fd_to_last_seen_seq_[socket_fd] = msg_seq_no;

      } else {

        if (!dbglogger_.CheckLoggingLevel(SKIP_PACKET_DROPS_INFO)) {
          // Log that we have dropped packets and move on
          DBGLOG_CLASS_FUNC_LINE_ERROR << "DROPPED PACKETS FROM SOCKET : " << socket_fd
                                       << " RECEVIED MSGSEQ IS : " << msg_seq_no << " EXPECTED SEQUENCE NUMBER WAS : "
                                       << (1 + socket_fd_to_last_seen_seq_[socket_fd])
                                       << " JUMPING SEQ AND CONTINUING... " << DBGLOG_ENDL_NOFLUSH;
        }
        //        DBGLOG_DUMP;
        // Set the sequence to what's received
        socket_fd_to_last_seen_seq_[socket_fd] = msg_seq_no;
      }

      //      HFSAT::CpucycleProfiler::GetUniqueInstance().End(6);
      //      HFSAT::CpucycleProfiler::GetUniqueInstance().Start(7);
      int32_t processed_length = nse_tbt_data_decoder_.DecodeEvents(msg_ptr, msg_seq_no, segment_marking);
      //      HFSAT::CpucycleProfiler::GetUniqueInstance().End(7);

      // a hack to quick return
      if (msg_length < 50) return;

      msg_length -= processed_length;
      msg_ptr += processed_length;

      if (msg_length > 0) {  // want to know if NSE ever sends multiple packets
        DBGLOG_CLASS_FUNC_LINE_INFO << " MSG LENGTH : " << msg_length << DBGLOG_ENDL_NOFLUSH;
        return;
      }
    }
  }

  inline void ProcessAllEvents(int32_t socket_fd) {
    // DBGLOG_CLASS_FUNC_LINE_FATAL << "process all events called..." << DBGLOG_ENDL_NOFLUSH;
    // DBGLOG_DUMP;

    //    HFSAT::CpucycleProfiler::GetUniqueInstance().Start(5);
    int32_t msg_length = socket_fd_to_multicast_receiver_sockets_[socket_fd]->ReadN(MAX_NSE_DATA_BUFFER, data_buffer);
    //    HFSAT::CpucycleProfiler::GetUniqueInstance().End(5);
    //    HFSAT::CpucycleProfiler::GetUniqueInstance().Start(6);
    char* msg_ptr = data_buffer;
    SocketMetaData& meta = socket_fd_to_meta_data_[socket_fd];

    char segment_marking = meta.segment_type;

    if (meta.is_trade_exec_fd) {
      if ((HFSAT::kBardataLogger != daemon_mode_) && (HFSAT::kBardataRecoveryHost != daemon_mode_)) {
        nse_tbt_data_decoder_.DecodeTradeExecutionRange((const unsigned char*)msg_ptr, segment_marking);
      }
      return;
    }
    else if (meta.is_spot_index_fd) {
      nse_tbt_data_decoder_.DecodeSpotIndexValue((const unsigned char*)msg_ptr, segment_marking);
      return;
    }

    // Decoding, keep on decoding all packets until buffer is consumed
    while (msg_length > 0) {
      if (true == using_exanic_direct && (msg_length != 42 && msg_length != 49 && msg_length != 22)) {
        DBGLOG_CLASS_FUNC_LINE_INFO << "Irregular Msg : " << DBGLOG_ENDL_NOFLUSH;
        return;
      }

      uint32_t msg_seq_no = *((uint32_t*)((char*)(msg_ptr + NSE_TBT_DATA_HEADER_SEQUENCE_NUMBER_OFFSET))); // version 6.3 int to unsigned int for seqnumber FO

      if (-1 == socket_fd_to_last_seen_seq_[socket_fd]) {  // First Packet Seen From
                                                           // The Stream

        DBGLOG_CLASS_FUNC_LINE_INFO << "STARTED RECEIVING DATA FROM SOCKET : " << socket_fd << " SOCKET : " << socket_fd
                                    << " STARTING SEQUENCE IS : " << msg_seq_no << DBGLOG_ENDL_NOFLUSH;

        // Set the sequence to what's received
        socket_fd_to_last_seen_seq_[socket_fd] = msg_seq_no;

      } else if (socket_fd_to_last_seen_seq_[socket_fd] >= msg_seq_no) {  // Duplicate / Already Seen
/*
        if (0 != msg_seq_no) {  // MsgSeq 0 Corrosponds To Heartbeat

          DBGLOG_CLASS_FUNC_LINE_ERROR << "DUPLICATE PACKETS FROM SOCKET : " << socket_fd
                                       << " RECEVIED MSGSEQ IS : " << msg_seq_no << " EXPECTED SEQUENCE NUMBER WAS : "
                                       << (1 + socket_fd_to_last_seen_seq_[socket_fd]) << DBGLOG_ENDL_NOFLUSH; 
          DBGLOG_CLASS_FUNC_LINE_ERROR << "READ SIZE FROM SOCKET : " << msg_length << DBGLOG_ENDL_NOFLUSH; 
        }
*/
        return;  // Nothing to do

      } else if ((1 + socket_fd_to_last_seen_seq_[socket_fd]) == msg_seq_no) {  // Nothing Missed

        // Set the sequence to what's received
        socket_fd_to_last_seen_seq_[socket_fd] = msg_seq_no;

      } else {
/*
        struct timeval drop_time;
        gettimeofday(&drop_time,NULL);

        if (!dbglogger_.CheckLoggingLevel(SKIP_PACKET_DROPS_INFO)) {
          // Log that we have dropped packets and move on
          DBGLOG_CLASS_FUNC_LINE_ERROR << "DROPPED PACKETS FROM SOCKET : " << socket_fd
                                       << " RECEVIED MSGSEQ IS : " << msg_seq_no << " EXPECTED SEQUENCE NUMBER WAS : "
                                       << (1 + socket_fd_to_last_seen_seq_[socket_fd])
                                       << " DROP OF : " << ( msg_seq_no -  (1 + socket_fd_to_last_seen_seq_[socket_fd]) )
                                       << " JUMPING SEQ AND CONTINUING... " << drop_time.tv_sec << "." << drop_time.tv_usec <<DBGLOG_ENDL_NOFLUSH;
        }

        //        DBGLOG_DUMP;
*/
        // Set the sequence to what's received
        socket_fd_to_last_seen_seq_[socket_fd] = msg_seq_no;
      }

      //      HFSAT::CpucycleProfiler::GetUniqueInstance().End(6);
      //      HFSAT::CpucycleProfiler::GetUniqueInstance().Start(7);

      int32_t processed_length;
      if ((HFSAT::kBardataLogger == daemon_mode_) || (HFSAT::kBardataRecoveryHost == daemon_mode_)) {
        processed_length = nse_tbt_data_decoder_.DecodeBardataEvents(msg_ptr, msg_seq_no, segment_marking);
      }
      else {
        processed_length = nse_tbt_data_decoder_.DecodeEvents(msg_ptr, msg_seq_no, segment_marking);
      }
      //      HFSAT::CpucycleProfiler::GetUniqueInstance().End(7);

      // a hack to quick return
      if (msg_length < 50) return;

      msg_length -= processed_length;
      msg_ptr += processed_length;

      if (msg_length > 0) {  // want to know if NSE ever sends multiple packets
        DBGLOG_CLASS_FUNC_LINE_INFO << " MSG LENGTH : " << msg_length << DBGLOG_ENDL_NOFLUSH;
      }
    }
  }

  inline void ProcessEventsFromUDPDirectRead(char const* msg_ptr, int32_t msg_length, char seg_type,
                                             bool is_trade_exec_fd, bool is_spot_index_fd, bool is_oi_data_fd_, uint32_t& udp_msg_seq_no) {

    char segment_marking = seg_type;
    if (is_trade_exec_fd) {
      if ((HFSAT::kBardataLogger != daemon_mode_) && (HFSAT::kBardataRecoveryHost != daemon_mode_)) {
        nse_tbt_data_decoder_.DecodeTradeExecutionRange((const unsigned char*)msg_ptr, segment_marking);
      }
      return;
    } else if (is_spot_index_fd) {
      nse_tbt_data_decoder_.DecodeSpotIndexValue((const unsigned char*)msg_ptr, segment_marking);
      return;
    } else if (is_oi_data_fd_){
      nse_tbt_data_decoder_.DecodeOpenInterestValue((const unsigned char*)msg_ptr, segment_marking);
      return;
    }

    // Decoding, keep on decoding all packets until buffer is consumed
    while (msg_length > 0) {
      if (true == using_exanic_direct && (msg_length != 42 && msg_length != 49 && msg_length != 22)) {
        //      Ignore packets
	//      42 and 49 udp packets from exchange and 22 is heartbeat
                              DBGLOG_CLASS_FUNC_LINE_INFO << "Irregular Msg : " << msg_length
  					 << DBGLOG_ENDL_NOFLUSH;
	//        std::cout << " Irregular Message msg_length " << msg_length << " " << std::endl;
        return;
      }

      uint32_t msg_seq_no = *((uint32_t*)((char*)(msg_ptr + NSE_TBT_DATA_HEADER_SEQUENCE_NUMBER_OFFSET)));

      if (UINT_MAX == udp_msg_seq_no) {  // First Packet Seen From

        DBGLOG_CLASS_FUNC_LINE_INFO << "STARTED RECEIVING NSE DATA FROM SOCKET : "
                                    << " STARTING SEQUENCE IS : " << msg_seq_no << DBGLOG_ENDL_NOFLUSH;

        // Set the sequence to what's received
        udp_msg_seq_no = msg_seq_no;

      } else if (udp_msg_seq_no >= msg_seq_no) {  // Duplicate / Already Seen

        if (0 != msg_seq_no) {  // MsgSeq 0 Corrosponds To Heartbeat

          DBGLOG_CLASS_FUNC_LINE_ERROR << "DUPLICATE PACKETS FROM SOCKET : "
                                       << " RECEVIED MSGSEQ IS : " << msg_seq_no
                                       << " EXPECTED SEQUENCE NUMBER WAS : " << (1 + udp_msg_seq_no)
                                       << DBGLOG_ENDL_NOFLUSH;
          DBGLOG_CLASS_FUNC_LINE_ERROR << "READ SIZE FROM SOCKET : " << msg_length << DBGLOG_ENDL_NOFLUSH;
        }
        return;  // Nothing to do

      } else if ((1 + udp_msg_seq_no) == msg_seq_no) {  // Nothing Missed

        // Set the sequence to what's received
        udp_msg_seq_no = msg_seq_no;

      } else {

        struct timeval drop_time;
        gettimeofday(&drop_time,NULL);

        if (!dbglogger_.CheckLoggingLevel(SKIP_PACKET_DROPS_INFO)) {
          // Log that we have dropped packets and move on
          DBGLOG_CLASS_FUNC_LINE_ERROR << "DROPPED PACKETS FROM SOCKET : "
                                       << " RECEVIED MSGSEQ IS : " << msg_seq_no
                                       << " EXPECTED SEQUENCE NUMBER WAS : " << (1 + udp_msg_seq_no)
                                       << " DROP OF : " << ( msg_seq_no - (1 + udp_msg_seq_no) )
                                       << " JUMPING SEQ AND CONTINUING... " << drop_time.tv_sec << "." << drop_time.tv_usec <<DBGLOG_ENDL_NOFLUSH;
        }
        //        DBGLOG_DUMP;
        // Set the sequence to what's received
        udp_msg_seq_no = msg_seq_no;
      }

      //      HFSAT::CpucycleProfiler::GetUniqueInstance().End(6);
      //      HFSAT::CpucycleProfiler::GetUniqueInstance().Start(7);
      int32_t processed_length;
      if ((HFSAT::kBardataLogger == daemon_mode_) || (HFSAT::kBardataRecoveryHost == daemon_mode_)) {
        processed_length = nse_tbt_data_decoder_.DecodeBardataEvents(msg_ptr, msg_seq_no, segment_marking);
      }
      else {
        processed_length = nse_tbt_data_decoder_.DecodeEvents(msg_ptr, msg_seq_no, segment_marking);
      }
      //      HFSAT::CpucycleProfiler::GetUniqueInstance().End(7);

      // a hack to quick return
      if (msg_length < 50) return;

      msg_length -= processed_length;
      msg_ptr += processed_length;

      if (msg_length > 0) {  // want to know if NSE ever sends multiple packets
        DBGLOG_CLASS_FUNC_LINE_INFO << " MSG LENGTH : " << msg_length << DBGLOG_ENDL_NOFLUSH;
      }
    }
  }
};
}  // namespace NSEMD
}  // namespace HFSAT
