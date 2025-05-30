/**
   \file dvccode/CDef/mds_utils.hpp

   \author: (c) Copyright Two Roads Technological Solutions Pvt Ltd 2011
       Address:
       Suite 217, Level 2, Prestige Omega,
       No 104, EPIP Zone, Whitefield,
       Bangalore - 560066
       India
       +91 80 4060 0717
*/
#pragma once
#include "dvccode/CDef/stored_market_data_common_message_defines.hpp"

namespace HFSAT {
namespace MDS_UTILS {
inline uint32_t static getTradeVol(const EUREX_MDS::EUREXCommonStruct& msg, bool& isTrade) {
  isTrade = msg.msg_ == EUREX_MDS::EUREX_TRADE;
  if (!isTrade) return 0;
  return msg.data_.eurex_trds_.trd_qty_;
}

inline uint32_t static getTradeVol(const CME_MDS::CMECommonStruct& msg, bool& isTrade) {
  isTrade = msg.msg_ == CME_MDS::CME_TRADE;
  if (!isTrade) return 0;
  return msg.data_.cme_trds_.trd_qty_;
}

inline uint32_t static getTradeVol(const BMF_MDS::BMFCommonStruct& msg, bool& isTrade) {
  isTrade = msg.msg_ == BMF_MDS::BMF_TRADE;
  if (!isTrade) return 0;
  return msg.data_.bmf_trds_.trd_qty_;
}

inline uint32_t static getTradeVol(const NTP_MDS::NTPCommonStruct& msg, bool& isTrade) {
  isTrade = msg.msg_ == NTP_MDS::NTP_TRADE;
  if (!isTrade) return 0;
  return msg.data_.ntp_trds_.trd_qty_;
}

inline uint32_t static getTradeVol(const TMX_MDS::TMXCommonStruct& msg, bool& isTrade) {
  isTrade = msg.msg_ == TMX_MDS::TMX_TRADE;
  if (!isTrade) return 0;
  return msg.data_.tmx_trds_.trd_qty_;
}

inline uint32_t static getTradeVol(const LIFFE_MDS::LIFFECommonStruct& msg, bool& isTrade) {
  isTrade = msg.msg_ == LIFFE_MDS::LIFFE_TRADE;
  if (!isTrade) return 0;
  return msg.data_.liffe_trds_.trd_qty_;
}

inline uint32_t static getTradeVol(const HKEX_MDS::HKEXCommonStruct& next_event_, bool& isTrade) {
  if (next_event_.msg_ == HKEX_MDS::HKEX_TRADE) {
    isTrade = true;
    return next_event_.data_.hkex_trds_.trd_qty_;
  }
  return 0;
}

inline uint32_t static getTradeVol(const OSE_MDS::OSECommonStruct& next_event_, bool& isTrade) {
  if (next_event_.msg_ == OSE_MDS::OSE_TRADE) {
    isTrade = true;
    return next_event_.data_.ose_trds_.trd_qty_;
  }
  return 0;
}

inline uint32_t static getTradeVol(const OSE_MDS::OSEPriceFeedCommonStruct& next_event_, bool& isTrade) {
  if (next_event_.type_ == 2) {
    isTrade = true;
    return next_event_.size;
  }
  return 0;
}
}
}
