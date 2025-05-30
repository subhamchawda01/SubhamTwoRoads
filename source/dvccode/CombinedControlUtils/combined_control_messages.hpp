/**
    \file dvccode/ORSMessages/combined_control_messages.hpp

    \author: (c) Copyright Two Roads Technological Solutions Pvt Ltd 2011
     Address:
         Suite No 353, Evoma, #14, Bhattarhalli,
         Old Madras Road, Near Garden City College,
         KR Puram, Bangalore 560049, India
         +91 80 4190 3551
 */

#pragma once

#include <sys/time.h>
#include <string>
#include <sstream>
#include <vector>
#include "dvccode/CDef/defines.hpp"
#include "dvccode/CDef/ttime.hpp"
#include "dvccode/CDef/trading_location_manager.hpp"
#include "dvccode/CommonDataStructures/perishable_string_tokenizer.hpp"

#define COMBINED_CONTROL_MSG_MAX_LEN 50
#define ADD_RM_SHC_MSG_BUFFER_LEN 100

namespace HFSAT {

/**
 * \brief combined control messages to the writer from a user
 *
 * typically this will be a message from the user :
 * like add ref data,
 */
// ----------------------------- MsgTypes of CombinedControl Message-------------------
struct CombinedMsgChangeTolerance {
  HFSAT::ExchSource_t exch_src_;
  int tolerance_;
  bool on_location_only_;  // if this is true, affects only the writers at that location.

  inline std::string ToString() const {
    std::ostringstream t_oss_;
    t_oss_ << " ExchSrc: " << HFSAT::ExchSourceStringForm(exch_src_) << " "
           << " Tolerance: " << tolerance_ << " OnLocationOnly: " << on_location_only_;
    return t_oss_.str();
  }
};

struct CombinedMsgDumpLiveOrders {
  int trader_id_;

  inline std::string ToString() const {
    std::ostringstream t_oss_;
    t_oss_ << " TradeId: " << trader_id_;
    return t_oss_.str();
  }
};

struct CombinedMsgDumpMdsFiles {
  bool dump_only_ors_files_;

  inline std::string ToString() const {
    std::ostringstream t_oss_;
    t_oss_ << " DumpOnlyORSFiles: " << dump_only_ors_files_;
    return t_oss_.str();
  }
};

typedef enum { kSignalInvalid, kSignalAdd, kSignalRemove, kSignalShow } SignalType;

struct CombinedMsgAddRemoveShortcode {
  int query_id_;
  int len_;
  char buffer[ADD_RM_SHC_MSG_BUFFER_LEN];
  SignalType signal_;

  std::vector<std::string> GetShortcodes() {
    std::vector<std::string> out;
    buffer[len_] = '\0';
    PerishableStringTokenizer::StringSplit(buffer, '|', out);
    return out;
  }

  inline std::string ToString() const {
    std::ostringstream t_oss_;
    t_oss_ << " QueryID: " << query_id_ << " Len: " << len_ << " Buffer: " << buffer << " Signal: " << signal_;
    return t_oss_.str();
  }
};
//--------------------------------------------------------------------------------------

typedef enum {
  kCmdControlMessageCodeChangeTolerance = 0,
  kCmdControlMessageCodeDumpLiveOrders,
  kCmdControlMessageCodeDumpMdsFiles,
  kCmdControlMessageCodeAddRemoveShortcode,
  kCmdControlMessageCodeMax  // should remain at the end , error condition
} CombinedControlMessageCode_t;

inline const char *GetCombinedControlMessageCodeString(CombinedControlMessageCode_t _combined_control_message_code_) {
  switch (_combined_control_message_code_) {
    case kCmdControlMessageCodeChangeTolerance:
      return "kCmdControlMessageCodeChangeTolerance";
      break;
    case kCmdControlMessageCodeDumpLiveOrders:
      return "kCmdControlMessageCodeDumpLiveOrders";
      break;
    case kCmdControlMessageCodeDumpMdsFiles:
      return "kCmdControlMessadeCodeDumpMdsFiles";
      break;
    case kCmdControlMessageCodeAddRemoveShortcode:
      return "kCmdControlMessageCodeAddRemoveShortcode";
      break;
    default:
      break;
  }
  return "kCmdControlMessageCodeUnknown";
}

struct CombinedControlMessage {
 public:
  inline std::string ToString() const {
    std::ostringstream t_oss_;
    t_oss_ << " Type: " << GetCombinedControlMessageCodeString(message_code_) << " "
           << " Location: " << location_ << " ";

    switch (message_code_) {
      case kCmdControlMessageCodeChangeTolerance: {
        t_oss_ << generic_combined_control_msg_.tolerance_msg_.ToString();
      } break;

      case kCmdControlMessageCodeDumpLiveOrders: {
        t_oss_ << generic_combined_control_msg_.dump_live_orders_msg_.ToString();
      } break;

      case kCmdControlMessageCodeDumpMdsFiles: {
        t_oss_ << generic_combined_control_msg_.dump_mds_files_.ToString();
      } break;

      case kCmdControlMessageCodeAddRemoveShortcode: {
        t_oss_ << generic_combined_control_msg_.add_rm_shortcode_.ToString();
      } break;

      default:
        break;
    }
    return t_oss_.str();
  }

  friend std::ostream &operator<<(std::ostream &out, const CombinedControlMessage &control_message_);

 public:
  CombinedControlMessageCode_t message_code_;
  char location_[40];

  union CombinedControlMessaseTypes {
    CombinedMsgChangeTolerance tolerance_msg_;
    CombinedMsgDumpLiveOrders dump_live_orders_msg_;
    CombinedMsgDumpMdsFiles dump_mds_files_;
    CombinedMsgAddRemoveShortcode add_rm_shortcode_;

    CombinedControlMessaseTypes() {}
    ~CombinedControlMessaseTypes() {}

  } generic_combined_control_msg_;
};

inline std::ostream &operator<<(std::ostream &out, const CombinedControlMessage &control_message_) {
  out << control_message_.ToString();
  return out;
}
}
