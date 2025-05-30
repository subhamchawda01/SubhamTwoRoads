/**
   \file Tools/mds_logger.cpp

   \author: (c) Copyright Two Roads Technological Solutions Pvt Ltd 2011
   Address:
   Suite 217, Level 2, Prestige Omega,
   No 104, EPIP Zone, Whitefield,
   Bangalore - 560066
   India
   +91 80 4060 0717
 */
#include <iostream>
#include <stdlib.h>

#include "dvccode/CDef/bse_mds_defines.hpp"
#include "dvccode/CDef/defines.hpp"
#include "dvccode/CDef/ttime.hpp"
#include "dvccode/CDef/sgx_mds_defines.hpp"
#include "dvccode/CDef/stored_market_data_common_message_defines.hpp"
#include "dvccode/CDef/stored_market_data_common_message_defines.hpp"
#include "dvccode/CDef/mds_messages.hpp"
#include "dvccode/CDef/generic_l1_data_struct.hpp"

#include "dvccode/Utils/bulk_file_reader.hpp"

template <class T>
class MDSLogReader {
 public:
  static void ReadMDSStructs(HFSAT::BulkFileReader& bulk_file_reader_, const int& st, const int& et) {
    T next_event_;
    if (bulk_file_reader_.is_open()) {
      while (true) {
        size_t available_len_ = bulk_file_reader_.read(&next_event_, sizeof(T));
        if (available_len_ < sizeof(next_event_)) break;
        //            HFSAT::ttime_t next_event_timestamp_ = next_event_.time_;
        //            if(et < next_event_timestamp_.tv_sec   || next_event_timestamp_.tv_sec < st) //time filter
        //              continue;
        std::cout << next_event_.ToString();
      }
      bulk_file_reader_.close();
    }
  }

  static void ReadRealDataMDSStructs(HFSAT::BulkFileReader& bulk_file_reader_);
};

int main(int argc, char** argv) {
  if (argc != 3 && argc != 5) {
    std::cout
        << " USAGE: EXEC <exchange> <file_path> <start-time-optional-unix-time-sec> <end-time-optional-unix-time-sec>"
        << std::endl;
    exit(0);
  }
  std::string exch = argv[1];

  HFSAT::BulkFileReader reader;
  reader.open(argv[2]);

  int st = 0;
  int et = 0x7fffffff;
  if (argc == 5) {
    st = atol(argv[3]);
    et = atol(argv[4]);
  }

  if (exch == "EUREX")
    MDSLogReader<EUREX_MDS::EUREXCommonStruct>::ReadMDSStructs(reader, st, et);
  else if (exch == "CME")
    MDSLogReader<CME_MDS::CMECommonStruct>::ReadMDSStructs(reader, st, et);
  else if (exch == "CMEOBF")
    MDSLogReader<CME_MDS::CMEOBFCommonStruct>::ReadMDSStructs(reader, st, et);
  else if (exch == "BMF")
    MDSLogReader<BMF_MDS::BMFCommonStruct>::ReadMDSStructs(reader, st, et);
  else if (exch == "NTP")
    MDSLogReader<NTP_MDS::NTPCommonStruct>::ReadMDSStructs(reader, st, et);
  else if (exch == "TMX")
    MDSLogReader<TMX_MDS::TMXCommonStruct>::ReadMDSStructs(reader, st, et);
  else if (exch == "TMX_LS")
    MDSLogReader<TMX_MDS::TMXLSCommonStruct>::ReadMDSStructs(reader, st, et);
  else if (exch == "LIFFE")
    MDSLogReader<LIFFE_MDS::LIFFECommonStruct>::ReadMDSStructs(reader, st, et);
  else if (exch == "RTS")
    MDSLogReader<RTS_MDS::RTSCommonStruct>::ReadMDSStructs(reader, st, et);
  else if (exch == "RTS_OF")
    MDSLogReader<RTS_MDS::RTSOFCommonStruct>::ReadMDSStructs(reader, st, et);
  else if (exch == "RTS_OFv2")
    MDSLogReader<RTS_MDS::RTSOFCommonStructv2>::ReadMDSStructs(reader, st, et);
  else if (exch == "RTSNew")
    MDSLogReader<RTS_MDS::RTSNewCommonStruct>::ReadMDSStructs(reader, st, et);
  else if (exch == "RTS_P2")
    MDSLogReader<MDS_PLAZA2::P2CommonStruct>::ReadMDSStructs(reader, st, et);
  else if (exch == "MICEX")
    MDSLogReader<MICEX_MDS::MICEXCommonStruct>::ReadMDSStructs(reader, st, et);
  else if (exch == "MICEX_OF")
    MDSLogReader<MICEX_OF_MDS::MICEXOFCommonStruct>::ReadMDSStructs(reader, st, et);
  else if (exch == "CHIX")
    MDSLogReader<HFSAT::BATSCHI_MDS::BATSCHICommonStruct>::ReadMDSStructs(reader, st, et);
  else if (exch == "CHIX_L1")
    MDSLogReader<HFSAT::BATSCHI_PL_MDS::BatsChiPLCommonStruct>::ReadMDSStructs(reader, st, et);
  else if (exch == "HKEX")
    MDSLogReader<HKEX_MDS::HKEXCommonStruct>::ReadMDSStructs(reader, st, et);
  else if (exch == "OSE")
    MDSLogReader<OSE_MDS::OSECommonStruct>::ReadMDSStructs(reader, st, et);
  else if (exch == "OSE_ITCH")
    MDSLogReader<OSE_ITCH_MDS::OSECommonStruct>::ReadMDSStructs(reader, st, et);
  else if (exch == "OSE_ITCH_OLD")
    MDSLogReader<OSE_ITCH_MDS::OSECommonStructOld>::ReadMDSStructs(reader, st, et);
  else if (exch == "OSE_ITCH_PF")
    MDSLogReader<OSE_ITCH_MDS::OSEPFCommonStruct>::ReadMDSStructs(reader, st, et);
  else if (exch == "OSE_L1")
    MDSLogReader<OSE_MDS::OSEPLCommonStruct>::ReadMDSStructs(reader, st, et);
  else if (exch == "OSE_PF")
    MDSLogReader<OSE_MDS::OSEPriceFeedCommonStruct>::ReadMDSStructs(reader, st, et);
  else if (exch == "OSE_CF")
    MDSLogReader<OSE_MDS::OSECombinedCommonStruct>::ReadMDSStructs(reader, st, et);
  else if (exch == "GENERIC")
    MDSLogReader<HFSAT::MDS_MSG::GenericMDSMessage>::ReadMDSStructs(reader, st, et);
  else if (exch == "EOBI")
    MDSLogReader<EOBI_MDS::EOBICommonStruct>::ReadMDSStructs(reader, st, et);
  else if (exch == "HKOMD")
    MDSLogReader<HKOMD_MDS::HKOMDCommonStruct>::ReadMDSStructs(reader, st, et);
  else if (exch == "HKOMD_PF")
    MDSLogReader<HKOMD_MDS::HKOMDPFCommonStruct>::ReadMDSStructs(reader, st, et);
  else if (exch == "CSM")
    MDSLogReader<CSM_MDS::CSMCommonStruct>::ReadMDSStructs(reader, st, et);
  else if (exch == "ICE")
    MDSLogReader<ICE_MDS::ICECommonStruct>::ReadMDSStructs(reader, st, et);
  else if (exch == "ICE_LIVE")
    MDSLogReader<ICE_MDS::ICECommonStructLive>::ReadMDSStructs(reader, st, et);
  else if (exch == "ICE_CF")
    MDSLogReader<ICE_MDS::ICECombinedCommonStruct>::ReadMDSStructs(reader, st, et);
  else if (exch == "ICE_CF_LIVE")
    MDSLogReader<ICE_MDS::ICECombinedCommonStructLive>::ReadMDSStructs(reader, st, et);
  else if (exch == "CONTROL")
    MDSLogReader<HFSAT::GenericControlRequestStruct>::ReadMDSStructs(reader, st, et);
  else if (exch == "ASX")
    MDSLogReader<ASX_MDS::ASXCommonStruct>::ReadMDSStructs(reader, st, et);
  else if (exch == "ASX_ITCH")
    MDSLogReader<ASX_ITCH_MDS::ASXItchOrder>::ReadMDSStructs(reader, st, et);
  else if (exch == "ASX_PF")
    MDSLogReader<ASX_MDS::ASXPFCommonStruct>::ReadMDSStructs(reader, st, et);
  else if (exch == "AFLASH")
    MDSLogReader<AFLASH_MDS::AFlashCommonStruct>::ReadMDSStructs(reader, st, et);
  else if (exch == "AFLASH_LIVE")
    MDSLogReader<AFLASH_MDS::AFlashCommonStructLive>::ReadMDSStructs(reader, st, et);
  else if (exch == "NSE")
    MDSLogReader<NSE_MDS::NSEDotexOfflineCommonStruct>::ReadMDSStructs(reader, st, et);
  else if (exch == "NSE_LIVE")
    MDSLogReader<NSE_MDS::NSETBTDataCommonStruct>::ReadMDSStructs(reader, st, et);
  else if (exch == "RETAIL")
    MDSLogReader<RETAIL_MDS::RETAILCommonStruct>::ReadMDSStructs(reader, st, et);
  else if (exch == "BSE")
    MDSLogReader<BSE_MDS::BSEOrder>::ReadMDSStructs(reader, st, et);
  else if (exch == "BSE_PF")
    MDSLogReader<BSE_MDS::BSEPFCommonStruct>::ReadMDSStructs(reader, st, et);
  else if (exch == "SGX")
    MDSLogReader<SGX_MDS::SGXOrder>::ReadMDSStructs(reader, st, et);
  else if (exch == "SGX_ITCH")
    MDSLogReader<SGX_ITCH_MDS::SGXItchOrder>::ReadMDSStructs(reader, st, et);
  else if (exch == "SGX_PF")
    MDSLogReader<SGX_MDS::SGXPFCommonStruct>::ReadMDSStructs(reader, st, et);
  else if (exch == "KRX")
    MDSLogReader<KRX_MDS::KRXCommonStruct>::ReadMDSStructs(reader, st, et);
  else if (exch == "ORS_REPLY")
    MDSLogReader<HFSAT::GenericORSReplyStruct>::ReadMDSStructs(reader, st, et);
  else if (exch == "ORS_REPLY_LIVE")
    MDSLogReader<HFSAT::GenericORSReplyStructLive>::ReadMDSStructs(reader, st, et);
  else if (exch == "L1_DATA")
    MDSLogReader<HFSAT::GenericL1DataStruct>::ReadMDSStructs(reader, st, et);
  else if (exch == "TMX_OBF")
    MDSLogReader<TMX_OBF_MDS::TMXCommonStruct>::ReadMDSStructs(reader, st, et);
  else if (exch == "CME_FPGA")
    MDSLogReader<CME_MDS::CMEFPGACommonStruct>::ReadMDSStructs(reader, st, et);
  else if (exch == "TMX_OBF_PF")
    MDSLogReader<TMX_OBF_MDS::TMXPFCommonStruct>::ReadMDSStructs(reader, st, et);
  else if (exch == "BMF_FPGA")
    MDSLogReader<FPGA_MDS::BMFFPGACommonStruct>::ReadMDSStructs(reader, st, et);
  else if (exch == "ALL_INORDER") {
    MDSLogReader<class T>::ReadRealDataMDSStructs(reader);
  } else
    std::cout << "Wrong exchange...!!" << exch << std::endl;
  return 0;
}

template <class T>
void MDSLogReader<T>::ReadRealDataMDSStructs(HFSAT::BulkFileReader& bulk_file_reader_) {
  HFSAT::MDS_MSG::MDSMessageExchType exch_type;
  if (bulk_file_reader_.is_open()) {
    while (true) {
      size_t exch_available_len_ = bulk_file_reader_.read(&exch_type, sizeof(HFSAT::MDS_MSG::MDSMessageExchType));
      if (exch_available_len_ < sizeof(exch_type)) break;
      size_t mds_available_len_;
      switch (exch_type) {
        case HFSAT::MDS_MSG::CME_LS:
        case HFSAT::MDS_MSG::CME: {
          CME_MDS::CMECommonStruct next_event;
          mds_available_len_ = bulk_file_reader_.read(&next_event, sizeof(CME_MDS::CMECommonStruct));
          if (mds_available_len_ < sizeof(next_event)) break;
          std::cout << next_event.ToString();
        } break;
        case HFSAT::MDS_MSG::LIFFE:
        case HFSAT::MDS_MSG::LIFFE_LS: {
          LIFFE_MDS::LIFFECommonStruct next_event;
          mds_available_len_ = bulk_file_reader_.read(&next_event, sizeof(LIFFE_MDS::LIFFECommonStruct));
          if (mds_available_len_ < sizeof(next_event)) break;
          std::cout << next_event.ToString();

        } break;
        case HFSAT::MDS_MSG::EUREX_LS:
        case HFSAT::MDS_MSG::EUREX:
        case HFSAT::MDS_MSG::EOBI_LS: {
          EUREX_MDS::EUREXCommonStruct next_event;
          mds_available_len_ = bulk_file_reader_.read(&next_event, sizeof(EUREX_MDS::EUREXCommonStruct));
          if (mds_available_len_ < sizeof(next_event)) break;
          std::cout << next_event.ToString();
        } break;

        case HFSAT::MDS_MSG::CONTROL: {
          HFSAT::GenericControlRequestStruct next_event;
          mds_available_len_ = bulk_file_reader_.read(&next_event, sizeof(HFSAT::GenericControlRequestStruct));
          if (mds_available_len_ < sizeof(next_event)) break;
          std::cout << next_event.ToString();

        } break;

        case HFSAT::MDS_MSG::ORS_REPLY: {
          HFSAT::GenericORSReplyStructLive next_event;
          mds_available_len_ = bulk_file_reader_.read(&next_event, sizeof(HFSAT::GenericORSReplyStructLive));
          if (mds_available_len_ < sizeof(next_event)) break;
          std::cout << next_event.ToString();

        } break;

        case HFSAT::MDS_MSG::RTS: {
          RTS_MDS::RTSCommonStruct next_event;
          mds_available_len_ = bulk_file_reader_.read(&next_event, sizeof(RTS_MDS::RTSCommonStruct));
          if (mds_available_len_ < sizeof(next_event)) break;
          std::cout << next_event.ToString();

        } break;

        // Same Handling for MICEX_CR and MICEX_EQ
        case HFSAT::MDS_MSG::MICEX: {
          MICEX_MDS::MICEXCommonStruct next_event;
          mds_available_len_ = bulk_file_reader_.read(&next_event, sizeof(MICEX_MDS::MICEXCommonStruct));
          if (mds_available_len_ < sizeof(next_event)) break;
          std::cout << next_event.ToString();

        } break;
        case HFSAT::MDS_MSG::NTP:
        case HFSAT::MDS_MSG::NTP_LS: {
          NTP_MDS::NTPCommonStruct next_event;
          mds_available_len_ = bulk_file_reader_.read(&next_event, sizeof(NTP_MDS::NTPCommonStruct));
          if (mds_available_len_ < sizeof(next_event)) break;
          std::cout << next_event.ToString();

        } break;

        case HFSAT::MDS_MSG::BMF_EQ: {  // BMF_EQ and NTP have same common struct
          NTP_MDS::NTPCommonStruct next_event;
          mds_available_len_ = bulk_file_reader_.read(&next_event, sizeof(NTP_MDS::NTPCommonStruct));
          if (mds_available_len_ < sizeof(next_event)) break;
          std::cout << next_event.ToString();
        } break;
        case HFSAT::MDS_MSG::EOBI_PF: {
          EUREX_MDS::EUREXCommonStruct next_event;
          mds_available_len_ = bulk_file_reader_.read(&next_event, sizeof(EUREX_MDS::EUREXCommonStruct));
          if (mds_available_len_ < sizeof(next_event)) break;
          std::cout << next_event.ToString();

        } break;

        case HFSAT::MDS_MSG::OSE_PF: {
          OSE_MDS::OSEPriceFeedCommonStruct next_event;
          mds_available_len_ = bulk_file_reader_.read(&next_event, sizeof(OSE_MDS::OSEPriceFeedCommonStruct));
          if (mds_available_len_ < sizeof(next_event)) break;
          std::cout << next_event.ToString();

        } break;

        case HFSAT::MDS_MSG::OSE_CF: {
          OSE_MDS::OSECombinedCommonStruct next_event;
          mds_available_len_ = bulk_file_reader_.read(&next_event, sizeof(OSE_MDS::OSECombinedCommonStruct));
          if (mds_available_len_ < sizeof(next_event)) break;
          std::cout << next_event.ToString();

        } break;

        case HFSAT::MDS_MSG::CSM: {
          CSM_MDS::CSMCommonStruct next_event;
          mds_available_len_ = bulk_file_reader_.read(&next_event, sizeof(CSM_MDS::CSMCommonStruct));
          if (mds_available_len_ < sizeof(next_event)) break;
          std::cout << next_event.ToString();

        } break;

        case HFSAT::MDS_MSG::OSE_L1: {
          OSE_MDS::OSEPLCommonStruct next_event;
          mds_available_len_ = bulk_file_reader_.read(&next_event, sizeof(OSE_MDS::OSEPLCommonStruct));
          if (mds_available_len_ < sizeof(next_event)) break;
          std::cout << next_event.ToString();

        } break;

        case HFSAT::MDS_MSG::TMX:
        case HFSAT::MDS_MSG::TMX_LS: {
          TMX_MDS::TMXLSCommonStruct next_event;
          mds_available_len_ = bulk_file_reader_.read(&next_event, sizeof(TMX_MDS::TMXLSCommonStruct));
          if (mds_available_len_ < sizeof(next_event)) break;
          std::cout << next_event.ToString();
        } break;

        case HFSAT::MDS_MSG::TMX_OBF: {
          TMX_OBF_MDS::TMXPFCommonStruct next_event;
          mds_available_len_ = bulk_file_reader_.read(&next_event, sizeof(TMX_OBF_MDS::TMXPFCommonStruct));
          if (mds_available_len_ < sizeof(next_event)) break;
          std::cout << next_event.ToString();
        } break;

        case HFSAT::MDS_MSG::ICE:
        case HFSAT::MDS_MSG::ICE_LS: {
          ICE_MDS::ICECommonStructLive next_event;
          mds_available_len_ = bulk_file_reader_.read(&next_event, sizeof(ICE_MDS::ICECommonStructLive));
          if (mds_available_len_ < sizeof(next_event)) break;
          std::cout << next_event.ToString();
        } break;

        case HFSAT::MDS_MSG::ASX: {
          ASX_MDS::ASXPFCommonStruct next_event;
          mds_available_len_ = bulk_file_reader_.read(&next_event, sizeof(ASX_MDS::ASXPFCommonStruct));
          if (mds_available_len_ < sizeof(next_event)) break;
          std::cout << next_event.ToString();
        } break;

        case HFSAT::MDS_MSG::SGX: {
          SGX_MDS::SGXPFCommonStruct next_event;
          mds_available_len_ = bulk_file_reader_.read(&next_event, sizeof(SGX_MDS::SGXPFCommonStruct));
          if (mds_available_len_ < sizeof(next_event)) break;
          std::cout << next_event.ToString();
        } break;
        case HFSAT::MDS_MSG::OSE_ITCH_PF: {
          OSE_ITCH_MDS::OSEPFCommonStruct next_event;
          mds_available_len_ = bulk_file_reader_.read(&next_event, sizeof(OSE_ITCH_MDS::OSEPFCommonStruct));
          if (mds_available_len_ < sizeof(next_event)) break;
          std::cout << next_event.ToString();
        } break;
        case HFSAT::MDS_MSG::HKOMDPF: {
          HKOMD_MDS::HKOMDPFCommonStruct next_event;
          mds_available_len_ = bulk_file_reader_.read(&next_event, sizeof(HKOMD_MDS::HKOMDPFCommonStruct));
          if (mds_available_len_ < sizeof(next_event)) break;
          std::cout << next_event.ToString();
        } break;
        case HFSAT::MDS_MSG::AFLASH: {
          AFLASH_MDS::AFlashCommonStructLive next_event;
          mds_available_len_ = bulk_file_reader_.read(&next_event, sizeof(AFLASH_MDS::AFlashCommonStructLive));
          if (mds_available_len_ < sizeof(next_event)) break;
          std::cout << next_event.ToString();
        } break;
        case HFSAT::MDS_MSG::RETAIL: {
          RETAIL_MDS::RETAILCommonStruct next_event;
          mds_available_len_ = bulk_file_reader_.read(&next_event, sizeof(RETAIL_MDS::RETAILCommonStruct));
          if (mds_available_len_ < sizeof(next_event)) break;
          std::cout << next_event.ToString();
        } break;
        case HFSAT::MDS_MSG::NSE: {
          NSE_MDS::NSETBTDataCommonStruct next_event;
          mds_available_len_ = bulk_file_reader_.read(&next_event, sizeof(NSE_MDS::NSETBTDataCommonStruct));
          if (mds_available_len_ < sizeof(next_event)) break;
          std::cout << next_event.ToString();
        } break;
        case HFSAT::MDS_MSG::NSE_L1: {
          HFSAT::GenericL1DataStruct next_event;
          mds_available_len_ = bulk_file_reader_.read(&next_event, sizeof(HFSAT::GenericL1DataStruct));
          if (mds_available_len_ < sizeof(next_event)) break;
          std::cout << next_event.ToString();
        } break;
        case HFSAT::MDS_MSG::EOBI_OF: {
          EOBI_MDS::EOBICompactOrder next_event;
          mds_available_len_ = bulk_file_reader_.read(&next_event, sizeof(EOBI_MDS::EOBICompactOrder));
          if (mds_available_len_ < sizeof(next_event)) break;
          std::cout << next_event.ToString();
        } break;
        default: {
          std::cerr << " UNKNOWN EXCHANGE, SHOULD NEVER REACH HERE "
                    << "exch_type: " << exch_type << std::endl;
          exit(1);

        } break;
      }
    }
    bulk_file_reader_.close();
  }
}
