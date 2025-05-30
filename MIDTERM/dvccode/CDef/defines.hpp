/**
   \file dvccode/CDef/defines.hpp

   \author: (c) Copyright Two Roads Technological Solutions Pvt Ltd 2011
   Address:
   Suite No 353, Evoma, #14, Bhattarhalli,
   Old Madras Road, Near Garden City College,
   KR Puram, Bangalore 560049, India
   +91 80 4190 3551
 */
#pragma once

#include <string.h>
#include <strings.h>
#include <string>
#include <unistd.h>
#include <map>
#include <sstream>
#include <fstream>

#include "dvccode/CDef/assumptions.hpp"
#include "dvccode/Utils/exchange_names.hpp"

#define PROD_DIR "/home/pengine/prod/"
#define PROD_CONFIGS_DIR PROD_DIR "live_configs/"
#define PROD_EXECS_DIR PROD_DIR "live_execs/"
#define PROD_SCRIPTS_DIR PROD_DIR "live_scripts/"

#define IS_SAME_SIGN(a, b) (((a) > 0) ^ ((b) < 0))

#define SPREADINFO_DIR "/spare/local/tradeinfo/SpreadInfo/"

#define USING_NSE_FULLBOOK_DATA_FROM 20150810
#define USING_TBT_FROM 20150910
#define USING_OSE_ITCH_FROM 20160719
#define USING_TMX_OBF_FROM 20161025
#define USING_ASX_NTP_ITCH_FROM 20170319
#define USING_CME_OBF 20170613
#define USING_OSE_OF_BOOK_FROM 20171214
#define USING_BMF_FPGA_FROM 20171225

#define kDefFileName 512
#define kSecNameLen 16
#define kAccountLen 16
#define THROTTLE_MSG_LIMIT 135    // Max msgs per seconds
#define THROTTLE_MSG_LIMIT_IOC 0  // Separate IOC order throttling
#define DEF_MARKET_DEPTH 15
#define MAX_BMF_ORDER_BOOK_DEPTH 800
#define MAX_OSE_ORDER_BOOK_DEPTH 5000
#define kInvalidArrayIndex -1

// color codes
#define RED "\x1B[31m"
#define GREEN "\x1B[32m"
#define BLUE "\x1B[34m"

#define kInvalidPosition 100000
#define kInvalidPrice -1000.00
#define kInvalidIntPrice -100000

#define NOT_USING_ZERO_LOGGIN_ORS 0

#define BASETRADEINFODIR "/spare/local/tradeinfo/"
#define BASESYSINFODIR "infracore_install/SysInfo/"  // always to be used with FileUtils::AppendHome
#define NEWBASESYSINFODIR \
  "/spare/local/tradeinfo/SysInfo/"  // always to be used with FileUtils::AppendHome (New SysInfo dir)
#define BASEFILESDIR "infracore_install/files/"

#define USE_SHM_FOR_LIVESOURCES 1

#if USE_SHM_FOR_LIVESOURCES

#define DUMMY_MCAST_IP_FOR_SHM "225.2.2.4"
#define DUMMY_MCAST_PORT_FOR_SHM 12345

#define EUREX_NTA_IP "239.23.0.10"
#define EUREX_NTA_PORT 27610

#define SHM_KEY_EUREX 5678
#define SHM_KEY_CME 5679
#define SHM_KEY_TMX 5680
#define SHM_KEY_BMF 5681
#define SHM_KEY_NTP 5682
#define SHM_KEY_NTP_ORD 5683
#define SHM_KEY_LIFFE 5684
#define SHM_KEY_LIFFE_RAW 5691
#define SHM_KEY_HK 5692
#define SHM_KEY_OSE 5693
#define SHM_KEY_OSE_PRICEFEED 5694
#define SHM_KEY_TSE 5695
#define SHM_KEY_BCASTER 5696
#define SHM_CME_GLOBEX_FULLBOOK_KEY 5697
#define SHM_KEY_QUINCY 5698
#define SHM_KEY_EBS 8000
#define SHM_KEY_ESPEED_RAW 5698
#define SHM_KEY_PLAZA2 5701
#define SHM_KEY_RTS 5702
#define SHM_KEY_OSE_COMBINED_FEED 5703

#define QUINCY_LS_SHM_QUEUE_SIZE 4096
#define ESPEED_RAW_SHM_QUEUE_SIZE 4096
#define CME_LS_SHM_QUEUE_SIZE 8192
#define EUREX_LS_SHM_QUEUE_SIZE 4096
#define TMX_LS_SHM_QUEUE_SIZE 4096
#define HK_SHM_QUEUE_SIZE 4096
#define LIFFE_RAW_SHM_QUEUE_SIZE 4096
#define CHIX_L1_SHM_QUEUE_SIZE 4096
#define OSE_SHM_QUEUE_SIZE 8192
#define TSE_SHM_QUEUE_SIZE 4096
#define CME_GLOBEX_FULLBOOK_SHM_QUEUE_SIZE 4096
#define PLAZA2_SHM_QUEUE_SIZE 4096
#define RTS_SHM_QUEUE_SIZE 4096
#define EBS_SHM_QUEUE_SIZE 4096

#endif

#define SHM_KEY_EUREX_RAW 5685
#define SHM_KEY_CME_RAW 5686
#define SHM_KEY_NTP_RAW 5687
#define SHM_KEY_CHIX_L1 5688
#define SHM_KEY_EUREX_NTA 5695
#define SHM_KEY_EOBI_RAW 5700
#define SHM_KEY_CSM_RAW 5701
#define SHM_KEY_HKOMD_RAW 5710
#define SHM_KEY_HKOMD_PF_RAW 5711
#define SHM_KEY_BMF_FPGA 5712

#define CME_RAW_SHM_QUEUE_SIZE 8192
#define ESPEED_RAW_SHM_QUEUE_SIZE 4096
#define EUREX_RAW_SHM_QUEUE_SIZE 4096
#define EUREX_NTA_RAW_SHM_QUEUE_SIZE 4096
#define EOBI_RAW_SHM_QUEUE_SIZE 4096
#define NTP_RAW_SHM_QUEUE_SIZE 4096
#define PUMA_RAW_SHM_QUEUE_SIZE 4096
#define CSM_RAW_SHM_QUEUE_SIZE 4096
#define BMF_FPGA_SHM_QUEUE_SIZE 131072

#define BASETRADEOFFLINECONFIGSDIR "basetrade/OfflineConfigs/"
#define BASEINFRAOFFLINECONFIGSDIR "baseinfra/OfflineConfigs/"
#define BASETRADEFILESDIR "basetrade/files/"

#define ORS_SHM_KEY_CONFIG_FILE "/spare/local/files/ORS_SHM_CONFIG_FILE.txt"
#define SIMULATION_SERVER "sdv-cfe-srv13"

// To enable use of Order level Book define ENABLE_USE_OF_ORDER_LEVEL_DEPTH_BOOK
//#define ENABLE_USE_OF_ORDER_LEVEL_DEPTH_BOOK
//

enum IPCMode { MULTICAST = 1, SHAREDMEM };

typedef int ClientSeq;
typedef int ServerSeq;
typedef unsigned long ulong;

namespace HFSAT {
#define USE_LOW_BANDWIDTH_EUREX_MDS_STRUCTS true
#define USE_LOW_BANDWIDTH_CME_MDS_STRUCTS true

#define COMPUTE_TIME_ON_LOW_BANDWIDTH_LS true
#define SEND_TIME_OVER_LIVE_SOURCE false

typedef enum {
  kORSOrderAllowed = 0,  // this is not really a rejection reason, set so that return value of MarginChecker::Allows is
                         // a ORSRejectionReason_t, and is 0 when trade is allowed
  kORSRejectSecurityNotFound,
  kORSRejectMarginCheckFailedOrderSizes,
  kORSRejectMarginCheckFailedMaxPosition,
  kORSRejectMarginCheckFailedWorstCasePosition,
  kExchOrderReject,
  kSendOrderRejectNotMinOrderSizeMultiple,
  kORSRejectMarginCheckFailedMaxLiveOrders,
  kORSRejectSelfTradeCheck,
  kORSRejectThrottleLimitReached,
  kORSRejectMarketClosed,
  kORSRejectNewOrdersDisabled,
  kORSRejectFailedPriceCheck,
  kORSRejectETIAlgoTaggingFailure,
  kORSRejectFOKSendFailed,
  kORSRejectFailedIOCOrder,
  kORSRejectMarginCheckFailedBidNotionalValue,
  kORSRejectMarginCheckFailedAskNotionalValue,
  kORSRejectNSEComplianceFailure,
  kTAPRejectThrottleLimitReached,
  kORSRejectModifyOrdersDisabled,
  kORSRejectMarginCheckFailedSumQueryWorstCasePosition,
  kORSRejectFailedPnlCheck,
  kORSRejectFailedGrossMarginCheck,
  kORSRejectFailedNetMarginCheck,
  kORSRejectICEAlgoTaggingFailure,
  kORSRejectMiFidLimitsBreached
} ORSRejectionReason_t;

typedef enum {
  kCxlRejectReasonTooLate = 0,
  kCxlRejectReasonUnknownOrder = 1,
  kCxlRejectReasonOrderNotConfirmed = 2,
  kCxlRejectReasonOrderAlreadyInPendingQueue = 3,
  kCxlRejectReasonDuplicateClOrdID = 6,
  kCxlRejectReasonThrottle,
  kCxlRejectReasonMarketClosed,
  kCxlRejectReasonOther = 15,
  kCxlRejectReasonTAPThrottle,
  kExchCancelReject
} CxlRejectReason_t;

typedef enum {
  kORSModifyOrderAllowed =
      0,  // this is not really a rejection reason, set so that return value of MarginChecker::Allows is
          // a ORSRejectionReason_t, and is 0 when trade is allowed
  kORSCxReRejectSecurityNotFound,
  kORSCxReRejectMarginCheckFailedOrderSizes,
  kORSCxReRejectMarginCheckFailedMaxPosition,
  kORSCxReRejectMarginCheckFailedWorstCasePosition,
  kExchCancelReplaceReject,
  kModifyOrderRejectNotMinOrderSizeMultiple,
  kORSCxReRejectMarginCheckFailedMaxLiveOrders,
  kORSCxReRejectSelfTradeCheck,
  kORSCxReRejectThrottleLimitReached,
  kORSCxReRejectMarketClosed,
  kORSCxReRejectNewOrdersDisabled,
  kORSCxReRejectFailedPriceCheck,
  kORSCxReRejectETIAlgoTaggingFailure,
  kORSCxReRejectFOKSendFailed,
  kORSCxReRejectFailedIOCOrder,
  kORSCxReRejectMarginCheckFailedBidNotionalValue,
  kORSCxReRejectMarginCheckFailedAskNotionalValue,
  kORSCxReRejectNSEComplianceFailure,
  kCxReTAPRejectThrottleLimitReached,
  kCxlReRejectOrderNotFound
} CxlReplaceRejectReason_t;

typedef enum {
  kExchSourceInvalid,
  kExchSourceCME,
  kExchSourceEUREX,
  kExchSourceBMF,
  kExchSourceBMFEQ,
  kExchSourceNTP,
  kExchSourceTMX,
  kExchSourceMEFF,
  kExchSourceIDEM,
  kExchSourceHONGKONG,
  kExchSourceREUTERS,
  kExchSourceICE,
  kExchSourceEBS,
  kExchSourceLIFFE,
  kExchSourceRTS,
  kExchSourceMICEX,
  kExchSourceMICEX_EQ,
  kExchSourceMICEX_CR,
  kExchSourceLSE,
  kExchSourceNASDAQ,
  kExchSourceBATSCHI,
  kExchSourceHYB,
  kExchSourceJPY,
  kExchSourceJPY_L1,
  kExchSourceTSE,
  kExchSourceQUINCY,
  kExchSourceCMEMDP,
  kExchSourceEOBI,
  kExchSourceESPEED,
  kExchSourcePUMA,
  kExchSourceCFE,
  kExchSourceCFNMicroware,
  kExchSourceHKOMD,
  kExchSourceHKOMDPF,
  kExchSourceHKOMDCPF,
  kExchSourceASX,
  kExchSourceSGX,
  kExchSourceAFLASH,
  kExchSourceRETAIL,
  kExchSourceNSE,
  kExchSourceBSE,
  kExchSourceNSE_FO,
  kExchSourceNSE_CD,
  kExchSourceNSE_EQ,
  kExchSourceICEFOD,
  kExchSourceICEPL,
  kExchSourceICECF,
  kExchSourceAFLASH_NEW,
  kExchSourceKRX,
  kExchSourceCME_FPGA,
  kExchSourceORS,
  kExchSourceBMF_FPGA,
  kExchSourceMAX
} ExchSource_t;

typedef enum { kInsTypeCR, kInsTypeEQ, kInsTypeFO } InsType_t;

/// Enum that describes location of an action, or where data was logged
typedef enum {
  kTLocCHI,
  kTLocFR2,
  kTLocNY4,
  kTLocTMX,
  kTLocBMF,
  kTLocMAD,
  kTLocMIL,
  kTLocHK,
  kTLocJPY,
  kTLocIX4,
  kTLocBSL,
  kTLocCRT,
  kTLocOTK,
  kTLocM1,
  kTLocM1Old,
  kTLocCFE,
  kTLocSYD,
  kTLocNSE,
  kTLocSPR,
  kTLocBSE,
  kTLocKRX,
  kTLocMAX  ///< introduced to keep an error case
} TradingLocation_t;

typedef enum { kTradeTypeBuy = 0, kTradeTypeSell, kTradeTypeNoInfo } TradeType_t;

typedef enum {
  // powers of two to support bitmask-based checks for multiple simultaneous modes (eg- DATA and LOGGER)
  kReference = 1,
  kRaw = 1 << 1,
  kMcast = 1 << 2,
  kLogger = 1 << 3,
  kMcastL1 = 1 << 4,
  kProShm = 1 << 5,
  kOfflineRaw = 1 << 6,
  kComShm = 1 << 7,
  kComShmConsumer = 1 << 8,
  kComShmLogger = 1 << 9,
  kLiveConsumer = 1 << 10,
  kPriceFeedLogger = 1 << 11,
  kConvertLogger = 1 << 12,
  kRecoveryHost = 1 << 13,
  kMidTerm = 1 << 14,
  kMcastOF = 1 << 15,
  kModeMax = 1 << 16
} FastMdConsumerMode_t;

// Custom | function to support multiple modes simultaneously
inline FastMdConsumerMode_t operator|(FastMdConsumerMode_t mode_1, FastMdConsumerMode_t mode_2) {
  return static_cast<FastMdConsumerMode_t>(static_cast<int>(mode_1) | static_cast<int>(mode_2));
}

// Custom & function to support multiple modes simultaneously
inline FastMdConsumerMode_t operator&(FastMdConsumerMode_t mode_1, FastMdConsumerMode_t mode_2) {
  return static_cast<FastMdConsumerMode_t>(static_cast<int>(mode_1) & static_cast<int>(mode_2));
}

// Custom ~ function to support multiple modes simultaneously
inline FastMdConsumerMode_t operator~(FastMdConsumerMode_t mode_1) {
  return static_cast<FastMdConsumerMode_t>(~static_cast<int>(mode_1));
}

// String -> FastMdConsumerMode_t
inline HFSAT::FastMdConsumerMode_t GetModeFromString(const std::string mode) {
  if (mode == "REFERENCE") {
    return HFSAT::kReference;
  } else if (mode == "LOGGER") {
    return HFSAT::kLogger;
  } else if (mode == "SHMWRITER") {
    return HFSAT::kProShm;
  } else if (mode == "CONVERT_LOGGER") {
    return HFSAT::kConvertLogger;
  } else if (mode == "PF_CONVERTOR") {
    return HFSAT::kPriceFeedLogger;
  } else if (mode == "PF_SHMWRITER") {
    return HFSAT::kComShmLogger;
  } else if (mode == "DATA") {
    return HFSAT::kMcast;
  } else {
    return HFSAT::kModeMax;
  }
}

inline const char GetTradeTypeChar(TradeType_t _trade_type_) {
  switch (_trade_type_) {
    case kTradeTypeBuy:
      return 'B';
    case kTradeTypeSell:
      return 'S';
    default:
      return '-';
  }
  return '-';
}

struct Char16_t {
  char c_str_[16];

  Char16_t() { bzero(c_str_, 16); }
  Char16_t(const char* _src_str_) {
    bzero(c_str_, 16);
    if (_src_str_ != NULL) {
      strncpy(c_str_, _src_str_, 16);
    }
  }
  inline void SetString(const char* _src_str_) {
    if (_src_str_ != NULL) {
      strncpy(c_str_, _src_str_, 16);
    }
  }
  const char* operator()() const { return c_str_; }
};

struct char8_t {
  char c_str_[8];

  char8_t() { bzero(c_str_, 8); }
};

struct DataInfo {
  std::string bcast_ip_;
  int bcast_port_;

  DataInfo() : bcast_ip_("127.127.127.127"), bcast_port_(11111) {}
  DataInfo(const std::string& _bcast_ip_, const int& _bcast_port_) : bcast_ip_(_bcast_ip_), bcast_port_(_bcast_port_) {}

  inline bool operator==(const DataInfo& _other_) const {
    return ((bcast_ip_.compare(_other_.bcast_ip_) == 0) && (bcast_port_ == _other_.bcast_port_));
  }
  inline bool operator<(const DataInfo& _other_) const {
    if (bcast_port_ < _other_.bcast_port_) {
      return true;
    }
    if (bcast_port_ > _other_.bcast_port_) {
      return false;
    }
    if (bcast_ip_.compare(_other_.bcast_ip_) < 0) {
      return true;
    }
    if (bcast_ip_.compare(_other_.bcast_ip_) > 0) {
      return false;
    }
    return false;
  }
};

struct TradeInfo {
  char account_[128];
  char host_ip_[128];
  int host_port_;
  char bcast_ip_[128];
  int bcast_port_;

  TradeInfo() : account_(), host_ip_(), host_port_(11111), bcast_ip_(), bcast_port_(11111) {
    memcpy((void*)account_, "INVALID", 7);
    memcpy((void*)host_ip_, "INVALID", 7);
    memcpy((void*)bcast_ip_, "INVALID", 7);
  }
  TradeInfo(const std::string& _account_, const std::string& _host_ip_, const int& _host_port_,
            const std::string& _bcast_ip_, const int& _bcast_port_)
      : account_(), host_ip_(), host_port_(_host_port_), bcast_ip_(), bcast_port_(_bcast_port_) {
    memcpy((void*)account_, _account_.c_str(), std::min(128, (int32_t)_account_.length()));
    memcpy((void*)host_ip_, _host_ip_.c_str(), std::min(128, (int32_t)_host_ip_.length()));
    memcpy((void*)bcast_ip_, _bcast_ip_.c_str(), std::min(128, (int32_t)_bcast_ip_.length()));
  }
  inline bool operator==(const TradeInfo& _other_) const {
    return ((std::string(host_ip_).compare(std::string(_other_.host_ip_)) == 0) && (host_port_ == _other_.host_port_));
  }
  inline bool operator<(const TradeInfo& _other_) const {
    if (host_port_ < _other_.host_port_) {
      return true;
    }
    if (host_port_ > _other_.host_port_) {
      return false;
    }
    if (std::string(host_ip_).compare(std::string(_other_.host_ip_)) < 0) {
      return true;
    }
    if (std::string(host_ip_).compare(std::string(_other_.host_ip_)) > 0) {
      return false;
    }
    return false;
  }
  inline DataInfo GetBcastDataInfo() const { return DataInfo(bcast_ip_, bcast_port_); }
};

inline InsType_t InsType(char* ins_) {
  if (strcmp(ins_, "EQ") == 0) return kInsTypeEQ;
  if (strcmp(ins_, "FO") == 0) return kInsTypeFO;
  if (strcmp(ins_, "CR") == 0)
    return kInsTypeCR;
  else
    return kInsTypeFO;
}
inline std::string InsCode(InsType_t ins) {
  std::string retval;
  switch (ins) {
    case kInsTypeFO:
      retval = "FO";
      break;
    case kInsTypeCR:
      retval = "CR";
      break;
    case kInsTypeEQ:
      retval = "EQ";
      break;
    default:
      retval = "ERR";
      break;
  }
  return retval;
}

inline ExchSource_t StringToExchSource(const std::string& _exch_str_) {
  using namespace EXCHANGE_KEYS;
  if (strcmp(_exch_str_.c_str(), kExchSourceCMEStr) == 0) return kExchSourceCME;
  if (strcmp(_exch_str_.c_str(), kExchSourceCMEMDPStr) == 0) return kExchSourceCMEMDP;
  if (strcmp(_exch_str_.c_str(), kExchSourceEUREXStr) == 0) return kExchSourceEUREX;
  if (strcmp(_exch_str_.c_str(), kExchSourceBMFStr) == 0) return kExchSourceBMF;
  if (strcmp(_exch_str_.c_str(), kExchSourceBMFEQStr) == 0) return kExchSourceBMFEQ;
  if (strcmp(_exch_str_.c_str(), kExchSourceBMFEPStr) == 0) return kExchSourceBMF;
  if (strcmp(_exch_str_.c_str(), kExchSourceNTPStr) == 0) return kExchSourceNTP;
  if (strcmp(_exch_str_.c_str(), kExchSourceTMXStr) == 0) return kExchSourceTMX;
  if (strcmp(_exch_str_.c_str(), kExchSourceMEFFStr) == 0) return kExchSourceMEFF;
  if (strcmp(_exch_str_.c_str(), kExchSourceIDEMStr) == 0) return kExchSourceIDEM;
  if (strcmp(_exch_str_.c_str(), kExchSourceHONGKONGStr) == 0 || strcmp(_exch_str_.c_str(), "HKEX") == 0)
    return kExchSourceHONGKONG;
  if (strcmp(_exch_str_.c_str(), kExchSourceREUTERSStr) == 0) return kExchSourceREUTERS;
  if (strcmp(_exch_str_.c_str(), kExchSourceICEStr) == 0) return kExchSourceICE;
  if (strcmp(_exch_str_.c_str(), kExchSourceICEFODStr) == 0) return kExchSourceICEFOD;
  if (strcmp(_exch_str_.c_str(), kExchSourceICEPLStr) == 0) return kExchSourceICEPL;
  if (strcmp(_exch_str_.c_str(), kExchSourceICECFStr) == 0) return kExchSourceICECF;
  if (strcmp(_exch_str_.c_str(), kExchSourceEBSStr) == 0) return kExchSourceEBS;
  if (strcmp(_exch_str_.c_str(), kExchSourceLIFFEStr) == 0) return kExchSourceLIFFE;
  if (strcmp(_exch_str_.c_str(), kExchSourceRTSStr) == 0) return kExchSourceRTS;
  if (strcmp(_exch_str_.c_str(), kExchSourceMICEXStr) == 0) return kExchSourceMICEX;
  if (strcmp(_exch_str_.c_str(), kExchSourceMICEX_EQStr) == 0) return kExchSourceMICEX_EQ;
  if (strcmp(_exch_str_.c_str(), kExchSourceMICEX_CRStr) == 0) return kExchSourceMICEX_CR;
  if (strcmp(_exch_str_.c_str(), kExchSourceLSEStr) == 0) return kExchSourceLSE;
  if (strcmp(_exch_str_.c_str(), kExchSourceNASDAQStr) == 0) return kExchSourceNASDAQ;
  if (strcmp(_exch_str_.c_str(), kExchSourceBATSCHIStr) == 0) return kExchSourceBATSCHI;
  if (strcmp(_exch_str_.c_str(), kExchSourceHYBStr) == 0) return kExchSourceHYB;
  if (strcmp(_exch_str_.c_str(), kExchSourceJPYStr) == 0) return kExchSourceJPY;
  if (strcmp(_exch_str_.c_str(), kExchSourceJPY_L1Str) == 0) return kExchSourceJPY_L1;
  if (strcmp(_exch_str_.c_str(), kExchSourceTSEStr) == 0) return kExchSourceTSE;
  if (strcmp(_exch_str_.c_str(), kExchSourceQUINCYStr) == 0) return kExchSourceQUINCY;
  if (strcmp(_exch_str_.c_str(), kExchSourceESPEEDStr) == 0) return kExchSourceESPEED;
  if (strcmp(_exch_str_.c_str(), kExchSourceCFEStr) == 0) return kExchSourceCFE;
  if (strcmp(_exch_str_.c_str(), kExchSourceEOBIStr) == 0) return kExchSourceEOBI;
  if (strcmp(_exch_str_.c_str(), kExchSourceCFNMicrowaveStr) == 0) return kExchSourceCFNMicroware;
  if (strcmp(_exch_str_.c_str(), kExchSourceHKOMDStr) == 0) return kExchSourceHKOMD;
  if (strcmp(_exch_str_.c_str(), kExchSourceHKOMDPFStr) == 0) return kExchSourceHKOMDPF;
  if (strcmp(_exch_str_.c_str(), kExchSourceHKOMDCPFStr) == 0) return kExchSourceHKOMDCPF;
  if (strcmp(_exch_str_.c_str(), kExchSourceASXStr) == 0) return kExchSourceASX;
  if (strcmp(_exch_str_.c_str(), kExchSourceSGXStr) == 0) return kExchSourceSGX;
  if (strcmp(_exch_str_.c_str(), kExchSourceBSEStr) == 0) return kExchSourceBSE;
  if (strcmp(_exch_str_.c_str(), kExchSourceAFLASHStr) == 0) return kExchSourceAFLASH;
  if (strcmp(_exch_str_.c_str(), kExchSourceAFLASHNewStr) == 0) return kExchSourceAFLASH_NEW;
  if (strcmp(_exch_str_.c_str(), kExchSourceRETAILStr) == 0) return kExchSourceRETAIL;
  if (strcmp(_exch_str_.c_str(), kExchSourceNSEStr) == 0) return kExchSourceNSE;
  if (strcmp(_exch_str_.c_str(), kExchSourceNSE_FOStr) == 0) return kExchSourceNSE_FO;
  if (strcmp(_exch_str_.c_str(), kExchSourceNSE_CDStr) == 0) return kExchSourceNSE_CD;
  if (strcmp(_exch_str_.c_str(), kExchSourceNSE_EQStr) == 0) return kExchSourceNSE_EQ;
  if (strcmp(_exch_str_.c_str(), kExchSourcePUMAStr) == 0) return kExchSourcePUMA;
  if (strcmp(_exch_str_.c_str(), kExchSourceKRXStr) == 0) return kExchSourceKRX;
  return kExchSourceInvalid;
}

inline std::string ExchSourceStringForm(ExchSource_t exch) {
  using namespace EXCHANGE_KEYS;
  std::string retval;
  switch (exch) {
    case kExchSourceCME:
      retval = kExchSourceCMEStr;
      break;

    case kExchSourceEUREX:
      retval = kExchSourceEUREXStr;
      break;

    case kExchSourceEOBI:
      retval = kExchSourceEOBIStr;
      break;

    case kExchSourceBMF:
      retval = kExchSourceBMFStr;
      break;

    case kExchSourceBMFEQ:
      retval = kExchSourceBMFEQStr;
      break;

    case kExchSourceNTP:
      retval = kExchSourceNTPStr;
      break;

    case kExchSourcePUMA:
      retval = kExchSourcePUMAStr;
      break;

    case kExchSourceTMX:
      retval = kExchSourceTMXStr;
      break;

    case kExchSourceMEFF:
      retval = kExchSourceMEFFStr;
      break;

    case kExchSourceIDEM:
      retval = kExchSourceIDEMStr;
      break;

    case kExchSourceHONGKONG:
      retval = kExchSourceHONGKONGStr;
      break;

    case kExchSourceREUTERS:
      retval = kExchSourceREUTERSStr;
      break;

    case kExchSourceICE:
      retval = kExchSourceICEStr;
      break;

    case kExchSourceICEFOD:
      retval = kExchSourceICEFODStr;
      break;

    case kExchSourceICEPL:
      retval = kExchSourceICEPLStr;
      break;

    case kExchSourceICECF:
      retval = kExchSourceICECFStr;
      break;

    case kExchSourceEBS:
      retval = kExchSourceEBSStr;
      break;

    case kExchSourceLIFFE:
      retval = kExchSourceLIFFEStr;
      break;

    case kExchSourceRTS:
      retval = kExchSourceRTSStr;
      break;

    case kExchSourceMICEX:
      retval = kExchSourceMICEXStr;
      break;

    case kExchSourceMICEX_EQ:
      retval = kExchSourceMICEX_EQStr;
      break;

    case kExchSourceMICEX_CR:
      retval = kExchSourceMICEX_CRStr;
      break;

    case kExchSourceLSE:
      retval = kExchSourceLSEStr;
      break;

    case kExchSourceNASDAQ:
      retval = kExchSourceNASDAQStr;
      break;

    case kExchSourceBATSCHI:
      retval = kExchSourceBATSCHIStr;
      break;

    case kExchSourceHYB:
      retval = kExchSourceHYBStr;
      break;

    case kExchSourceJPY:
      retval = kExchSourceJPYStr;
      break;

    case kExchSourceJPY_L1:
      retval = kExchSourceJPY_L1Str;
      break;

    case kExchSourceTSE:
      retval = kExchSourceTSEStr;
      break;

    case kExchSourceQUINCY:
      retval = kExchSourceQUINCYStr;
      break;

    case kExchSourceCMEMDP:
      retval = kExchSourceCMEMDPStr;
      break;

    case kExchSourceESPEED:
      retval = kExchSourceESPEEDStr;
      break;

    case kExchSourceCFE:
      retval = kExchSourceCFEStr;
      break;

    case kExchSourceCFNMicroware:
      retval = kExchSourceCFNMicrowaveStr;
      break;

    case kExchSourceHKOMD:
      retval = kExchSourceHKOMDStr;
      break;

    case kExchSourceHKOMDPF:
      retval = kExchSourceHKOMDPFStr;
      break;

    case kExchSourceHKOMDCPF:
      retval = kExchSourceHKOMDCPFStr;
      break;

    case kExchSourceASX:
      retval = kExchSourceASXStr;
      break;
    case kExchSourceSGX:
      retval = kExchSourceSGXStr;
      break;
    case kExchSourceBSE:
      retval = "BSE";
      break;
    case kExchSourceAFLASH:
      retval = kExchSourceAFLASHStr;
      break;
    case kExchSourceAFLASH_NEW:
      retval = kExchSourceAFLASHNewStr;
      break;

    case kExchSourceRETAIL:
      retval = kExchSourceRETAILStr;
      break;

    case kExchSourceNSE:
      retval = kExchSourceNSEStr;
      break;

    case kExchSourceNSE_FO:
      retval = kExchSourceNSE_FOStr;
      break;

    case kExchSourceNSE_CD:
      retval = kExchSourceNSE_CDStr;
      break;

    case kExchSourceNSE_EQ:
      retval = kExchSourceNSE_EQStr;
      break;

    case kExchSourceKRX:
      retval = kExchSourceKRXStr;
      break;

    default:
      retval = kExchSourceINVALIDStr;
      break;
  }
  return retval;
}

inline const char* ORSRejectionReasonStr(const ORSRejectionReason_t ors_r_t) {
  switch (ors_r_t) {
    case kORSOrderAllowed:
      return "Allowed";
      break;
    case kORSRejectSecurityNotFound:
      return "SecNotFound";
      break;
    case kORSRejectMarginCheckFailedOrderSizes:
      return "Failed OrderSize Check";
      break;
    case kORSRejectMarginCheckFailedMaxPosition:
      return "Failed Maxposition Check";
      break;
    case kORSRejectMarginCheckFailedWorstCasePosition:
      return "Failed Worstcaseposition Check";
      break;
    case kExchOrderReject:
      return "Exch OrderReject General";
      break;
    case kORSRejectMarginCheckFailedMaxLiveOrders:
      return "Failed max live orders Check";
      break;
    case kORSRejectSelfTradeCheck:
      return "Failed Self Trade Check";
      break;
    case kORSRejectThrottleLimitReached:
      return "Throttle limit reached";
      break;
    case kORSRejectMarketClosed:
      return "Market closed";
      break;
    case kORSRejectNewOrdersDisabled:
      return "New Orders disabled";
      break;
    case kORSRejectFailedPriceCheck:
      return "Failed Price Limit Check";
      break;
    case kORSRejectETIAlgoTaggingFailure:
      return "Failed To Tag ETI Algo";
      break;
    case kORSRejectFOKSendFailed:
      return "Failed FOK Check";
      break;
    case kORSRejectFailedIOCOrder:
      return "Failed IOC Send";
    case kORSRejectMarginCheckFailedBidNotionalValue:
      return "Failed BID Notional Value Check";
      break;
    case kORSRejectMarginCheckFailedAskNotionalValue:
      return "Failed ASK Notional Value Check";
      break;
    case kORSRejectNSEComplianceFailure:
      return "Failed NSE Compliance Check";
      break;
    case kTAPRejectThrottleLimitReached:
      return "TAP Throttle limit reached";
      break;
    case kORSRejectMarginCheckFailedSumQueryWorstCasePosition:
      return "Failed Sum Query Worstcase Position Check";
      break;
    case kORSRejectFailedPnlCheck:
      return "kORSRejectFailedPnlCheck";
    case kORSRejectFailedGrossMarginCheck:
      return "kORSRejectFailedGrossMarginCheck";
    case kORSRejectFailedNetMarginCheck:
      return "kORSRejectFailedNetMarginCheck";
    case kORSRejectICEAlgoTaggingFailure:
      return "kORSRejectICEAlgoTaggingFailure";
    case kORSRejectMiFidLimitsBreached:
      return "kORSRejectMiFidLimitsBreached";
    default:
      return "UNDEF";
  }
}

inline const char* CancelRejectReasonStr(const CxlRejectReason_t cxl_r_t) {
  switch (cxl_r_t) {
    case kCxlRejectReasonTooLate:
      return "TooLate";
      break;
    case kCxlRejectReasonUnknownOrder:
      return "UnknownOrder";
      break;
    case kCxlRejectReasonOrderNotConfirmed:
      return "OrderNotConfirmed";
      break;
    case kCxlRejectReasonOrderAlreadyInPendingQueue:
      return "AlreadyInPendingQueue";
      break;
    case kCxlRejectReasonDuplicateClOrdID:
      return "DuplicateClOrdID";
      break;
    case kCxlRejectReasonThrottle:
      return "ThrottleLimit";
      break;
    case kCxlRejectReasonTAPThrottle:
      return "TAP Throttle limit reached";
      break;
    case kCxlRejectReasonOther:
    default:
      return "Other";
  }
}

inline const char* CancelReplaceRejectReasonStr(const CxlReplaceRejectReason_t ors_r_t) {
  switch (ors_r_t) {
    case kORSModifyOrderAllowed:
      return "Allowed";
      break;
    case kORSCxReRejectSecurityNotFound:
      return "SecNotFound";
      break;
    case kORSCxReRejectMarginCheckFailedOrderSizes:
      return "Failed OrderSize Check";
      break;
    case kORSCxReRejectMarginCheckFailedMaxPosition:
      return "Failed Maxposition Check";
      break;
    case kORSCxReRejectMarginCheckFailedWorstCasePosition:
      return "Failed Worstcaseposition Check";
      break;
    case kExchCancelReplaceReject:
      return "Exch OrderReject General";
      break;
    case kORSCxReRejectMarginCheckFailedMaxLiveOrders:
      return "Failed max live orders Check";
      break;
    case kORSCxReRejectSelfTradeCheck:
      return "Failed Self Trade Check";
      break;
    case kORSCxReRejectThrottleLimitReached:
      return "Throttle limit reached";
      break;
    case kORSCxReRejectMarketClosed:
      return "Market closed";
      break;
    case kORSCxReRejectNewOrdersDisabled:
      return "New Orders disabled";
      break;
    case kORSCxReRejectFailedPriceCheck:
      return "Failed Price Limit Check";
      break;
    case kORSCxReRejectETIAlgoTaggingFailure:
      return "Failed To Tag ETI Algo";
      break;
    case kORSCxReRejectFOKSendFailed:
      return "Failed FOK Check";
      break;
    case kORSCxReRejectFailedIOCOrder:
      return "Failed IOC Send";
    case kORSCxReRejectMarginCheckFailedBidNotionalValue:
      return "Failed BID Notional Value Check";
      break;
    case kORSCxReRejectMarginCheckFailedAskNotionalValue:
      return "Failed ASK Notional Value Check";
      break;
    case kORSCxReRejectNSEComplianceFailure:
      return "Failed NSE Compliance Check";
      break;
    case kCxReTAPRejectThrottleLimitReached:
      return "TAP Throttle limit reached";
    case kCxlReRejectOrderNotFound:
      return "OrderNotFound";
      break;
    default:
      return "UNDEF";
  }
}
enum StrategyType {
  kPriceBasedTrading = 1,
  kPriceBasedAggressiveTrading,
  kPriceBasedSecurityAggressiveTrading,
  kPricePairBasedAggressiveTrading,
  kPricePairBasedAggressiveTradingV2,
  kTradeBasedAggressiveTrading,
  kPriceBasedScalper,
  kPriceBasedAggressiveScalper,
  kPriceBasedVolTrading,
  kDirectionalAggressiveTrading,
  kDirectionalInterventionAggressiveTrading,
  kDirectionalInterventionLogisticTrading,
  kDirectionalLogisticTrading,
  kDirectionalPairAggressiveTrading,
  kDesiredPositionTrading,
  kReturnsBasedAggressiveTrading
};

typedef enum {
  kMktTradingStatusOpen = 0,
  kMktTradingStatusClosed,
  kMktTradingStatusPause,
  kMktTradingStatusUnknown,
  kMktTradingStatusForbidden,
  kMktTradingStatusPreOpen,
  kMktTradingStatusReserved,
  kMktTradingStatuFinalClosingCall
} MktStatus_t;

inline const char* MktTradingStatusStr(MktStatus_t _this_status_) {
  switch (_this_status_) {
    case kMktTradingStatusOpen: {
      return "OPEN";
    } break;
    case kMktTradingStatusClosed: {
      return "CLOSED";
    } break;
    case kMktTradingStatusPreOpen: {
      return "PREOPEN";
    } break;
    case kMktTradingStatusReserved: {
      return "RESERVED/AUCTION";
    } break;
    case kMktTradingStatuFinalClosingCall: {
      return "FINAL_CLOSING_CALL";
    } break;
    case kMktTradingStatusUnknown: {
      return "UNKNOWN";
    } break;
    case kMktTradingStatusForbidden: {
      return "FORBIDDEN";
    } break;
    default: {
      return "NOINFO";
      break;
    }
  }
  return "NOT_RECOGNIZED";
}

// Given int contract month, return corresponding char
// 2 -> 'G', 9 -> 'U'
inline char GetCharContractMonth(int month) {
  switch (month) {
    case 1: {
      return 'F';
      break;
    }
    case 2: {
      return 'G';
      break;
    }
    case 3: {
      return 'H';
      break;
    }
    case 4: {
      return 'J';
      break;
    }
    case 5: {
      return 'K';
      break;
    }
    case 6: {
      return 'M';
      break;
    }
    case 7: {
      return 'N';
      break;
    }
    case 8: {
      return 'Q';
      break;
    }
    case 9: {
      return 'U';
      break;
    }
    case 10: {
      return 'V';
      break;
    }
    case 11: {
      return 'X';
      break;
    }
    case 12: {
      return 'Z';
      break;
    }
    default: {
      return 'A';
      break;
    }
  }

  return 'A';
}

// Given char contract month, return corresponding int
// 'G' -> 2, 'U' -> 9
inline int GetIntContractMonth(char month) {
  switch (month) {
    case 'F': {
      return 1;
      break;
    }
    case 'G': {
      return 2;
      break;
    }
    case 'H': {
      return 3;
      break;
    }
    case 'J': {
      return 4;
      break;
    }
    case 'K': {
      return 5;
      break;
    }
    case 'M': {
      return 6;
      break;
    }
    case 'N': {
      return 7;
      break;
    }
    case 'Q': {
      return 8;
      break;
    }
    case 'U': {
      return 9;
      break;
    }
    case 'V': {
      return 10;
      break;
    }
    case 'X': {
      return 11;
      break;
    }
    case 'Z': {
      return 12;
      break;
    }
    default: {
      return 0;
      break;
    }
  }

  return 0;
}

// Given char contract month, return corresponding int
// 'G' -> 2, 'U' -> 9
inline std::string GetStrContractMonth(char month) {
  switch (month) {
    case 'F': {
      return "01";
      break;
    }
    case 'G': {
      return "02";
      break;
    }
    case 'H': {
      return "03";
      break;
    }
    case 'J': {
      return "04";
      break;
    }
    case 'K': {
      return "05";
      break;
    }
    case 'M': {
      return "06";
      break;
    }
    case 'N': {
      return "07";
      break;
    }
    case 'Q': {
      return "08";
      break;
    }
    case 'U': {
      return "09";
      break;
    }
    case 'V': {
      return "10";
      break;
    }
    case 'X': {
      return "11";
      break;
    }
    case 'Z': {
      return "12";
      break;
    }
    default: {
      return "00";
      break;
    }
  }

  return "00";
}

typedef enum { kOrderDay = 1, kOrderFOK, kOrderIOC } OrderType_t;

enum class NSESegmentType { NSE_INVALID = 0, NSE_CD, NSE_FO, NSE_CM };

const char* const kPriceTypeOpenStr = "OpenPrice";
const char* const kPriceTypeCloseStr = "ClosePrice";
const char* const kPriceTypeHighStr = "HighPrice";
const char* const kPriceTypeLowStr = "LowPrice";
const char* const kPriceTypeAverageTradeStr = "AverageTradePrice";

enum class MinuteBarPriceType {
  kPriceTypeOpen,
  kPriceTypeHigh,
  kPriceTypeLow,
  kPriceTypeClose,
  kPriceTypeAvgTrade,
  kPriceTypeMax
};

inline std::string MinuteBarPriceTypeToString(MinuteBarPriceType price_type) {
  switch (price_type) {
    case MinuteBarPriceType::kPriceTypeOpen: {
      return kPriceTypeOpenStr;
    } break;
    case MinuteBarPriceType::kPriceTypeClose: {
      return kPriceTypeCloseStr;
    } break;
    case MinuteBarPriceType::kPriceTypeHigh: {
      return kPriceTypeHighStr;
    } break;
    case MinuteBarPriceType::kPriceTypeLow: {
      return kPriceTypeLowStr;
    } break;
    case MinuteBarPriceType::kPriceTypeAvgTrade: {
      return kPriceTypeAverageTradeStr;
    } break;
    default: { return "InvalidPrice"; } break;
  }
  return "InvalidPrice";
}

inline MinuteBarPriceType StringToMinuteBarPriceType(std::string price_type) {
  if (strcmp(price_type.c_str(), kPriceTypeOpenStr) == 0) {
    return MinuteBarPriceType::kPriceTypeOpen;
  }
  if (strcmp(price_type.c_str(), kPriceTypeCloseStr) == 0) {
    return MinuteBarPriceType::kPriceTypeClose;
  }
  if (strcmp(price_type.c_str(), kPriceTypeHighStr) == 0) {
    return MinuteBarPriceType::kPriceTypeHigh;
  }
  if (strcmp(price_type.c_str(), kPriceTypeLowStr) == 0) {
    return MinuteBarPriceType::kPriceTypeLow;
  }
  if (strcmp(price_type.c_str(), kPriceTypeAverageTradeStr) == 0) {
    return MinuteBarPriceType::kPriceTypeAvgTrade;
  }
  return MinuteBarPriceType::kPriceTypeMax;
}

inline std::string GetCurrentHostName() {
  char hostname[128];
  hostname[127] = '\0';
  gethostname(hostname, 127);
  return hostname;
}

inline bool IsItSimulationServer() {
  std::stringstream ss;
  ss << PROD_CONFIGS_DIR << GetCurrentHostName() << "_"
     << "is_simulation_server.cfg";
  std::string filename = ss.str();

  int is_sim_server = 0;

  std::ifstream is_sim_server_file;
  is_sim_server_file.open(filename.c_str());
  if (is_sim_server_file.is_open()) {
    if (is_sim_server_file.good()) {
      char buffer[128];
      is_sim_server_file.getline(buffer, 128);
      is_sim_server = atoi(buffer);
      is_sim_server_file.close();
    }
  }

  return (is_sim_server > 0);
}

inline bool IsItMidTermLegacyServer() {
  std::stringstream ss;
  ss << PROD_CONFIGS_DIR << GetCurrentHostName() << "_"
     << "is_midterm_legacy_server.cfg";
  std::string filename = ss.str();

  int is_legacy_server = 0;

  std::ifstream is_legacy_server_file;
  is_legacy_server_file.open(filename.c_str());
  if (is_legacy_server_file.is_open()) {
    if (is_legacy_server_file.good()) {
      char buffer[128];
      is_legacy_server_file.getline(buffer, 128);
      is_legacy_server = atoi(buffer);
      is_legacy_server_file.close();
    }
  }

  return (is_legacy_server > 0);
}

inline bool IsItOverClockedServer() {
  std::stringstream ss;
  ss << PROD_CONFIGS_DIR << GetCurrentHostName() << "_"
     << "overclocked_tsc_freq.txt";
  std::string filename = ss.str();

  int is_oc_server = 0;

  std::ifstream is_oc_server_file;
  is_oc_server_file.open(filename.c_str());
  if (is_oc_server_file.is_open()) {
    if (is_oc_server_file.good()) {
      is_oc_server = 1;
      is_oc_server_file.close();
    }
  }

  return (is_oc_server > 0);
}

inline bool UseShmforORSReply() {
  std::stringstream ss;
  ss << PROD_CONFIGS_DIR << GetCurrentHostName() << "_"
     << "use_shm_for_ors_reply.txt";
  std::string filename = ss.str();

  int use_shm = 0;

  std::ifstream use_shm_file;
  use_shm_file.open(filename.c_str());
  if (use_shm_file.is_open()) {
    if (use_shm_file.good()) {
      char buffer[128];
      use_shm_file.getline(buffer, 128);
      use_shm = atoi(buffer);
      use_shm_file.close();
    }
  }

  return (use_shm > 0);
}

inline bool IsMiFidCheckApplicable(HFSAT::ExchSource_t exch_source) {
  if (exch_source == HFSAT::kExchSourceEUREX) {
    return true;
  }
  return false;
}

// this map is used in case of recovery manager ,both host as well as on the client side
static std::map<std::string, const int> exchange_name_str_to_int_mapping = {{"NSE", 1},
                                                                            {"TMX_OBF", 2},
                                                                            {"OSE_ITCH", 3},
                                                                            {"RTS_OF", 4},
                                                                            {"NSE_FO", 5},
                                                                            {"NSE_EQ", 6},
                                                                            {"NSE_CD", 7},
                                                                            {"MICEX_CR", 8}};
}
