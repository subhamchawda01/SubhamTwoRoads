// =====================================================================================
// 
//       Filename:  ibkr_l1_md_handler.hpp
// 
//    Description:  
// 
//        Version:  1.0
//        Created:  10/21/2024 08:31:47 AM
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

#define IBKR_L1_MD_CHANNELS "ibkr-l1-mcast.txt"
#define IBKR_MAX_FD_SUPPORTED 1024

#include <iostream>
#include <fstream>
#include <ctime>
#include <cstdlib>

#include "dvccode/IBUtils/contract_manager.hpp"
#include "dvccode/Utils/rdtsc_timer.hpp"
#include "dvccode/Utils/mds_logger.hpp"
#include "dvccode/ExternalData/simple_live_dispatcher.hpp"
#include "dvccode/Utils/multicast_receiver_socket.hpp"
#include "dvccode/CommonDataStructures/perishable_string_tokenizer.hpp"
#include "dvccode/CDef/online_debug_logger.hpp"
#include "dvccode/CDef/cboe_security_definition.hpp"

namespace HFSAT {
namespace IBKRMD {

class MarketEventListener {
 public:
  ~MarketEventListener() {}
  virtual void OnMarketEventDispatch(IBL1UpdateTick* market_event, bool is_timeout) {
    std::cout << "SHOULDN'T POINT HERE FROM HFSAT->IBKRMD->MarketEventListener: " << std::endl;
    std::exit(-1);
  }
};
  
class IBKRL1MDHandler : public HFSAT::SimpleExternalDataLiveListener {
 private:
  HFSAT::DebugLogger& dbglogger_;
  int32_t trading_date_;
  HFSAT::SimpleLiveDispatcher& simple_live_dispatcher_;
  MarketEventListener *market_event_listener_;
  IBL1UpdateTick ib_data_struct_;
  HFSAT::FastMdConsumerMode_t daemon_mode_;
  MDSLogger<CBOEBarDataCommonStruct>* bardata_logger_thread_;
  uint32_t bardata_period_;
  int bardata_time_;
  std::unordered_map<std::string, CBOEBarDataCommonStruct*> shortcode_to_bardata_map_;
  std::unordered_map<std::string, std::map<int32_t, CBOEBarDataCommonStruct*>> shortcode_to_live_bardata_map_;

  // This will allow us to pick sockets from which we need to read data based on socket fd
  HFSAT::MulticastReceiverSocket* socket_fd_to_multicast_receiver_sockets_[IBKR_MAX_FD_SUPPORTED];
  int64_t socket_fd_to_last_seen_seq_[IBKR_MAX_FD_SUPPORTED];

  void CreateSockets() {

    char hostname[128];
    hostname[127] = '\0';
    gethostname(hostname, 127);

    std::ostringstream ibkr_channels_filename;
    ibkr_channels_filename << PROD_CONFIGS_DIR << hostname << "_" << IBKR_L1_MD_CHANNELS;
    std::string ibkr_l1_filename = ibkr_channels_filename.str();

    std::ifstream ibkr_l1_channels_file;
    ibkr_l1_channels_file.open(ibkr_l1_filename.c_str());

    if (!ibkr_l1_channels_file.is_open()) {
      DBGLOG_CLASS_FUNC_LINE_ERROR << "Failed To Load The IBKR L1 Multicast File : " << ibkr_l1_filename
                                   << DBGLOG_ENDL_NOFLUSH;
      DBGLOG_DUMP;
      exit(-1);
    }

#undef MAX_LINE_SIZE
#define MAX_LINE_SIZE 1024

    char buffer[1024];

    while (ibkr_l1_channels_file.good()) {
      ibkr_l1_channels_file.getline(buffer, MAX_LINE_SIZE);
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
          HFSAT::NetworkAccountInterfaceManager::instance().GetInterface(HFSAT::kExchSourceCBOE, HFSAT::k_MktDataRaw));
      DBGLOG_CLASS_FUNC_LINE_INFO << "CREATING SOCKET : " << tokens[1] << " " << tokens[2] << " Interface : "
                                  << HFSAT::NetworkAccountInterfaceManager::instance().GetInterface(
                                         HFSAT::kExchSourceCBOE, HFSAT::k_MktDataRaw)
                                  << DBGLOG_ENDL_NOFLUSH;
      DBGLOG_DUMP;
      new_multicast_receiver_socket->SetNonBlocking();

      if (new_multicast_receiver_socket->socket_file_descriptor() >= IBKR_MAX_FD_SUPPORTED) {
        DBGLOG_CLASS_FUNC_LINE_FATAL
            << "SOMETHING HAS GONE WRONG IT SEEMS, DIDN'T EXPECT TO SEE A SOCKET FD WITH DESCRIPTOR : "
            << new_multicast_receiver_socket->socket_file_descriptor()
            << " MAX SUPPOERTED RANGE IS UPTO : " << IBKR_MAX_FD_SUPPORTED << DBGLOG_ENDL_NOFLUSH;
        DBGLOG_DUMP;
        exit(-1);
      }

      socket_fd_to_multicast_receiver_sockets_[new_multicast_receiver_socket->socket_file_descriptor()] =
          new_multicast_receiver_socket;

      DBGLOG_CLASS_FUNC_LINE_INFO << "SOCKET FD :" << new_multicast_receiver_socket->socket_file_descriptor()
                                  << " SETUP ON : " << tokens[0] << DBGLOG_ENDL_NOFLUSH;
      DBGLOG_DUMP;

      simple_live_dispatcher_.AddSimpleExternalDataLiveListenerSocket(
          this, new_multicast_receiver_socket->socket_file_descriptor(), true);
    }

    ibkr_l1_channels_file.close();
    DBGLOG_DUMP;

#undef MAX_LINE_SIZE
  }


  void ProcessEventsForBardataMode() {
    std::string exchsymbol = HFSAT::CBOESecurityDefinitions::ConvertDataSourceNametoExchSymbol(ib_data_struct_.symbol);
    std::string shortcode = HFSAT::CBOESecurityDefinitions::GetShortCodeFromExchangeSymbol(exchsymbol);

    //std::cout << "ProcessEventsForBardataMode:shortcode: " << shortcode << " exchsymbol: " << exchsymbol << std::endl;
    if ((exchsymbol == "INVALID") || (shortcode == "INVALID")) { return; }

    if (shortcode_to_bardata_map_.find(shortcode) == shortcode_to_bardata_map_.end()) {
      shortcode_to_bardata_map_[shortcode] = new CBOEBarDataCommonStruct();
      CBOEBarDataCommonStruct* temp_bardata_struct_ = shortcode_to_bardata_map_[shortcode];

      int bardata_time_ = ib_data_struct_.time.tv_sec - (ib_data_struct_.time.tv_sec % bardata_period_);
      int expiry_ = HFSAT::CBOESecurityDefinitions::GetExpiryFromShortCode(shortcode);

      temp_bardata_struct_->bardata_time = bardata_time_;
      strcpy(temp_bardata_struct_->shortcode, shortcode.c_str());

      temp_bardata_struct_->start_time = ib_data_struct_.time.tv_sec;
      temp_bardata_struct_->close_time = ib_data_struct_.time.tv_sec;
      temp_bardata_struct_->expiry = expiry_;

      temp_bardata_struct_->open_price = ib_data_struct_.trade_price;
      temp_bardata_struct_->close_price = ib_data_struct_.trade_price;
      temp_bardata_struct_->low_price = ib_data_struct_.trade_price;
      temp_bardata_struct_->high_price = ib_data_struct_.trade_price;
      temp_bardata_struct_->volume = ib_data_struct_.trade_size;

      temp_bardata_struct_->trades = 1;
/*
      std::cout << "if:UpdateBardataStruct: " << temp_bardata_struct_->shortcode << " " << temp_bardata_struct_->bardata_time << " " 
                << temp_bardata_struct_->start_time << " " << temp_bardata_struct_->close_time << " " << temp_bardata_struct_->expiry <<  " " 
                << temp_bardata_struct_->open_price << " " << temp_bardata_struct_->close_price << " " << temp_bardata_struct_->low_price << " " 
                << temp_bardata_struct_->high_price << " " << temp_bardata_struct_->volume << " " << temp_bardata_struct_->trades << std::endl;
*/
    } else {
      CBOEBarDataCommonStruct* temp_bardata_struct_ = shortcode_to_bardata_map_[shortcode];

      temp_bardata_struct_->close_time = ib_data_struct_.time.tv_sec;

      temp_bardata_struct_->close_time = ib_data_struct_.time.tv_sec;
      temp_bardata_struct_->close_price = ib_data_struct_.trade_price;
      if (temp_bardata_struct_->low_price > (ib_data_struct_.trade_price))
        temp_bardata_struct_->low_price = ib_data_struct_.trade_price;
      if (temp_bardata_struct_->high_price < (ib_data_struct_.trade_price))
        temp_bardata_struct_->high_price = ib_data_struct_.trade_price;
      temp_bardata_struct_->volume += ib_data_struct_.trade_size;

      temp_bardata_struct_->trades += 1;
/*
      std::cout << "else:UpdateBardataStruct: " << temp_bardata_struct_->shortcode << " " << temp_bardata_struct_->bardata_time << " " 
                << temp_bardata_struct_->start_time << " " << temp_bardata_struct_->close_time << " " << temp_bardata_struct_->expiry <<  " " 
                << temp_bardata_struct_->open_price << " " << temp_bardata_struct_->close_price << " " << temp_bardata_struct_->low_price << " " 
                << temp_bardata_struct_->high_price << " " << temp_bardata_struct_->volume << " " << temp_bardata_struct_->trades << std::endl;
*/
    }

  }

  void DumpBardata() {
    bardata_logger_thread_->logBardata(shortcode_to_bardata_map_);
    shortcode_to_bardata_map_.clear();
  }

  void UpdateProcessedBardataMap() {
    std::cout << "UpdateProcessedBardataMap(): " << shortcode_to_bardata_map_.size() << std::endl;
    for (auto map_itr : shortcode_to_bardata_map_) {
      int32_t bardata_time = map_itr.second->bardata_time;
      shortcode_to_live_bardata_map_[map_itr.first][bardata_time] = map_itr.second;
    }
  }

  void AddBardataToRecoveryMap() {
    std::cout << "AddBardataToRecoveryMap()" << std::endl;
    UpdateProcessedBardataMap();
    shortcode_to_bardata_map_.clear();
  }

  bool SendEmptyBardataUpdate(std::string& shortcode_, int32_t const& start_hhmm_, int32_t const& end_hhmm_, char* data_packet, int32_t const& MAX_BUFFER_SIZE) {
    CBOEBarDataCommonStruct temp_bardata_struct_;
    temp_bardata_struct_.bardata_time = -1;
    strcpy(temp_bardata_struct_.shortcode, shortcode_.c_str());
    temp_bardata_struct_.start_time = -1;
    temp_bardata_struct_.close_time = -1;
    temp_bardata_struct_.expiry = -1;
    temp_bardata_struct_.open_price = -1;
    temp_bardata_struct_.close_price = -1;
    temp_bardata_struct_.low_price = -1;
    temp_bardata_struct_.high_price = -1;
    temp_bardata_struct_.volume = -1;
    temp_bardata_struct_.trades = -1;

    DBGLOG_CLASS_FUNC_LINE_INFO << "INCOMING BARDATA RECOVERY REQUEST FOR : " << shortcode_
                                << " LIVE BARDATA : " << "1"
                                << " LIVE SIZE : " << sizeof(CBOEBarDataCommonStruct)
                                << " START_TIME: " << start_hhmm_
                                << " END_TIME: " << end_hhmm_
                                << DBGLOG_ENDL_NOFLUSH;
    DBGLOG_DUMP;

    // Check whether the buffer storage can hold enough elements
    if ((int32_t)(4 + (sizeof(CBOEBarDataCommonStruct))) > (int32_t)MAX_BUFFER_SIZE){
      DBGLOG_CLASS_FUNC_LINE_INFO << "Live Bardata Size " << sizeof(CBOEBarDataCommonStruct)
        << " Greater than Max buffer Size: " << MAX_BUFFER_SIZE << DBGLOG_ENDL_NOFLUSH;
      DBGLOG_DUMP;
      return false;
    }

    int32_t data_size = sizeof(CBOEBarDataCommonStruct);

    // Insert length to buffer
    memcpy((void*)data_packet, (void*)&data_size, sizeof(int32_t));

    DBGLOG_CLASS_FUNC_LINE_INFO << "4 BYTES OF DATA : " << *((int32_t*)((char*)(data_packet))) << DBGLOG_ENDL_NOFLUSH;
    DBGLOG_DUMP;

    data_packet += 4;

    // Fill up the buffer with data
    memcpy((void*)data_packet, (void*)&(temp_bardata_struct_), sizeof(CBOEBarDataCommonStruct));
    data_packet += sizeof(CBOEBarDataCommonStruct);

    DBGLOG_CLASS_FUNC_LINE_INFO << "SENDING MAX SEQUENCE IN RECOVERY AS :" << end_hhmm_ << DBGLOG_ENDL_NOFLUSH;
    DBGLOG_DUMP;

    return true;
  }

  bool FetchProcessedBardataMap(std::string& temp_shortcode_, int32_t const& start_hhmm_, int32_t const& end_hhmm_, char* data_packet, int32_t const& MAX_BUFFER_SIZE) {
    std::string shortcode_;
    HFSAT::PerishableStringTokenizer::TrimString(temp_shortcode_.c_str(),shortcode_);

    std::cout << "shortcode_:" << shortcode_ << " start_hhmm_: " << start_hhmm_ << " end_hhmm_: " << end_hhmm_
              << " shortcode_to_live_bardata_map_.size(): " << shortcode_to_live_bardata_map_.size()
              << " shortcode_to_live_bardata_map_.size():bardata: " << shortcode_to_live_bardata_map_[shortcode_].size() << std::endl;

    if (shortcode_to_live_bardata_map_.end() != shortcode_to_live_bardata_map_.find(shortcode_)) {
      std::cout << "IsShortcodeAvailableForRecovery true: " << std::endl;
      // It's time to now construct buffer
      std::map<int32_t, CBOEBarDataCommonStruct*> live_bardata = shortcode_to_live_bardata_map_[shortcode_];

      int bardata_count = 0;
      for (auto& itr : live_bardata) {
        int32_t bardata_time = (itr.second)->bardata_time;
        if ((bardata_time >= start_hhmm_) && (bardata_time <= end_hhmm_)) {
          ++bardata_count;
        }
      }

      if ((live_bardata.size() == 0) || (bardata_count == 0)) {
        std::cout << "SendEmptyBardataUpdate:size: " << live_bardata.size() << " count: " << bardata_count << std::endl;
        return SendEmptyBardataUpdate(shortcode_, start_hhmm_, end_hhmm_, data_packet, MAX_RECOVERY_PACKET_SIZE);
      }

      DBGLOG_CLASS_FUNC_LINE_INFO << "INCOMING BARDATA RECOVERY REQUEST FOR : " << shortcode_
                                  << " LIVE BARDATA : " << live_bardata.size()
                                  << " LIVE SIZE : " << live_bardata.size() * sizeof(CBOEBarDataCommonStruct)
                                  << " START_TIME: " << start_hhmm_
                                  << " END_TIME: " << end_hhmm_
                                  << DBGLOG_ENDL_NOFLUSH;
      DBGLOG_DUMP;

      if (live_bardata.size() == 0) {
        DBGLOG_CLASS_FUNC_LINE_INFO << "Live Bardata Size 0 "<< DBGLOG_ENDL_NOFLUSH;
        DBGLOG_DUMP;
        return false;
      }
      // Check whether the buffer storage can hold enough elements
      if ((int32_t)(4 + (live_bardata.size() * sizeof(CBOEBarDataCommonStruct))) > (int32_t)MAX_BUFFER_SIZE){
        DBGLOG_CLASS_FUNC_LINE_INFO << "Live Bardata Size " << live_bardata.size() * sizeof(CBOEBarDataCommonStruct)
          << " Greater than Max buffer Size: " << MAX_BUFFER_SIZE << DBGLOG_ENDL_NOFLUSH;
        DBGLOG_DUMP;
        return false;
      }

      int32_t data_size = bardata_count * sizeof(CBOEBarDataCommonStruct);

      // Insert length to buffer
      memcpy((void*)data_packet, (void*)&data_size, sizeof(int32_t));

      DBGLOG_CLASS_FUNC_LINE_INFO << "4 BYTES OF DATA : " << *((int32_t*)((char*)(data_packet))) << DBGLOG_ENDL_NOFLUSH;
      DBGLOG_DUMP;

      data_packet += 4;

      int32_t max_bardata_time = 0;

      // Fill up the buffer with data
      for (auto& itr : live_bardata) {
        int32_t bardata_time = (itr.second)->bardata_time;
        if ((bardata_time >= start_hhmm_) && (bardata_time <= end_hhmm_)) {
          max_bardata_time = std::max(max_bardata_time, bardata_time);
          memcpy((void*)data_packet, (void*)&(*(itr.second)), sizeof(CBOEBarDataCommonStruct));
          data_packet += sizeof(CBOEBarDataCommonStruct);
        }
      }

      DBGLOG_CLASS_FUNC_LINE_INFO << "SENDING MAX BARDATA TIME IN RECOVERY AS :" << max_bardata_time << DBGLOG_ENDL_NOFLUSH;
      DBGLOG_DUMP;
    }
    else {
      return SendEmptyBardataUpdate(shortcode_, start_hhmm_, end_hhmm_, data_packet, MAX_RECOVERY_PACKET_SIZE);
    }

    // Request was handled successfully
    return true;
  }


  IBKRL1MDHandler(HFSAT::DebugLogger& dbglogger, HFSAT::SimpleLiveDispatcher& simple_live_dispatcher,
                     HFSAT::FastMdConsumerMode_t daemon_mode, MarketEventListener* market_event_listener = nullptr,
                     std::string channels_file_name = "DEFAULT")
      : dbglogger_(dbglogger),
        trading_date_(HFSAT::DateTime::GetCurrentIsoDateLocal()),
        simple_live_dispatcher_(simple_live_dispatcher),
        market_event_listener_(market_event_listener),
        ib_data_struct_(),
	daemon_mode_(daemon_mode),
        bardata_period_(60),
        bardata_time_(-1),
        shortcode_to_bardata_map_(),
        shortcode_to_live_bardata_map_(),
        socket_fd_to_multicast_receiver_sockets_(){

    for (int32_t ctr = 0; ctr < IBKR_MAX_FD_SUPPORTED; ctr++) {
      socket_fd_to_multicast_receiver_sockets_[ctr] = NULL;
      socket_fd_to_last_seen_seq_[ctr] = -1;
    }

    CreateSockets();

    switch (daemon_mode_) {
      case HFSAT::kBardataLogger: {
        std::cout << "CREATED MDSLogger\n";
        bardata_logger_thread_ = new MDSLogger<CBOEBarDataCommonStruct>("BARDATA");
        bardata_logger_thread_->EnableAffinity("CBOEBardataLogger");
        bardata_logger_thread_->run();
      } break;
      case HFSAT::kRaw: {
        DBGLOG_CLASS_FUNC_LINE_INFO << "INITIATING THE ProcessEventsForRawMode " << DBGLOG_ENDL_NOFLUSH;
        DBGLOG_DUMP;
      } break;


      case HFSAT::kBardataRecoveryHost: {
	DBGLOG_CLASS_FUNC_LINE_INFO << "INITIATING THE kBardataRecoveryHost " << DBGLOG_ENDL_NOFLUSH;
        DBGLOG_DUMP;

      } break;

      default: {
        DBGLOG_CLASS_FUNC_LINE_FATAL << "OOPS !!, THIS MODE IS NOT YET SUPPORTED BY THE PROCESSOR, MODE VALUE : ";
        DBGLOG_DUMP;
        exit(-1);

      } break;
    }
  }

 public:
  static IBKRL1MDHandler& GetUniqueInstance(HFSAT::DebugLogger& dbglogger,
                                               HFSAT::SimpleLiveDispatcher& simple_live_dispatcher, HFSAT::FastMdConsumerMode_t daemon_mode,
                                               MarketEventListener* market_event_listener = nullptr,
                                               std::string channels_file_name = "DEFAULT"){
    static IBKRL1MDHandler unique_instance(dbglogger, simple_live_dispatcher, daemon_mode, market_event_listener, channels_file_name);
    return unique_instance;
  }

  void SetBardataPeriod(uint32_t bardata_period) {
    bardata_period_ = bardata_period;
  }

  bool ProcessBardataRequest(char* in_buffer, int32_t length, char* out_buffer) {
    char segment = in_buffer[0];

    if (CBOE_FO_SEGMENT_MARKING != segment && CBOE_IX_SEGMENT_MARKING != segment) {
      return false;
    }

    in_buffer += BARDATA_RECOVERY_REQUEST_SEGMENT_LENGTH;
    std::string shortcode_((in_buffer),BARDATA_RECOVERY_REQUEST_SYMBOL_LENGTH);

    in_buffer += BARDATA_RECOVERY_REQUEST_SYMBOL_LENGTH;
    int32_t start_hhmm_ = *((int32_t*)((char*)in_buffer));

    in_buffer += BARDATA_RECOVERY_REQUEST_START_TIME_LENGTH;
    int32_t end_hhmm_ = *((int32_t*)((char*)in_buffer));

    std::cout << "ProcessBardataRequest: " << shortcode_ << " start_hhmm_: " << start_hhmm_ << " end_hhmm_: " << end_hhmm_ << std::endl;
    return FetchProcessedBardataMap(shortcode_, start_hhmm_, end_hhmm_, out_buffer, MAX_RECOVERY_PACKET_SIZE);
  }

  inline void ProcessAllEvents(int32_t socket_fd) {

    // std::cout<<socket_fd<<"\n";
    if(-1 == socket_fd){
      market_event_listener_->OnMarketEventDispatch(nullptr, true);
      return;
    }

    int32_t msg_length = socket_fd_to_multicast_receiver_sockets_[socket_fd]->ReadN(sizeof(IBL1UpdateTick), (void*)(&ib_data_struct_));
    // DBGLOG_CLASS_FUNC_LINE_INFO << "MESSAGE LENGTH:" << msg_length << DBGLOG_ENDL_NOFLUSH;
    //   DBGLOG_DUMP;
    if(msg_length < (int32_t)sizeof(IBL1UpdateTick)){
      DBGLOG_CLASS_FUNC_LINE_ERROR << "INVALID PACKETS RECEIVED FROM SOCKET : " << socket_fd << " EXPECTED SIZE : " << sizeof(IBL1UpdateTick) << " RECEIVED IS : " << msg_length << DBGLOG_ENDL_NOFLUSH;
      DBGLOG_DUMP;
      return;
    }

    int64_t this_pkt_seq = ib_data_struct_.packet_seq;
    // DBGLOG_CLASS_FUNC_LINE_INFO << "Sequence No.:" << this_pkt_seq << DBGLOG_ENDL_NOFLUSH;
    //   DBGLOG_DUMP;
    if( -1 == socket_fd_to_last_seen_seq_[socket_fd]){
      DBGLOG_CLASS_FUNC_LINE_INFO << "STARTED RECEIVING DATA FROM SOCKET : " << socket_fd << " SOCKET : " << socket_fd
                                  << " STARTING SEQUENCE IS : " << this_pkt_seq << DBGLOG_ENDL_NOFLUSH;

      // Set the sequence to what's received
      socket_fd_to_last_seen_seq_[socket_fd] = this_pkt_seq;
    }else if(socket_fd_to_last_seen_seq_[socket_fd] >= this_pkt_seq){
      return;
    }else if((1 + socket_fd_to_last_seen_seq_[socket_fd]) == this_pkt_seq){
      socket_fd_to_last_seen_seq_[socket_fd] = this_pkt_seq;
    }else{
      struct timeval drop_time;
      gettimeofday(&drop_time,NULL);

      if (!dbglogger_.CheckLoggingLevel(SKIP_PACKET_DROPS_INFO)) {
        // Log that we have dropped packets and move on
        DBGLOG_CLASS_FUNC_LINE_ERROR << "DROPPED PACKETS FROM SOCKET : " << socket_fd
                                     << " RECEVIED MSGSEQ IS : " << this_pkt_seq << " EXPECTED SEQUENCE NUMBER WAS : "
                                     << (1 + socket_fd_to_last_seen_seq_[socket_fd])
                                     << " DROP OF : " << ( this_pkt_seq -  (1 + socket_fd_to_last_seen_seq_[socket_fd]) )
                                     << " JUMPING SEQ AND CONTINUING... " << drop_time.tv_sec << "." << drop_time.tv_usec <<DBGLOG_ENDL_NOFLUSH;
      }
      DBGLOG_DUMP;

      socket_fd_to_last_seen_seq_[socket_fd] = this_pkt_seq;
    }

    //process event
    if ((HFSAT::kBardataLogger == daemon_mode_) || (HFSAT::kBardataRecoveryHost == daemon_mode_)) {
      switch(ib_data_struct_.ib_update_type) {
        case IB_TRADE: {
          int temp_bardata_time_ = ib_data_struct_.time.tv_sec - (ib_data_struct_.time.tv_sec % bardata_period_);
          if (temp_bardata_time_ > bardata_time_) {
            if (HFSAT::kBardataLogger == daemon_mode_) {
              DumpBardata();
              std::cout << "Case:bardata_time_: " << bardata_time_ << " temp_bardata_time_ " << temp_bardata_time_ << std::endl;
	    } else if (HFSAT::kBardataRecoveryHost == daemon_mode_) {
              AddBardataToRecoveryMap();
              std::cout << "Case:bardata_time_: " << bardata_time_ << " temp_bardata_time_ " << temp_bardata_time_ << std::endl;
            }
            bardata_time_ = temp_bardata_time_;
          }
	  ProcessEventsForBardataMode();
        } break;
        default: {
          // TODO Register Error and return message length from nse packet
          struct timeval source_time;
          gettimeofday(&source_time, NULL);
  
          int temp_bardata_time_ = source_time.tv_sec - (source_time.tv_sec % bardata_period_);
  
          if (temp_bardata_time_ > bardata_time_) {
            if (HFSAT::kBardataLogger == daemon_mode_) {
              DumpBardata();
              std::cout << "Default:bardata_time_: " << bardata_time_ << " temp_bardata_time_ " << temp_bardata_time_ << std::endl;
            } else if (HFSAT::kBardataRecoveryHost == daemon_mode_) {
	      AddBardataToRecoveryMap();
	      std::cout << "Default:bardata_time_: " << bardata_time_ << " temp_bardata_time_ " << temp_bardata_time_ << std::endl;
	    }
            bardata_time_ = temp_bardata_time_;
          }
        } break;
      }
    } else if(nullptr != market_event_listener_){
      // DBGLOG_CLASS_FUNC_LINE_INFO << "Market event listener is not null sending data to direct processor" << DBGLOG_ENDL_NOFLUSH;
      // DBGLOG_DUMP;
      market_event_listener_->OnMarketEventDispatch(&ib_data_struct_, false);
    }else{
      DBGLOG_CLASS_FUNC_LINE_INFO << "Market event listener is null" << DBGLOG_ENDL_NOFLUSH;
      DBGLOG_DUMP;
    }
  }

};
} 
}
