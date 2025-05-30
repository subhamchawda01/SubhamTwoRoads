/**
    \file dvccode/ORSMessages/defines.hpp

    \author: (c) Copyright Two Roads Technological Solutions Pvt Ltd 2011
     Address:
         Suite 217, Level 2, Prestige Omega,
         No 104, EPIP Zone, Whitefield,
         Bangalore - 560066, India
         +91 80 4060 0717
*/
#ifndef BASE_ORSMESSAGES_DEFINES_H
#define BASE_ORSMESSAGES_DEFINES_H

#include "dvccode/CDef/defines.hpp"

namespace HFSAT {
typedef enum {
  ORQ_SEND = 0,
  ORQ_CANCEL,
  ORQ_CXLREPLACE,
  ORQ_REPLAY,
  ORQ_FOK_SEND,
  ORQ_IOC,
  ORQ_RECOVER,
  ORQ_HEARTBEAT,
  ORQ_PROCESS_QUEUE,
  ORQ_FAKE_SEND,
  ORQ_RISK
} ORQType_t;

typedef enum {
  kORRType_None = 0,
  kORRType_Seqd,
  kORRType_Conf,
  kORRType_CxRe,
  kORRType_Cxld,
  kORRType_Exec,
  kORRType_Rejc,
  kORRType_CxlRejc,
  kORRType_ORSConf,
  kORRType_IntExec,
  kORRType_Rejc_Funds,
  kORRType_Wake_Funds,
  kORRType_CxlSeqd,
  kORRType_CxReRejc,
  kORRType_CxReSeqd
} ORRType_t;

inline const char* ToString(const ORRType_t t_orr_type_t_) {
  switch (t_orr_type_t_) {
    case kORRType_Seqd:
      return "Seqd";
    case kORRType_Conf:
      return "Conf";
    case kORRType_CxRe:
      return "CxRe";
    case kORRType_Cxld:
      return "Cxld";
    case kORRType_Exec:
      return "Exec";
    case kORRType_Rejc:
      return "Rejc";
    case kORRType_CxlRejc:
      return "CxlRejc";
    case kORRType_ORSConf:
      return "ORSConf";
    case kORRType_IntExec:
      return "IntExec";
    case kORRType_Rejc_Funds:
      return "Rejc_Funds";
    case kORRType_Wake_Funds:
      return "Wake_Funds";
    case kORRType_CxlSeqd:
      return "CxlSeqd";
    case kORRType_CxReRejc:
      return "CxReRejc";
    case kORRType_CxReSeqd:
      return "CxReSeqd";
    default:
      return "kORRType_None";
  }
  return "kORRType_None";
}

const char* const kExceptionProductKey = "Exception";
}
#endif  // BASE_ORSMESSAGES_DEFINES_H
