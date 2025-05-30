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
#define NSE_RAW_MD_FO_ONLY_CHANNELS "nse-tbt-fo-mcast.txt"
#define NSE_RAW_MD_CD_ONLY_CHANNELS "nse-tbt-cd-mcast.txt"
#define NSE_RAW_MD_EQ_ONLY_CHANNELS "nse-tbt-eq-mcast.txt"
#define MAX_NSE_DATA_BUFFER 65536

#include <iostream>
#include <fstream>
#include <ctime>
#include <cstdlib>

#include "infracore/NSEMD/nse_tbt_templates.hpp"
#include "infracore/NSEMD/nse_tbt_data_decoder.hpp"
#include "infracore/NSEMD/nse_tbt_data_processor.hpp"

#include "dvccode/ExternalData/simple_live_dispatcher.hpp"
#include "dvccode/Utils/multicast_receiver_socket.hpp"
#include "dvccode/CommonDataStructures/perishable_string_tokenizer.hpp"

namespace HFSAT {
namespace NSEMD {

struct SocketMetaData {
  char segment_type;
  bool is_trade_exec_fd;

  SocketMetaData() {
    segment_type = 'I';
    is_trade_exec_fd = false;
  }
};

class NSETBTRawMDHandler : public HFSAT::SimpleExternalDataLiveListener {
 private:
  HFSAT::DebugLogger& dbglogger_;
  int32_t trading_date_;
  HFSAT::SimpleLiveDispatcher& simple_live_dispatcher_;
  char data_buffer[MAX_NSE_DATA_BUFFER];
  HFSAT::NSEMD::NSETBTDataProcessor& nse_tbt_data_processor_;

  // This will allow us to pick sockets from which we need to read data based on socket fd
  HFSAT::MulticastReceiverSocket* socket_fd_to_multicast_receiver_sockets_[NSE_MAX_FD_SUPPORTED];
  int32_t socket_fd_to_last_seen_seq_[NSE_MAX_FD_SUPPORTED];

  // Data Decoder
  NSETBTDataDecoder& nse_tbt_data_decoder_;

  // Segment Identifier
  SocketMetaData socket_fd_to_meta_data_[NSE_MAX_FD_SUPPORTED];

  bool is_local_data_;
  char hostname[128];

  void CreateSockets(HFSAT::FastMdConsumerMode_t daemon_mode, std::string exch) {
    std::ostringstream host_nse_tbt_filename;
    host_nse_tbt_filename << PROD_CONFIGS_DIR << hostname << "_" << NSE_RAW_MD_CHANNELS;
    std::string nse_tbt_filename = host_nse_tbt_filename.str();

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

    std::ifstream nse_tbt_channels_file;
    nse_tbt_channels_file.open(nse_tbt_filename.c_str());

    if (!nse_tbt_channels_file.is_open()) {
      DBGLOG_CLASS_FUNC_LINE_ERROR << "Failed To Load The TBT Multicast File : " << nse_tbt_filename
                                   << DBGLOG_ENDL_NOFLUSH;
      DBGLOG_DUMP;
      exit(-1);
    }

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
                                         HFSAT::kExchSourceNSE, HFSAT::k_MktDataRaw) << DBGLOG_ENDL_NOFLUSH;
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
                                        "UNEXPECTED CHANNEL ADDRESS : " << tokens[1] << DBGLOG_ENDL_NOFLUSH;
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
                     std::string exch)
      : dbglogger_(dbglogger),
        trading_date_(HFSAT::DateTime::GetCurrentIsoDateLocal()),
        simple_live_dispatcher_(simple_live_dispatcher),
        data_buffer(),
        nse_tbt_data_processor_(HFSAT::NSEMD::NSETBTDataProcessor::GetUniqueInstance(dbglogger, daemon_mode, nullptr)),
        socket_fd_to_multicast_receiver_sockets_(),
        nse_tbt_data_decoder_(NSETBTDataDecoder::GetUniqueInstance(dbglogger, daemon_mode, p_ccm_livesource)) {
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
    CreateSockets(daemon_mode, exch);
  }

 public:
  static NSETBTRawMDHandler& GetUniqueInstance(HFSAT::DebugLogger& dbglogger,
                                               HFSAT::SimpleLiveDispatcher& simple_live_dispatcher,
                                               HFSAT::CombinedControlMessageLiveSource* p_ccm_livesource,
                                               HFSAT::FastMdConsumerMode_t daemon_mode, std::string exch = "NSE") {
    static NSETBTRawMDHandler unique_instance(dbglogger, simple_live_dispatcher, p_ccm_livesource, daemon_mode, exch);
    return unique_instance;
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

  inline void ProcessAllEvents(int32_t socket_fd) {
    // DBGLOG_CLASS_FUNC_LINE_FATAL << "process all events called..." << DBGLOG_ENDL_NOFLUSH;
    // DBGLOG_DUMP;

    //    HFSAT::CpucycleProfiler::GetUniqueInstance().Start(5);
    int32_t msg_length = socket_fd_to_multicast_receiver_sockets_[socket_fd]->ReadN(MAX_NSE_DATA_BUFFER, data_buffer);
    //    HFSAT::CpucycleProfiler::GetUniqueInstance().End(5);
    //    HFSAT::CpucycleProfiler::GetUniqueInstance().Start(6);

   //  if (is_local_data_) {
   //    HFSAT::CpucycleProfiler::GetUniqueInstance().End(1);
   //  }

    char* msg_ptr = data_buffer;
    SocketMetaData& meta = socket_fd_to_meta_data_[socket_fd];

    char segment_marking = meta.segment_type;

    // removed support for TradeExecution Range
//    if (meta.is_trade_exec_fd) {
//      nse_tbt_data_decoder_.DecodeTradeExecutionRange((const unsigned char*)msg_ptr, segment_marking);
//      return;
//    }

    // Decoding, keep on decoding all packets until buffer is consumed
    while (msg_length > 0) {
      int32_t msg_seq_no = *((int32_t*)((char*)(msg_ptr + NSE_TBT_DATA_HEADER_SEQUENCE_NUMBER_OFFSET)));

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
        // Log that we have dropped packets and move on
/* temp comment to see impact of logging
         DBGLOG_CLASS_FUNC_LINE_ERROR << "DROPPED PACKETS FROM SOCKET : " << socket_fd
                                     << " RECEVIED MSGSEQ IS : " << msg_seq_no << " EXPECTED SEQUENCE NUMBER WAS : "
                                     << (1 + socket_fd_to_last_seen_seq_[socket_fd])
                                     << " JUMPING SEQ AND CONTINUING... " << DBGLOG_ENDL_NOFLUSH;
*/
        //        DBGLOG_DUMP;

        // Set the sequence to what's received
        socket_fd_to_last_seen_seq_[socket_fd] = msg_seq_no;
      }

      // if (is_local_data_) {
      //  HFSAT::CpucycleProfiler::GetUniqueInstance().Start(2);
      // }

      //      HFSAT::CpucycleProfiler::GetUniqueInstance().End(6);
      //      HFSAT::CpucycleProfiler::GetUniqueInstance().Start(7);
      int32_t processed_length = nse_tbt_data_decoder_.DecodeEvents(msg_ptr, msg_seq_no, segment_marking);
      //      HFSAT::CpucycleProfiler::GetUniqueInstance().End(7);
      // if (is_local_data_) {
      //   HFSAT::CpucycleProfiler::GetUniqueInstance().End(2);
      // }

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
}
}
