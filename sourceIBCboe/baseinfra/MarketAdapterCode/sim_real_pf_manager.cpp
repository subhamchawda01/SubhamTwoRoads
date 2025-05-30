/**
   \file MarketAdapter/sim_real_pf_manager.cpp

   \author: (c) Copyright Two Roads Technological Solutions Pvt Ltd 2011
   Address:
   Suite No 162, Evoma, #14, Bhattarhalli,
   Old Madras Road, Near Garden City College,
   KR Puram, Bangalore 560049, India
   +91 80 4190 3551
 */
#include "dvccode/CDef/stored_market_data_common_message_defines.hpp"
#include "dvccode/CommonTradeUtils/live_struct_sim_struct.hpp"
#include "baseinfra/MarketAdapter/sim_real_pf_manager.hpp"

#include <string>

namespace HFSAT {

template <>
void SimRealPFManager::ReadAndCompareRealData<HFSAT::GenericORSReplyStructLive>(
    void *ptr_to_price_level_update, int length_of_bytes, BulkFileReader &bulk_file_reader_,
    HFSAT::MDS_MSG::MDSMessageExchType real_exch_type, HFSAT::MDS_MSG::MDSMessageExchType sim_exch_type) {
  size_t mds_available_len;
  HFSAT::GenericORSReplyStructLive next_live_real_event;
  mds_available_len = bulk_file_reader_.read(&next_live_real_event, sizeof(HFSAT::GenericORSReplyStructLive));
  HFSAT::GenericORSReplyStruct next_real_event;
  HFSAT::Utils::ConvertORSLiveStructToSimStruct(next_live_real_event, next_real_event);
  if (length_of_bytes != sizeof(HFSAT::GenericORSReplyStruct)) {
    std::cerr << "Size of the Sim & Real structs mismatch \n";
    exit(1);
  } else if (mds_available_len < sizeof(HFSAT::GenericORSReplyStructLive)) {
    std::cerr << "Unable to read the Real struct \n";
    exit(1);
  } else {
    if (next_real_event.ToString() != (*((HFSAT::GenericORSReplyStruct *)ptr_to_price_level_update)).ToString()) {
      std::cerr << "Sim Real Packet Order mismatch detected \n";
      exit(1);
    }
  }
}

template <>
void SimRealPFManager::ReadAndCompareRealData<ICE_MDS::ICECommonStructLive>(
    void *ptr_to_price_level_update, int length_of_bytes, BulkFileReader &bulk_file_reader_,
    HFSAT::MDS_MSG::MDSMessageExchType real_exch_type, HFSAT::MDS_MSG::MDSMessageExchType sim_exch_type) {
  size_t mds_available_len;
  ICE_MDS::ICECommonStructLive next_live_real_event;
  mds_available_len = bulk_file_reader_.read(&next_live_real_event, sizeof(ICE_MDS::ICECommonStructLive));
  ICE_MDS::ICECommonStruct next_real_event;
  HFSAT::Utils::ConvertICELiveStructToSimStruct(next_live_real_event, next_real_event);

  if (length_of_bytes != sizeof(ICE_MDS::ICECommonStruct)) {
    std::cerr << "Size of the Sim & Real structs mismatch \n";
    exit(1);
  } else if (mds_available_len < sizeof(ICE_MDS::ICECommonStructLive)) {
    std::cerr << "Unable to read the Real struct \n";
    exit(1);
  } else {
    if (!memcmp(ptr_to_price_level_update, &next_real_event, length_of_bytes)) {
      std::cerr << "Sim Real Packet Order mismatch detected \n";
    }
  }
}

void SimRealPFManager::RemoveMatchedPackets() {
  for (auto sim_it = sim_mismatch_vec.begin(); sim_it != sim_mismatch_vec.end();) {
    bool sim_data_deleted = false;
    SimRealPacketinfo *sim_data_ptr = *sim_it;
    for (auto real_it = real_mismatch_vec.begin(); real_it != real_mismatch_vec.end();) {
      SimRealPacketinfo *real_data_ptr = *real_it;
      if (sim_data_ptr->exch_type_ == real_data_ptr->exch_type_ &&
          sim_data_ptr->size_of_struct_ == real_data_ptr->size_of_struct_) {
        if (!memcmp(sim_data_ptr->ptr_, real_data_ptr->ptr_, sim_data_ptr->size_of_struct_)) {
          free(sim_data_ptr->ptr_);
          free(real_data_ptr->ptr_);
          free(sim_data_ptr);
          free(real_data_ptr);

          real_it = real_mismatch_vec.erase(real_it);
          sim_it = sim_mismatch_vec.erase(sim_it);
          sim_data_deleted = true;
          break;
        }
      }
      ++real_it;
    }
    if (!sim_data_deleted) {
      ++sim_it;
    }
  }
}

timeval SimRealPFManager::GetPacketTimeStamp(void *ptr_to_sim_real_data, HFSAT::MDS_MSG::MDSMessageExchType exch_type) {
  timeval packet_time_stamp{0, 0};

  switch (exch_type) {
    case HFSAT::MDS_MSG::CME_LS:
    case HFSAT::MDS_MSG::CME: {
      CME_MDS::CMECommonStruct sim_real_data = *((CME_MDS::CMECommonStruct *)ptr_to_sim_real_data);
      packet_time_stamp = sim_real_data.time_;
    } break;
    case HFSAT::MDS_MSG::LIFFE:
    case HFSAT::MDS_MSG::LIFFE_LS: {
      LIFFE_MDS::LIFFECommonStruct sim_real_data = *((LIFFE_MDS::LIFFECommonStruct *)ptr_to_sim_real_data);
      packet_time_stamp = sim_real_data.time_;
    } break;
    case HFSAT::MDS_MSG::EUREX_LS:
    case HFSAT::MDS_MSG::EUREX:
    case HFSAT::MDS_MSG::EOBI_LS: {
      EUREX_MDS::EUREXCommonStruct sim_real_data = *((EUREX_MDS::EUREXCommonStruct *)ptr_to_sim_real_data);
      packet_time_stamp = sim_real_data.time_;
    } break;
    case HFSAT::MDS_MSG::CONTROL: {
      HFSAT::GenericControlRequestStruct sim_real_data = *((HFSAT::GenericControlRequestStruct *)ptr_to_sim_real_data);
      packet_time_stamp.tv_sec = sim_real_data.time_set_by_frontend_.tv_sec;
      packet_time_stamp.tv_usec = sim_real_data.time_set_by_frontend_.tv_usec;
    } break;
    case HFSAT::MDS_MSG::ORS_REPLY: {
      HFSAT::GenericORSReplyStructLive sim_real_data = *((HFSAT::GenericORSReplyStructLive *)ptr_to_sim_real_data);
      packet_time_stamp.tv_sec = sim_real_data.time_set_by_server_.tv_sec;
      packet_time_stamp.tv_usec = sim_real_data.time_set_by_server_.tv_usec;
    } break;
    case HFSAT::MDS_MSG::RTS: {
      RTS_MDS::RTSCommonStruct sim_real_data = *((RTS_MDS::RTSCommonStruct *)ptr_to_sim_real_data);
      packet_time_stamp = sim_real_data.time_;
    } break;
    // Same Handling for MICEX_CR and MICEX_EQ
    case HFSAT::MDS_MSG::MICEX: {
      MICEX_MDS::MICEXCommonStruct sim_real_data = *((MICEX_MDS::MICEXCommonStruct *)ptr_to_sim_real_data);
      packet_time_stamp = sim_real_data.time_;
    } break;
    case HFSAT::MDS_MSG::NTP:
    case HFSAT::MDS_MSG::NTP_LS: {
      NTP_MDS::NTPCommonStruct sim_real_data = *((NTP_MDS::NTPCommonStruct *)ptr_to_sim_real_data);
      packet_time_stamp = sim_real_data.time_;
    } break;
    case HFSAT::MDS_MSG::BMF_EQ: {  // BMF_EQ and NTP have same common struct
      NTP_MDS::NTPCommonStruct sim_real_data = *((NTP_MDS::NTPCommonStruct *)ptr_to_sim_real_data);
      packet_time_stamp = sim_real_data.time_;
    } break;
    case HFSAT::MDS_MSG::EOBI_PF: {
      EUREX_MDS::EUREXCommonStruct sim_real_data = *((EUREX_MDS::EUREXCommonStruct *)ptr_to_sim_real_data);
      packet_time_stamp = sim_real_data.time_;
    } break;
    case HFSAT::MDS_MSG::OSE_PF: {
      OSE_MDS::OSEPriceFeedCommonStruct sim_real_data = *((OSE_MDS::OSEPriceFeedCommonStruct *)ptr_to_sim_real_data);
      packet_time_stamp = sim_real_data.time_;
    } break;
    case HFSAT::MDS_MSG::OSE_CF: {
      OSE_MDS::OSECombinedCommonStruct sim_real_data = *((OSE_MDS::OSECombinedCommonStruct *)ptr_to_sim_real_data);
      packet_time_stamp = sim_real_data.time_;
    } break;
    case HFSAT::MDS_MSG::CSM: {
      CSM_MDS::CSMCommonStruct sim_real_data = *((CSM_MDS::CSMCommonStruct *)ptr_to_sim_real_data);
      packet_time_stamp = sim_real_data.time_;
    } break;
    case HFSAT::MDS_MSG::OSE_L1: {
      OSE_MDS::OSEPLCommonStruct sim_real_data = *((OSE_MDS::OSEPLCommonStruct *)ptr_to_sim_real_data);
      packet_time_stamp = sim_real_data.time_;
    } break;
    case HFSAT::MDS_MSG::TMX:
    case HFSAT::MDS_MSG::TMX_LS: {
      packet_time_stamp = timeval{0, 0};
    } break;
    case HFSAT::MDS_MSG::TMX_OBF: {
      TMX_OBF_MDS::TMXPFCommonStruct sim_real_data = *((TMX_OBF_MDS::TMXPFCommonStruct *)ptr_to_sim_real_data);
      packet_time_stamp = sim_real_data.time_;
    } break;
    case HFSAT::MDS_MSG::ICE:
    case HFSAT::MDS_MSG::ICE_LS: {
      ICE_MDS::ICECommonStructLive sim_real_data = *((ICE_MDS::ICECommonStructLive *)ptr_to_sim_real_data);
      packet_time_stamp = sim_real_data.time_;
    } break;
    case HFSAT::MDS_MSG::ASX: {
      ASX_MDS::ASXPFCommonStruct sim_real_data = *((ASX_MDS::ASXPFCommonStruct *)ptr_to_sim_real_data);
      packet_time_stamp = sim_real_data.time_;
    } break;
    case HFSAT::MDS_MSG::SGX: {
      SGX_MDS::SGXPFCommonStruct sim_real_data = *((SGX_MDS::SGXPFCommonStruct *)ptr_to_sim_real_data);
      packet_time_stamp = sim_real_data.time_;
    } break;
    case HFSAT::MDS_MSG::OSE_ITCH_PF: {
      OSE_ITCH_MDS::OSEPFCommonStruct sim_real_data = *((OSE_ITCH_MDS::OSEPFCommonStruct *)ptr_to_sim_real_data);
      packet_time_stamp = sim_real_data.time_;
    } break;
    case HFSAT::MDS_MSG::HKOMDPF: {
      HKOMD_MDS::HKOMDPFCommonStruct sim_real_data = *((HKOMD_MDS::HKOMDPFCommonStruct *)ptr_to_sim_real_data);
      packet_time_stamp = sim_real_data.time_;
    } break;
    case HFSAT::MDS_MSG::AFLASH: {
      AFLASH_MDS::AFlashCommonStructLive sim_real_data = *((AFLASH_MDS::AFlashCommonStructLive *)ptr_to_sim_real_data);
      packet_time_stamp = sim_real_data.time_;
    } break;
    case HFSAT::MDS_MSG::RETAIL: {
      RETAIL_MDS::RETAILCommonStruct sim_real_data = *((RETAIL_MDS::RETAILCommonStruct *)ptr_to_sim_real_data);
      packet_time_stamp = sim_real_data.time_;
    } break;
    case HFSAT::MDS_MSG::NSE: {
      NSE_MDS::NSETBTDataCommonStruct sim_real_data = *((NSE_MDS::NSETBTDataCommonStruct *)ptr_to_sim_real_data);
      packet_time_stamp = sim_real_data.source_time;
    } break;
    case HFSAT::MDS_MSG::NSE_L1: {
      HFSAT::GenericL1DataStruct sim_real_data = *((HFSAT::GenericL1DataStruct *)ptr_to_sim_real_data);
      packet_time_stamp.tv_sec = sim_real_data.time.tv_sec;
      packet_time_stamp.tv_usec = sim_real_data.time.tv_usec;
    } break;
    case HFSAT::MDS_MSG::EOBI_OF: {
      EOBI_MDS::EOBICompactOrder sim_real_data = *((EOBI_MDS::EOBICompactOrder *)ptr_to_sim_real_data);
      packet_time_stamp = sim_real_data.time_;
    } break;
    default: {
      std::cerr << " Sim exchange type currently not supported  "
                << "exch_type: " << exch_type << std::endl;
      exit(1);
    } break;
  }

  return packet_time_stamp;
}

SimRealPFManager::SimRealPFManager(std::string file_name_) : bulk_file_reader_(), packet_mismatch_time_{0, 0} {
  bulk_file_reader_.open(file_name_);
}

void SimRealPFManager::OnPriceLevelUpdate(void *ptr_to_price_level_update, int length_of_bytes,
                                          HFSAT::MDS_MSG::MDSMessageExchType exch_type) {
  if (bulk_file_reader_.is_open()) {
    HFSAT::MDS_MSG::MDSMessageExchType real_exch_type;
    size_t exch_available_len = bulk_file_reader_.read(&real_exch_type, sizeof(HFSAT::MDS_MSG::MDSMessageExchType));
    if (exch_available_len < sizeof(real_exch_type)) {
      return;
    }
    switch (real_exch_type) {
      case HFSAT::MDS_MSG::CME_LS:
      case HFSAT::MDS_MSG::CME: {
        ReadAndCompareRealData<CME_MDS::CMECommonStruct>(ptr_to_price_level_update, length_of_bytes, bulk_file_reader_,
                                                         real_exch_type, exch_type);
      } break;
      case HFSAT::MDS_MSG::LIFFE:
      case HFSAT::MDS_MSG::LIFFE_LS: {
        ReadAndCompareRealData<LIFFE_MDS::LIFFECommonStruct>(ptr_to_price_level_update, length_of_bytes,
                                                             bulk_file_reader_, real_exch_type, exch_type);
      } break;
      case HFSAT::MDS_MSG::EUREX_LS:
      case HFSAT::MDS_MSG::EUREX:
      case HFSAT::MDS_MSG::EOBI_LS: {
        ReadAndCompareRealData<EUREX_MDS::EUREXCommonStruct>(ptr_to_price_level_update, length_of_bytes,
                                                             bulk_file_reader_, real_exch_type, exch_type);
      } break;
      case HFSAT::MDS_MSG::CONTROL: {
        ReadAndCompareRealData<HFSAT::GenericControlRequestStruct>(ptr_to_price_level_update, length_of_bytes,
                                                                   bulk_file_reader_, real_exch_type, exch_type);
      } break;
      case HFSAT::MDS_MSG::ORS_REPLY: {
        ReadAndCompareRealData<HFSAT::GenericORSReplyStructLive>(ptr_to_price_level_update, length_of_bytes,
                                                                 bulk_file_reader_, real_exch_type, exch_type);
      } break;
      case HFSAT::MDS_MSG::RTS: {
        ReadAndCompareRealData<RTS_MDS::RTSCommonStruct>(ptr_to_price_level_update, length_of_bytes, bulk_file_reader_,
                                                         real_exch_type, exch_type);
      } break;
      // Same Handling for MICEX_CR and MICEX_EQ
      case HFSAT::MDS_MSG::MICEX: {
        ReadAndCompareRealData<MICEX_MDS::MICEXCommonStruct>(ptr_to_price_level_update, length_of_bytes,
                                                             bulk_file_reader_, real_exch_type, exch_type);
      } break;
      case HFSAT::MDS_MSG::NTP:
      case HFSAT::MDS_MSG::NTP_LS: {
        ReadAndCompareRealData<NTP_MDS::NTPCommonStruct>(ptr_to_price_level_update, length_of_bytes, bulk_file_reader_,
                                                         real_exch_type, exch_type);
      } break;
      case HFSAT::MDS_MSG::BMF_EQ: {  // BMF_EQ and NTP have same common struct
        ReadAndCompareRealData<NTP_MDS::NTPCommonStruct>(ptr_to_price_level_update, length_of_bytes, bulk_file_reader_,
                                                         real_exch_type, exch_type);
      } break;
      case HFSAT::MDS_MSG::EOBI_PF: {
        ReadAndCompareRealData<EUREX_MDS::EUREXCommonStruct>(ptr_to_price_level_update, length_of_bytes,
                                                             bulk_file_reader_, real_exch_type, exch_type);
      } break;
      case HFSAT::MDS_MSG::OSE_PF: {
        ReadAndCompareRealData<OSE_MDS::OSEPriceFeedCommonStruct>(ptr_to_price_level_update, length_of_bytes,
                                                                  bulk_file_reader_, real_exch_type, exch_type);
      } break;
      case HFSAT::MDS_MSG::OSE_CF: {
        ReadAndCompareRealData<OSE_MDS::OSECombinedCommonStruct>(ptr_to_price_level_update, length_of_bytes,
                                                                 bulk_file_reader_, real_exch_type, exch_type);
      } break;
      case HFSAT::MDS_MSG::CSM: {
        ReadAndCompareRealData<CSM_MDS::CSMCommonStruct>(ptr_to_price_level_update, length_of_bytes, bulk_file_reader_,
                                                         real_exch_type, exch_type);
      } break;
      case HFSAT::MDS_MSG::OSE_L1: {
        ReadAndCompareRealData<OSE_MDS::OSEPLCommonStruct>(ptr_to_price_level_update, length_of_bytes,
                                                           bulk_file_reader_, real_exch_type, exch_type);
      } break;
      case HFSAT::MDS_MSG::TMX:
      case HFSAT::MDS_MSG::TMX_LS: {
        ReadAndCompareRealData<TMX_MDS::TMXLSCommonStruct>(ptr_to_price_level_update, length_of_bytes,
                                                           bulk_file_reader_, real_exch_type, exch_type);
      } break;
      case HFSAT::MDS_MSG::TMX_OBF: {
        ReadAndCompareRealData<TMX_OBF_MDS::TMXPFCommonStruct>(ptr_to_price_level_update, length_of_bytes,
                                                               bulk_file_reader_, real_exch_type, exch_type);
      } break;
      case HFSAT::MDS_MSG::ICE:
      case HFSAT::MDS_MSG::ICE_LS: {
        ReadAndCompareRealData<ICE_MDS::ICECommonStructLive>(ptr_to_price_level_update, length_of_bytes,
                                                             bulk_file_reader_, real_exch_type, exch_type);
      } break;
      case HFSAT::MDS_MSG::ASX: {
        ReadAndCompareRealData<ASX_MDS::ASXPFCommonStruct>(ptr_to_price_level_update, length_of_bytes,
                                                           bulk_file_reader_, real_exch_type, exch_type);
      } break;
      case HFSAT::MDS_MSG::SGX: {
        ReadAndCompareRealData<SGX_MDS::SGXPFCommonStruct>(ptr_to_price_level_update, length_of_bytes,
                                                           bulk_file_reader_, real_exch_type, exch_type);
      } break;
      case HFSAT::MDS_MSG::OSE_ITCH_PF: {
        ReadAndCompareRealData<OSE_ITCH_MDS::OSEPFCommonStruct>(ptr_to_price_level_update, length_of_bytes,
                                                                bulk_file_reader_, real_exch_type, exch_type);
      } break;
      case HFSAT::MDS_MSG::HKOMDPF: {
        ReadAndCompareRealData<HKOMD_MDS::HKOMDPFCommonStruct>(ptr_to_price_level_update, length_of_bytes,
                                                               bulk_file_reader_, real_exch_type, exch_type);
      } break;
      case HFSAT::MDS_MSG::AFLASH: {
        ReadAndCompareRealData<AFLASH_MDS::AFlashCommonStructLive>(ptr_to_price_level_update, length_of_bytes,
                                                                   bulk_file_reader_, real_exch_type, exch_type);
      } break;
      case HFSAT::MDS_MSG::RETAIL: {
        ReadAndCompareRealData<RETAIL_MDS::RETAILCommonStruct>(ptr_to_price_level_update, length_of_bytes,
                                                               bulk_file_reader_, real_exch_type, exch_type);
      } break;
      case HFSAT::MDS_MSG::NSE: {
        ReadAndCompareRealData<NSE_MDS::NSETBTDataCommonStruct>(ptr_to_price_level_update, length_of_bytes,
                                                                bulk_file_reader_, real_exch_type, exch_type);
      } break;
      case HFSAT::MDS_MSG::NSE_L1: {
        ReadAndCompareRealData<HFSAT::GenericL1DataStruct>(ptr_to_price_level_update, length_of_bytes,
                                                           bulk_file_reader_, real_exch_type, exch_type);
      } break;
      case HFSAT::MDS_MSG::EOBI_OF: {
        ReadAndCompareRealData<EOBI_MDS::EOBICompactOrder>(ptr_to_price_level_update, length_of_bytes,
                                                           bulk_file_reader_, real_exch_type, exch_type);
      } break;
      default: {
        std::cerr << " Invalid exchange type read from the Real packets file. "
                  << "exch_type: " << real_exch_type << std::endl;
        exit(1);
      } break;
    }
  } else {
    std::cerr << "Real data file to compare is closed. Unable to compare the sim and real packets.\n";
    exit(1);
  }
}
}
