// =====================================================================================
//
//       Filename:  mds_messages.hpp
//
//    Description:
//
//        Version:  1.0
//        Created:  12/13/2013 10:16:38 AM
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

#include "dvccode/CDef/stored_market_data_common_message_defines.hpp"
#include "dvccode/CDef/bmf_common_message_defines.hpp"
#include "dvccode/CDef/ors_messages.hpp"
#include "dvccode/CDef/control_messages.hpp"
#include "dvccode/CDef/sgx_mds_defines.hpp"
#include "dvccode/CDef/generic_l1_data_struct.hpp"
#include "dvccode/Utils/exchange_names.hpp"
#include "dvccode/CDef/tmx_obf_mds_defines.hpp"

namespace HFSAT {
namespace MDS_MSG {

// What this defines is the exchange identifier, will be useful for clients
enum MDSMessageExchType {
  CME = 0,
  EUREX,
  TMX,
  BMF,
  NTP,
  LIFFE,
  CME_LS,
  EUREX_LS,
  OSE_PF,
  OSE_L1,
  CHIX,
  CHIX_L1,
  HKEX,
  RTS,
  MICEX,
  TSE,
  EOBI_LS,
  CONTROL,
  ORS_REPLY,
  ESPEED,
  EOBI_PF,
  LIFFE_LS,
  QUINCY_LS,
  BMF_EQ,
  CSM,
  OSE_CF,
  ICE,
  EBS,
  HKOMD,
  HKOMDPF,
  ICE_LS,
  AFLASH,
  ICE_CF,
  RETAIL,
  NTP_LS,
  NSE,
  BSE,
  ASX,  // Here ASX corresponds actually to ASX_PF, as generic will only contain price feed converted data, never order
  ASX_LS,
  TMX_LS,
  OSE_ITCH_PF,
  EOBI_OF,
  SGX,
  TMX_OBF,
  NSE_L1,
  RTS_OF,
  // for the purpose of book building modifications
  TMX_OBF_OF,
  OSE_ITCH_OF,
  MICEX_OF,
  ORS_REPLY_MULTISHM,
  INVALID,
  IBKR
};

struct CommonTradeMessage {
  std::string contract_;
  uint32_t trd_qty_;   ///< quantity in this trade
  uint32_t seqno_;     ///< instrument based seqno .. to order trades and quotes
  double trd_px_;      ///< trade price
  uint32_t agg_side_;  // 1- Buy 2-sell
};


struct GenericMDSMessage {
  struct timeval time_;

  union GenericData {
    CME_MDS::CMECommonStruct cme_data_;
    CME_MDS::CMELSCommonStruct cme_ls_data_;
    EUREX_MDS::EUREXCommonStruct eurex_data_;
    EUREX_MDS::EUREXCommonStruct eobi_pf_data_;
    EUREX_MDS::EUREXLSCommonStruct eurex_ls_data_;
    EUREX_MDS::EUREXLSCommonStruct eobi_ls_data_;
    LIFFE_MDS::LIFFECommonStruct liffe_data_;  // Not being used in live currently.
    RTS_MDS::RTSCommonStruct rts_data_;
    MICEX_MDS::MICEXCommonStruct micex_data_;
    HKEX_MDS::HKEXCommonStruct hkex_data_;              // Not being used in live currently.
    OSE_MDS::OSEPLCommonStruct ose_l1_data_;            // Not being used in live currently.
    OSE_MDS::OSEPriceFeedCommonStruct ose_pf_data_;     // Not being used in live currently.
    OSE_MDS::OSECombinedCommonStruct ose_cf_data_;      // Not being used in live currently.
    OSE_ITCH_MDS::OSEPFCommonStruct ose_itch_pf_data_;  // Not being used in live currently.
    NTP_MDS::NTPCommonStruct ntp_data_;
    NTP_MDS::NTPCommonStruct bmf_eq_data_;
    BMF_MDS::BMFCommonStruct bmf_data_;
    HFSAT::BATSCHI_MDS::BATSCHICommonStruct chix_data_;          // Not being used in live currently.
    HFSAT::BATSCHI_PL_MDS::BatsChiPLCommonStruct chix_l1_data_;  // Not being used in live currently.
    HFSAT::GenericControlRequestStruct control_data_;
    HFSAT::GenericORSReplyStructLive ors_reply_data_;
    CSM_MDS::CSMCommonStruct csm_data_;
    TMX_MDS::TMXLSCommonStruct tmx_data_;  // Not being used in live currently.
    ICE_MDS::ICECommonStructLive ice_data_;
    ICE_MDS::ICECombinedCommonStructLive ice_cf_data_;
    HKOMD_MDS::HKOMDPFCommonStruct hkomd_pf_data_;  // Not being used in live currently.
    AFLASH_MDS::AFlashCommonStructLive aflash_data_;
    RETAIL_MDS::RETAILCommonStruct retail_data_;
    NSE_MDS::NSETBTDataCommonStruct nse_data_;
    NSE_MDS::NSEBarDataCommonStruct nse_bardata_;
    ASX_MDS::ASXPFCommonStruct asx_data_;
    EOBI_MDS::EOBICompactOrder eobi_of_data_;
    SGX_MDS::SGXPFCommonStruct sgx_data_;
    TMX_OBF_MDS::TMXPFCommonStruct tmx_obf_data_;  // Not being used in live currently.
    HFSAT::GenericL1DataStruct nse_l1_data_;
    RTS_MDS::RTSOFCommonStructv2 rts_of_data_;
    EOBI_MDS::EOBICommonStruct bse_data_;

    // for the purpose of book building modifications
    TMX_OBF_MDS::TMXCommonStruct tmx_orderfeed_data_;
    MICEX_OF_MDS::MICEXOFCommonStruct micex_of_data_;
    OSE_ITCH_MDS::OSECommonStruct ose_itch_of_data_;

    GenericData() {}
    ~GenericData() {}

  } generic_data_;

  MDSMessageExchType mds_msg_exch_;
  int32_t t2t_cshmw_time_;
  uint64_t t2t_cshmw_start_time_;

 public:
  double GetTradeDoublePrice() {
    double price = -1;
    if (IsTrade()) {
      switch (mds_msg_exch_) {
        case CME: {
          price = generic_data_.cme_data_.GetTradeDoublePrice();
          break;
        }
        case EUREX: {
          price = generic_data_.eurex_data_.GetTradeDoublePrice();
          break;
        }
        case EOBI_PF: {
          price = generic_data_.eobi_pf_data_.GetTradeDoublePrice();
          break;
        }
        case BMF: {
          price = generic_data_.bmf_data_.GetTradeDoublePrice();
          break;
        }
        case NTP: {
          price = generic_data_.ntp_data_.GetTradeDoublePrice();
          break;
        }
        case BMF_EQ: {
          price = generic_data_.bmf_eq_data_.GetTradeDoublePrice();
          break;
        }
        case LIFFE: {
          price = generic_data_.liffe_data_.GetTradeDoublePrice();
          break;
        }
        case LIFFE_LS: {
          price = generic_data_.liffe_data_.GetTradeDoublePrice();
          break;
        }
        case CME_LS: {
          price = generic_data_.cme_ls_data_.GetTradeDoublePrice();
          break;
        }
        case EUREX_LS: {
          price = generic_data_.eurex_ls_data_.GetTradeDoublePrice();
          break;
        }
        case EOBI_LS: {
          price = generic_data_.eobi_ls_data_.GetTradeDoublePrice();
          break;
        }
        case OSE_PF: {
          price = generic_data_.ose_pf_data_.GetTradeDoublePrice();
          break;
        }
        case OSE_CF: {
          price = generic_data_.ose_cf_data_.GetTradeDoublePrice();
          break;
        }
        case OSE_L1: {
          price = generic_data_.ose_l1_data_.GetTradeDoublePrice();
          break;
        }
        case CHIX: {
          price = generic_data_.chix_data_.GetTradeDoublePrice();
          break;
        }
        case CHIX_L1: {
          price = generic_data_.chix_l1_data_.GetTradeDoublePrice();
          break;
        }
        case HKEX: {
          price = generic_data_.hkex_data_.GetTradeDoublePrice();
          break;
        }
        case RTS: {
          price = generic_data_.rts_data_.GetTradeDoublePrice();
          break;
        }
        case RTS_OF: {
          price = generic_data_.rts_of_data_.GetTradeDoublePrice();
          break;
        }
        case MICEX: {
          price = generic_data_.micex_data_.GetTradeDoublePrice();
          break;
        }
        case CSM: {
          // price = generic_data_.csm_data_.GetTradeDoublePrice();
          break;
        }
        case TMX:
        case TMX_LS: {
          // price = generic_data_.tmx_data_.GetTradeDoublePrice();
          break;
        }
        case ICE: {
          price = generic_data_.ice_data_.GetTradeDoublePrice();
          break;
        }
        case ICE_LS: {
          price = generic_data_.ice_data_.GetTradeDoublePrice();
          break;
        }
        case ICE_CF: {
          price = generic_data_.ice_cf_data_.GetTradeDoublePrice();
          break;
        }
        case HKOMDPF: {
          // price = generic_data_.hkomd_pf_data_.GetTradeDoublePrice();
          break;
        }
        case AFLASH: {
          // price = generic_data_.aflash_data_.GetTradeDoublePrice();
          break;
        }
        case RETAIL: {
          price = generic_data_.retail_data_.GetTradeDoublePrice();
          break;
        }
        case NSE: {
          price = generic_data_.nse_data_.GetTradeDoublePrice();
          break;
        }
        case ASX:
        case ASX_LS: {
          price = generic_data_.asx_data_.GetTradeDoublePrice();
          break;
        }
        case OSE_ITCH_PF: {
          price = generic_data_.ose_itch_pf_data_.GetTradeDoublePrice();
          break;
        }
        case EOBI_OF: {
          price = generic_data_.eobi_of_data_.GetTradeDoublePrice();
          break;
        }
        case SGX: {
          price = generic_data_.sgx_data_.GetTradeDoublePrice();
          break;
        }
        case TMX_OBF: {
          break;
        }
        case NSE_L1: {
          price = generic_data_.nse_l1_data_.GetTradeDoublePrice();
          break;
        }
        case TMX_OBF_OF: {
          price = generic_data_.tmx_orderfeed_data_.GetTradeDoublePrice();
          break;
        }
        case OSE_ITCH_OF: {
          price = generic_data_.ose_itch_of_data_.GetTradeDoublePrice();
          break;
        }
        default: { } break; }
    }
    return price;
  }

  inline HFSAT::TradeType_t GetTradeAggressorSide() {
    HFSAT::TradeType_t side = HFSAT::TradeType_t::kTradeTypeNoInfo;
    if (IsTrade()) {
      switch (mds_msg_exch_) {
        case CME: {
          side = generic_data_.cme_data_.GetTradeAggressorSide();
          break;
        }
        case EUREX: {
          side = generic_data_.eurex_data_.GetTradeAggressorSide();
          break;
        }
        case EOBI_PF: {
          side = generic_data_.eobi_pf_data_.GetTradeAggressorSide();
          break;
        }
        case BMF: {
          side = generic_data_.bmf_data_.GetTradeAggressorSide();
          break;
        }
        case NTP: {
          side = generic_data_.ntp_data_.GetTradeAggressorSide();
          break;
        }
        case BMF_EQ: {
          side = generic_data_.bmf_eq_data_.GetTradeAggressorSide();
          break;
        }
        case LIFFE: {
          side = generic_data_.liffe_data_.GetTradeAggressorSide();
          break;
        }
        case LIFFE_LS: {
          side = generic_data_.liffe_data_.GetTradeAggressorSide();
          break;
        }
        case CME_LS: {
          side = generic_data_.cme_ls_data_.GetTradeAggressorSide();
          break;
        }
        case EUREX_LS: {
          side = generic_data_.eurex_ls_data_.GetTradeAggressorSide();
          break;
        }
        case EOBI_LS: {
          side = generic_data_.eobi_ls_data_.GetTradeAggressorSide();
          break;
        }
        case OSE_PF: {
          side = generic_data_.ose_pf_data_.GetTradeAggressorSide();
          break;
        }
        case OSE_CF: {
          side = generic_data_.ose_cf_data_.GetTradeAggressorSide();
          break;
        }
        case OSE_L1: {
          side = generic_data_.ose_l1_data_.GetTradeAggressorSide();
          break;
        }
        case CHIX: {
          side = generic_data_.chix_data_.GetTradeAggressorSide();
          break;
        }
        case CHIX_L1: {
          side = generic_data_.chix_l1_data_.GetTradeAggressorSide();
          break;
        }
        case HKEX: {
          side = generic_data_.hkex_data_.GetTradeAggressorSide();
          break;
        }
        case RTS: {
          side = generic_data_.rts_data_.GetTradeAggressorSide();
          break;
        }
        case RTS_OF: {
          side = generic_data_.rts_of_data_.GetTradeAggressorSide();
          break;
        }
        case MICEX: {
          side = generic_data_.micex_data_.GetTradeAggressorSide();
          break;
        }
        case CSM: {
          // side = generic_data_.csm_data_.GetTradeAggressorSide();
          break;
        }
        case TMX:
        case TMX_LS: {
          // side = generic_data_.tmx_data_.GetTradeAggressorSide();
          break;
        }
        case ICE: {
          side = generic_data_.ice_data_.GetTradeAggressorSide();
          break;
        }
        case ICE_LS: {
          side = generic_data_.ice_data_.GetTradeAggressorSide();
          break;
        }
        case ICE_CF: {
          side = generic_data_.ice_cf_data_.GetTradeAggressorSide();
          break;
        }
        case HKOMDPF: {
          // side = generic_data_.hkomd_pf_data_.GetTradeAggressorSide();
          break;
        }
        case AFLASH: {
          // side = generic_data_.aflash_data_.GetTradeAggressorSide();
          break;
        }
        case RETAIL: {
          side = generic_data_.retail_data_.GetTradeAggressorSide();
          break;
        }
        case NSE: {
          side = generic_data_.nse_data_.GetTradeAggressorSide();
          break;
        }
        case ASX:
        case ASX_LS: {
          side = generic_data_.asx_data_.GetTradeAggressorSide();
          break;
        }
        case OSE_ITCH_PF: {
          side = generic_data_.ose_itch_pf_data_.GetTradeAggressorSide();
          break;
        }
        case EOBI_OF: {
          side = generic_data_.eobi_of_data_.GetTradeAggressorSide();
          break;
        }
        case SGX: {
          side = generic_data_.sgx_data_.GetTradeAggressorSide();
          break;
        }
        case TMX_OBF: {
          break;
        }
        case NSE_L1: {
          side = generic_data_.nse_l1_data_.GetTradeAggressorSide();
          break;
        }
        case TMX_OBF_OF: {
          side = generic_data_.tmx_orderfeed_data_.GetTradeAggressorSide();
          break;
        }
        case OSE_ITCH_OF: {
          side = generic_data_.ose_itch_of_data_.GetTradeAggressorSide();
          break;
        }
        default: { } break; }
    }
    return side;
  }

  inline uint32_t GetTradeSize() {
    uint32_t size = -1;
    if (IsTrade()) {
      switch (mds_msg_exch_) {
        case CME: {
          size = generic_data_.cme_data_.GetTradeSize();
          break;
        }
        case EUREX: {
          size = generic_data_.eurex_data_.GetTradeSize();
          break;
        }
        case EOBI_PF: {
          size = generic_data_.eobi_pf_data_.GetTradeSize();
          break;
        }
        case BMF: {
          size = generic_data_.bmf_data_.GetTradeSize();
          break;
        }
        case NTP: {
          size = generic_data_.ntp_data_.GetTradeSize();
          break;
        }
        case BMF_EQ: {
          size = generic_data_.bmf_eq_data_.GetTradeSize();
          break;
        }
        case LIFFE: {
          size = generic_data_.liffe_data_.GetTradeSize();
          break;
        }
        case LIFFE_LS: {
          size = generic_data_.liffe_data_.GetTradeSize();
          break;
        }
        case CME_LS: {
          size = generic_data_.cme_ls_data_.GetTradeSize();
          break;
        }
        case EUREX_LS: {
          size = generic_data_.eurex_ls_data_.GetTradeSize();
          break;
        }
        case EOBI_LS: {
          size = generic_data_.eobi_ls_data_.GetTradeSize();
          break;
        }
        case OSE_PF: {
          size = generic_data_.ose_pf_data_.GetTradeSize();
          break;
        }
        case OSE_CF: {
          size = generic_data_.ose_cf_data_.GetTradeSize();
          break;
        }
        case OSE_L1: {
          size = generic_data_.ose_l1_data_.GetTradeSize();
          break;
        }
        case CHIX: {
          size = generic_data_.chix_data_.GetTradeSize();
          break;
        }
        case CHIX_L1: {
          size = generic_data_.chix_l1_data_.GetTradeSize();
          break;
        }
        case HKEX: {
          size = generic_data_.hkex_data_.GetTradeSize();
          break;
        }
        case RTS: {
          size = generic_data_.rts_data_.GetTradeSize();
          break;
        }
        case RTS_OF: {
          size = generic_data_.rts_of_data_.GetTradeSize();
          break;
        }
        case MICEX: {
          size = generic_data_.micex_data_.GetTradeSize();
          break;
        }
        case CSM: {
          // size = generic_data_.csm_data_.GetTradeSize();
          break;
        }
        case TMX:
        case TMX_LS: {
          // size = generic_data_.tmx_data_.GetTradeSize();
          break;
        }
        case ICE: {
          size = generic_data_.ice_data_.GetTradeSize();
          break;
        }
        case ICE_LS: {
          size = generic_data_.ice_data_.GetTradeSize();
          break;
        }
        case ICE_CF: {
          size = generic_data_.ice_cf_data_.GetTradeSize();
          break;
        }
        case HKOMDPF: {
          // size = generic_data_.hkomd_pf_data_.GetTradeSize();
          break;
        }
        case AFLASH: {
          // size = generic_data_.aflash_data_.GetTradeSize();
          break;
        }
        case RETAIL: {
          size = generic_data_.retail_data_.GetTradeSize();
          break;
        }
        case NSE: {
          size = generic_data_.nse_data_.GetTradeSize();
          break;
        }
        case ASX:
        case ASX_LS: {
          size = generic_data_.asx_data_.GetTradeSize();
          break;
        }
        case OSE_ITCH_PF: {
          size = generic_data_.ose_itch_pf_data_.GetTradeSize();
          break;
        }
        case EOBI_OF: {
          size = generic_data_.eobi_of_data_.GetTradeSize();
          break;
        }
        case SGX: {
          size = generic_data_.sgx_data_.GetTradeSize();
          break;
        }
        case TMX_OBF: {
          break;
        }
        case NSE_L1: {
          size = generic_data_.nse_l1_data_.GetTradeSize();
          break;
        }
        case TMX_OBF_OF: {
          size = generic_data_.tmx_orderfeed_data_.GetTradeSize();
          break;
        }
        case OSE_ITCH_OF: {
          size = generic_data_.ose_itch_of_data_.GetTradeSize();
          break;
        }
        default: { } break; }
    }
    return size;
  }

  std::string ToStringLine() {
   switch (mds_msg_exch_){
      case NSE: {
          return generic_data_.nse_data_.ToStringLine();
        }
      default:  { return "NOT IMPLEMENTED FOR THIS EXCH"; } 
	  break;
    }
   return "NOT IMPLEMENTED FOR THIS EXCH";
  }

  std::string ToString() {
    switch (mds_msg_exch_) {
      case CME: {
	std::cout << "CME" << std::endl;
        return generic_data_.cme_data_.ToString();
      }
      case EUREX: {
	std::cout << "EUREX:" << std::endl;
        return generic_data_.eurex_data_.ToString();
      }
      case EOBI_PF: {
	std::cout << "EOBI_PF:" << std::endl;
        return generic_data_.eobi_pf_data_.ToString();
      }
      case BMF: {
	std::cout << "BMF:" << std::endl;
        return generic_data_.bmf_data_.ToString();
      }
      case NTP: {
	std::cout << "NTP:" << std::endl;
        return generic_data_.ntp_data_.ToString();
      }
      case BMF_EQ: {
	std::cout << "BMF_EQ:" << std::endl;
        return generic_data_.bmf_eq_data_.ToString();
      }
      case LIFFE: {
	std::cout << "LIFFE:" << std::endl;
        return generic_data_.liffe_data_.ToString();
      }
      case LIFFE_LS: {
	std::cout << "LIFFE_LS:" << std::endl;
        return generic_data_.liffe_data_.ToString();
      }
      case CME_LS: {
	std::cout << "CME_LS:" << std::endl;
        return generic_data_.cme_ls_data_.ToString();
      }
      case EUREX_LS: {
	std::cout << "EUREX_LS:" << std::endl;
        return generic_data_.eurex_ls_data_.ToString();
      }
      case EOBI_LS: {
	std::cout << "EOBI_LS:" << std::endl;
        return generic_data_.eobi_ls_data_.ToString();
      }
      case OSE_PF: {
	std::cout << "OSE_PF:" << std::endl;
        return generic_data_.ose_pf_data_.ToString();
      }
      case OSE_CF: {
	std::cout << "OSE_CF:" << std::endl;
        return generic_data_.ose_cf_data_.ToString();
      }
      case OSE_L1: {
	std::cout << "OSE_L1:" << std::endl;
        return generic_data_.ose_l1_data_.ToString();
      }
      case CHIX: {
	std::cout << "CHIX:" << std::endl;
        return generic_data_.chix_data_.ToString();
      }
      case CHIX_L1: {
	std::cout << "CHIX_L1:" << std::endl;
        return generic_data_.chix_l1_data_.ToString();
      }
      case HKEX: {
	std::cout << "HKEX:" << std::endl;
        return generic_data_.hkex_data_.ToString();
      }
      case RTS: {
	std::cout << "RTS:" << std::endl;
        return generic_data_.rts_data_.ToString();
      }
      case RTS_OF: {
	std::cout << "RTS_OF:" << std::endl;
        return generic_data_.rts_of_data_.ToString();
      }
      case MICEX: {
	std::cout << "MICEX:" << std::endl;
        return generic_data_.micex_data_.ToString();
      }
      case CONTROL: {
	std::cout << "CONTROL:" << std::endl;
        return generic_data_.control_data_.ToString();
      }
      case ORS_REPLY: {
	std::cout << "ORS_REPLY:" << std::endl;
        return generic_data_.ors_reply_data_.ToString();
      }
      case CSM: {
	std::cout << "CSM:" << std::endl;
        return generic_data_.csm_data_.ToString();
      }
      case TMX:
      case TMX_LS: {
	std::cout << "TMX: or TMX_LS:" << std::endl;
        std::ostringstream temp_oss;
        temp_oss << "Time: " << time_.tv_sec << "." << std::setw(6) << std::setfill('0') << time_.tv_usec << "\n";
        temp_oss << generic_data_.tmx_data_.ToString();
        return temp_oss.str();
      }
      case ICE: {
	std::cout << "ICE:" << std::endl;
        return generic_data_.ice_data_.ToString();
      }
      case ICE_LS: {
	std::cout << "ICE_LS:" << std::endl;
        return generic_data_.ice_data_.ToString();
      }
      case ICE_CF: {
	std::cout << "ICE_CF:" << std::endl;
        return generic_data_.ice_cf_data_.ToString();
      }
      case HKOMDPF: {
	std::cout << "HKOMDPF:" << std::endl;
        return generic_data_.hkomd_pf_data_.ToString();
      }
      case AFLASH: {
	std::cout << "AFLASH:" << std::endl;
        return generic_data_.aflash_data_.ToString();
      }
      case RETAIL: {
	std::cout << "RETAIL:" << std::endl;
        return generic_data_.retail_data_.ToString();
      }
      case NSE: {
	std::cout << "NSE:" << std::endl;
        return generic_data_.nse_data_.ToString();
      }
      case BSE: {
	std::cout << "BSE:" << std::endl;
        return generic_data_.bse_data_.ToString();
      }
      case ASX:
      case ASX_LS: {
	std::cout << "ASX: or ASX_LS:" << std::endl;
        return generic_data_.asx_data_.ToString();
      }
      case OSE_ITCH_PF: {
	std::cout << "OSE_ITCH_PF:" << std::endl;
        return generic_data_.ose_itch_pf_data_.ToString();
      }
      case EOBI_OF: {
	std::cout << "EOBI_OF:" << std::endl;
        return generic_data_.eobi_of_data_.ToString();
      }
      case SGX: {
	std::cout << "SGX:" << std::endl;
        return generic_data_.sgx_data_.ToString();
      }
      case TMX_OBF: {
	std::cout << "TMX_OBF:" << std::endl;
        return generic_data_.tmx_obf_data_.ToString();
      }
      case NSE_L1: {
	std::cout << "NSE_L1:" << std::endl;
        return generic_data_.nse_l1_data_.ToString();
      }
      case TMX_OBF_OF: {
	std::cout << "TMX_OBF_OF:" << std::endl;
        return generic_data_.tmx_orderfeed_data_.ToString();
      }
      case OSE_ITCH_OF: {
	std::cout << "OSE_ITCH_OF:" << std::endl;
        return generic_data_.ose_itch_of_data_.ToString();
      }
      case MICEX_OF: {
	std::cout << "MICEX_OF:" << std::endl;
        return generic_data_.micex_of_data_.ToString();
      }
      default: { return "NOT IMPLEMENTED FOR THIS EXCH"; } break;
    }

    return "NOT IMPLEMENTED FOR THIS EXCH";
  }

  struct timeval getTime() {
    switch (mds_msg_exch_) {
      case CME: {
        return generic_data_.cme_data_.time_;
      }
      case EUREX: {
        return generic_data_.eurex_data_.time_;
      }
      case EOBI_PF: {
        return generic_data_.eobi_pf_data_.time_;
      }
      case BMF: {
        return generic_data_.bmf_data_.time_;
      }
      case NTP:
      case NTP_LS: {
        return generic_data_.ntp_data_.time_;
      }
      case BMF_EQ: {
        return generic_data_.bmf_eq_data_.time_;
      }
      case LIFFE: {
        return generic_data_.liffe_data_.time_;
      }
      case LIFFE_LS: {
        return generic_data_.liffe_data_.time_;
      }
      case OSE_PF: {
        return generic_data_.ose_pf_data_.time_;
      }
      case OSE_CF: {
        return generic_data_.ose_cf_data_.time_;
      }
      case OSE_L1: {
        return generic_data_.ose_l1_data_.time_;
      }
      case CHIX: {
        return generic_data_.chix_data_.time_;
      }
      case CHIX_L1: {
        return generic_data_.chix_l1_data_.time_;
      }
      case HKEX: {
        return generic_data_.hkex_data_.time_;
      }
      case RTS: {
        return generic_data_.rts_data_.time_;
      }
      case RTS_OF: {
        return generic_data_.rts_of_data_.time_;
      }
      case MICEX: {
        return generic_data_.micex_data_.time_;
      }
      case CSM: {
        return generic_data_.csm_data_.time_;
      }
      case TMX:
      case TMX_LS: {
        return time_;
      }
      case ICE: {
        return generic_data_.ice_data_.time_;
      }
      case ICE_LS: {
        return generic_data_.ice_data_.time_;
      }
      case ICE_CF: {
        return generic_data_.ice_cf_data_.time_;
      }
      case HKOMDPF: {
        return generic_data_.hkomd_pf_data_.time_;
      }
      case AFLASH: {
        return generic_data_.aflash_data_.time_;
      }
      case RETAIL: {
        return generic_data_.retail_data_.time_;
      }
      case NSE: {
        return generic_data_.nse_data_.source_time;
      }
      case ASX:
      case ASX_LS: {
        return generic_data_.asx_data_.time_;
      }
      case OSE_ITCH_PF: {
        return generic_data_.ose_itch_pf_data_.time_;
      }
      case EOBI_OF: {
        return generic_data_.eobi_of_data_.time_;
      }
      case SGX: {
        return generic_data_.sgx_data_.time_;
      }
      case TMX_OBF: {
        return generic_data_.tmx_obf_data_.time_;
      }
      case OSE_ITCH_OF: {
        return generic_data_.ose_itch_of_data_.time_;
      }
      case TMX_OBF_OF: {
        return generic_data_.tmx_orderfeed_data_.time_;
      }
      case MICEX_OF: {
        return generic_data_.micex_of_data_.time_;
      }
      case NSE_L1: {
        struct timeval tv;
        tv.tv_sec = generic_data_.nse_l1_data_.time.tv_sec;
        tv.tv_usec = generic_data_.nse_l1_data_.time.tv_usec;
        return tv;
      }
      default: { } break; }
    struct timeval tv;
    return tv;
  }

  bool IsQuincyFeed() { return (mds_msg_exch_ == QUINCY_LS); }

  const char* getContract() {
//    std::cout << "GENERIC: getContract: " << std::endl;
    switch (mds_msg_exch_) {
      case CME: {
        return (generic_data_.cme_data_.getContract());

      } break;
      case CME_LS: {
        return generic_data_.cme_ls_data_.getContract();
      } break;
      case EUREX_LS:
      case EOBI_LS: {
        return generic_data_.eurex_ls_data_.getContract();

      } break;

      case LIFFE_LS:
      case LIFFE: {
        return (generic_data_.liffe_data_.getContract());

      } break;

      case EUREX: {
        return (generic_data_.eurex_data_.getContract());

      } break;

      case EOBI_PF: {
        return (generic_data_.eobi_pf_data_.getContract());

      } break;

      case CHIX_L1: {
        return (generic_data_.chix_l1_data_.getContract());

      } break;

      case OSE_PF: {
        return (generic_data_.ose_pf_data_.getContract());

      } break;

      case OSE_CF: {
        return (generic_data_.ose_cf_data_.getContract());

      } break;

      case CONTROL: {
        return "CONTROL";

      } break;

      case ORS_REPLY: {
        return (generic_data_.ors_reply_data_.getContract());

      } break;

      case RTS: {
        return (generic_data_.rts_data_.getContract());

      } break;

      case RTS_OF: {
        return (generic_data_.rts_of_data_.getContract());
      } break;

      case MICEX: {
        return (generic_data_.micex_data_.getContract());

      } break;

      case NTP:
      case NTP_LS: {
        return (generic_data_.ntp_data_.getContract());

      } break;

      case BMF_EQ: {
        return (generic_data_.bmf_eq_data_.getContract());

      } break;

      case CSM: {
        return (generic_data_.csm_data_.getContract());

      } break;

      case ICE:
      case ICE_LS: {
        return (generic_data_.ice_data_.getContract());

      } break;
      case ICE_CF: {
        return (generic_data_.ice_cf_data_.getContract());
      } break;

      case HKEX: {
        return (generic_data_.hkex_data_.getContract());

      } break;

      case OSE_L1: {
        return (generic_data_.ose_l1_data_.getContract());

      } break;

      case TMX:
      case TMX_LS: {
        return generic_data_.tmx_data_.getContract();
      } break;
      case HKOMDPF: {
        return generic_data_.hkomd_pf_data_.getContract();
      }
      case AFLASH: {
        return generic_data_.aflash_data_.getContract();
      } break;
      case RETAIL: {
        return generic_data_.retail_data_.getContract();
      } break;
      case NSE: {
        return generic_data_.nse_data_.getContract();
      } break;
      case BSE: {
        return generic_data_.bse_data_.getContract();
      } break;
      case ASX:
      case ASX_LS: {
        return generic_data_.asx_data_.getContract();
      } break;
      case OSE_ITCH_PF: {
        return generic_data_.ose_itch_pf_data_.getContract();
      } break;
      case EOBI_OF: {
        return generic_data_.eobi_of_data_.getContract();
      } break;
      case SGX: {
        return generic_data_.sgx_data_.getContract();
      } break;
      case TMX_OBF: {
        return generic_data_.tmx_obf_data_.getContract();
      } break;
      case TMX_OBF_OF: {
        return generic_data_.tmx_orderfeed_data_.getContract();
      } break;
      case NSE_L1: {
        return generic_data_.nse_l1_data_.getContract();
      } break;
      case OSE_ITCH_OF: {
        return generic_data_.ose_itch_of_data_.getContract();
      } break;
      case MICEX_OF: {
        return generic_data_.micex_of_data_.getContract();
      }

      default: { return NULL; } break;
    }
  }
  void SetIntermendiate(bool flag) {
    switch (mds_msg_exch_) {
      case CME: {
        generic_data_.cme_data_.SetIntermediate(flag);

      } break;

      case EUREX_LS:
      case EOBI_LS: {
        generic_data_.eurex_ls_data_.SetIntermediate(flag);
      } break;

      case LIFFE_LS:
      case LIFFE: {
        generic_data_.liffe_data_.SetIntermediate(flag);

      } break;

      case EUREX: {
        generic_data_.eurex_data_.SetIntermediate(flag);

      } break;

      case EOBI_PF: {
        generic_data_.eobi_pf_data_.SetIntermediate(flag);

      } break;

      case CHIX_L1: {
        generic_data_.chix_l1_data_.SetIntermediate(flag);

      } break;

      case OSE_PF: {
        generic_data_.ose_pf_data_.SetIntermediate(flag);

      } break;

      case OSE_CF: {
        generic_data_.ose_cf_data_.SetIntermediate(flag);

      } break;

      case CONTROL: {
      } break;

      case ORS_REPLY: {
      } break;

      case RTS: {
        generic_data_.rts_data_.SetIntermediate(flag);

      } break;

      case RTS_OF: {
        generic_data_.rts_of_data_.SetIntermediate(flag);
      } break;

      case MICEX: {
        generic_data_.micex_data_.SetIntermediate(flag);

      } break;

      case NTP:
      case NTP_LS: {
        generic_data_.ntp_data_.SetIntermediate(flag);

      } break;

      case BMF_EQ: {
        generic_data_.bmf_eq_data_.SetIntermediate(flag);

      } break;

      case CSM: {
        generic_data_.csm_data_.SetIntermediate(flag);

      } break;

      case ICE:
      case ICE_LS: {
        generic_data_.ice_data_.SetIntermediate(flag);

      } break;
      case ICE_CF: {
        generic_data_.ice_cf_data_.SetIntermediate(flag);
      } break;

      case HKEX: {
        generic_data_.hkex_data_.SetIntermediate(flag);

      } break;

      case OSE_L1: {
        generic_data_.ose_l1_data_.SetIntermediate(flag);

      } break;

      case TMX:
      case TMX_LS: {
        //        generic_data_.tmx_data_.SetIntermediate(flag);
      } break;
      case HKOMDPF: {
      } break;
      case AFLASH: {
      } break;
      case RETAIL: {
        generic_data_.retail_data_.SetIntermediate(flag);
      } break;
      case NSE: {
        generic_data_.nse_data_.SetIntermediate(flag);
      } break;
      case ASX:
      case ASX_LS: {
        generic_data_.asx_data_.SetIntermediate(flag);
      } break;
      case OSE_ITCH_PF: {
        generic_data_.ose_itch_pf_data_.SetIntermediate(flag);
      } break;
      case OSE_ITCH_OF: {
        generic_data_.ose_itch_of_data_.SetIntermediate(flag);
      } break;
      case EOBI_OF: {
        generic_data_.eobi_of_data_.SetIntermediate(flag);
      } break;
      case SGX: {
        generic_data_.sgx_data_.SetIntermediate(flag);
      } break;
      case TMX_OBF: {
        generic_data_.tmx_obf_data_.SetIntermediate(flag);
      } break;
      case TMX_OBF_OF: {
        generic_data_.tmx_orderfeed_data_.SetIntermediate(flag);
      } break;
      case NSE_L1: {
        generic_data_.nse_l1_data_.SetIntermediate(flag);
      } break;
      default: { } break; }
  }
  bool IsTrade() {
    switch (mds_msg_exch_) {
      case CME: {
        return (generic_data_.cme_data_.isTradeMsg());

      } break;

      case EUREX_LS:
      case EOBI_LS: {
        return generic_data_.eurex_ls_data_.isTradeMsg();
      } break;

      case LIFFE_LS:
      case LIFFE: {
        return (generic_data_.liffe_data_.isTradeMsg());

      } break;

      case EUREX: {
        return (generic_data_.eurex_data_.isTradeMsg());

      } break;

      case EOBI_PF: {
        return (generic_data_.eobi_pf_data_.isTradeMsg());

      } break;

      case CHIX_L1: {
        return (generic_data_.chix_l1_data_.isTradeMsg());

      } break;

      case OSE_PF: {
        return (generic_data_.ose_pf_data_.isTradeMsg());

      } break;

      case OSE_CF: {
        return (generic_data_.ose_cf_data_.isTradeMsg());

      } break;

      case CONTROL: {
        return false;

      } break;

      case ORS_REPLY: {
        return false;

      } break;

      case RTS: {
        return (generic_data_.rts_data_.isTradeMsg());

      } break;

      case RTS_OF: {
        return (generic_data_.rts_of_data_.isTradeMsg());
      } break;

      case MICEX: {
        return (generic_data_.micex_data_.isTradeMsg());

      } break;

      case NTP:
      case NTP_LS: {
        return (generic_data_.ntp_data_.isTradeMsg());

      } break;

      case BMF_EQ: {
        return (generic_data_.bmf_eq_data_.isTradeMsg());

      } break;

      case CSM: {
        return (generic_data_.csm_data_.isTradeMsg());

      } break;

      case ICE:
      case ICE_LS: {
        return (generic_data_.ice_data_.isTradeMsg());

      } break;
      case ICE_CF: {
        return (generic_data_.ice_cf_data_.isTradeMsg());
      } break;

      case HKEX: {
        return (generic_data_.hkex_data_.isTradeMsg());

      } break;

      case OSE_L1: {
        return (generic_data_.ose_l1_data_.isTradeMsg());

      } break;

      case TMX:
      case TMX_LS: {
        return generic_data_.tmx_data_.isTradeMsg();
      } break;
      case HKOMDPF: {
        return false;  // TODO
      }
      case AFLASH: {
        return false;  // TODO
      } break;
      case RETAIL: {
        return generic_data_.retail_data_.isTradeMsg();
      } break;
      case NSE: {
        return generic_data_.nse_data_.isTradeMsg();
      } break;
      case ASX:
      case ASX_LS: {
        return generic_data_.asx_data_.isTradeMsg();
      } break;
      case OSE_ITCH_PF: {
        return generic_data_.ose_itch_pf_data_.isTradeMsg();
      } break;
      case OSE_ITCH_OF: {
        return generic_data_.ose_itch_of_data_.isTradeMsg();
      } break;
      case EOBI_OF: {
        return generic_data_.eobi_of_data_.isTradeMsg();
      } break;
      case SGX: {
        return generic_data_.sgx_data_.isTradeMsg();
      } break;
      case TMX_OBF: {
        return generic_data_.tmx_obf_data_.isTradeMsg();
      } break;
      case TMX_OBF_OF: {
        return generic_data_.tmx_orderfeed_data_.isTradeMsg();
      } break;
      case MICEX_OF: {
        return generic_data_.micex_of_data_.isTradeMsg();
      }
      case NSE_L1: {
        return generic_data_.nse_l1_data_.isTradeMsg();
      } break;
      default: { return false; } break;
    }
  }

  HFSAT::ExchSource_t GetExchangeSourceFromGenericStruct() { return GetExchangeFromMDSMessageType(mds_msg_exch_); }

  static HFSAT::ExchSource_t GetExchangeFromMDSMessageType(MDSMessageExchType mds_msg_exch) {
    HFSAT::ExchSource_t this_struct_exchange_source = HFSAT::kExchSourceMAX;

    switch (mds_msg_exch) {
      case CME: {
        this_struct_exchange_source = HFSAT::kExchSourceCME;
      } break;
      case EUREX: {
        this_struct_exchange_source = HFSAT::kExchSourceEUREX;
      } break;
      case TMX:
      case TMX_LS: {
        this_struct_exchange_source = HFSAT::kExchSourceTMX;
      } break;
      case BMF: {
        this_struct_exchange_source = HFSAT::kExchSourceBMF;
      } break;
      case BMF_EQ: {
        this_struct_exchange_source = HFSAT::kExchSourceBMFEQ;
      } break;
      case NTP:
      case NTP_LS: {
        this_struct_exchange_source = HFSAT::kExchSourceNTP;
      } break;
      case LIFFE: {
        this_struct_exchange_source = HFSAT::kExchSourceLIFFE;
      } break;
      case CME_LS: {
        this_struct_exchange_source = HFSAT::kExchSourceCME;
      } break;
      case EUREX_LS: {
        this_struct_exchange_source = HFSAT::kExchSourceEUREX;
      } break;
      case OSE_PF: {
        this_struct_exchange_source = HFSAT::kExchSourceJPY;
      } break;
      case OSE_CF: {
        this_struct_exchange_source = HFSAT::kExchSourceJPY;
      } break;
      case OSE_L1: {
        this_struct_exchange_source = HFSAT::kExchSourceJPY;
      } break;
      case CHIX: {
        this_struct_exchange_source = HFSAT::kExchSourceBATSCHI;
      } break;
      case CHIX_L1: {
        this_struct_exchange_source = HFSAT::kExchSourceBATSCHI;
      } break;
      case HKEX: {
        this_struct_exchange_source = HFSAT::kExchSourceHONGKONG;
      } break;
      case RTS: {
        this_struct_exchange_source = HFSAT::kExchSourceRTS;
      } break;
      case RTS_OF: {
        this_struct_exchange_source = HFSAT::kExchSourceRTS;
      } break;
      case MICEX: {
        this_struct_exchange_source = HFSAT::kExchSourceMICEX;
      } break;
      case EOBI_LS: {
        this_struct_exchange_source = HFSAT::kExchSourceEUREX;
      } break;
      case EOBI_PF: {
        this_struct_exchange_source = HFSAT::kExchSourceEUREX;
      } break;
      case EOBI_OF: {
        this_struct_exchange_source = HFSAT::kExchSourceEUREX;
      } break;
      case LIFFE_LS: {
        this_struct_exchange_source = HFSAT::kExchSourceLIFFE;
      } break;
      case CSM: {
        this_struct_exchange_source = HFSAT::kExchSourceCFE;
      } break;
      case ICE: {
        this_struct_exchange_source = HFSAT::kExchSourceICE;
      } break;
      case ICE_LS: {
        this_struct_exchange_source = HFSAT::kExchSourceICE;
      } break;
      case ICE_CF: {
        this_struct_exchange_source = HFSAT::kExchSourceICE;
      } break;
      case EBS: {
        this_struct_exchange_source = HFSAT::kExchSourceEBS;
      } break;
      case HKOMD: {
        this_struct_exchange_source = HFSAT::kExchSourceHKOMD;
      } break;
      case HKOMDPF: {
        this_struct_exchange_source = HFSAT::kExchSourceHKOMDPF;
      } break;
      case AFLASH: {
        this_struct_exchange_source = HFSAT::kExchSourceAFLASH;
      } break;
      case RETAIL: {
        this_struct_exchange_source = HFSAT::kExchSourceRETAIL;
      } break;
      case NSE: {
        this_struct_exchange_source = HFSAT::kExchSourceNSE;
      } break;
      case BSE: {
        this_struct_exchange_source = HFSAT::kExchSourceBSE;
      } break;
      case ASX:
      case ASX_LS: {
        this_struct_exchange_source = HFSAT::kExchSourceASX;
      } break;
      case OSE_ITCH_PF: {
        this_struct_exchange_source = HFSAT::kExchSourceJPY;
      } break;
      case SGX: {
        this_struct_exchange_source = HFSAT::kExchSourceSGX;
      } break;
      case TMX_OBF: {
        this_struct_exchange_source = HFSAT::kExchSourceTMX;
      } break;
      case OSE_ITCH_OF: {
        this_struct_exchange_source = HFSAT::kExchSourceJPY;
      } break;
      case NSE_L1: {
        this_struct_exchange_source = HFSAT::kExchSourceNSE;
      } break;
      case TMX_OBF_OF: {
        this_struct_exchange_source = HFSAT::kExchSourceTMX;
      } break;
      case MICEX_OF: {
        this_struct_exchange_source = HFSAT::kExchSourceMICEX;
      }

      case ORS_REPLY:
      case CONTROL: {
        this_struct_exchange_source = HFSAT::kExchSourceORS;
      } break;

      default: { this_struct_exchange_source = HFSAT::kExchSourceMAX; } break;
    }

    return this_struct_exchange_source;
  }

  std::string GetExchangeSourceStringFromGenericStruct() {
    std::string exchange = "";

    switch (mds_msg_exch_) {
      case CME: {
        exchange = EXCHANGE_KEYS::kExchSourceCMEStr;
      } break;
      case EUREX: {
        exchange = EXCHANGE_KEYS::kExchSourceEUREXStr;
      } break;
      case TMX: {
        exchange = EXCHANGE_KEYS::kExchSourceTMXStr;
      } break;
      case BMF: {
        exchange = EXCHANGE_KEYS::kExchSourceBMFStr;
      } break;
      case BMF_EQ: {
        exchange = "BMF_EQ";
      } break;
      case NTP:
      case NTP_LS: {
        exchange = EXCHANGE_KEYS::kExchSourceNTPStr;
      } break;
      case LIFFE: {
        exchange = EXCHANGE_KEYS::kExchSourceLIFFEStr;
      } break;
      case CME_LS: {
        exchange = "CME_LS";
      } break;
      case EUREX_LS: {
        exchange = "EUREX_LS";
      } break;
      case OSE_PF: {
        exchange = "OSE_PF";
      } break;
      case OSE_CF: {
        exchange = "OSE_CF";
      } break;
      case OSE_ITCH_PF:
      case OSE_ITCH_OF: {
        exchange = "OSE_ITCH";
      } break;
      case OSE_L1: {
        exchange = EXCHANGE_KEYS::kExchSourceJPY_L1Str;
      } break;
      case CHIX: {
        exchange = EXCHANGE_KEYS::kExchSourceBATSCHIStr;
      } break;
      case CHIX_L1: {
        exchange = "CHIX_L1";
      } break;
      case HKEX: {
        exchange = "HKEX";
      } break;
      case RTS: {
        exchange = EXCHANGE_KEYS::kExchSourceRTSStr;
      } break;
      case MICEX: {
        exchange = EXCHANGE_KEYS::kExchSourceMICEXStr;
      } break;
      case EOBI_LS: {
        exchange = "EOBI_LS";
      } break;
      case EOBI_PF: {
        exchange = "EOBI_PF";
      } break;
      case EOBI_OF: {
        exchange = "EOBI_OF";
      } break;
      case LIFFE_LS: {
        exchange = "LIFFE_LS";
      } break;
      case CSM: {
        exchange = "CSM";
      } break;
      case ICE: {
        exchange = EXCHANGE_KEYS::kExchSourceICEStr;
      } break;
      case ICE_LS: {
        exchange = "ICE_LS";
      } break;
      case ICE_CF: {
        exchange = EXCHANGE_KEYS::kExchSourceICECFStr;
      } break;
      case EBS: {
        exchange = EXCHANGE_KEYS::kExchSourceEBSStr;
      } break;
      case HKOMD: {
        exchange = EXCHANGE_KEYS::kExchSourceHKOMDStr;
      } break;
      case HKOMDPF: {
        exchange = "HKOMDPF";
      } break;
      case AFLASH: {
        exchange = EXCHANGE_KEYS::kExchSourceAFLASHStr;
      } break;
      case RETAIL: {
        exchange = EXCHANGE_KEYS::kExchSourceRETAILStr;
      } break;
      case NSE: {
        exchange = EXCHANGE_KEYS::kExchSourceNSEStr;
      } break;
      case BSE: {
        exchange = EXCHANGE_KEYS::kExchSourceBSEStr;
      } break;
      case ASX:
      case ASX_LS: {
        exchange = EXCHANGE_KEYS::kExchSourceASXStr;
      } break;
      case SGX: {
        exchange = EXCHANGE_KEYS::kExchSourceSGXStr;
      } break;
      case TMX_OBF: {
        exchange = EXCHANGE_KEYS::kExchSourceTMXStr;
      } break;
      case NSE_L1: {
        exchange = EXCHANGE_KEYS::kExchSourceNSEStr;
      } break;
      case CONTROL: {
        exchange = "CONTROL";
      } break;
      case MICEX_OF: {
        exchange = EXCHANGE_KEYS::kExchSourceMICEXStr;
      }
      default: { exchange = ""; } break;
    }

    return exchange;
  }

  char const* getShortcode() {
    return "INVALID";
  }
};
}
}
