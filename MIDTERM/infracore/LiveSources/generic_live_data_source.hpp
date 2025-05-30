// =====================================================================================
//
//       Filename:  generic_live_data_source.hpp
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
#include "dvccode/Utils/load_low_bandwidth_code_mapping.hpp"
#include "dvccode/Utils/rdtscp_timer.hpp"

#define MAX_LIVE_SOCKETS_FD 1024

namespace HFSAT {
namespace MDSMessages {

template <class T>  // A Generilized Class
class GenericLiveDataSource : public HFSAT::SimpleExternalDataLiveListener,
                              public LiveProductsManagerListener {  // A Simple Listener Of Event Dispatcher

 private:
  DebugLogger& dbglogger_;
  const HFSAT::SecurityNameIndexer& sec_name_indexer_;                          // Unused Right Now, Can be Used Later
  std::vector<HFSAT::MulticastReceiverSocket*> multicast_receiver_socket_vec_;  // Multicast Socket Reader
  std::vector<int32_t> socket_fd_to_actual_socket_index_map_;
  T next_event_;            // The Underlying Common Struct
  const int32_t pkt_size_;  // So That we don't keep calling sizeof

  // These are used for generic write
  HFSAT::Utils::MDSShmInterface& mds_shm_interface_;
  HFSAT::MDS_MSG::MDSMessageExchType source_md_exch_type_;
  bool is_primary_location_and_should_give_priority_;
  HFSAT::MDS_MSG::GenericMDSMessage generic_mds_message_;
  HFSAT::FastMdConsumerMode_t mode_;
  MDSLogger<HFSAT::MDS_MSG::GenericMDSMessage>* mds_logger_;

  int32_t socket_read_size_;         // Avoiding Local Variables Is The First Step Towards Improving Cache-Miss
  std::map<int, int> previous_uid_;  // used to store the uid sent in the previous msg for this channel (important to
                                     // discard duplicates in case of AFLASH)

  uint32_t eobi_special_arbitration_seq_;

  HFSAT::EnhancedSecurityNameIndexer requested_products_indexer_;
  HFSAT::Utils::CombinedSourceGenericLogger& generic_logger_;

  // These Are Specific Handling WHich We Need to Convert the Low Bandwidth Structs Sent Over Multicast to The Original
  // Common Struct Form For the Compatibility Reasons
  std::map<uint8_t, std::string> eurex_product_code_to_shortcode_;
  std::map<std::string, std::string> eurex_shortcode_to_exchange_symbol_;

  std::map<uint8_t, std::string> cme_product_code_to_shortcode_;
  std::map<std::string, std::string> cme_shortcode_to_exchange_symbol_;

  EUREX_MDS::EUREXCommonStruct eurex_cstr_;
  CME_MDS::CMECommonStruct cme_cstr_;

  HFSAT::ClockSource& clock_source_;
  bool using_simulated_clocksource_;

  bool using_shm_for_ors_reply_;
  bool is_overclocked_server_;
  bool is_midterm_legacy_server_;

 public:
  GenericLiveDataSource(DebugLogger& _dbglogger_, HFSAT::SecurityNameIndexer& _sec_name_indexer_,
                        const HFSAT::MDS_MSG::MDSMessageExchType& _source_md_exch_type_,
                        bool _is_primary_location_and_should_give_priority_ = false,
                        HFSAT::FastMdConsumerMode_t mode = HFSAT::kComShm)
      : dbglogger_(_dbglogger_),
        sec_name_indexer_(_sec_name_indexer_),
        multicast_receiver_socket_vec_(),
        socket_fd_to_actual_socket_index_map_(),
        next_event_(),
        pkt_size_(sizeof(T)),
        mds_shm_interface_(HFSAT::Utils::MDSShmInterface::GetUniqueInstance()),
        source_md_exch_type_(_source_md_exch_type_),
        is_primary_location_and_should_give_priority_(_is_primary_location_and_should_give_priority_),
        generic_mds_message_(),
        mode_(mode),
        mds_logger_(new MDSLogger<HFSAT::MDS_MSG::GenericMDSMessage>("GENERIC")),
        socket_read_size_(-1),
        previous_uid_(),
        eobi_special_arbitration_seq_(0),
        generic_logger_(HFSAT::Utils::CombinedSourceGenericLogger::GetUniqueInstance()),
        eurex_cstr_(),
        cme_cstr_(),
        clock_source_(HFSAT::ClockSource::GetUniqueInstance()),
        using_simulated_clocksource_(clock_source_.AreWeUsingSimulatedClockSource()),
        using_shm_for_ors_reply_(HFSAT::UseShmforORSReply()),
        is_overclocked_server_(HFSAT::IsItOverClockedServer()),
        is_midterm_legacy_server_(HFSAT::IsItMidTermLegacyServer()) {

          std::cout << " MID TERM ? : " << is_midterm_legacy_server_ << std::endl ;

    if (mode_ == HFSAT::kLogger) {
      mds_logger_->run();
    }
    if (mode_ == HFSAT::kComShm) {
      HFSAT::LiveProductsManager::GetUniqueInstance().AddListener(this);
    }

    // Reset Memory
    memset((void*)&generic_mds_message_, 0, sizeof(HFSAT::MDS_MSG::GenericMDSMessage));
    // Don't Need to Copy Everytime
    generic_mds_message_.mds_msg_exch_ = source_md_exch_type_;

    socket_fd_to_actual_socket_index_map_.resize(MAX_LIVE_SOCKETS_FD, -1);

    // Load the Simple Utility Class Which Know How to Convert the Low Bandwidth Struct To The Original Compatible
    // Formaat
    if (source_md_exch_type_ == HFSAT::MDS_MSG::EUREX_LS || source_md_exch_type_ == HFSAT::MDS_MSG::EOBI_LS) {
      HFSAT::Utils::LowBWCodeMappingLoader::LoadMappingForGivenFile(
          DEF_LS_PRODUCTCODE_SHORTCODE_, eurex_product_code_to_shortcode_, eurex_shortcode_to_exchange_symbol_);
    }

    if (source_md_exch_type_ == HFSAT::MDS_MSG::CME_LS) {
      HFSAT::Utils::LowBWCodeMappingLoader::LoadMappingForGivenFile(
          DEF_CME_LS_PRODUCTCODE_SHORTCODE_, cme_product_code_to_shortcode_, cme_shortcode_to_exchange_symbol_);
    }
  }

  // A call to include all live sockets
  void AddLiveSocket(const std::string& _mcast_ip_, const int32_t& _mcast_port_, const std::string& _interface_) {
    HFSAT::MulticastReceiverSocket* new_multicast_receiver_socket =
        new HFSAT::MulticastReceiverSocket(_mcast_ip_, _mcast_port_, _interface_);
    multicast_receiver_socket_vec_.push_back(new_multicast_receiver_socket);

    // Check If the FD -> index of the socket can be stored directly or we have to increase the capacitiy
    if ((int32_t)socket_fd_to_actual_socket_index_map_.size() <
        new_multicast_receiver_socket->socket_file_descriptor()) {
      // <= is imporant since we are not utilising the [ 0 ] element, FD can't be zero,
      for (int32_t start_ptr = socket_fd_to_actual_socket_index_map_.size();
           start_ptr <= (new_multicast_receiver_socket->socket_file_descriptor()); start_ptr++) {
        socket_fd_to_actual_socket_index_map_.push_back(-1);
      }
    }

    // At this point the capacity has been set correctly for storing the FD -> socket_index
    socket_fd_to_actual_socket_index_map_[new_multicast_receiver_socket->socket_file_descriptor()] =
        (multicast_receiver_socket_vec_.size() - 1);
  }

  // Fill Up All the socket fd so the caller can add the same to it's event dispatch loop
  void GetLiveSocketsFdList(std::vector<int32_t>& _all_active_socket_fd_vec_) {
    for (uint32_t socket_counter = 0; socket_counter < multicast_receiver_socket_vec_.size(); socket_counter++) {
      _all_active_socket_fd_vec_.push_back(multicast_receiver_socket_vec_[socket_counter]->socket_file_descriptor());
      multicast_receiver_socket_vec_[socket_counter]->SetNonBlocking();
    }
  }

  // Overridden Callback - A listenr of the event dispatcher
  inline void ProcessAllEvents(int32_t _socket_fd_with_data_) {
    while (true) {
      // Get the correct socket from which to read
      socket_read_size_ =
          multicast_receiver_socket_vec_[socket_fd_to_actual_socket_index_map_[_socket_fd_with_data_]]->ReadN(
              pkt_size_, (void*)&next_event_);

      // ERROR handling - We expect this to happen as this is the only way of getting out of loop, hence not printing it
      // as error
      if (pkt_size_ > socket_read_size_) {
        //            DBGLOG_CLASS_FUNC_LINE_ERROR << "SOCKET READ RETURNED : " << socket_read_size_ << "
        //            SYSTEM_ERROR_DESCRIPTION -> " << strerror ( errno ) << DBGLOG_ENDL_FLUSH ;
        //            DBGLOG_DUMP ;

        return;
      }

      // Timestamping
      if (true == using_simulated_clocksource_) {
        generic_mds_message_.time_ = clock_source_.GetTimeOfDay();
      } else {
        gettimeofday(&(generic_mds_message_.time_), NULL);
      }

      // Process the Data Based on Underlying Exchange
      switch (source_md_exch_type_) {
        case HFSAT::MDS_MSG::CME: {
          memcpy((void*)&(generic_mds_message_.generic_data_.cme_ls_data_), (void*)&next_event_, pkt_size_);
        } break;
        case HFSAT::MDS_MSG::EUREX: {
          memcpy((void*)&(generic_mds_message_.generic_data_.eurex_data_), (void*)&next_event_, pkt_size_);
          generic_mds_message_.generic_data_.eurex_data_.time_ = generic_mds_message_.time_;
        } break;
        case HFSAT::MDS_MSG::TMX:
        case HFSAT::MDS_MSG::TMX_LS: {
          memcpy((void*)&(generic_mds_message_.generic_data_.tmx_data_), (void*)&next_event_, pkt_size_);
        } break;
        case HFSAT::MDS_MSG::BMF: {
          memcpy((void*)&(generic_mds_message_.generic_data_.bmf_data_), (void*)&next_event_, pkt_size_);
          generic_mds_message_.generic_data_.bmf_data_.time_ = generic_mds_message_.time_;
        } break;
        case HFSAT::MDS_MSG::NTP: {
          memcpy((void*)&(generic_mds_message_.generic_data_.ntp_data_), (void*)&next_event_, pkt_size_);
        } break;
        case HFSAT::MDS_MSG::LIFFE: {
          memcpy((void*)&(generic_mds_message_.generic_data_.liffe_data_), (void*)&next_event_, pkt_size_);
          generic_mds_message_.generic_data_.liffe_data_.time_ = generic_mds_message_.time_;
        } break;
        case HFSAT::MDS_MSG::CME_LS: {
          memcpy((void*)&(generic_mds_message_.generic_data_.cme_ls_data_), (void*)&next_event_, pkt_size_);
          generic_mds_message_.mds_msg_exch_ = source_md_exch_type_;

        } break;
        case HFSAT::MDS_MSG::EUREX_LS: {
          memcpy((void*)&(generic_mds_message_.generic_data_.eurex_ls_data_), (void*)&next_event_, pkt_size_);
          generic_mds_message_.mds_msg_exch_ = source_md_exch_type_;

        } break;

        case HFSAT::MDS_MSG::OSE_ITCH_PF: {
          memcpy((void*)&(generic_mds_message_.generic_data_.ose_itch_pf_data_), (void*)&next_event_, pkt_size_);
          generic_mds_message_.generic_data_.ose_itch_pf_data_.time_ = generic_mds_message_.time_;
        } break;

        case HFSAT::MDS_MSG::RTS: {
          memcpy((void*)&(generic_mds_message_.generic_data_.rts_data_), (void*)&next_event_, pkt_size_);
          generic_mds_message_.generic_data_.rts_data_.time_ = generic_mds_message_.time_;

        } break;
        case HFSAT::MDS_MSG::MICEX: {
          memcpy((void*)&(generic_mds_message_.generic_data_.micex_data_), (void*)&next_event_, pkt_size_);
          generic_mds_message_.generic_data_.micex_data_.time_ = generic_mds_message_.time_;
        } break;
        case HFSAT::MDS_MSG::EOBI_LS: {  // Handling here instead of at clients end to save some latency
          uint32_t this_msg_seq = ((EUREX_MDS::EUREXLSCommonStruct*)(&next_event_))->msg_sequence_;

          if (1 == this_msg_seq) {  // Reset - This is to take care of the case when Source is restarted
            eobi_special_arbitration_seq_ = 0;
          }

          // Packet Already seen
          if (this_msg_seq <= eobi_special_arbitration_seq_) return;  // No need to process this
          memcpy((void*)&(generic_mds_message_.generic_data_.eobi_ls_data_), (void*)&next_event_, pkt_size_);
          generic_mds_message_.mds_msg_exch_ = source_md_exch_type_;
          eobi_special_arbitration_seq_ = this_msg_seq;  // Update SeqNumber

        } break;
        case HFSAT::MDS_MSG::CONTROL: {
          memcpy((void*)&(generic_mds_message_.generic_data_.control_data_), (void*)&next_event_, pkt_size_);
          generic_mds_message_.generic_data_.control_data_.time_set_by_frontend_ = generic_mds_message_.time_;
        } break;
        case HFSAT::MDS_MSG::ORS_REPLY: {
          memcpy((void*)&(generic_mds_message_.generic_data_.ors_reply_data_), (void*)&next_event_, pkt_size_);
          if ( true == is_midterm_legacy_server_ ){
            generic_mds_message_.generic_data_.ors_reply_data_.time_set_by_server_ = generic_mds_message_.time_ ;
          }

        } break;
        case HFSAT::MDS_MSG::EOBI_PF: {
          memcpy((void*)&(generic_mds_message_.generic_data_.eobi_pf_data_), (void*)&next_event_, pkt_size_);
          generic_mds_message_.generic_data_.eobi_pf_data_.time_ = generic_mds_message_.time_;
        } break;
        case HFSAT::MDS_MSG::LIFFE_LS: {
          memcpy((void*)&(generic_mds_message_.generic_data_.liffe_data_), (void*)&next_event_, pkt_size_);
          generic_mds_message_.generic_data_.liffe_data_.time_ = generic_mds_message_.time_;
        } break;
        case HFSAT::MDS_MSG::BMF_EQ: {
          memcpy((void*)&(generic_mds_message_.generic_data_.bmf_eq_data_), (void*)&next_event_, pkt_size_);
          generic_mds_message_.generic_data_.bmf_eq_data_.time_ = generic_mds_message_.time_;
        } break;
        case HFSAT::MDS_MSG::CSM: {
          memcpy((void*)&(generic_mds_message_.generic_data_.csm_data_), (void*)&next_event_, pkt_size_);
          generic_mds_message_.generic_data_.csm_data_.time_ = generic_mds_message_.time_;
        } break;

        case HFSAT::MDS_MSG::ICE: {
          memcpy((void*)&(generic_mds_message_.generic_data_.ice_data_), (void*)&next_event_, pkt_size_);
          generic_mds_message_.generic_data_.ice_data_.time_ = generic_mds_message_.time_;
        } break;
        case HFSAT::MDS_MSG::ICE_LS: {
          memcpy((void*)&(generic_mds_message_.generic_data_.ice_data_), (void*)&next_event_, pkt_size_);
          generic_mds_message_.generic_data_.ice_data_.time_ = generic_mds_message_.time_;
        } break;
        case HFSAT::MDS_MSG::HKOMDPF: {
          memcpy((void*)&(generic_mds_message_.generic_data_.hkomd_pf_data_), (void*)&next_event_, pkt_size_);
          generic_mds_message_.generic_data_.hkomd_pf_data_.time_ = generic_mds_message_.time_;
        } break;
        case HFSAT::MDS_MSG::AFLASH: {
          memcpy((void*)&(generic_mds_message_.generic_data_.aflash_data_), (void*)&next_event_, pkt_size_);
          if (previous_uid_[_socket_fd_with_data_] == generic_mds_message_.generic_data_.aflash_data_.uid_) {
            // consecutive duplicates => discard pkt
            continue;
          }
          generic_mds_message_.generic_data_.aflash_data_.time_ = generic_mds_message_.time_;
          previous_uid_[_socket_fd_with_data_] = generic_mds_message_.generic_data_.aflash_data_.uid_;

        } break;
        case HFSAT::MDS_MSG::RETAIL: {
          memcpy((void*)&(generic_mds_message_.generic_data_.retail_data_), (void*)&next_event_, pkt_size_);
          generic_mds_message_.generic_data_.retail_data_.time_ = generic_mds_message_.time_;
        } break;
        case HFSAT::MDS_MSG::NTP_LS: {  // Using NTP as it is is fine currently since it's either used raw or used live
                                        // but not mixed
          memcpy((void*)&(generic_mds_message_.generic_data_.ntp_data_), (void*)&next_event_, pkt_size_);
          generic_mds_message_.generic_data_.ntp_data_.time_ = generic_mds_message_.time_;
        } break;

        case HFSAT::MDS_MSG::SGX: {
          memcpy((void*)&(generic_mds_message_.generic_data_.sgx_data_), (void*)&next_event_, pkt_size_);
          generic_mds_message_.generic_data_.sgx_data_.time_ = generic_mds_message_.time_;
        } break;

        case HFSAT::MDS_MSG::TMX_OBF: {
          memcpy((void*)&(generic_mds_message_.generic_data_.tmx_obf_data_), (void*)&next_event_, pkt_size_);
          generic_mds_message_.generic_data_.tmx_obf_data_.time_ = generic_mds_message_.time_;
        } break;

        case HFSAT::MDS_MSG::NSE_L1: {
          memcpy((void*)&(generic_mds_message_.generic_data_.nse_l1_data_), (void*)&next_event_, pkt_size_);
        } break;

        default: {
          DBGLOG_CLASS_FUNC_LINE_ERROR << "UNEXPECTED DATA SOURCE TYPE : " << source_md_exch_type_
                                       << " DISCARDING PACKET " << DBGLOG_ENDL_FLUSH;
          DBGLOG_DUMP;
        } break;
      }

      if (mode_ == HFSAT::kLogger)
        mds_logger_->log(generic_mds_message_);

      else {
        // Handle ORS reply separately as writing to SHM is conditional and moreover there is a race condition:
        // (https://app.asana.com/0/211978977921812/399853641763115)
        if (source_md_exch_type_ == HFSAT::MDS_MSG::ORS_REPLY) {
          if (!using_shm_for_ors_reply_) {
            mds_shm_interface_.WriteGenericStruct(&generic_mds_message_,
                                                  false);  // Shm Write Call, Handled By A Separate Utility Class
          } else {
//            if (is_overclocked_server_) {
//              generic_mds_message_.generic_data_.ors_reply_data_.time_set_by_server_.val = HFSAT::GetCpucycleCountForTimeTick();
//            }
          }

        } else if (source_md_exch_type_ == HFSAT::MDS_MSG::CONTROL || source_md_exch_type_ == HFSAT::MDS_MSG::AFLASH ||
                   source_md_exch_type_ == HFSAT::MDS_MSG::CME_LS || source_md_exch_type_ == HFSAT::MDS_MSG::EUREX_LS ||
                   source_md_exch_type_ == HFSAT::MDS_MSG::EOBI_LS || source_md_exch_type_ == HFSAT::MDS_MSG::NSE_L1 ||
                   source_md_exch_type_ == HFSAT::MDS_MSG::RETAIL ||
                   requested_products_indexer_.GetIdFromSecname(generic_mds_message_.getContract()) >= 0) {
          /* Note that getContract call has been intentionally put to last of OR condition for two reasons:
          * 1) To avoid unnecessary expensive call to getContract in case of exchanges where we write all data to SHM
          * 2) To avoid race condition for ORS-reply struct (https://app.asana.com/0/211978977921812/399853641763115)
          */
          mds_shm_interface_.WriteGenericStruct(&generic_mds_message_,
                                                false);  // Shm Write Call, Handled By A Separate Utility Class
        }

        if (source_md_exch_type_ == HFSAT::MDS_MSG::CME_LS) {
          HFSAT::Utils::LowBWCodeMappingLoader::GetCMEStructFromLowBWStruct(
              cme_product_code_to_shortcode_, cme_shortcode_to_exchange_symbol_, cme_cstr_,
              *((CME_MDS::CMELSCommonStruct*)&next_event_));

          cme_cstr_.time_ = generic_mds_message_.time_;
          generic_mds_message_.mds_msg_exch_ = HFSAT::MDS_MSG::CME;
          memcpy(&(generic_mds_message_.generic_data_.cme_data_), &(cme_cstr_), sizeof(CME_MDS::CMECommonStruct));
        } else if (source_md_exch_type_ == HFSAT::MDS_MSG::EUREX_LS ||
                   source_md_exch_type_ == HFSAT::MDS_MSG::EOBI_LS) {
          HFSAT::Utils::LowBWCodeMappingLoader::GetEurexStructFromLowBWStruct(
              eurex_product_code_to_shortcode_, eurex_shortcode_to_exchange_symbol_, eurex_cstr_,
              *((EUREX_MDS::EUREXLSCommonStruct*)&next_event_));

          eurex_cstr_.time_ = generic_mds_message_.time_;

          generic_mds_message_.mds_msg_exch_ = HFSAT::MDS_MSG::EUREX;
          memcpy(&(generic_mds_message_.generic_data_.eurex_data_), &(eurex_cstr_),
                 sizeof(EUREX_MDS::EUREXCommonStruct));
        }
        generic_logger_.Log(generic_mds_message_);
      }

      // Have Processed 1 Event, If Non Priority Source Return From This Point
      if (false == is_primary_location_and_should_give_priority_) break;
    }
  }

  void OnLiveProductChange(LiveProductsChangeAction action, std::string exchange_symbol, std::string shortcode,
                           ExchSource_t exchange) override {
    if (mode_ == HFSAT::kComShm) {
      if (action == LiveProductsChangeAction::kAdd) {
        std::cout << "GenericRavLiveDataSource Added " << shortcode << std::endl;
        requested_products_indexer_.AddString(exchange_symbol.c_str(), shortcode);
      } else if (action == LiveProductsChangeAction::kRemove) {
        std::cout << "GenericRavLiveDataSource Removed " << shortcode << std::endl;
        requested_products_indexer_.RemoveString(exchange_symbol.c_str(), shortcode);
      }
    }
  }

  void CleanUp() {
    for (uint32_t socket_counter = 0; socket_counter < multicast_receiver_socket_vec_.size(); socket_counter++) {
      if (NULL != multicast_receiver_socket_vec_[socket_counter]) {
        delete multicast_receiver_socket_vec_[socket_counter];
        multicast_receiver_socket_vec_[socket_counter] = NULL;
      }
    }
    if (mds_logger_->isRunning()) mds_logger_->stop();
  }
};
}
}
