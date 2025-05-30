// =====================================================================================
//
//       Filename:  common_live_source.hpp
//
//    Description:  A Simple Utility Class Which Will Only Operate On Processing Live Data And Providing The Same To
//    CombinedSource ( Writer )
//
//        Version:  1.0
//        Created:  11/11/2014 07:25:47 AM
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

#include <errno.h>  //To get The description of the last system error stored

#include "dvccode/CDef/ttime.hpp"
#include "dvccode/CommonTradeUtils/date_time.hpp"
#include "dvccode/CommonTradeUtils/live_struct_sim_struct.hpp"
#include "dvccode/Utils/multicast_receiver_socket.hpp"
#include "dvccode/CDef/debug_logger.hpp"
#include "dvccode/CommonDataStructures/security_name_indexer.hpp"
#include "dvccode/ExternalData/simple_external_data_live_listener.hpp"
#include "dvccode/Utils/mds_shm_interface.hpp"
#include "dvccode/CDef/mds_messages.hpp"
#include "dvccode/CommonDataStructures/enhanced_security_name_indexer.hpp"
#include "dvccode/Listeners/live_products_manager_listener.hpp"
#include "dvccode/Utils/combined_source_generic_logger.hpp"
#include "infracore/Tools/live_products_manager.hpp"

#define MAX_LIVE_SOCKETS_FD 1024
#define MAX_UDP_LEN 1500

namespace HFSAT {
namespace MDSMessages {

class CommonLiveSource : public HFSAT::SimpleExternalDataLiveListener {  // A Simple Listener Of Event Dispatcher

 private:
  DebugLogger& dbglogger_;
  HFSAT::MulticastReceiverSocket* multicast_receiver_socket_;  // Multicast Socket Reader

  // These are used for generic write
  HFSAT::Utils::MDSShmInterface& mds_shm_interface_;
  HFSAT::MDS_MSG::GenericMDSMessage generic_mds_message_;
  HFSAT::FastMdConsumerMode_t mode_;
  bool is_first_packet_;
  uint64_t last_packet_time_;
  uint64_t last_sim_data_time_;
  int date_;

 public:
  CommonLiveSource(DebugLogger& _dbglogger_, HFSAT::SecurityNameIndexer& _sec_name_indexer_,
                   HFSAT::FastMdConsumerMode_t mode = HFSAT::kComShm)
      : dbglogger_(_dbglogger_),
        multicast_receiver_socket_(nullptr),
        mds_shm_interface_(HFSAT::Utils::MDSShmInterface::GetUniqueInstance()),
        generic_mds_message_(),
        mode_(mode),
        is_first_packet_(true),
        last_packet_time_(),
        last_sim_data_time_(),
        date_(-1) {
    // Reset Memory
    memset((void*)&generic_mds_message_, 0, sizeof(HFSAT::MDS_MSG::GenericMDSMessage));
  }

  // A call to include all live sockets
  void AddLiveSocket(const std::string& _mcast_ip_, const int32_t& _mcast_port_, const std::string& _interface_) {
    multicast_receiver_socket_ = new HFSAT::MulticastReceiverSocket(_mcast_ip_, _mcast_port_, _interface_);
    multicast_receiver_socket_->SetNonBlocking();
  }

  // Fill Up All the socket fd so the caller can add the same to it's event dispatch loop
  int GetLiveSocketsFd() { return multicast_receiver_socket_->socket_file_descriptor(); }

  // Overridden Callback - A listenr of the event dispatcher
  inline void ProcessAllEvents(int32_t _socket_fd_with_data_) {
    char buffer[MAX_UDP_LEN];
    char* ptr = buffer;

    int read_len = multicast_receiver_socket_->ReadN(MAX_UDP_LEN, buffer);

    while (read_len > 0) {
      int32_t exch_struct_type = *((int32_t*)ptr);
      ptr += 4;
      read_len -= 4;

      // Process the Data Based on Underlying Exchange
      switch (exch_struct_type) {
        case HFSAT::MDS_MSG::CME: {
          CME_MDS::CMECommonStruct next_event;
          int pkt_sz = sizeof(CME_MDS::CMECommonStruct);
          memcpy(&next_event, ptr, pkt_sz);
          ptr += pkt_sz;
          read_len -= pkt_sz;

          SetTimeStamp(next_event.time_.tv_sec, next_event.time_.tv_usec);

          generic_mds_message_.mds_msg_exch_ = static_cast<HFSAT::MDS_MSG::MDSMessageExchType>(exch_struct_type);
          next_event.time_ = generic_mds_message_.time_;
          memcpy((void*)&(generic_mds_message_.generic_data_.cme_data_), (void*)&next_event,
                 sizeof(CME_MDS::CMECommonStruct));
        } break;
        case HFSAT::MDS_MSG::NTP: {
          NTP_MDS::NTPCommonStruct next_event;
          int pkt_sz = sizeof(NTP_MDS::NTPCommonStruct);
          memcpy(&next_event, ptr, pkt_sz);
          ptr += pkt_sz;
          read_len -= pkt_sz;

          SetTimeStamp(next_event.time_.tv_sec, next_event.time_.tv_usec);

          generic_mds_message_.mds_msg_exch_ = static_cast<HFSAT::MDS_MSG::MDSMessageExchType>(exch_struct_type);
          next_event.time_ = generic_mds_message_.time_;
          memcpy((void*)&(generic_mds_message_.generic_data_.ntp_data_), (void*)&next_event,
                 sizeof(NTP_MDS::NTPCommonStruct));
        } break;
        case HFSAT::MDS_MSG::ICE: {
          ICE_MDS::ICECommonStruct next_event;
          ICE_MDS::ICECommonStructLive live_struct;
          int pkt_sz = sizeof(ICE_MDS::ICECommonStruct);
          memcpy(&next_event, ptr, pkt_sz);
          ptr += pkt_sz;
          read_len -= pkt_sz;
          HFSAT::Utils::ConvertICESimStructToLiveStruct(live_struct, next_event);

          SetTimeStamp(live_struct.time_.tv_sec, live_struct.time_.tv_usec);

          generic_mds_message_.mds_msg_exch_ = static_cast<HFSAT::MDS_MSG::MDSMessageExchType>(exch_struct_type);
          live_struct.time_ = generic_mds_message_.time_;
          memcpy((void*)&(generic_mds_message_.generic_data_.ice_data_), (void*)&live_struct,
                 sizeof(ICE_MDS::ICECommonStructLive));
        } break;
        case HFSAT::MDS_MSG::ASX: {
          ASX_MDS::ASXPFCommonStruct next_event;
          int pkt_sz = sizeof(ASX_MDS::ASXPFCommonStruct);
          memcpy(&next_event, ptr, pkt_sz);
          ptr += pkt_sz;
          read_len -= pkt_sz;
          SetTimeStamp(next_event.time_.tv_sec, next_event.time_.tv_usec);

          generic_mds_message_.mds_msg_exch_ = static_cast<HFSAT::MDS_MSG::MDSMessageExchType>(exch_struct_type);
          next_event.time_ = generic_mds_message_.time_;
          memcpy((void*)&(generic_mds_message_.generic_data_.asx_data_), (void*)&next_event,
                 sizeof(ASX_MDS::ASXPFCommonStruct));
        } break;
        case HFSAT::MDS_MSG::SGX: {
          SGX_MDS::SGXPFCommonStruct next_event;
          int pkt_sz = sizeof(SGX_MDS::SGXPFCommonStruct);
          memcpy(&next_event, ptr, pkt_sz);
          ptr += pkt_sz;
          read_len -= pkt_sz;
          SetTimeStamp(next_event.time_.tv_sec, next_event.time_.tv_usec);

          generic_mds_message_.mds_msg_exch_ = static_cast<HFSAT::MDS_MSG::MDSMessageExchType>(exch_struct_type);
          next_event.time_ = generic_mds_message_.time_;
          memcpy((void*)&(generic_mds_message_.generic_data_.sgx_data_), (void*)&next_event,
                 sizeof(SGX_MDS::SGXPFCommonStruct));
        } break;
        case HFSAT::MDS_MSG::LIFFE: {
          LIFFE_MDS::LIFFECommonStruct next_event;
          int pkt_sz = sizeof(LIFFE_MDS::LIFFECommonStruct);
          memcpy(&next_event, ptr, pkt_sz);
          ptr += pkt_sz;
          read_len -= pkt_sz;
          SetTimeStamp(next_event.time_.tv_sec, next_event.time_.tv_usec);

          generic_mds_message_.mds_msg_exch_ = static_cast<HFSAT::MDS_MSG::MDSMessageExchType>(exch_struct_type);
          next_event.time_ = generic_mds_message_.time_;
          memcpy((void*)&(generic_mds_message_.generic_data_.liffe_data_), (void*)&next_event,
                 sizeof(LIFFE_MDS::LIFFECommonStruct));
        } break;
        case HFSAT::MDS_MSG::TMX_OBF: {
          TMX_OBF_MDS::TMXPFCommonStruct next_event;
          int pkt_sz = sizeof(TMX_OBF_MDS::TMXPFCommonStruct);
          memcpy(&next_event, ptr, pkt_sz);
          ptr += pkt_sz;
          read_len -= pkt_sz;
          SetTimeStamp(next_event.time_.tv_sec, next_event.time_.tv_usec);

          generic_mds_message_.mds_msg_exch_ = static_cast<HFSAT::MDS_MSG::MDSMessageExchType>(exch_struct_type);
          next_event.time_ = generic_mds_message_.time_;
          memcpy((void*)&(generic_mds_message_.generic_data_.tmx_obf_data_), (void*)&next_event,
                 sizeof(TMX_OBF_MDS::TMXPFCommonStruct));
        } break;
        case HFSAT::MDS_MSG::CSM: {
          CSM_MDS::CSMCommonStruct next_event;
          int pkt_sz = sizeof(CSM_MDS::CSMCommonStruct);
          memcpy(&next_event, ptr, pkt_sz);
          ptr += pkt_sz;
          read_len -= pkt_sz;
          SetTimeStamp(next_event.time_.tv_sec, next_event.time_.tv_usec);

          generic_mds_message_.mds_msg_exch_ = static_cast<HFSAT::MDS_MSG::MDSMessageExchType>(exch_struct_type);
          next_event.time_ = generic_mds_message_.time_;
          memcpy((void*)&(generic_mds_message_.generic_data_.csm_data_), (void*)&next_event,
                 sizeof(CSM_MDS::CSMCommonStruct));
        } break;
        case HFSAT::MDS_MSG::MICEX: {
          MICEX_MDS::MICEXCommonStruct next_event;
          int pkt_sz = sizeof(MICEX_MDS::MICEXCommonStruct);
          memcpy(&next_event, ptr, pkt_sz);
          ptr += pkt_sz;
          read_len -= pkt_sz;
          SetTimeStamp(next_event.time_.tv_sec, next_event.time_.tv_usec);

          generic_mds_message_.mds_msg_exch_ = static_cast<HFSAT::MDS_MSG::MDSMessageExchType>(exch_struct_type);
          next_event.time_ = generic_mds_message_.time_;
          memcpy((void*)&(generic_mds_message_.generic_data_.micex_data_), (void*)&next_event,
                 sizeof(MICEX_MDS::MICEXCommonStruct));
        } break;
        case HFSAT::MDS_MSG::RTS: {
          RTS_MDS::RTSCommonStruct next_event;
          int pkt_sz = sizeof(RTS_MDS::RTSCommonStruct);
          memcpy(&next_event, ptr, pkt_sz);
          ptr += pkt_sz;
          read_len -= pkt_sz;
          SetTimeStamp(next_event.time_.tv_sec, next_event.time_.tv_usec);

          generic_mds_message_.mds_msg_exch_ = static_cast<HFSAT::MDS_MSG::MDSMessageExchType>(exch_struct_type);
          next_event.time_ = generic_mds_message_.time_;
          memcpy((void*)&(generic_mds_message_.generic_data_.rts_data_), (void*)&next_event,
                 sizeof(RTS_MDS::RTSCommonStruct));
        } break;
        case HFSAT::MDS_MSG::EOBI_PF: {
          EUREX_MDS::EUREXCommonStruct next_event;
          int pkt_sz = sizeof(EUREX_MDS::EUREXCommonStruct);
          memcpy(&next_event, ptr, pkt_sz);
          ptr += pkt_sz;
          read_len -= pkt_sz;
          SetTimeStamp(next_event.time_.tv_sec, next_event.time_.tv_usec);

          generic_mds_message_.mds_msg_exch_ = static_cast<HFSAT::MDS_MSG::MDSMessageExchType>(exch_struct_type);
          next_event.time_ = generic_mds_message_.time_;
          memcpy((void*)&(generic_mds_message_.generic_data_.eobi_pf_data_), (void*)&next_event,
                 sizeof(EUREX_MDS::EUREXCommonStruct));
        } break;
        case HFSAT::MDS_MSG::OSE_ITCH_PF: {
          OSE_ITCH_MDS::OSEPFCommonStruct next_event;
          int pkt_sz = sizeof(OSE_ITCH_MDS::OSEPFCommonStruct);
          memcpy(&next_event, ptr, pkt_sz);
          ptr += pkt_sz;
          read_len -= pkt_sz;
          SetTimeStamp(next_event.time_.tv_sec, next_event.time_.tv_usec);

          generic_mds_message_.mds_msg_exch_ = static_cast<HFSAT::MDS_MSG::MDSMessageExchType>(exch_struct_type);
          next_event.time_ = generic_mds_message_.time_;
          memcpy((void*)&(generic_mds_message_.generic_data_.ose_itch_pf_data_), (void*)&next_event,
                 sizeof(OSE_ITCH_MDS::OSEPFCommonStruct));
        } break;
        case HFSAT::MDS_MSG::HKOMDPF: {
          HKOMD_MDS::HKOMDPFCommonStruct next_event;
          int pkt_sz = sizeof(HKOMD_MDS::HKOMDPFCommonStruct);
          memcpy(&next_event, ptr, pkt_sz);
          ptr += pkt_sz;
          read_len -= pkt_sz;
          SetTimeStamp(next_event.time_.tv_sec, next_event.time_.tv_usec);

          generic_mds_message_.mds_msg_exch_ = static_cast<HFSAT::MDS_MSG::MDSMessageExchType>(exch_struct_type);
          next_event.time_ = generic_mds_message_.time_;
          memcpy((void*)&(generic_mds_message_.generic_data_.hkomd_pf_data_), (void*)&next_event,
                 sizeof(HKOMD_MDS::HKOMDPFCommonStruct));
        } break;
        case HFSAT::MDS_MSG::NSE: {
          NSE_MDS::NSEDotexOfflineCommonStruct next_event;
          NSE_MDS::NSETBTDataCommonStruct tbt_struct;
          int pkt_sz = sizeof(NSE_MDS::NSEDotexOfflineCommonStruct);
          memcpy(&next_event, ptr, pkt_sz);
          ptr += pkt_sz;
          read_len -= pkt_sz;
          HFSAT::Utils::ConvertNSELiveStructToSimStruct(next_event, &tbt_struct, date_);
          SetTimeStamp(tbt_struct.source_time.tv_sec, tbt_struct.source_time.tv_usec);

          generic_mds_message_.mds_msg_exch_ = static_cast<HFSAT::MDS_MSG::MDSMessageExchType>(exch_struct_type);
          tbt_struct.source_time = generic_mds_message_.time_;
          memcpy((void*)&(generic_mds_message_.generic_data_.nse_data_), (void*)&tbt_struct,
                 sizeof(NSE_MDS::NSETBTDataCommonStruct));
        } break;

        default: {
          DBGLOG_CLASS_FUNC_LINE_ERROR << "UNEXPECTED DATA SOURCE TYPE : " << exch_struct_type << " DISCARDING PACKET "
                                       << DBGLOG_ENDL_FLUSH;
          DBGLOG_DUMP;
        } break;
      }

      //      std::cout << "GENERIC Msg: " << generic_mds_message_.ToString() << std::endl;
      //      std::cout << generic_mds_message_.GetExchangeSourceStringFromGenericStruct() << " "
      //                << generic_mds_message_.time_.tv_sec << "." << generic_mds_message_.time_.tv_usec << std::endl;
      mds_shm_interface_.WriteGenericStruct(&generic_mds_message_,
                                            false);  // Shm Write Call, Handled By A Separate Utility Class
    }
  }

  void SetTimeStamp(uint64_t tv_sec, uint64_t tv_usec) {
    if (is_first_packet_) {
      is_first_packet_ = false;

      ttime_t temp_time((long int)tv_sec, 0);
      date_ = HFSAT::DateTime::Get_UTC_YYYYMMDD_from_ttime(temp_time);
      //	generic_mds_message_.time_.tv_sec = HFSAT::DateTime::GetTimeUTC(
      //    	HFSAT::DateTime::GetCurrentIsoDateLocal(), HFSAT::DateTime::GetUTCHHMMFromTime(temp_time.tv_sec));
      gettimeofday(&(generic_mds_message_.time_), NULL);
      last_packet_time_ = generic_mds_message_.time_.tv_sec * 1000000 + generic_mds_message_.time_.tv_usec;
      last_sim_data_time_ = tv_sec * 1000000 + tv_usec;

      std::cout << "FirstPacket: Orig: " << tv_sec << "." << tv_usec << " Curr: " << generic_mds_message_.time_.tv_sec
                << "." << generic_mds_message_.time_.tv_usec << " LastSim: " << last_sim_data_time_
                << " DataDate: " << date_ << " UTC_HHMM: " << HFSAT::DateTime::GetUTCHHMMFromTime(temp_time.tv_sec)
                << std::endl;
    } else {
      uint64_t new_time = tv_sec * 1000000 + tv_usec;
      uint64_t time_diff = new_time - last_sim_data_time_;
      last_sim_data_time_ = new_time;

      if (time_diff > 0) {
        last_packet_time_ += time_diff;
      }

      generic_mds_message_.time_.tv_sec = last_packet_time_ / 1000000;
      generic_mds_message_.time_.tv_usec = last_packet_time_ % 1000000;

      //      std::cout << "NewOrigTime: " << tv_sec << "." << tv_usec << " NewOrigIntTime: " << last_sim_data_time_
      //                << " NewProcessedTime: " << generic_mds_message_.time_.tv_sec << "."
      //                << generic_mds_message_.time_.tv_usec << " NewProcessedIntTime: " << last_packet_time_
      //                << " TimeDiff: " << time_diff << std::endl;
    }
  }

  void CleanUp() {
    if (multicast_receiver_socket_ != nullptr) {
      delete multicast_receiver_socket_;
      multicast_receiver_socket_ = nullptr;
    }
  }
};
}
}
