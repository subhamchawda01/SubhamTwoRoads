/**
   \file MarketAdapter/market_defines.hpp

   \author: (c) Copyright Two Roads Technological Solutions Pvt Ltd 2011
   Address:
   Suite No 353, Evoma, #14, Bhattarhalli,
   Old Madras Road, Near Garden City College,
   KR Puram, Bangalore 560049, India
   +91 80 4190 3551
 */
#ifndef BASE_MARKETADAPTER_MARKET_DEFINES_H
#define BASE_MARKETADAPTER_MARKET_DEFINES_H

#include <string>
#include "dvccode/CDef/defines.hpp"

#define MAX_LEVELS 10
#define NUM_PRICETYPES_IN_OFFLINEMIXMMS 5
#define NUM_CONST_IN_ONLINEMIXPRICE 2
#define DEFAULT_OFFLINEMIXMMS_FILE "/spare/local/tradeinfo/OfflineInfo/offline_mix_mms.txt"
#define DEFAULT_OFFLINEMIXMMS_FILE_AS "/spare/local/tradeinfo/OfflineInfo/offline_mix_mms_as.txt"
#define DEFAULT_OFFLINEMIXMMS_FILE_EU "/spare/local/tradeinfo/OfflineInfo/offline_mix_mms_eu.txt"
#define DEFAULT_OFFLINEMIXMMS_FILE_US "/spare/local/tradeinfo/OfflineInfo/offline_mix_mms_us.txt"
#define DEFAULT_ONLINE_MIX_PRICE_FILE "/spare/local/tradeinfo/OnlineInfo/online_price.txt"
#define DEFAULT_ONLINE_MIX_PRICE_FILE_AS "/spare/local/tradeinfo/OnlineInfo/online_price_as.txt"
#define DEFAULT_ONLINE_MIX_PRICE_FILE_EU "/spare/local/tradeinfo/OnlineInfo/online_price_eu.txt"
#define DEFAULT_ONLINE_MIX_PRICE_FILE_US "/spare/local/tradeinfo/OnlineInfo/online_price_us.txt"
#define DEFAULT_ONLINE_BETA_KALMAN_FILE "/spare/local/tradeinfo/OnlineInfo/online_beta_kalman.txt"
#define DEFAULT_ONLINE_BETA_KALMAN_FILE_AS "/spare/local/tradeinfo/OnlineInfo/online_beta_kalman_as.txt"
#define DEFAULT_ONLINE_BETA_KALMAN_FILE_EU "/spare/local/tradeinfo/OnlineInfo/online_beta_kalman_eu.txt"
#define DEFAULT_ONLINE_BETA_KALMAN_FILE_US "/spare/local/tradeinfo/OnlineInfo/online_beta_kalman_us.txt"

namespace HFSAT {
typedef enum {
  kPriceTypeMidprice,              // 0
  kPriceTypeMktSizeWPrice,         // 1
  kPriceTypeMktSinusoidal,         // 2
  kPriceTypeOrderWPrice,           // 3
  kPriceTypeTradeWPrice,           // 4
  kPriceTypeOfflineMixMMS,         // 5
  kPriceTypeValidLevelMidPrice,    // 6
  kPriceTypeSpreadAggLegBidPrice,  // 7
  kPriceTypeSpreadAggLegAskPrice,  // 8
  kPriceTypeBidPrice,              // 9
  kPriceTypeAskPrice,              // 10
  kPriceTypeTradeBasePrice,        // 11
  kPriceTypeTradeMktSizeWPrice,    // 12
  kPriceTypeTradeMktSinPrice,      // 13
  kPriceTypeTradeOrderWPrice,      // 14
  kPriceTypeTradeTradeWPrice,      // 15
  kPriceTypeTradeOmixPrice,        // 16
  kPriceTypeOrderSizeWPrice,       // 17
  kPriceTypeOnlineMixPrice,        // 18
  kPriceTypeStableBidPrice,        // 19
  kPriceTypeStableAskPrice,        // 20
  kPriceTypeImpliedVol,            // 21
  kPriceTypeBandPrice,             // 22
  kPriceTypeProRataMktSizeWPrice,  // 23
  kPriceTypeDUMMY,                 // 24
  kPriceTypeMax                    // 25
} PriceType_t;

static std::string PriceTypeStrings[] = {
    "Midprice", "MktSizeWPrice", "MktSinusoidal", "OrderWPrice", "TradeWPrice", "OfflineMixMMS", "ValidLevelMidPrice",
    "SpreadAggLegBidPrice", "SpreadAggLegAskPrice", "BidPrice", "AskPrice", "TradeBasePrice", "TradeMktSizeWPrice",
    "TradeMktSinPrice", "TradeOrderWPrice", "TradeTradeWPrice", "TradeOmixPrice", "OrderSizeWPrice", "OnlineMixPrice",
    "StableBidPrice", "StableAskPrice", "ImpliedVol", "BandPrice", "ProRataMktSizeWPrice", "DUMMY", "Max"};

PriceType_t StringToPriceType_t(std::string t_in_str_);

inline std::string PriceType_t_To_FullString(PriceType_t t_pxtype_) {
  if (t_pxtype_ >= 0 && t_pxtype_ < kPriceTypeMax) {
    return PriceTypeStrings[t_pxtype_];
  } else {
    return PriceTypeStrings[kPriceTypeMax];
  }
}

inline const char* PriceType_t_To_String(PriceType_t t_pxtype_) {
  switch (t_pxtype_) {
    case kPriceTypeMidprice:
      return "Mid";
    case kPriceTypeMktSizeWPrice:
      return "Mkt";
    case kPriceTypeMktSinusoidal:
      return "Sin";
    case kPriceTypeOrderWPrice:
      return "Owp";
    case kPriceTypeTradeWPrice:
      return "Twp";
    case kPriceTypeOfflineMixMMS:
      return "OMix";
    case kPriceTypeValidLevelMidPrice:
      return "VLMid";
    case kPriceTypeSpreadAggLegBidPrice:
      return "SpdAggLegBid";
    case kPriceTypeSpreadAggLegAskPrice:
      return "SpdAggLegAsk";
    case kPriceTypeBidPrice:
      return "Bid";
    case kPriceTypeAskPrice:
      return "Ask";
    case kPriceTypeTradeBasePrice:
      return "Tbp";
    case kPriceTypeTradeMktSizeWPrice:
      return "TMSzP";
    case kPriceTypeTradeMktSinPrice:
      return "TMSinP";
    case kPriceTypeTradeOrderWPrice:
      return "Top";
    case kPriceTypeTradeTradeWPrice:
      return "Ttp";
    case kPriceTypeTradeOmixPrice:
      return "TOmixP";
    case kPriceTypeOrderSizeWPrice:
      return "Osp";
    case kPriceTypeOnlineMixPrice:
      return "OnMix";
    case kPriceTypeStableBidPrice:
      return "SBid";
    case kPriceTypeStableAskPrice:
      return "SAsk";
    case kPriceTypeImpliedVol:
      return "IVol";
    case kPriceTypeBandPrice:
      return "BandPx";
    case kPriceTypeProRataMktSizeWPrice:
      return "ProRataMkt";
    case kPriceTypeDUMMY:
      return "DUMMY";
    default:
      return "-P-";
  }
  return "-P-";
}
}
#endif  // BASE_MARKETADAPTER_MARKET_DEFINES_H
