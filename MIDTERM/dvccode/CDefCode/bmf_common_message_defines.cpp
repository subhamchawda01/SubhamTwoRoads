/**
   \file CDefCode/bmf_common_message_defines.cpp

   \author: (c) Copyright Two Roads Technological Solutions Pvt Ltd 2011
   Address:
   Suite 217, Level 2, Prestige Omega,
   No 104, EPIP Zone, Whitefield,
   Bangalore - 560066
   India
   +91 80 4060 0717
 */

#include "dvccode/CDef/bmf_common_message_defines.hpp"

namespace NTP_MDS {

std::string GetStatusString(int status) {
  switch (status) {
    case NTP_SEC_TRADING_STATUS_PAUSE: {
      return "Pause";
      break;
    }

    case NTP_SEC_TRADING_STATUS_CLOSE: {
      return "Close";
      break;
    }

    case NTP_SEC_TRADING_STATUS_OPEN: {
      return "Open";
      break;
    }

    case NTP_SEC_TRADING_STATUS_FORBIDDEN: {
      return "Forbidden";
      break;
    }

    case NTP_SEC_TRADING_STATUS_UNKNOWN: {
      return "Unknown";
      break;
    }

    case NTP_SEC_TRADING_STATUS_RESERVED: {
      return "Reserved";
      break;
    }

    case NTP_SEC_TRADING_STATUS_FINAL_CLOSING_CALL: {
      return "FinalClosingCall";
      break;
    }

    default:
      break;
  }

  std::ostringstream oss;
  oss << status;
  return oss.str();
}

// Returns true if the trading_status is a valid one
bool IsStatusValid(int status) {
  switch (status) {
    case NTP_SEC_TRADING_STATUS_PAUSE:
    case NTP_SEC_TRADING_STATUS_CLOSE:
    case NTP_SEC_TRADING_STATUS_OPEN:
    case NTP_SEC_TRADING_STATUS_FORBIDDEN:
    case NTP_SEC_TRADING_STATUS_UNKNOWN:
    case NTP_SEC_TRADING_STATUS_RESERVED:
    case NTP_SEC_TRADING_STATUS_FINAL_CLOSING_CALL: {
      return true;
    }

    default: { return false; }
  }

  return false;
}
}
