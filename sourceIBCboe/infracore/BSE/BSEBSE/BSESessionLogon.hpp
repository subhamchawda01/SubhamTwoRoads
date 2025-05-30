// =====================================================================================
//
//       Filename:  BSESessionLogon.hpp
//
//    Description:
//
//        Version:  1.0
//        Created:  11/04/2012 11:24:47 AM
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

#include "infracore/BSE/BSEBSE/BSEMessageDefs.hpp"

namespace HFSAT {
namespace BSE {

class BSESessionLogon {
 public:
  LogonRequestT bse_session_logon_request_;

 public:
  BSESessionLogon() {
    // initialize logon message
    memset((void*)(&bse_session_logon_request_), 0, sizeof(LogonRequestT));

    // fill up the Messageheader
    bse_session_logon_request_.MessageHeaderIn.BodyLen = sizeof(LogonRequestT);
    bse_session_logon_request_.MessageHeaderIn.TemplateID = TID_LOGON_REQUEST;

    // fill up the request struct
    bse_session_logon_request_.RequestHeader.MsgSeqNum = 1;

    // now fill up the logon request struct
    memcpy((void*)(&bse_session_logon_request_.DefaultCstmApplVerID), ETI_INTERFACE_VERSION,
           sizeof(ETI_INTERFACE_VERSION));

    // AppUsageOrderOptions

    // A - Automated
    // M - Manual
    // B - Both ( Automated and Manual )
    // N - None
    bse_session_logon_request_.ApplUsageOrders[LEN_APPL_USAGE_ORDERS - 1] =
        ENUM_APPL_USAGE_ORDERS_AUTOMATED_CHAR;  // Automated

    // AppUsageQuoteOptions

    // A - Automated
    // M - Manual
    // B - Both ( Automated and Manual )
    // N - None
    bse_session_logon_request_.ApplUsageQuotes[LEN_APPL_USAGE_QUOTES - 1] = ENUM_APPL_USAGE_QUOTES_NONE_CHAR;  // None

    // OrderRoutingIndicatorOptions
    // Y - Yes
    // N - No
    bse_session_logon_request_.OrderRoutingIndicator[LEN_ORDER_ROUTING_INDICATOR - 1] = ENUM_ORDER_ROUTING_INDICATOR_YES_CHAR; //Yes

    // FIXEngineName, FIXEngineVersion, FIXEngineVendor  -- Internally used by BSE developers (send empty)
    // ApplicationSystemName, ApplicationSystemVersion, ApplicationSystemVendor -- Internally used by BSE developers (send empty)

    memcpy((void*)(bse_session_logon_request_.FIXEngineName), " ", LEN_FIX_ENGINE_NAME);
    memcpy((void*)(bse_session_logon_request_.FIXEngineVersion), " ", LEN_FIX_ENGINE_VERSION);
    memcpy((void*)(bse_session_logon_request_.FIXEngineVendor), " ", LEN_FIX_ENGINE_VENDOR);
    memcpy((void*)(bse_session_logon_request_.ApplicationSystemName), " ", LEN_APPLICATION_SYSTEM_NAME);
    memcpy((void*)(bse_session_logon_request_.ApplicationSystemVersion), " ", LEN_APPLICATION_SYSTEM_VERSION);
    memcpy((void*)(bse_session_logon_request_.ApplicationSystemVendor), " ", LEN_APPLICATION_SYSTEM_VENDOR);

  }

  // These fields would be read from ORS Config File
  void setBSESessionLogonStaticFields(const uint32_t _heartbeat_interval_, const uint32_t _party_session_id_,
                                      const char* _password_, const uint8_t _password_fill_length_) {
    bse_session_logon_request_.HeartBtInt = _heartbeat_interval_;
    bse_session_logon_request_.PartyIDSessionID = _party_session_id_;
    memcpy((void*)(bse_session_logon_request_.Password), (void*)(_password_), _password_fill_length_);
  }

  // get MsgLength to write
  int getBSESessionLogonMsgLength() { return (sizeof(LogonRequestT)); }

  LogonRequestT *ProcessLogonRequest(char const *msg_ptr) {
    bse_session_logon_request_.MessageHeaderIn.BodyLen = *((uint32_t *)(msg_ptr + 0));
    bse_session_logon_request_.MessageHeaderIn.TemplateID = *((uint16_t *)(msg_ptr + 4));
    bse_session_logon_request_.RequestHeader.MsgSeqNum = *((uint32_t *)(msg_ptr + 16));
    bse_session_logon_request_.HeartBtInt = *((uint32_t *)(msg_ptr + 24));
    bse_session_logon_request_.PartyIDSessionID = *((uint32_t *)(msg_ptr + 28));
    memcpy((void*)bse_session_logon_request_.DefaultCstmApplVerID,
           (void*)(msg_ptr + 32),LEN_DEFAULT_CSTM_APPL_VERID);
    memcpy((void*)bse_session_logon_request_.Password,
           (void*)(msg_ptr + 62),LEN_PASSWORD);
    memcpy((void*)bse_session_logon_request_.ApplUsageOrders,
           (void*)(msg_ptr + 94),LEN_APPL_USAGE_ORDERS);
    memcpy((void*)bse_session_logon_request_.OrderRoutingIndicator,
           (void*)(msg_ptr + 96),LEN_ORDER_ROUTING_INDICATOR);
    memcpy((void*)bse_session_logon_request_.FIXEngineName,
           (void*)(msg_ptr + 97),LEN_FIX_ENGINE_NAME);
    memcpy((void*)bse_session_logon_request_.FIXEngineVersion,
           (void*)(msg_ptr + 127),LEN_FIX_ENGINE_VERSION);
    memcpy((void*)bse_session_logon_request_.FIXEngineVendor,
           (void*)(msg_ptr + 157),LEN_FIX_ENGINE_VENDOR);
    memcpy((void*)bse_session_logon_request_.ApplicationSystemName,
           (void*)(msg_ptr + 187),LEN_APPLICATION_SYSTEM_NAME);
    memcpy((void*)bse_session_logon_request_.ApplicationSystemVersion,
           (void*)(msg_ptr + 217),LEN_APPLICATION_SYSTEM_VERSION);
    memcpy((void*)bse_session_logon_request_.ApplicationSystemVendor,
           (void*)(msg_ptr + 247),LEN_APPLICATION_SYSTEM_VENDOR);

    return &bse_session_logon_request_;
  }

};
}
}
