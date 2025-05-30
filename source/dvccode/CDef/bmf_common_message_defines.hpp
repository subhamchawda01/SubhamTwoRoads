/**
    \file bmf_common_message_defines.hpp

    \author: (c) Copyright Two Roads Technological Solutions Pvt Ltd 2011
     Address:
         Suite 217, Level 2, Prestige Omega,
         No 104, EPIP Zone, Whitefield,
         Bangalore - 560066
         India
         +91 80 4060 0717
 */
#ifndef BASE_CDEF_BMF_COMMON_MESSAGE_STRUCTS_HPP
#define BASE_CDEF_BMF_COMMON_MESSAGE_STRUCTS_HPP

#include <stdio.h>
#include <stdint.h>
#include <sys/time.h>
#include <string>
#include <sstream>
#include <arpa/inet.h>
#include <iomanip>
#include <iostream>
#include <dvccode/CDef/defines.hpp>

/// only used in daemon side template parsing
namespace BMF_NS {
enum TIDS {
  SEQRESET_10 = 10,
  HEARTBEAT = 11,
  SECLIST_30 = 30,
  REFRESH_25 = 25,
  REFRESH_26 = 26,
  NEWS_29 = 29,
  SNAPSHOT_27 = 27,
  SNAPSHOT_28 = 28,
  SECSTATUS_21 = 21,
  SECSTATUS_22 = 22
};
}

/// BMF broadcast structs
namespace BMF_MDS {
#define BMF_MDS_CONTRACT_TEXT_SIZE 12

struct BMF_Header {
  uint32_t msg_seq;
  uint16_t num_cnk;
  uint16_t cur_cnk;
  uint16_t msg_len;

  void toHost() {
    msg_seq = ntohl(msg_seq);
    num_cnk = ((num_cnk >> 8) & 0x00FF) | (num_cnk << 8);
    cur_cnk = ((cur_cnk >> 8) & 0x00FF) | (cur_cnk << 8);
    msg_len = ((msg_len >> 8) & 0x00FF) | (msg_len << 8);
  };

  inline std::string ToString() {
    std::ostringstream t_temp_oss_;
    t_temp_oss_ << "Msg_Seqnum:   " << msg_seq << "\n";
    t_temp_oss_ << "Num_Chunks:   " << num_cnk << "\n";
    t_temp_oss_ << "Curr_Chunk:   " << cur_cnk << "\n";
    t_temp_oss_ << "Msg_Length:   " << msg_len << "\n";
    return t_temp_oss_.str();
  }

  inline void print() { fprintf(stderr, "%s\n", ToString().c_str()); }
};

struct BMFTradeStruct {
  char contract_[BMF_MDS_CONTRACT_TEXT_SIZE];  ///< contract name
  uint32_t trd_qty_;                           ///< quantity in this trade
  uint32_t tot_qty_;                           ///< total quantity
  uint32_t seqno_;                             ///< instrument based seqno .. to order trades and quotes
  double trd_px_;                              ///< trade price
  uint16_t buyer_;
  uint16_t seller_;
  bool is_last_;
  char flags_[3];

  inline void print() { fprintf(stderr, "%s\n", ToString().c_str()); }

  inline std::string ToString() {
    std::ostringstream t_temp_oss_;
    t_temp_oss_ << "Contract:   " << contract_ << '\n';
    t_temp_oss_ << "Trade_Size: " << trd_qty_ << '\n';
    t_temp_oss_ << "Tot_Qty:    " << tot_qty_ << '\n';
    t_temp_oss_ << "Trade_Px:   " << trd_px_ << '\n';
    t_temp_oss_ << "InstSeqNum: " << seqno_ << '\n';
    t_temp_oss_ << "Buyer:      " << buyer_ << '\n';
    t_temp_oss_ << "Seller:     " << seller_ << '\n';
    t_temp_oss_ << "Is_Last:    " << (is_last_ ? "YES" : "NO") << '\n';
    return t_temp_oss_.str();
  }
};

struct BMFMktDeltaStruct {
  char contract_[BMF_MDS_CONTRACT_TEXT_SIZE];
  uint16_t buyer_;
  uint16_t seller_;
  uint16_t level_;
  uint16_t num_ords_;
  double price_;
  uint16_t size_;
  uint32_t trd_qty_;
  uint32_t seqno_;
  char type_;    /// entry type
  char action_;  /// update action

  inline void print() { fprintf(stderr, "%s\n", ToString().c_str()); }

  inline std::string ToString() {
    std::ostringstream t_temp_oss_;
    t_temp_oss_ << "Contract:   " << contract_ << '\n';
    t_temp_oss_ << "Buyer:      " << buyer_ << '\n';
    t_temp_oss_ << "Seller:     " << seller_ << '\n';
    t_temp_oss_ << "Level:      " << level_ << '\n';
    t_temp_oss_ << "Size:       " << size_ << '\n';
    t_temp_oss_ << "Num_Ords:   " << num_ords_ << '\n';
    t_temp_oss_ << "Price:      " << std::fixed << std::setprecision(6) << price_ << '\n';
    t_temp_oss_ << "InstSeqNum: " << seqno_ << '\n';
    t_temp_oss_ << "Trd_Qty:    " << trd_qty_ << '\n';

    switch (type_) {
      case '1':
        t_temp_oss_ << "Type:   "
                    << "ASK" << '\n';
        break;
      case '0':
        t_temp_oss_ << "Type:   "
                    << "BID" << '\n';
        break;
      default:
        t_temp_oss_ << "Type:   "
                    << "---" << '\n';
        break;
    }

    switch (action_) {
      case '0':
        t_temp_oss_ << "Action: "
                    << "NEW" << '\n';
        break;
      case '1':
        t_temp_oss_ << "Action: "
                    << "CHANGE" << '\n';
        break;
      case '2':
        t_temp_oss_ << "Action: "
                    << "DELETE" << '\n';
        break;
      case '3':
        t_temp_oss_ << "Action: "
                    << "DEL_FROM" << '\n';
        break;
      case '4':
        t_temp_oss_ << "Action: "
                    << "DEL_THRU" << '\n';
        break;
      default:
        t_temp_oss_ << "Action: "
                    << "--------" << '\n';
        break;
    }

    return t_temp_oss_.str();
  }
};

struct BMFOrderStruct {  // --
  char contract_[BMF_MDS_CONTRACT_TEXT_SIZE];
  uint16_t buyer_;
  uint16_t seller_;
  uint16_t level_;
  uint16_t num_ords_;
  double price_;
  uint16_t size_;
  uint32_t trd_qty_;
  uint32_t seqno_;
  char type_;          /// entry type
  char action_;        /// update action
  char order_id_[20];  // GTS order ids are too large to fit into an int

  inline void print() { fprintf(stderr, "%s\n", ToString().c_str()); }

  inline std::string ToString() {
    std::ostringstream t_temp_oss_;
    t_temp_oss_ << "Contract:   " << contract_ << '\n';
    t_temp_oss_ << "Buyer:      " << buyer_ << '\n';
    t_temp_oss_ << "Seller:     " << seller_ << '\n';
    t_temp_oss_ << "Level:      " << level_ << '\n';
    t_temp_oss_ << "Size:       " << size_ << '\n';
    t_temp_oss_ << "Num_Ords:   " << num_ords_ << '\n';
    t_temp_oss_ << "Price:      " << std::fixed << std::setprecision(6) << price_ << '\n';
    t_temp_oss_ << "InstSeqNum: " << seqno_ << '\n';
    t_temp_oss_ << "Trd_Qty:    " << trd_qty_ << '\n';

    switch (type_) {
      case '1':
        t_temp_oss_ << "Type:   "
                    << "ASK" << '\n';
        break;
      case '0':
        t_temp_oss_ << "Type:   "
                    << "BID" << '\n';
        break;
      default:
        t_temp_oss_ << "Type:   "
                    << "---" << '\n';
        break;
    }

    switch (action_) {
      case '0':
        t_temp_oss_ << "Action: "
                    << "NEW" << '\n';
        break;
      case '1':
        t_temp_oss_ << "Action: "
                    << "CHANGE" << '\n';
        break;
      case '2':
        t_temp_oss_ << "Action: "
                    << "DELETE" << '\n';
        break;
      case '3':
        t_temp_oss_ << "Action: "
                    << "DEL_FROM" << '\n';
        break;
      case '4':
        t_temp_oss_ << "Action: "
                    << "DEL_THRU" << '\n';
        break;
      case '9':
        t_temp_oss_ << "Action: "
                    << "BOOK_RESET" << '\n';
        break;
      default:
        t_temp_oss_ << "Action: "
                    << "--------" << '\n';
        break;
    }

    t_temp_oss_ << "OrderID: " << order_id_ << "\n";

    return t_temp_oss_.str();
  }
};

/// would include all message types eventually
enum msgType { BMF_DELTA = 5, BMF_TRADE, BMF_ORDER };  // --

/// common struct .. will support all messages eventually
struct BMFCommonStruct {
  msgType msg_;
  timeval time_;
  union {
    BMFTradeStruct bmf_trds_;
    BMFMktDeltaStruct bmf_dels_;
    BMFOrderStruct bmf_ordr_;  // --
  } data_;

  inline void print() { fprintf(stderr, "%s\n", ToString().c_str()); }
  inline bool isTradeMsg() { return (BMF_TRADE == msg_); }
  inline double GetTradeDoublePrice() { return data_.bmf_trds_.trd_px_; }
  inline HFSAT::TradeType_t GetTradeAggressorSide() { return HFSAT::TradeType_t::kTradeTypeNoInfo; }
  inline uint32_t GetTradeSize() { return data_.bmf_trds_.trd_qty_; }
  inline void SetIntermediate(bool flag) {
    switch (msg_) {
      case BMF_TRADE:
        break;
      case BMF_DELTA:
        // No intermediate flag
        break;
      case BMF_ORDER:
        // no intermediate flag
        break;
      default:
        break;
    }
  }
  char* getContract() {
    switch (msg_) {
      case BMF_TRADE:
        return data_.bmf_trds_.contract_;
        break;
      case BMF_DELTA:
        return data_.bmf_dels_.contract_;
        break;
      case BMF_ORDER:
        return data_.bmf_ordr_.contract_;
        break;
      default:
        return NULL;
    }
  }

  bool isTradeMsg() const { return msg_ == BMF_TRADE; }

  inline std::string ToString() {
    switch (msg_) {
      case BMF_DELTA: {
        std::ostringstream t_temp_oss_;
        t_temp_oss_ << "\n============== BMF BookDelta Message ================\n\n";
        t_temp_oss_ << "Time: " << time_.tv_sec << "." << std::setw(6) << std::setfill('0') << time_.tv_usec << "\n";

        return (t_temp_oss_.str() + data_.bmf_dels_.ToString() +
                "===================================================\n");
      } break;
      case BMF_TRADE: {
        std::ostringstream t_temp_oss_;
        t_temp_oss_ << "\n============== BMF Trade Message ================\n\n";
        t_temp_oss_ << "Time: " << time_.tv_sec << "." << std::setw(6) << std::setfill('0') << time_.tv_usec << "\n";

        return (t_temp_oss_.str() + data_.bmf_trds_.ToString() +
                "===================================================\n");
      } break;
      case BMF_ORDER:  // --
      {
        std::ostringstream t_temp_oss_;
        t_temp_oss_ << "\n============== BMF BookOrder Message ================\n\n";
        t_temp_oss_ << "Time: " << time_.tv_sec << "." << std::setw(6) << std::setfill('0') << time_.tv_usec << "\n";

        return (t_temp_oss_.str() + data_.bmf_ordr_.ToString() +
                "===================================================\n");
      } break;

      default: {
        std::ostringstream t_temp_oss_;
        t_temp_oss_ << "NOT IMPLEMENTED FOR THIS EVENT : " << msg_ << '\n';

        return t_temp_oss_.str();
      } break;
    }
  }
};
}

namespace NTP_NS {
enum TIDS {
  MDSECLIST_111 = 111,
  MDSEQRESET_122 = 122,
  MDSNAPSHOT_124 = 124,
  MDHEARTBEAT_101 = 101,
  MDNEWS_120 = 120,
  MDSECSTATUS_125 = 125,

  // Incremental refresh templates
  MDIncRefresh_126 = 126,
  MDIncRefresh_123 = 123,

  // Empty message for book reset
  MDIncRefresh_81 = 81,
  // ITC-FIX (Open Interest, Actual Volume, Theoretical Settles, and Fixing Prices from ePRS)
  MDIncRefresh_97 = 97,
  // Equity Futures and Commodity Futures (Channel 7 and 113)
  MDIncRefresh_103 = 103,
  MDIncRefresh_83 = 83,
  MDIncRefresh_84 = 84,
  /*
   *Interest Rate Futures, Nymex Futures (Channel 9, 13, 31, 32, 115, 118, 120)
   *NYMEX Crude Futures (Channel 30)
   *CBOT Commodity Futures (Channel 111)
   *DME Futures (Channel 28)
   *BMF (Channel 24, 25) will use template 59 only
   */
  MDIncRefresh_86 = 86,
  MDIncRefresh_87 = 87,
  MDIncRefresh_88 = 88,
  MDIncRefresh_104 = 104,  //   for BMF
  // FX Futures (Channel 6, 11, 22)
  MDIncRefresh_105 = 105,
  MDIncRefresh_90 = 90,
  MDIncRefresh_91 = 91
};
}

/// NTP broadcast structs
namespace NTP_MDS {
#define NTP_MDS_CONTRACT_TEXT_SIZE 12

struct NTP_Header {
  uint32_t msg_seq;
  uint16_t num_cnk;
  uint16_t cur_cnk;
  uint16_t msg_len;

  void toHost() {
    msg_seq = ntohl(msg_seq);
    num_cnk = ((num_cnk >> 8) & 0x00FF) | (num_cnk << 8);
    cur_cnk = ((cur_cnk >> 8) & 0x00FF) | (cur_cnk << 8);
    msg_len = ((msg_len >> 8) & 0x00FF) | (msg_len << 8);
  };

  inline std::string ToString() {
    std::ostringstream t_temp_oss_;
    t_temp_oss_ << "Msg_Seqnum:   " << msg_seq << "\n";
    t_temp_oss_ << "Num_Chunks:   " << num_cnk << "\n";
    t_temp_oss_ << "Curr_Chunk:   " << cur_cnk << "\n";
    t_temp_oss_ << "Msg_Length:   " << msg_len << "\n";
    return t_temp_oss_.str();
  }

  inline void print() { fprintf(stderr, "%s\n", ToString().c_str()); }
};

#define NTP_SEC_TRADING_STATUS_PAUSE 2
#define NTP_SEC_TRADING_STATUS_CLOSE 4
#define NTP_SEC_TRADING_STATUS_OPEN 17
#define NTP_SEC_TRADING_STATUS_FORBIDDEN 18
#define NTP_SEC_TRADING_STATUS_UNKNOWN 20
#define NTP_SEC_TRADING_STATUS_RESERVED 21
#define NTP_SEC_TRADING_STATUS_FINAL_CLOSING_CALL 101

std::string GetStatusString(int status);
bool IsStatusValid(int status);

struct NTPTradeStruct {
  char contract_[NTP_MDS_CONTRACT_TEXT_SIZE];  ///< contract name
  int32_t trd_qty_;                            ///< quantity in this trade
  uint64_t tot_qty_;                           ///< total quantity
  uint32_t seqno_;                             ///< instrument based seqno .. to order trades and quotes
  double trd_px_;                              ///< trade price
  int buyer_;
  int seller_;
  bool is_last_;
  char flags_[3];

  inline void print() { fprintf(stderr, "%s\n", ToString().c_str()); }

  inline std::string ToString() {
    std::ostringstream t_temp_oss_;
    t_temp_oss_ << "Contract:   " << contract_ << '\n';
    t_temp_oss_ << "Trade_Size: " << trd_qty_ << '\n';
    t_temp_oss_ << "Tot_Qty:    " << tot_qty_ << '\n';
    t_temp_oss_ << "Trade_Px:   " << trd_px_ << '\n';
    t_temp_oss_ << "InstSeqNum: " << seqno_ << '\n';
    t_temp_oss_ << "Buyer:      " << buyer_ << '\n';
    t_temp_oss_ << "Seller:     " << seller_ << '\n';
    t_temp_oss_ << "Is_Last:    " << (is_last_ ? "YES" : "NO") << '\n';
    t_temp_oss_ << "Crossed:    " << (flags_[0] == 'X' ? "YES" : "NO") << '\n';
    return t_temp_oss_.str();
  }
};

struct NTPDeltaStruct {
  char contract_[NTP_MDS_CONTRACT_TEXT_SIZE];
  uint16_t buyer_;
  uint16_t seller_;
  uint16_t level_;
  uint16_t num_ords_;
  double price_;
  uint32_t size_;
  uint32_t trd_qty_;
  uint32_t seqno_;
  uint32_t action_;  /// update action
  char type_;        /// entry type
  bool intermediate_;
  char flags[2];

  inline void print() { fprintf(stderr, "%s\n", ToString().c_str()); }

  inline std::string ToString() {
    std::ostringstream t_temp_oss_;
    t_temp_oss_ << "Contract:   " << contract_ << '\n';
    t_temp_oss_ << "Buyer:      " << buyer_ << '\n';
    t_temp_oss_ << "Seller:     " << seller_ << '\n';
    t_temp_oss_ << "Level:      " << level_ << '\n';
    t_temp_oss_ << "Size:       " << size_ << '\n';
    t_temp_oss_ << "Num_Ords:   " << num_ords_ << '\n';
    t_temp_oss_ << "Price:      " << std::fixed << std::setprecision(6) << price_ << '\n';
    t_temp_oss_ << "InstSeqNum: " << seqno_ << '\n';
    t_temp_oss_ << "Trd_Qty:    " << trd_qty_ << '\n';

    t_temp_oss_ << "TradingStatus: " << GetStatusString((int)flags[1]) << '\n';

    switch (type_) {
      case '0':
        t_temp_oss_ << "Type:   "
                    << "Bid" << '\n';
        break;
      case '1':
        t_temp_oss_ << "Type:   "
                    << "Offer" << '\n';
        break;
      case '2':
        t_temp_oss_ << "Type:   "
                    << "Trade" << '\n';
        break;
      case '3':
        t_temp_oss_ << "Type:   "
                    << "Index Value  " << '\n';
        break;
      case '4':
        t_temp_oss_ << "Type:   "
                    << "Opening price  " << '\n';
        break;
      case '5':
        t_temp_oss_ << "Type:   "
                    << "Closing price  " << '\n';
        break;
      case '6':
        t_temp_oss_ << "Type:   "
                    << "Settlement price  " << '\n';
        break;
      case '7':
        t_temp_oss_ << "Type:   "
                    << "Trading session high price" << '\n';
        break;
      case '8':
        t_temp_oss_ << "Type:   "
                    << "Trading session low price" << '\n';
        break;
      case '9':
        t_temp_oss_ << "Type:   "
                    << "Trading session VWAP price" << '\n';
        break;
      case 'A':
        t_temp_oss_ << "Type:   "
                    << "Imbalance" << '\n';
        break;
      case 'B':
        t_temp_oss_ << "Type:   "
                    << "Trade volume  " << '\n';
        break;
      case 'C':
        t_temp_oss_ << "Type:   "
                    << "Open Interest  " << '\n';
        break;
      case 'J':
        t_temp_oss_ << "Type:   "
                    << "Empty Book  " << '\n';
        break;
      case 'c':
        t_temp_oss_ << "Type:   "
                    << "Security trading state " << '\n';
        break;
      case 'g':
        t_temp_oss_ << "Type:   "
                    << "Price bands  " << '\n';
        break;
      default:
        t_temp_oss_ << "Type:   "
                    << "---" << '\n';
        break;
    }

    switch (action_) {
      case 0:
        t_temp_oss_ << "Action: "
                    << "NEW" << '\n';
        break;
      case 1:
        t_temp_oss_ << "Action: "
                    << "CHANGE" << '\n';
        break;
      case 2:
        t_temp_oss_ << "Action: "
                    << "DELETE" << '\n';
        break;
      case 3:
        t_temp_oss_ << "Action: "
                    << "DEL_THRU" << '\n';
        break;
      case 4:
        t_temp_oss_ << "Action: "
                    << "DEL_FROM" << '\n';
        break;
      case 5:
        t_temp_oss_ << "Action: "
                    << "OVERLAY" << '\n';
        break;
      default:
        t_temp_oss_ << "Action: "
                    << "--------" << '\n';
        break;
    }

    t_temp_oss_ << "Intermediate: " << (intermediate_ ? "Y" : "N") << '\n';

    return t_temp_oss_.str();
  }
};

struct NTPOrderStruct {
  char contract_[NTP_MDS_CONTRACT_TEXT_SIZE];
  uint16_t level_;
  uint16_t seller_;
  //    uint16_t num_ords_;
  double price_;
  uint32_t size_;
  uint32_t trd_qty_;
  uint32_t seqno_;
  uint32_t action_;  /// update action
  char type_;        /// entry type
  bool intermediate_;
  //    char flags[2];
  uint16_t buyer_;
  uint64_t order_id_;

  inline void print() { fprintf(stderr, "%s\n", ToString().c_str()); }

  inline std::string ToString() {
    std::ostringstream t_temp_oss_;
    t_temp_oss_ << "Contract:   " << contract_ << '\n';
    t_temp_oss_ << "Level:      " << level_ << '\n';
    t_temp_oss_ << "Size:       " << size_ << '\n';
    t_temp_oss_ << "Price:      " << std::fixed << std::setprecision(6) << price_ << '\n';
    t_temp_oss_ << "InstSeqNum: " << seqno_ << '\n';
    t_temp_oss_ << "Trd_Qty:    " << trd_qty_ << '\n';
    t_temp_oss_ << "OrderId:    " << order_id_ << '\n';
    t_temp_oss_ << "Buyer:      " << buyer_ << '\n';
    t_temp_oss_ << "Seller:     " << seller_ << '\n';

    switch (type_) {
      case '0':
        t_temp_oss_ << "Type:   "
                    << "Bid" << '\n';
        break;
      case '1':
        t_temp_oss_ << "Type:   "
                    << "Offer" << '\n';
        break;
      case '2':
        t_temp_oss_ << "Type:   "
                    << "Trade" << '\n';
        break;
      case '3':
        t_temp_oss_ << "Type:   "
                    << "Index Value  " << '\n';
        break;
      case '4':
        t_temp_oss_ << "Type:   "
                    << "Opening price  " << '\n';
        break;
      case '5':
        t_temp_oss_ << "Type:   "
                    << "Closing price  " << '\n';
        break;
      case '6':
        t_temp_oss_ << "Type:   "
                    << "Settlement price  " << '\n';
        break;
      case '7':
        t_temp_oss_ << "Type:   "
                    << "Trading session high price" << '\n';
        break;
      case '8':
        t_temp_oss_ << "Type:   "
                    << "Trading session low price" << '\n';
        break;
      case '9':
        t_temp_oss_ << "Type:   "
                    << "Trading session VWAP price" << '\n';
        break;
      case 'A':
        t_temp_oss_ << "Type:   "
                    << "Imbalance" << '\n';
        break;
      case 'B':
        t_temp_oss_ << "Type:   "
                    << "Trade volume  " << '\n';
        break;
      case 'C':
        t_temp_oss_ << "Type:   "
                    << "Open Interest  " << '\n';
        break;
      case 'J':
        t_temp_oss_ << "Type:   "
                    << "Empty Book  " << '\n';
        break;
      case 'c':
        t_temp_oss_ << "Type:   "
                    << "Security trading state " << '\n';
        break;
      case 'g':
        t_temp_oss_ << "Type:   "
                    << "Price bands  " << '\n';
        break;
      default:
        t_temp_oss_ << "Type:   "
                    << "---" << '\n';
        break;
    }

    switch (action_) {
      case 0:
        t_temp_oss_ << "Action: "
                    << "NEW" << '\n';
        break;
      case 1:
        t_temp_oss_ << "Action: "
                    << "CHANGE" << '\n';
        break;
      case 2:
        t_temp_oss_ << "Action: "
                    << "DELETE" << '\n';
        break;
      case 3:
        t_temp_oss_ << "Action: "
                    << "DEL_THRU" << '\n';
        break;
      case 4:
        t_temp_oss_ << "Action: "
                    << "DEL_FROM" << '\n';
        break;
      case 5:
        t_temp_oss_ << "Action: "
                    << "OVERLAY" << '\n';
        break;
      default:
        t_temp_oss_ << "Action: "
                    << "--------" << '\n';
        break;
    }

    t_temp_oss_ << "Intermediate: " << (intermediate_ ? "Y" : "N") << '\n';

    return t_temp_oss_.str();
  }
};

struct NTPOpeningPrice {
  char contract[NTP_MDS_CONTRACT_TEXT_SIZE];
  double price;
  int size;
  uint32_t seq_no;
  uint32_t theoretical;

  inline void print() { fprintf(stderr, "%s\n", ToString().c_str()); }

  inline std::string ToString() {
    std::ostringstream temp_oss;
    temp_oss << "Contract:    " << contract << '\n';
    temp_oss << "Size:        " << size << '\n';
    temp_oss << "Price:       " << std::fixed << std::setprecision(6) << price << '\n';
    temp_oss << "InstSeqNum:  " << seq_no << '\n';
    temp_oss << "Theoretical: " << theoretical << '\n';

    return temp_oss.str();
  }
};

struct NTPImbalance {
  char contract[NTP_MDS_CONTRACT_TEXT_SIZE];
  int size;
  uint32_t seq_no;
  char condition;

  inline void print() { fprintf(stderr, "%s\n", ToString().c_str()); }

  inline std::string ToString() {
    std::ostringstream temp_oss;
    temp_oss << "Contract:    " << contract << '\n';
    temp_oss << "Size:        " << size << '\n';
    temp_oss << "InstSeqNum:  " << seq_no << '\n';
    temp_oss << "Condition:   " << condition << '\n';

    return temp_oss.str();
  }
};

struct NTPSecStatus {
  char contract[NTP_MDS_CONTRACT_TEXT_SIZE];
  int security_status;
  uint32_t trading_event;
  uint64_t open_time;
  uint64_t transact_time;

  inline void print() { fprintf(stderr, "%s\n", ToString().c_str()); }

  inline std::string ToString() {
    std::ostringstream temp_oss;
    temp_oss << "Contract:     " << contract << '\n';
    temp_oss << "Status:       " << GetStatusString(security_status) << '\n';
    temp_oss << "TradingEvent: " << trading_event << '\n';
    temp_oss << "OpenTime:     " << open_time << '\n';
    temp_oss << "TransactTime: " << transact_time << '\n';

    return temp_oss.str();
  }
};

enum msgType { NTP_TRADE, NTP_DELTA, NTP_ORDER, NTP_OPENPRICE, NTP_IMBALANCE, NTP_STATUS };

/// common struct .. will support all messages eventually
struct NTPCommonStruct {
  msgType msg_;
  timeval time_;
  union {
    NTPTradeStruct ntp_trds_;
    NTPDeltaStruct ntp_dels_;
    NTPOrderStruct ntp_ordr_;
    NTPOpeningPrice ntp_open;
    NTPImbalance ntp_imbalance;
    NTPSecStatus ntp_status;
  } data_;
  inline double GetTradeDoublePrice() { return data_.ntp_trds_.trd_px_; }
  inline HFSAT::TradeType_t GetTradeAggressorSide() { return HFSAT::TradeType_t::kTradeTypeNoInfo; }
  inline uint32_t GetTradeSize() { return data_.ntp_trds_.trd_qty_; }
  inline void SetIntermediate(bool flag) {
    switch (msg_) {
      case NTP_TRADE:
        break;
      case NTP_DELTA:
        data_.ntp_dels_.intermediate_ = flag;
        break;
      case NTP_ORDER:
        break;
      case NTP_OPENPRICE: {
        break;
      }
      case NTP_IMBALANCE: {
        break;
      }
      case NTP_STATUS: {
        break;
      }
      default:
        break;
    }
  }
  char* getContract() {
    switch (msg_) {
      case NTP_TRADE:
        return data_.ntp_trds_.contract_;
        break;
      case NTP_DELTA:
        return data_.ntp_dels_.contract_;
        break;
      case NTP_ORDER:
        return data_.ntp_ordr_.contract_;
        break;
      case NTP_OPENPRICE: {
        return data_.ntp_open.contract;
        break;
      }
      case NTP_IMBALANCE: {
        return data_.ntp_imbalance.contract;
        break;
      }
      case NTP_STATUS: {
        return data_.ntp_status.contract;
        break;
      }
      default:
        return NULL;
    }
  }

  bool isTradeMsg() const { return (msg_ == NTP_TRADE && data_.ntp_trds_.flags_[0] != 'X'); }

  inline void print() { fprintf(stderr, "%s\n", ToString().c_str()); }

  inline std::string ToString() {
    switch (msg_) {
      case NTP_TRADE: {
        std::ostringstream t_temp_oss_;
        t_temp_oss_ << "\n============== NTP Trade Message ================\n\n";
        t_temp_oss_ << "Time: " << time_.tv_sec << "." << std::setw(6) << std::setfill('0') << time_.tv_usec << "\n";

        return (t_temp_oss_.str() + data_.ntp_trds_.ToString() +
                "===================================================\n");
      } break;
      case NTP_DELTA: {
        std::ostringstream t_temp_oss_;
        t_temp_oss_ << "\n============== NTP BookDelta Message ================\n\n";
        t_temp_oss_ << "Time: " << time_.tv_sec << "." << std::setw(6) << std::setfill('0') << time_.tv_usec << "\n";

        return (t_temp_oss_.str() + data_.ntp_dels_.ToString() +
                "===================================================\n");
      } break;
      case NTP_ORDER: {
        std::ostringstream t_temp_oss_;
        t_temp_oss_ << "\n============== NTP Order Message ================\n\n";
        t_temp_oss_ << "Time: " << time_.tv_sec << "." << std::setw(6) << std::setfill('0') << time_.tv_usec << "\n";

        return (t_temp_oss_.str() + data_.ntp_ordr_.ToString() +
                "===================================================\n");
        break;
      }
      case NTP_OPENPRICE: {
        std::ostringstream temp_oss;
        temp_oss << "\n============== NTP Opening Price ================\n\n";
        temp_oss << "Time: " << time_.tv_sec << "." << std::setw(6) << std::setfill('0') << time_.tv_usec << "\n";

        return (temp_oss.str() + data_.ntp_open.ToString() + "===================================================\n");
        break;
      }
      case NTP_IMBALANCE: {
        std::ostringstream temp_oss;
        temp_oss << "\n============== NTP Imbalance ================\n\n";
        temp_oss << "Time: " << time_.tv_sec << "." << std::setw(6) << std::setfill('0') << time_.tv_usec << "\n";

        return (temp_oss.str() + data_.ntp_imbalance.ToString() +
                "===================================================\n");
        break;
      }
      case NTP_STATUS: {
        std::ostringstream temp_oss;
        temp_oss << "\n============== NTP Status ================\n\n";
        temp_oss << "Time: " << time_.tv_sec << "." << std::setw(6) << std::setfill('0') << time_.tv_usec << "\n";

        return (temp_oss.str() + data_.ntp_status.ToString() + "===================================================\n");
        break;
      }
      default: {
        std::ostringstream t_temp_oss_;
        t_temp_oss_ << "NOT IMPLEMENTED FOR THIS EVENT : " << msg_ << '\n';

        return t_temp_oss_.str();
      } break;
    }
  }

  char const* getShortcode() {
    return "INVALID";
  }
};
}

/**************************** BMF EQUITIES PUMA ***************************************/

namespace PUMA_NS {
enum TIDS {
  MDTcpRequestReject_117 = 117,

  MDSecurityList_148 = 148,
  MDSecurityList_141 = 141,
  MDSecurityList_111 = 111,

  MDIncRefresh_81 = 81,
  MDIncRefresh_145 = 145,
  MDIncRefresh_138 = 138,
  MDIncRefresh_126 = 126,

  MDSecurityStatus_142 = 142,
  MDSecurityStatus_134 = 134,
  MDSecurityStatus_125 = 125,

  MDSnapshotFullRefresh_147 = 147,
  MDSnapshotFullRefresh_146 = 146,
  MDSnapshotFullRefresh_139 = 139,
  MDSnapshotFullRefresh_128 = 128,

  MDNewsMessage_143 = 143,
  MDNewsMessage_137 = 137,
  MDNewsMessage_120 = 120,

  MDHeartbeat_144 = 144,
  MDHeartbeat_129 = 129,
  MDHeartbeat_101 = 101,

  MDLogon_118 = 118,
  MDLogout_119 = 119

};
}

/// PUMA broadcast structs
namespace PUMA_MDS {
#define PUMA_MDS_CONTRACT_TEXT_SIZE 12

struct Puma_Header {
  uint32_t msg_seq;
  uint16_t num_cnk;
  uint16_t cur_cnk;
  uint16_t msg_len;

  void toHost() {
    msg_seq = ntohl(msg_seq);  // network byte order to host byte order
    num_cnk = ((num_cnk >> 8) & 0x00FF) |
              (num_cnk << 8);  // shift value 8 bits to the right ; reset 8 bits ;  bitwise or shift 8 bits to the left
    cur_cnk = ((cur_cnk >> 8) & 0x00FF) | (cur_cnk << 8);
    msg_len = ((msg_len >> 8) & 0x00FF) | (msg_len << 8);
  };

  inline std::string ToString() {
    std::ostringstream t_temp_oss_;
    t_temp_oss_ << "Msg_Seqnum:   " << msg_seq << "\n";
    t_temp_oss_ << "Num_Chunks:   " << num_cnk << "\n";
    t_temp_oss_ << "Curr_Chunk:   " << cur_cnk << "\n";
    t_temp_oss_ << "Msg_Length:   " << msg_len << "\n";
    return t_temp_oss_.str();
  }

  inline void print() { fprintf(stderr, "%s\n", ToString().c_str()); }
};

struct PumaTradeStruct {
  char contract_[PUMA_MDS_CONTRACT_TEXT_SIZE];  ///< contract name
  int32_t trd_qty_;                             ///< quantity in this trade
  uint64_t tot_qty_;                            ///< total quantity
  uint32_t seqno_;                              ///< instrument based seqno .. to order trades and quotes
  double trd_px_;                               ///< trade price
  int buyer_;
  int seller_;
  bool is_last_;
  char flags_[3];

  inline void print() { fprintf(stderr, "%s\n", ToString().c_str()); }

  inline std::string ToString() {
    std::ostringstream t_temp_oss_;
    t_temp_oss_ << "Contract:   " << contract_ << '\n';
    t_temp_oss_ << "Trade_Size: " << trd_qty_ << '\n';
    t_temp_oss_ << "Tot_Qty:    " << tot_qty_ << '\n';
    t_temp_oss_ << "Trade_Px:   " << trd_px_ << '\n';
    t_temp_oss_ << "InstSeqNum: " << seqno_ << '\n';
    t_temp_oss_ << "Buyer:      " << buyer_ << '\n';
    t_temp_oss_ << "Seller:     " << seller_ << '\n';
    t_temp_oss_ << "Is_Last:    " << (is_last_ ? "YES" : "NO") << '\n';
    return t_temp_oss_.str();
  }
};

struct PumaDeltaStruct {
  char contract_[PUMA_MDS_CONTRACT_TEXT_SIZE];
  uint16_t buyer_;
  uint16_t seller_;
  uint16_t level_;
  uint16_t num_ords_;
  double price_;
  uint32_t size_;
  uint32_t trd_qty_;
  uint32_t seqno_;
  uint32_t action_;  /// update action
  char type_;        /// entry type
  bool intermediate_;
  char flags[2];

  inline void print() { fprintf(stderr, "%s\n", ToString().c_str()); }

  inline std::string ToString() {
    std::ostringstream t_temp_oss_;
    t_temp_oss_ << "Contract:   " << contract_ << '\n';
    t_temp_oss_ << "Buyer:      " << buyer_ << '\n';
    t_temp_oss_ << "Seller:     " << seller_ << '\n';
    t_temp_oss_ << "Level:      " << level_ << '\n';
    t_temp_oss_ << "Size:       " << size_ << '\n';
    t_temp_oss_ << "Num_Ords:   " << num_ords_ << '\n';
    t_temp_oss_ << "Price:      " << std::fixed << std::setprecision(6) << price_ << '\n';
    t_temp_oss_ << "InstSeqNum: " << seqno_ << '\n';
    t_temp_oss_ << "Trd_Qty:    " << trd_qty_ << '\n';
    t_temp_oss_ << "TradingStatus:    " << (int32_t)flags[1] << '\n';

    switch (type_) {
      case '0':
        t_temp_oss_ << "Type:   "
                    << "Bid" << '\n';
        break;
      case '1':
        t_temp_oss_ << "Type:   "
                    << "Offer" << '\n';
        break;
      case '2':
        t_temp_oss_ << "Type:   "
                    << "Trade" << '\n';
        break;
      case '3':
        t_temp_oss_ << "Type:   "
                    << "Index Value  " << '\n';
        break;
      case '4':
        t_temp_oss_ << "Type:   "
                    << "Opening price  " << '\n';
        break;
      case '5':
        t_temp_oss_ << "Type:   "
                    << "Closing price  " << '\n';
        break;
      case '6':
        t_temp_oss_ << "Type:   "
                    << "Settlement price  " << '\n';
        break;
      case '7':
        t_temp_oss_ << "Type:   "
                    << "Trading session high price" << '\n';
        break;
      case '8':
        t_temp_oss_ << "Type:   "
                    << "Trading session low price" << '\n';
        break;
      case '9':
        t_temp_oss_ << "Type:   "
                    << "Trading session VWAP price" << '\n';
        break;
      case 'A':
        t_temp_oss_ << "Type:   "
                    << "Imbalance" << '\n';
        break;
      case 'B':
        t_temp_oss_ << "Type:   "
                    << "Trade volume  " << '\n';
        break;
      case 'C':
        t_temp_oss_ << "Type:   "
                    << "Open Interest  " << '\n';
        break;
      case 'J':
        t_temp_oss_ << "Type:   "
                    << "Empty Book  " << '\n';
        break;
      case 'c':
        t_temp_oss_ << "Type:   "
                    << "Security trading state " << '\n';
        break;
      case 'g':
        t_temp_oss_ << "Type:   "
                    << "Price bands  " << '\n';
        break;
      default:
        t_temp_oss_ << "Type:   "
                    << "---" << '\n';
        break;
    }

    switch (action_) {
      case 0:
        t_temp_oss_ << "Action: "
                    << "NEW" << '\n';
        break;
      case 1:
        t_temp_oss_ << "Action: "
                    << "CHANGE" << '\n';
        break;
      case 2:
        t_temp_oss_ << "Action: "
                    << "DELETE" << '\n';
        break;
      case 3:
        t_temp_oss_ << "Action: "
                    << "DEL_THRU" << '\n';
        break;
      case 4:
        t_temp_oss_ << "Action: "
                    << "DEL_FROM" << '\n';
        break;
      case 5:
        t_temp_oss_ << "Action: "
                    << "OVERLAY" << '\n';
        break;
      default:
        t_temp_oss_ << "Action: "
                    << "--------" << '\n';
        break;
    }

    t_temp_oss_ << "Intermediate: " << (intermediate_ ? "Y" : "N") << '\n';

    return t_temp_oss_.str();
  }
};

struct PumaOrderStruct {
  char contract_[PUMA_MDS_CONTRACT_TEXT_SIZE];
  //    uint16_t buyer_;
  //    uint16_t seller_;
  uint16_t level_;
  //    uint16_t num_ords_;
  double price_;
  uint32_t size_;
  uint32_t trd_qty_;
  uint32_t seqno_;
  uint32_t action_;  /// update action
  char type_;        /// entry type
  bool intermediate_;
  //    char flags[2];
  uint64_t order_id_;

  inline void print() { fprintf(stderr, "%s\n", ToString().c_str()); }

  inline std::string ToString() {
    std::ostringstream t_temp_oss_;
    t_temp_oss_ << "Contract:   " << contract_ << '\n';
    t_temp_oss_ << "Level:      " << level_ << '\n';
    t_temp_oss_ << "Size:       " << size_ << '\n';
    t_temp_oss_ << "Price:      " << std::fixed << std::setprecision(6) << price_ << '\n';
    t_temp_oss_ << "InstSeqNum: " << seqno_ << '\n';
    t_temp_oss_ << "Trd_Qty:    " << trd_qty_ << '\n';
    t_temp_oss_ << "OrderId:    " << order_id_ << '\n';

    switch (type_) {
      case '0':
        t_temp_oss_ << "Type:   "
                    << "Bid" << '\n';
        break;
      case '1':
        t_temp_oss_ << "Type:   "
                    << "Offer" << '\n';
        break;
      case '2':
        t_temp_oss_ << "Type:   "
                    << "Trade" << '\n';
        break;
      case '3':
        t_temp_oss_ << "Type:   "
                    << "Index Value  " << '\n';
        break;
      case '4':
        t_temp_oss_ << "Type:   "
                    << "Opening price  " << '\n';
        break;
      case '5':
        t_temp_oss_ << "Type:   "
                    << "Closing price  " << '\n';
        break;
      case '6':
        t_temp_oss_ << "Type:   "
                    << "Settlement price  " << '\n';
        break;
      case '7':
        t_temp_oss_ << "Type:   "
                    << "Trading session high price" << '\n';
        break;
      case '8':
        t_temp_oss_ << "Type:   "
                    << "Trading session low price" << '\n';
        break;
      case '9':
        t_temp_oss_ << "Type:   "
                    << "Trading session VWAP price" << '\n';
        break;
      case 'A':
        t_temp_oss_ << "Type:   "
                    << "Imbalance" << '\n';
        break;
      case 'B':
        t_temp_oss_ << "Type:   "
                    << "Trade volume  " << '\n';
        break;
      case 'C':
        t_temp_oss_ << "Type:   "
                    << "Open Interest  " << '\n';
        break;
      case 'J':
        t_temp_oss_ << "Type:   "
                    << "Empty Book  " << '\n';
        break;
      case 'c':
        t_temp_oss_ << "Type:   "
                    << "Security trading state " << '\n';
        break;
      case 'g':
        t_temp_oss_ << "Type:   "
                    << "Price bands  " << '\n';
        break;
      default:
        t_temp_oss_ << "Type:   "
                    << "---" << '\n';
        break;
    }

    switch (action_) {
      case 0:
        t_temp_oss_ << "Action: "
                    << "NEW" << '\n';
        break;
      case 1:
        t_temp_oss_ << "Action: "
                    << "CHANGE" << '\n';
        break;
      case 2:
        t_temp_oss_ << "Action: "
                    << "DELETE" << '\n';
        break;
      case 3:
        t_temp_oss_ << "Action: "
                    << "DEL_THRU" << '\n';
        break;
      case 4:
        t_temp_oss_ << "Action: "
                    << "DEL_FROM" << '\n';
        break;
      case 5:
        t_temp_oss_ << "Action: "
                    << "OVERLAY" << '\n';
        break;
      default:
        t_temp_oss_ << "Action: "
                    << "--------" << '\n';
        break;
    }

    t_temp_oss_ << "Intermediate: " << (intermediate_ ? "Y" : "N") << '\n';

    return t_temp_oss_.str();
  }
};

enum msgType { PUMA_TRADE, PUMA_DELTA, PUMA_ORDER, PUMA_OPENPRICE, PUMA_IMBALANCE, PUMA_STATUS };

/// common struct .. will support all messages eventually
struct PumaCommonStruct {
  msgType msg_;
  timeval time_;
  union {
    PumaTradeStruct puma_trds_;
    PumaDeltaStruct puma_dels_;
    PumaOrderStruct puma_ordr_;
  } data_;

  char* getContract() {
    switch (msg_) {
      case PUMA_TRADE:
        return data_.puma_trds_.contract_;
        break;
      case PUMA_DELTA:
        return data_.puma_dels_.contract_;
        break;
      case PUMA_ORDER:
        return data_.puma_ordr_.contract_;
        break;
      default:
        return NULL;
    }
  }

  inline void print() { fprintf(stderr, "%s\n", ToString().c_str()); }

  inline std::string ToString() {
    //     std::cerr << "CommonStruct_toString is called  " << msg_ << " " << PUMA_TRADE << " " <<  PUMA_DELTA << " " <<
    //     PUMA_ORDER  << "\n" ;

    switch (msg_) {
      case PUMA_TRADE: {
        std::ostringstream t_temp_oss_;
        t_temp_oss_ << "\n============== PUMA Trade Message ================\n\n";
        t_temp_oss_ << "Time: " << time_.tv_sec << "." << std::setw(6) << std::setfill('0') << time_.tv_usec << "\n";

        return (t_temp_oss_.str() + data_.puma_trds_.ToString() +
                "===================================================\n");
      } break;
      case PUMA_DELTA: {
        std::ostringstream t_temp_oss_;
        t_temp_oss_ << "\n============== PUMA BookDelta Message ================\n\n";
        t_temp_oss_ << "Time: " << time_.tv_sec << "." << std::setw(6) << std::setfill('0') << time_.tv_usec << "\n";

        return (t_temp_oss_.str() + data_.puma_dels_.ToString() +
                "===================================================\n");
      } break;
      case PUMA_ORDER: {
        std::ostringstream t_temp_oss_;
        t_temp_oss_ << "\n============== PUMA Order Message ================\n\n";
        t_temp_oss_ << "Time: " << time_.tv_sec << "." << std::setw(6) << std::setfill('0') << time_.tv_usec << "\n";

        return (t_temp_oss_.str() + data_.puma_ordr_.ToString() +
                "===================================================\n");
      } break;
      default: {
        std::ostringstream t_temp_oss_;
        t_temp_oss_ << "NOT IMPLEMENTED FOR THIS EVENT : " << msg_ << '\n';

        return t_temp_oss_.str();
      } break;
    }
  }
};
}

#endif  // BASE_CDEF_BMF_COMMON_MESSAGE_STRUCTS_HPP
