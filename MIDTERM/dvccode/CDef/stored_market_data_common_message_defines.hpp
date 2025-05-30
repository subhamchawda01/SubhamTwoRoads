/**
   \file stored_market_data_common_message_defines.hpp

   \author: (c) Copyright Two Roads Technological Solutions Pvt Ltd 2011
   Address:
   Suite 217, Level 2, Prestige Omega,
   No 104, EPIP Zone, Whitefield,
   Bangalore - 560066
   India
   +91 80 4060 0717
 */
#ifndef BASE_CDEF_STORED_MARKET_DATA_COMMON_MESSAGE_STRUCTS_HPP
#define BASE_CDEF_STORED_MARKET_DATA_COMMON_MESSAGE_STRUCTS_HPP

#include <cstdlib>
#include <vector>
#include <map>
#include "dvccode/CDef/bmf_common_message_defines.hpp"
#include "dvccode/CDef/eobi_mds_defines.hpp"
#include "dvccode/CDef/asx_mds_defines.hpp"
#include "dvccode/CDef/sgx_mds_defines.hpp"
#include "dvccode/CDef/hkomd_mds_defines.hpp"
#include "dvccode/CDef/chix_mds_defines.hpp"
#include "dvccode/CDef/defines.hpp"
#include "dvccode/CDef/af_msg_parser.hpp"
#include "dvccode/CDef/nse_mds_defines.hpp"
#include "dvccode/CDef/ose_itch_mds_defines.hpp"
#include "dvccode/CDef/rts_mds_defines.hpp"
#include "dvccode/CDef/tmx_obf_mds_defines.hpp"
#include "dvccode/CDef/krx_mds_defines.hpp"
#include "dvccode/CDef/micex_mds_defines.hpp"
#include "dvccode/Utils/nse_daily_token_symbol_handler.hpp"
#include "dvccode/CDef/assumptions.hpp"
#include "dvccode/CDef/ls_contract_map_singleton.hpp"
#include "dvccode/Utils/common_files_path.hpp"
namespace HFSAT {}

/// only used in daemon side template parsing
namespace EUREX_IDS {
enum TIDS {
  PACKETHEADER = 110,  // VERSION = 1 --replaced by PACKETHEADER in Eurex 14.0
  BEACON = 81,
  PRODUCTREF = 82,
  SLEGREF = 83,
  STRATEGYREF = 84,
  BOOKSNAP = 85,
  BOOKDELTA = 86,
  REQINFO = 87,
  TRADEINFO = 89,
  FASTRESET = 120,
  STARTSLEGSNAP = 178,
  ENDSLEGSNAP = 179,
  STARTSTRATSNAP = 180,
  ENDSTRATSNAP = 181,
};
};

/// only used in daemon side template parsing
namespace CME_NS {
enum TIDS {
  SECDEF_79 = 79,
  SNAPSHOT_80 = 80,
  REFRESH_81 = 81,
  REFRESH_83 = 83,
  REFRESH_84 = 84,
  REFRESH_86 = 86,
  REFRESH_87 = 87,
  REFRESH_88 = 88,
  REFRESH_90 = 90,
  REFRESH_91 = 91,
  REFRESH_97 = 97,
  QUOTEREQ_98 = 98,
  SECSTATUS = 99,
  HEARTBEAT = 101,
  REFRESH_103 = 103,
  REFRESH_104 = 104,
  REFRESH_105 = 105,
  REFRESH_109 = 109,
  FASTRESET = 120
};
};

/// modes of operation of MDHandler
enum HandlerMode { REFERENCE = 1, DATA, BINARY_DUMP, CERTIFY, LOGGER, DATAPL, LOGGERPL, SHMWRITER };

/// exchanges supported by FIXFAST data handlers
enum Exchange { CME, EUREX, BMF, NTP, NTP_ORD, RTS, MICEX, OSE, HKEX };

// Liffe OPTIQ structs
namespace LIFFE_MDS {
#define LIFFE_HEADER_LENGTH 16

#define LIFFE_HEADER_PACKET_LENGTH_OFFSET 0
#define LIFFE_HEADER_PACKET_TYPE_OFFSET 2
#define LIFFE_HEADER_PACKET_SEQ_NUM_OFFSET 4
#define LIFFE_HEADER_SEND_TIME_OFFSET 8
#define LIFFE_HEADER_SERVICE_ID_OFFSET 12
#define LIFFE_HEADER_DELIVERY_FLAG_OFFSET 14
#define LIFFE_HEADER_NUMBER_MSG_ENTRIES_OFFSET 15

struct LIFFE_Header {  // NYSE Liffe XDP Client Spec v1.4b
  int16_t packet_length_;
  int16_t packet_type_;
  int32_t packet_seq_num_;
  int32_t send_time_;
  int16_t service_id_;  // The Liffe equivalent of a "channel"
  int8_t delivery_flag_;
  int8_t number_msg_entries_;

  void toHost() {  // TODO_OPT : Not all will be needed. Figure out which can be removed - S
    packet_length_ = ntohs(packet_length_);
    packet_type_ = ntohs(packet_type_);
    packet_seq_num_ = ntohl(packet_seq_num_);
    send_time_ = ntohl(send_time_);
    service_id_ = ntohs(service_id_);
  }

  inline std::string ToString() {
    std::ostringstream t_temp_oss_;
    t_temp_oss_ << "LIFFE_Header LENGTH : " << packet_length_ << " TYPE : " << packet_type_
                << " SEQ_NUM : " << packet_seq_num_ << " SEND_TIME : " << send_time_ << " SERVICE_ID : " << service_id_
                << " DELIVERY_FLAG : " << (int)delivery_flag_ << " NUM_MSG_ENTRIES : " << (int)number_msg_entries_
                << "\n";
    return t_temp_oss_.str();
  }
};  // struct LIFFE_Header

#define LIFFE_MDS_CONTRACT_TEXT_SIZE 16

// TODO : Figure out market structure and fix delta & trade structs - S
struct LIFFETradeStruct {
  char contract_[LIFFE_MDS_CONTRACT_TEXT_SIZE];  ///< internal contract name
  uint32_t trd_qty_;                             ///< quantity in this trade
  uint32_t tot_qty_;                             ///< total quantity .. to find if trade precedes quote or not
  //    uint32_t num_buy_ords_;    ///< num buy orders involved in trade
  //    uint32_t num_sell_ords_;   ///< num sell orders involved in trade
  char agg_side_;  ///< aggressor .. can be neither B/S in case of complex trade (eg aggressive trade on future spread
  /// settling against indv leg)
  //    char     flags[3];         ///< reserved
  double trd_px_;  ///< trade price

  inline void print() { fprintf(stderr, "%s\n", ToString().c_str()); };

  inline std::string ToString() {
    std::ostringstream t_temp_oss_;
    t_temp_oss_ << "Contract:" << contract_ << '\n';
    t_temp_oss_ << "Agg_Side:" << agg_side_ << '\n';
    t_temp_oss_ << "Trade_Size:" << trd_qty_ << '\n';
    t_temp_oss_ << "Tot_Qty:" << tot_qty_ << '\n';
    //      t_temp_oss_ << "Num_Buy_Sell:" << num_buy_ords_ << "\t" << num_sell_ords_ << '\n';
    t_temp_oss_ << "Trade_Px:" << trd_px_ << '\n';
    return t_temp_oss_.str();
  }
};

/// Market Delta messages are sent in response to snapshot and market delta events.
struct LIFFEMktDeltaStruct {
  char contract_[LIFFE_MDS_CONTRACT_TEXT_SIZE];  ///< internal contract name
  uint32_t trd_qty_;                             ///< quantity traded till now (ignore if 0)
  uint32_t level_;     ///< level info not provided in optiq, is always =2 (set in decoder, consistency with XDP feed)
  uint32_t size_;      ///< size at level
  double price_;       ///< price at level
  char type_;          ///< type 1 - Ask 2 - Bid , other types filtered
  char status_;        ///< ' ' is normal operation. Currently not filtered
  char action_;        ///< 1 - New, 2 - Change, 3 - Delete, 4 - Delete From , 5 - Delete through
  bool intermediate_;  ///< if this is an intermediate message .. clients should not react to intermediates
  uint32_t num_ords_;  ///<  LIFFE 13.0 change, showing the number of price_size contributers ( orders ) at thislevel

  inline void print() { fprintf(stderr, "%s\n", ToString().c_str()); }

  inline std::string ToString() {
    std::ostringstream t_temp_oss_;
    t_temp_oss_ << "Contract: " << contract_ << '\n';
    t_temp_oss_ << "Tot_Qty:  " << trd_qty_ << '\n';
    t_temp_oss_ << "Level:    " << level_ << '\n';
    t_temp_oss_ << "Size:     " << size_ << '\n';
    t_temp_oss_ << "Num_Ords: " << num_ords_ << '\n';
    t_temp_oss_ << "Price:    " << std::fixed << std::setprecision(6) << price_ << '\n';

    switch (type_) {
      case '1':
        t_temp_oss_ << "Type:   "
                    << "ASK" << '\n';
        break;
      case '2':
        t_temp_oss_ << "Type:   "
                    << "BID" << '\n';
        break;
      default:
        t_temp_oss_ << "Type:   "
                    << "---" << '\n';
        break;
    }

    t_temp_oss_ << "Status: " << status_ << '\n';

    switch (action_) {
      case '1':
        t_temp_oss_ << "Action: "
                    << "NEW" << '\n';
        break;
      case '2':
        t_temp_oss_ << "Action: "
                    << "CHANGE" << '\n';
        break;
      case '3':
        t_temp_oss_ << "Action: "
                    << "DELETE" << '\n';
        break;
      case '4':
        t_temp_oss_ << "Action: "
                    << "DEL_FROM" << '\n';
        break;
      case '5':
        t_temp_oss_ << "Action: "
                    << "DEL_THRU" << '\n';
        break;
      default:
        t_temp_oss_ << "Action: "
                    << "--------" << '\n';
        break;
    }

    t_temp_oss_ << "Intermediate: " << (intermediate_ ? "Y" : "N") << '\n';

    return t_temp_oss_.str();
  };
};

// TODO : Figure out TRADE messges - S
enum msgType { LIFFE_DELTA = 1, LIFFE_TRADE };

struct LIFFECommonStruct {
  msgType msg_;
  timeval time_;
  uint64_t series_seq_num_;
  union {  // TODO : Add necessary structs - S
    LIFFETradeStruct liffe_trds_;
    LIFFEMktDeltaStruct liffe_dels_;
  } data_;
  inline void SetIntermediate(bool flag) {
    switch (msg_) {
      case LIFFE_DELTA:
        data_.liffe_dels_.intermediate_ = flag;
        break;
      case LIFFE_TRADE:
        break;
      default:
        break;
    }
  }

  inline bool isTradeMsg() { return (LIFFE_TRADE == msg_); }
  inline double GetTradeDoublePrice() { return data_.liffe_trds_.trd_px_; }
  inline HFSAT::TradeType_t GetTradeAggressorSide() {
    return (data_.liffe_trds_.agg_side_ == 'B')
               ? HFSAT::TradeType_t::kTradeTypeBuy
               : (data_.liffe_trds_.agg_side_ == 'S') ? HFSAT::TradeType_t::kTradeTypeSell
                                                      : HFSAT::TradeType_t::kTradeTypeNoInfo;
  }
  inline uint32_t GetTradeSize() { return data_.liffe_trds_.trd_qty_; }
  const char* getContract() const {
    switch (msg_) {
      case LIFFE_DELTA:
        return data_.liffe_dels_.contract_;
        break;
      case LIFFE_TRADE:
        return data_.liffe_trds_.contract_;
        break;
      default:
        return NULL;
    }
  }

  bool isTradeMsg() const { return msg_ == LIFFE_TRADE; }

  inline std::string ToString() {
    switch (msg_) {
      case LIFFE_DELTA: {
        std::ostringstream t_temp_oss_;
        t_temp_oss_ << "\n============== LIFFE BookDelta Message ================\n\n";
        t_temp_oss_ << "Time: " << time_.tv_sec << "." << std::setw(6) << std::setfill('0') << time_.tv_usec << "\n";
        t_temp_oss_ << "SerSeqNum: " << series_seq_num_ << "\n";

        return (t_temp_oss_.str() + data_.liffe_dels_.ToString() +
                "===================================================\n");
      } break;
      case LIFFE_TRADE: {
        std::ostringstream t_temp_oss_;
        t_temp_oss_ << "\n============== LIFFE Trade Message ================\n\n";
        t_temp_oss_ << "Time: " << time_.tv_sec << "." << std::setw(6) << std::setfill('0') << time_.tv_usec << "\n";
        t_temp_oss_ << "SerSeqNum: " << series_seq_num_ << "\n";

        return (t_temp_oss_.str() + data_.liffe_trds_.ToString() +
                "===================================================\n");
      } break;
      default: {
        std::ostringstream t_temp_oss_;
        t_temp_oss_ << "LIFFE: NOT IMPLEMENTED FOR THIS EVENT : " << msg_ << '\n';

        return t_temp_oss_.str();
      } break;
    }
  }
};  // struct LIFFECommonStruct
}  // namespace LIFFE_MDS

/// retail specific structs
namespace RETAIL_MDS {
#define RETAIL_MDS_CONTRACT_TEXT_SIZE 32
enum trdType { ACCEPTED = 1, REJECTED, PARTIAL_ACCEPTANCE };  // PARTIAL_ACCEPTANCE is not being used presently

struct RETAILTradeStruct {
  char contract_[RETAIL_MDS_CONTRACT_TEXT_SIZE];  ///< internal contract name
  uint32_t trd_qty_;                              ///< quantity in this trade
  char agg_side_;  ///< aggressor .. can be neither B/S in case of complex trade (eg aggressive trade on future spread
  /// settling against indv leg)
  double trd_px_;           ///< trade price
  uint32_t quoted_qty_;     ///< to check for how much fraction of offer, we are getting filled
  uint32_t requested_qty_;  ///< could be different from trd_qty_ when we have support for partial acceptance
  trdType trd_type_;        ///< this can be redundant as times

  inline void print() { fprintf(stderr, "%s\n", ToString().c_str()); };

  inline std::string ToString() {
    std::ostringstream t_temp_oss_;
    t_temp_oss_ << "Contract: " << contract_ << "\n";
    t_temp_oss_ << " Agg_Side: " << agg_side_ << "\n";
    t_temp_oss_ << " Trade_Size: " << trd_qty_ << "\n";
    t_temp_oss_ << " Trade_Px: " << trd_px_ << "\n";
    t_temp_oss_ << " Quote_Sz: " << quoted_qty_ << "\n";
    t_temp_oss_ << " Request_Sz: " << requested_qty_ << "\n";
    t_temp_oss_ << " trd_type_: " << trd_type_ << "\n";
    return t_temp_oss_.str();
  }
};

/// would include all message types eventually
enum msgType { RETAIL_TRADE = 1, RETAIL_HEARTBEAT };

/// common struct .. will support all messages eventually
struct RETAILCommonStruct {
  msgType msg_;
  timeval time_;
  union {
    RETAILTradeStruct retail_trds_;
  } data_;
  inline double GetTradeDoublePrice() { return data_.retail_trds_.trd_px_; }
  inline HFSAT::TradeType_t GetTradeAggressorSide() {
    return (data_.retail_trds_.agg_side_ == '1')
               ? HFSAT::TradeType_t::kTradeTypeBuy
               : (data_.retail_trds_.agg_side_ == '2') ? HFSAT::TradeType_t::kTradeTypeSell
                                                       : HFSAT::TradeType_t::kTradeTypeNoInfo;
  }
  inline uint32_t GetTradeSize() { return data_.retail_trds_.trd_qty_; }
  inline void print() { fprintf(stderr, "%s\n", ToString().c_str()); }
  inline void SetIntermediate(bool flag) {
    // DO Nothing
  }
  inline bool isTradeMsg() { return (RETAIL_TRADE == msg_); }
  const char* getContract() const {
    switch (msg_) {
      case RETAIL_TRADE:
        return (std::string("RETAIL_") + std::string(data_.retail_trds_.contract_)).c_str();
        break;
      default:
        return std::string("RETAIL_EXTRA").c_str();
    }
  }

  inline std::string ToString() {
    switch (msg_) {
      case RETAIL_TRADE: {
        std::ostringstream t_temp_oss_;
        t_temp_oss_ << "RETAILTradeMessage ";
        t_temp_oss_ << "Time: " << time_.tv_sec << "." << std::setw(6) << std::setfill('0') << time_.tv_usec << " ";
        t_temp_oss_ << data_.retail_trds_.ToString();

        return t_temp_oss_.str();
      } break;
      case RETAIL_HEARTBEAT: {
        return "RETAILHeartbeatMessage\n";
      } break;
      default: {
        std::ostringstream t_temp_oss_;
        t_temp_oss_ << "RETAIL: NOT IMPLEMENTED FOR THIS RETAIL EVENT : " << msg_ << '\n';

        return t_temp_oss_.str();
      } break;
    }
  }
};
}

/// eurex specific bcast structs
namespace EUREX_MDS {
#define EUREX_MDS_CONTRACT_TEXT_SIZE 12

#define EUREX_HEADER_LEN 18
#define EUREX_HEADER_SEQNO_OFFSET 4
#define EUREX_HEADER_SRCID_OFFSET 2

#define EUREX_NTA_REF_HEADER_LEN 17
#define EUREX_NTA_HEADER_LEN 23
#define EUREX_NTA_REF_HEADER_SEQNO_OFFSET 4
#define EUREX_NTA_HEADER_SEQNO_OFFSET 5
#define EUREX_NTA_REF_HEADER_SRCID_OFFSET 2
#define EUREX_NTA_HEADER_SRCID_OFFSET 3
#define DEF_LS_PRODUCTCODE_SHORTCODE_ "/spare/local/files/EUREX/eurex_livesource_contractcode_shortcode_mapping.txt"
struct EUREX_Header {
  char pmap;
  char template_id;
  char src_comp_id;
  char pack_len;
  int pack_seq_no;
  char tm_stamp_len;
  char tm_stamp[5];  // big endian, convert appropriately if it is used
  char performance_len;
  char performance_indicator[3];

  void toHost() { pack_seq_no = ntohl(pack_seq_no); };

  inline std::string ToString() {
    std::ostringstream t_temp_oss_;
    t_temp_oss_ << "packet sequence number:   " << pack_seq_no << "\n";
    return t_temp_oss_.str();
  }

  inline void print() { fprintf(stderr, "%s\n", ToString().c_str()); }
};

/// sent in response to trade message .. daemon only propagates forward.
/// trade messages affecting OB ..
struct EUREXTradeStruct {
  char contract_[EUREX_MDS_CONTRACT_TEXT_SIZE];  ///< internal contract name
  uint32_t trd_qty_;                             ///< quantity in this trade
  uint32_t tot_qty_;                             ///< total quantity .. to find if trade precedes quote or not
  uint32_t num_buy_ords_;                        ///< num buy orders involved in trade
  uint32_t num_sell_ords_;                       ///< num sell orders involved in trade
  char agg_side_;  ///< aggressor .. can be neither B/S in case of complex trade (eg aggressive trade on future spread
  /// settling against indv leg)
  char flags[3];   ///< reserved
  double trd_px_;  ///< trade price

  inline void print() { fprintf(stderr, "%s\n", ToString().c_str()); };

  inline std::string ToString() {
    std::ostringstream t_temp_oss_;
    t_temp_oss_ << "Contract:" << contract_ << '\n';
    t_temp_oss_ << "Agg_Side:" << agg_side_ << '\n';
    t_temp_oss_ << "Trade_Size:" << trd_qty_ << '\n';
    t_temp_oss_ << "Tot_Qty:" << tot_qty_ << '\n';
    t_temp_oss_ << "Num_Buy_Sell:" << num_buy_ords_ << "\t" << num_sell_ords_ << '\n';
    t_temp_oss_ << "Trade_Px:" << trd_px_ << '\n';
    return t_temp_oss_.str();
  }
};

/// Market Delta messages are sent in response to snapshot and market delta events.
struct EUREXMktDeltaStruct {
  char contract_[EUREX_MDS_CONTRACT_TEXT_SIZE];  ///< internal contract name
  uint32_t trd_qty_;                             ///< quantity traded till now (ignore if 0)
  uint32_t level_;                               ///< level >= 1 (level 0 is currently filtered at daemon)
  uint32_t size_;                                ///< size at level
  double price_;                                 ///< price at level
  char type_;                                    ///< type 1 - Ask 2 - Bid , other types filtered
  char status_;                                  ///< ' ' is normal operation. Currently not filtered
  char action_;        ///< 1 - New, 2 - Change, 3 - Delete, 4 - Delete From , 5 - Delete through
  bool intermediate_;  ///< if this is an intermediate message .. clients should not react to intermediates
  uint32_t num_ords_;  ///<  EUREX 13.0 change, showing the number of price_size contributers ( orders ) at thislevel
  char pxsrc_[3];      ///< ???
  char flags[5];       ///< reserved ?

  inline void print() { fprintf(stderr, "%s\n", ToString().c_str()); }

  inline std::string ToString() {
    std::ostringstream t_temp_oss_;
    t_temp_oss_ << "Contract: " << contract_ << '\n';
    t_temp_oss_ << "Tot_Qty:  " << trd_qty_ << '\n';
    t_temp_oss_ << "Level:    " << level_ << '\n';
    t_temp_oss_ << "Size:     " << size_ << '\n';
    t_temp_oss_ << "Num_Ords: " << num_ords_ << '\n';
    t_temp_oss_ << "Price:    " << std::fixed << std::setprecision(6) << price_ << '\n';

    switch (type_) {
      case '1':
        t_temp_oss_ << "Type:   "
                    << "ASK" << '\n';
        break;
      case '2':
        t_temp_oss_ << "Type:   "
                    << "BID" << '\n';
        break;
      default:
        t_temp_oss_ << "Type:   "
                    << "---" << '\n';
        break;
    }

    switch (action_) {
      case '1':
        t_temp_oss_ << "Action: "
                    << "NEW" << '\n';
        break;
      case '2':
        t_temp_oss_ << "Action: "
                    << "CHANGE" << '\n';
        break;
      case '3':
        t_temp_oss_ << "Action: "
                    << "DELETE" << '\n';
        break;
      case '4':
        t_temp_oss_ << "Action: "
                    << "DEL_FROM" << '\n';
        break;
      case '5':
        t_temp_oss_ << "Action: "
                    << "DEL_THRU" << '\n';
        break;
      case '6':
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
  };
};

/// would include all message types eventually
enum msgType { EUREX_DELTA = 1, EUREX_TRADE };

/// common struct .. will support all messages eventually
struct EUREXCommonStruct {
  msgType msg_;
  timeval time_;
  union {
    EUREXTradeStruct eurex_trds_;
    EUREXMktDeltaStruct eurex_dels_;
  } data_;

  inline void print() { fprintf(stderr, "%s\n", ToString().c_str()); }
  inline double GetTradeDoublePrice() { return data_.eurex_trds_.trd_px_; }
  inline HFSAT::TradeType_t GetTradeAggressorSide() {
    return data_.eurex_trds_.agg_side_ == 'B'
               ? HFSAT::TradeType_t::kTradeTypeBuy
               : data_.eurex_trds_.agg_side_ == 'S' ? HFSAT::TradeType_t::kTradeTypeSell
                                                    : HFSAT::TradeType_t::kTradeTypeNoInfo;
  }
  inline uint32_t GetTradeSize() { return data_.eurex_trds_.trd_qty_; }
  inline bool isTradeMsg() { return (EUREX_TRADE == msg_); }
  inline void SetIntermediate(bool flag) {
    switch (msg_) {
      case EUREX_DELTA:
        data_.eurex_dels_.intermediate_ = flag;
        break;
      case EUREX_TRADE:
        break;
      default:
        break;
    }
  }
  const char* getContract() const {
    switch (msg_) {
      case EUREX_DELTA:
        return data_.eurex_dels_.contract_;
        break;
      case EUREX_TRADE:
        return data_.eurex_trds_.contract_;
        break;
      default:
        return NULL;
    }
  }

  bool isTradeMsg() const { return msg_ == EUREX_TRADE; }

  inline std::string ToString() {
    switch (msg_) {
      case EUREX_DELTA: {
        std::ostringstream t_temp_oss_;
        t_temp_oss_ << "\n============== EUREX BookDelta Message ================\n\n";
        t_temp_oss_ << "Time: " << time_.tv_sec << "." << std::setw(6) << std::setfill('0') << time_.tv_usec << "\n";

        return (t_temp_oss_.str() + data_.eurex_dels_.ToString() +
                "===================================================\n");
      } break;
      case EUREX_TRADE: {
        std::ostringstream t_temp_oss_;
        t_temp_oss_ << "\n============== EUREX Trade Message ================\n\n";
        t_temp_oss_ << "Time: " << time_.tv_sec << "." << std::setw(6) << std::setfill('0') << time_.tv_usec << "\n";

        return (t_temp_oss_.str() + data_.eurex_trds_.ToString() +
                "===================================================\n");
      } break;
      default: {
        std::ostringstream t_temp_oss_;
        t_temp_oss_ << "EUREX: NOT IMPLEMENTED FOR THIS EVENT : " << msg_ << '\n';

        return t_temp_oss_.str();
      } break;
    }
  }
};

//=====================================================EUREX LIVE DATA STRUCTS
//===================================================//

/// sent in response to trade message .. daemon only propagates forward.
/// trade messages affecting OB ..
struct EUREXLSTradeStruct {
  uint8_t contract_code_;
  uint32_t trd_qty_;  ///< quantity in this trade
  char agg_side_;  ///< aggressor .. can be neither B/S in case of complex trade (eg aggressive trade on future spread
  /// settling against indv leg)
  double trd_px_;  ///< trade price

  inline void print() { fprintf(stderr, "%s\n", ToString().c_str()); };

  inline std::string ToString() {
    std::ostringstream t_temp_oss_;
    t_temp_oss_ << "Contract Code:" << (int)contract_code_ << '\n';
    t_temp_oss_ << "Agg_Side:" << agg_side_ << '\n';
    t_temp_oss_ << "Trade_Size:" << trd_qty_ << '\n';
    t_temp_oss_ << "Trade_Px:" << trd_px_ << '\n';
    return t_temp_oss_.str();
  }
};

/// Market Delta messages are sent in response to snapshot and market delta events.
struct EUREXLSMktDeltaStruct {
  uint8_t contract_code_;
  uint8_t level_;      ///< level >= 1 (level 0 is currently filtered at daemon)
  uint32_t size_;      ///< size at level
  double price_;       ///< price at level
  char type_;          ///< type 1 - Ask 2 - Bid , other types filtered
  char action_;        ///< 1 - New, 2 - Change, 3 - Delete, 4 - Delete From , 5 - Delete through
  bool intermediate_;  ///< if this is an intermediate message .. clients should not react to intermediates
  uint32_t num_ords_;  ///<  EUREX 13.0 change, showing the number of price_size contributers ( orders ) at thislevel

  inline void print() { fprintf(stderr, "%s\n", ToString().c_str()); }

  inline std::string ToString() {
    std::ostringstream t_temp_oss_;
    t_temp_oss_ << "Contract Code: " << (int)contract_code_ << '\n';
    t_temp_oss_ << "Level:    " << (int)level_ << '\n';
    t_temp_oss_ << "Size:     " << size_ << '\n';
    t_temp_oss_ << "Num_Ords: " << num_ords_ << '\n';
    t_temp_oss_ << "Price:    " << std::fixed << std::setprecision(6) << price_ << '\n';

    switch (type_) {
      case '1':
        t_temp_oss_ << "Type:   "
                    << "ASK" << '\n';
        break;
      case '2':
        t_temp_oss_ << "Type:   "
                    << "BID" << '\n';
        break;
      default:
        t_temp_oss_ << "Type:   "
                    << "---" << '\n';
        break;
    }

    switch (action_) {
      case '1':
        t_temp_oss_ << "Action: "
                    << "NEW" << '\n';
        break;
      case '2':
        t_temp_oss_ << "Action: "
                    << "CHANGE" << '\n';
        break;
      case '3':
        t_temp_oss_ << "Action: "
                    << "DELETE" << '\n';
        break;
      case '4':
        t_temp_oss_ << "Action: "
                    << "DEL_FROM" << '\n';
        break;
      case '5':
        t_temp_oss_ << "Action: "
                    << "DEL_THRU" << '\n';
        break;
      case '6':
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

/// would include all message types eventually
/// common struct .. will support all messages eventually
struct EUREXLSCommonStruct {
  msgType msg_;

#if SEND_TIME_OVER_LIVE_SOURCE

  struct timeval time_;

#endif

  unsigned int msg_sequence_;  // This is not an exchange provided sequence, we use it to arbitrate for CHI via CRT
                               // against dedicated line

  union {
    EUREXLSTradeStruct eurex_trds_;
    EUREXLSMktDeltaStruct eurex_dels_;
  } data_;

  inline void print() { fprintf(stderr, "%s\n", ToString().c_str()); }
  inline bool isTradeMsg() { return (EUREX_TRADE == msg_); }
  inline double GetTradeDoublePrice() { return data_.eurex_trds_.trd_px_; }
  inline HFSAT::TradeType_t GetTradeAggressorSide() {
    return (data_.eurex_trds_.agg_side_ == 'B')
               ? HFSAT::TradeType_t::kTradeTypeBuy
               : (data_.eurex_trds_.agg_side_ == 'S') ? HFSAT::TradeType_t::kTradeTypeSell
                                                      : HFSAT::TradeType_t::kTradeTypeNoInfo;
  }
  inline uint32_t GetTradeSize() { return data_.eurex_trds_.trd_qty_; }
  inline void SetIntermediate(bool flag) {
    switch (msg_) {
      case EUREX_DELTA:
        data_.eurex_dels_.intermediate_ = flag;
        break;
      case EUREX_TRADE:
        break;
      default:
        break;
    }
  }
  inline const char* getContract() {
    return HFSAT::LsContractInfoMap<HFSAT::FILEPATH::EUREX_CONTRACT_CODE_FILE>::GetUniqueInstance()
        .GetChar16ExchSymbolFromProductCode(getContractCode());
  }
  uint8_t getContractCode() const {
    switch (msg_) {
      case EUREX_DELTA:
        return data_.eurex_dels_.contract_code_;
        break;
      case EUREX_TRADE:
        return data_.eurex_trds_.contract_code_;
        break;
      default:
        return 0;  // 0 is invalid contract code
    }
  }

  inline std::string ToString() {
    switch (msg_) {
      case EUREX_DELTA: {
        std::ostringstream t_temp_oss_;
        t_temp_oss_ << "\n============== EUREX BookDelta Message ================\n\n";

#if SEND_TIME_OVER_LIVE_SOURCE

        t_temp_oss_ << "Time: " << time_.tv_sec << "." << std::setw(6) << std::setfill('0') << time_.tv_usec << "\n";

#endif

        t_temp_oss_ << " MsgSequence : " << msg_sequence_ << "\n";

        return (t_temp_oss_.str() + data_.eurex_dels_.ToString() +
                "===================================================\n");
      } break;
      case EUREX_TRADE: {
        std::ostringstream t_temp_oss_;
        t_temp_oss_ << "\n============== EUREX Trade Message ================\n\n";

#if SEND_TIME_OVER_LIVE_SOURCE

        t_temp_oss_ << "Time: " << time_.tv_sec << "." << std::setw(6) << std::setfill('0') << time_.tv_usec << "\n";

#endif

        t_temp_oss_ << " MsgSequence : " << msg_sequence_ << "\n";

        return (t_temp_oss_.str() + data_.eurex_trds_.ToString() +
                "===================================================\n");
      } break;
      default: {
        std::ostringstream t_temp_oss_;
        t_temp_oss_ << "EUREXLS: NOT IMPLEMENTED FOR THIS EVENT : " << msg_ << '\n';

        return t_temp_oss_.str();
      } break;
    }
  }
};

//===================================================================================================================//
}

namespace FPGA_MDS {
#define BMF_FPGA_CONTRACT_TEXT_SIZE 32
#define BMF_FPGA_MAX_BOOK_SIZE 5
struct FPGATradeStruct {
  char contract_[EUREX_MDS_CONTRACT_TEXT_SIZE];  ///< internal contract name
  uint32_t trd_qty_;                             ///< quantity in this trade
  uint32_t tot_qty_;                             ///< total quantity .. to find if trade precedes quote or not
  uint32_t num_buy_ords_;                        ///< num buy orders involved in trade
  uint32_t num_sell_ords_;                       ///< num sell orders involved in trade
  char agg_side_;  ///< aggressor .. can be neither B/S in case of complex trade (eg aggressive trade on future spread
  /// settling against indv leg)
  char flags[3];   ///< reserved
  double trd_px_;  ///< trade price

  inline void print() { fprintf(stderr, "%s\n", ToString().c_str()); };

  inline std::string ToString() {
    std::ostringstream t_temp_oss_;
    t_temp_oss_ << "Contract:" << contract_ << '\n';
    t_temp_oss_ << "Agg_Side:" << (agg_side_ == 0 ? "BID" : (agg_side_ == 1 ? "ASK" : "NoInfo")) << "\n";
    t_temp_oss_ << "Trade_Size:" << trd_qty_ << '\n';
    t_temp_oss_ << "Tot_Qty:" << tot_qty_ << '\n';
    t_temp_oss_ << "Num_Buy_Sell:" << num_buy_ords_ << "\t" << num_sell_ords_ << '\n';
    t_temp_oss_ << "Trade_Px:" << trd_px_ << '\n';
    return t_temp_oss_.str();
  }
};

/// Market Delta messages are sent in response to snapshot and market delta events.
struct FPGABookDeltaStruct {
  char contract_[EUREX_MDS_CONTRACT_TEXT_SIZE];
  uint8_t num_levels_;
  uint8_t buysell_;
  char flags_[2];
  double prices_[5];
  int sizes_[5];
  int16_t num_orders_[5];

  inline void print() { fprintf(stderr, "%s\n", ToString().c_str()); }

  inline std::string ToString() {
    std::ostringstream t_temp_oss_;
    t_temp_oss_ << "Contract:    " << contract_ << "\n";
    t_temp_oss_ << "Side:        " << (buysell_ == 0 ? "BID" : (buysell_ == 1 ? "ASK" : "NoInfo")) << "\n";
    t_temp_oss_ << "Num_Levels:  " << (int)num_levels_ << "\n";

    for (auto i = 0u; i < 5; i++) {
      t_temp_oss_ << sizes_[i] << " ( " << num_orders_[i] << " ) "
                  << " @ " << prices_[i] << "\n";
    }

    return t_temp_oss_.str();
  }
};

/// would include all message types eventually
enum msgType { FPGA_DELTA = 1, FPGA_TRADE };

/// common struct .. will support all messages eventually
struct FPGACommonStruct {
  msgType msg_;
  timeval time_;
  union {
    FPGATradeStruct fpga_trds_;
    FPGABookDeltaStruct fpga_dels_;
  } data_;

  inline void print() { fprintf(stderr, "%s\n", ToString().c_str()); }

  char* getContract() {
    switch (msg_) {
      case FPGA_DELTA:
        return data_.fpga_dels_.contract_;
        break;
      case FPGA_TRADE:
        return data_.fpga_trds_.contract_;
        break;
      default:
        return NULL;
    }
  }

  bool isTradeMsg() const { return msg_ == FPGA_TRADE; }

  inline std::string ToString() {
    switch (msg_) {
      case FPGA_DELTA: {
        std::ostringstream t_temp_oss_;
        t_temp_oss_ << "\n============== FPGA BookDelta Message ================\n\n";
        t_temp_oss_ << "Time: " << time_.tv_sec << "." << std::setw(6) << std::setfill('0') << time_.tv_usec << "\n";

        return (t_temp_oss_.str() + data_.fpga_dels_.ToString() +
                "===================================================\n");
      } break;
      case FPGA_TRADE: {
        std::ostringstream t_temp_oss_;
        t_temp_oss_ << "\n============== FPGA Trade Message ================\n\n";
        t_temp_oss_ << "Time: " << time_.tv_sec << "." << std::setw(6) << std::setfill('0') << time_.tv_usec << "\n";

        return (t_temp_oss_.str() + data_.fpga_trds_.ToString() +
                "===================================================\n");
      } break;
      default: {
        std::ostringstream t_temp_oss_;
        t_temp_oss_ << "FPGA: NOT IMPLEMENTED FOR THIS EVENT : " << msg_ << '\n';

        return t_temp_oss_.str();
      } break;
    }

    return "";
  }
};

// BMF FPGA Defines and Structs
enum bmf_fpga_msg_type { BMF_FPGA_BOOK, BMF_FPGA_TRADE, BMF_FPGA_TRADING_STATUS, BMF_FPGA_FAST_TRADE };

struct BMFFPGABookEntry {
  double price;
  int size;
  int number_of_orders;
};

struct BMFFPGABookDeltaStruct {
  char contract_[BMF_FPGA_CONTRACT_TEXT_SIZE];
  BMFFPGABookEntry bids[6];
  BMFFPGABookEntry offers[6];
  int bid_size;
  int ask_size;

  inline void print() { fprintf(stderr, "%s\n", ToString().c_str()); }

  inline std::string ToString() {
    std::ostringstream t_temp_oss_;
    t_temp_oss_ << "Contract:    " << contract_ << "\n";

    t_temp_oss_ << "Orders \t Size \t Price \t\t Price \t Size \t Orders \n";
    for (int i = 0; i < BMF_FPGA_MAX_BOOK_SIZE; i++) {
      if (i < bid_size && i < ask_size) {
        t_temp_oss_ << bids[i].number_of_orders << "\t" << bids[i].size << "\t" << bids[i].price << "\tX\t"
                    << offers[i].price << "\t" << offers[i].size << "\t" << offers[i].number_of_orders << "\n";
      } else {
        if (i < bid_size) {
          t_temp_oss_ << bids[i].number_of_orders << "\t" << bids[i].size << "\t" << bids[i].price << "\n";
        }
        if (i < ask_size) {
          t_temp_oss_ << "     \t      \t       \t\t ";
          t_temp_oss_ << offers[i].price << "\t" << offers[i].size << "\t" << offers[i].number_of_orders << "\n";
        }
      }
    }

    return t_temp_oss_.str();
  }
};

struct BMFFPGATradeStruct {
  char contract_[BMF_FPGA_CONTRACT_TEXT_SIZE];
  int size;
  double price;
  uint64_t rpt_seq_num;
  inline std::string ToString() {
    std::ostringstream t_temp_oss_;
    t_temp_oss_ << "Contract: " << contract_ << "\n";
    t_temp_oss_ << "Size: " << size << "\n";
    t_temp_oss_ << "Price: " << price << "\n";
    t_temp_oss_ << "InstSeqNum: " << rpt_seq_num << "\n";
    return t_temp_oss_.str();
  }
};

struct BMFFPGATradingStatusStruct {
  char contract_[BMF_FPGA_CONTRACT_TEXT_SIZE];
  int status;
  inline std::string ToString() {
    std::ostringstream t_temp_oss_;
    t_temp_oss_ << "Contract: " << contract_ << "\n";
    t_temp_oss_ << "Status: " << status << "\n";
    return t_temp_oss_.str();
  }
};

struct BMFFPGAFastTradeStruct {
  char contract_[BMF_FPGA_CONTRACT_TEXT_SIZE];
  double price;
  int size;
  inline std::string ToString() {
    std::ostringstream t_temp_oss_;
    t_temp_oss_ << "Contract: " << contract_ << "\n";
    t_temp_oss_ << "TradePx: " << price << "\n";
    t_temp_oss_ << "TradeSz: " << size << "\n";
    return t_temp_oss_.str();
  }
};

struct BMFFPGACommonStruct {
  bmf_fpga_msg_type msg_;
  bool is_closed_;
  timeval time_;
  union {
    BMFFPGATradeStruct fpga_trds_;
    BMFFPGABookDeltaStruct fpga_dels_;
    BMFFPGATradingStatusStruct fpga_status_;
  } data_;

  inline void print() { fprintf(stderr, "%s\n", ToString().c_str()); }

  char* getContract() {
    switch (msg_) {
      case BMF_FPGA_BOOK:
        return data_.fpga_dels_.contract_;
        break;
      case BMF_FPGA_TRADE:
        return data_.fpga_trds_.contract_;
        break;
      case BMF_FPGA_TRADING_STATUS:
        return data_.fpga_status_.contract_;
        break;
      default:
        return NULL;
    }
  }

  bool isTradeMsg() const { return msg_ == BMF_FPGA_TRADE; }

  inline std::string ToString() {
    switch (msg_) {
      case BMF_FPGA_BOOK: {
        std::ostringstream t_temp_oss_;
        t_temp_oss_ << "\n============== BMF FPGA BookDelta Message ================\n\n";
        t_temp_oss_ << "Time: " << time_.tv_sec << "." << std::setw(6) << std::setfill('0') << time_.tv_usec << "\n";
        t_temp_oss_ << "TradingStatus: " << (is_closed_ ? "CLOSED" : "OPEN") << "\n";

        return (t_temp_oss_.str() + data_.fpga_dels_.ToString() +
                "===================================================\n");
      } break;
      case BMF_FPGA_TRADE: {
        std::ostringstream t_temp_oss_;
        t_temp_oss_ << "\n============== BMF FPGA Trade Message ================\n\n";
        t_temp_oss_ << "Time: " << time_.tv_sec << "." << std::setw(6) << std::setfill('0') << time_.tv_usec << "\n";
        t_temp_oss_ << "TradingStatus: " << (is_closed_ ? "CLOSED" : "OPEN") << "\n";

        return (t_temp_oss_.str() + data_.fpga_trds_.ToString() +
                "===================================================\n");
      } break;
      case BMF_FPGA_TRADING_STATUS: {
        std::ostringstream t_temp_oss_;
        t_temp_oss_ << "\n============== BMF FPGA Trading Status Message ================\n\n";
        t_temp_oss_ << "Time: " << time_.tv_sec << "." << std::setw(6) << std::setfill('0') << time_.tv_usec << "\n";

        return (t_temp_oss_.str() + data_.fpga_status_.ToString() +
                "===================================================\n");
      } break;
      default: {
        std::ostringstream t_temp_oss_;
        t_temp_oss_ << "BMF FPGA: NOT IMPLEMENTED FOR THIS EVENT : " << msg_ << '\n';

        return t_temp_oss_.str();
      } break;
    }

    return "";
  }
};
}

/// CME broadcast structs
namespace CME_MDS {
#define DEF_CME_LS_PRODUCTCODE_SHORTCODE_ \
  "/spare/local/files/CMEMDP/cme_mdp_livesource_contractcode_shortcode_mapping.txt"
struct CMETradeStruct {
  char contract_[CME_MDS_CONTRACT_TEXT_SIZE];  ///< contract name
  uint32_t trd_qty_;                           ///< quantity in this trade
  uint32_t tot_qty_;                           ///< total quantity
  uint32_t seqno_;                             ///< instrument based seqno .. to order trades and quotes
  double trd_px_;                              ///< trade price
  uint32_t agg_side_;                          ///< 1 - buy, 2-sell
  char flags_[4];                              ///< reserved

  inline void print() { fprintf(stderr, "%s\n", ToString().c_str()); }

  inline std::string ToString() {
    std::ostringstream t_temp_oss_;
    t_temp_oss_ << "Contract:   " << contract_ << '\n';
    t_temp_oss_ << "Agg_Side:   " << agg_side_ << '\n';
    t_temp_oss_ << "Trade_Size: " << trd_qty_ << '\n';
    t_temp_oss_ << "Tot_Qty:    " << tot_qty_ << '\n';
    t_temp_oss_ << "Trade_Px:   " << trd_px_ << '\n';
    t_temp_oss_ << "InstSeqNum: " << seqno_ << '\n';
    return t_temp_oss_.str();
  }
};

struct CMEMktDeltaStruct {
  char contract_[CME_MDS_CONTRACT_TEXT_SIZE];  ///< internal contract name
  uint32_t level_;                             ///< level >= 1
  int32_t size_;                               ///< size at level
  uint32_t num_ords_;                          ///< number of orders at that level
  double price_;                               ///< price at level
  uint32_t seqno_;                             ///< ???
  char type_;                                  ///< type 0 - Bid , 1 - Offer
  char status_;                                ///< 'A' is normal operation. Not filtered right now
  char action_;        ///< 0 - New, 1 - Change, 2 - Delete, 3 - Delete Thru , 4 - Delete From, 9 - Synthetic Delete
  bool intermediate_;  ///< if this is an intermediate message .. clients should not react to intermediates

  inline void print() { fprintf(stderr, "%s\n", ToString().c_str()); }

  inline std::string ToString() {
    std::ostringstream t_temp_oss_;
    t_temp_oss_ << "Contract:   " << contract_ << '\n';
    t_temp_oss_ << "Level:      " << level_ << '\n';
    t_temp_oss_ << "Size:       " << size_ << '\n';
    t_temp_oss_ << "Num_Ords:   " << num_ords_ << '\n';
    t_temp_oss_ << "Price:      " << std::fixed << std::setprecision(6) << price_ << '\n';
    t_temp_oss_ << "InstSeqNum: " << seqno_ << '\n';

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
        t_temp_oss_ << "Type:   " << type_ << '\n';
        break;
    }

    t_temp_oss_ << "Status: " << status_ << '\n';

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
                    << "SYNTHETIC_DEL" << '\n';
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

/// would include all message types eventually
enum msgType { CME_DELTA = 3, CME_TRADE, CME_EXEC = 5, CME_RESET_BEGIN = 0, CME_RESET_END = 1 };

/// common struct .. will support all messages eventually
struct CMECommonStruct {
  msgType msg_;
  timeval time_;
  union {
    CMETradeStruct cme_trds_;
    CMEMktDeltaStruct cme_dels_;
  } data_;

  inline void print() { fprintf(stderr, "%s\n", ToString().c_str()); }
  inline bool isTradeMsg() { return (CME_TRADE == msg_); }
  inline double GetTradeDoublePrice() { return data_.cme_trds_.trd_px_; }
  inline HFSAT::TradeType_t GetTradeAggressorSide() {
    return (data_.cme_trds_.agg_side_ == 1) ? HFSAT::TradeType_t::kTradeTypeBuy
                                            : (data_.cme_trds_.agg_side_ == 2) ? HFSAT::TradeType_t::kTradeTypeSell
                                                                               : HFSAT::TradeType_t::kTradeTypeNoInfo;
  }
  inline uint32_t GetTradeSize() { return data_.cme_trds_.trd_qty_; }
  inline void SetIntermediate(bool flag) {
    switch (msg_) {
      case CME_DELTA:
        data_.cme_dels_.intermediate_ = flag;
        break;
      case CME_TRADE:
        break;
      default:
        break;
    }
  }
  const char* getContract() const {
    switch (msg_) {
      case CME_DELTA:
        return data_.cme_dels_.contract_;
        break;
      case CME_TRADE:
        return data_.cme_trds_.contract_;
        break;
      default:
        return NULL;
    }
  }

  bool isTradeMsg() const { return msg_ == CME_TRADE; }

  inline std::string ToString() {
    switch (msg_) {
      case CME_DELTA: {
        std::ostringstream t_temp_oss_;
        t_temp_oss_ << "\n============== CME BookDelta Message ================\n\n";
        t_temp_oss_ << "Time: " << time_.tv_sec << "." << std::setw(6) << std::setfill('0') << time_.tv_usec << "\n";

        return (t_temp_oss_.str() + data_.cme_dels_.ToString() +
                "===================================================\n");
      } break;
      case CME_TRADE: {
        std::ostringstream t_temp_oss_;
        t_temp_oss_ << "\n============== CME Trade Message ================\n\n";
        t_temp_oss_ << "Time: " << time_.tv_sec << "." << std::setw(6) << std::setfill('0') << time_.tv_usec << "\n";

        return (t_temp_oss_.str() + data_.cme_trds_.ToString() +
                "===================================================\n");
      } break;
      default: {
        std::ostringstream t_temp_oss_;
        t_temp_oss_ << "CME: NOT IMPLEMENTED FOR THIS EVENT : " << msg_ << '\n';

        return t_temp_oss_.str();
      } break;
    }
  }
};

// FPGA structs

struct CMEFPGAHalfBookDelta {
  double prices_[5];
  int sizes_[5];
  int16_t num_orders_[5];
  char type_;
};

struct CMEFPGAHalfBookTrade {
  double price_;
  int size_;
  char type_;

  inline std::string ToString() {
    std::ostringstream t_temp_oss_;

    t_temp_oss_ << "Agg_Side:   " << type_ << '\n';
    t_temp_oss_ << "Tot_Qty:    " << size_ << '\n';
    t_temp_oss_ << "Trade_Px:   " << std::fixed << std::setprecision(6) << price_ << '\n';
    return t_temp_oss_.str();
  }
};

struct CMEFPGACommonStruct {
  msgType msg_;
  timeval time_;
  char contract_[CME_MDS_CONTRACT_TEXT_SIZE];  ///< contract name
  union {
    CMEFPGAHalfBookDelta cme_dels_[2];
    CMEFPGAHalfBookTrade cme_trds_;
  } data_;

  const char* getContract() const { return contract_; }

  inline std::string PrintBook() {
    std::ostringstream t_temp_oss_;
    for (auto lvl = 0; lvl < 5; lvl++) {
      t_temp_oss_ << data_.cme_dels_[0].prices_[lvl] << "\t" << data_.cme_dels_[0].sizes_[lvl] << "\t"
                  << data_.cme_dels_[0].num_orders_[lvl] << "\t\t" << data_.cme_dels_[1].prices_[lvl] << "\t"
                  << data_.cme_dels_[1].sizes_[lvl] << "\t" << data_.cme_dels_[1].num_orders_[lvl] << "\n ";
    }
    return t_temp_oss_.str();
  }

  inline std::string ToString() {
    switch (msg_) {
      case CME_DELTA: {
        std::ostringstream t_temp_oss_;
        t_temp_oss_ << "\n============== CME FPGA BookDelta Message ================\n\n";
        t_temp_oss_ << "Time: " << time_.tv_sec << "." << std::setw(6) << std::setfill('0') << time_.tv_usec << "\n";
        t_temp_oss_ << "Contract: " << contract_ << "\n";

        return (t_temp_oss_.str() + PrintBook() + "===================================================\n");
      } break;
      case CME_TRADE: {
        std::ostringstream t_temp_oss_;
        t_temp_oss_ << "\n============== CME FPGA Trade Message ================\n\n";
        t_temp_oss_ << "Time: " << time_.tv_sec << "." << std::setw(6) << std::setfill('0') << time_.tv_usec << "\n";
        t_temp_oss_ << "Contract: " << contract_ << "\n";
        return (t_temp_oss_.str() + data_.cme_trds_.ToString() +
                "===================================================\n");
      } break;
      default: {
        std::ostringstream t_temp_oss_;
        t_temp_oss_ << "CME: NOT IMPLEMENTED FOR THIS EVENT : " << msg_ << '\n';

        return t_temp_oss_.str();
      } break;
    }
  }
};

//=====================================================CME LIVE DATA STRUCTS
//===================================================//

struct CMELSTradeStruct {
  double trd_px_;     ///< trade price
  uint32_t trd_qty_;  ///< quantity in this trade
  uint8_t contract_code_;
  uint8_t agg_side_;  ///< 1 - buy, 2-sell

  inline void print() { fprintf(stderr, "%s\n", ToString().c_str()); }

  inline std::string ToString() {
    std::ostringstream t_temp_oss_;
    t_temp_oss_ << "Contract Code:   " << (int)contract_code_ << '\n';

    if (agg_side_)
      t_temp_oss_ << "Agg_Side:   0" << '\n';
    else
      t_temp_oss_ << "Agg_Side:   1" << '\n';

    t_temp_oss_ << "Trade_Size: " << trd_qty_ << '\n';
    t_temp_oss_ << "Trade_Px:   " << trd_px_ << '\n';

    return t_temp_oss_.str();
  }
};

struct CMELSMktDeltaStruct {
  double price_;       ///< price at level
  int32_t size_;       ///< size at level
  uint32_t num_ords_;  ///< number of orders at that level
  uint8_t contract_code_;
  uint8_t level_;      ///< level >= 1
  char type_;          ///< type 0 - Bid , 1 - Offer
  char action_;        ///< 0 - New, 1 - Change, 2 - Delete, 3 - Delete Thru , 4 - Delete From, 9 - Synthetic Delete
  bool intermediate_;  ///< if this is an intermediate message .. clients should not react to intermediates

  inline void print() { fprintf(stderr, "%s\n", ToString().c_str()); }

  inline std::string ToString() {
    std::ostringstream t_temp_oss_;
    t_temp_oss_ << "Contract Code :   " << (int)contract_code_ << '\n';
    t_temp_oss_ << "Level:      " << (int)level_ << '\n';
    t_temp_oss_ << "Size:       " << size_ << '\n';
    t_temp_oss_ << "Num_Ords:   " << num_ords_ << '\n';
    t_temp_oss_ << "Price:      " << std::fixed << std::setprecision(6) << price_ << '\n';

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
                    << "SYNTHETIC_DEL" << '\n';
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

/// common struct .. will support all messages eventually
struct CMELSCommonStruct {
  msgType msg_;

#if SEND_TIME_OVER_LIVE_SOURCE

  struct timeval time_;

#endif

  union {
    CMELSTradeStruct cme_trds_;
    CMELSMktDeltaStruct cme_dels_;
  } data_;

  inline void print() { fprintf(stderr, "%s\n", ToString().c_str()); }
  inline double GetTradeDoublePrice() { return data_.cme_trds_.trd_px_; }
  inline HFSAT::TradeType_t GetTradeAggressorSide() {
    return (data_.cme_trds_.agg_side_ == 1) ? HFSAT::TradeType_t::kTradeTypeBuy
                                            : (data_.cme_trds_.agg_side_ == 2) ? HFSAT::TradeType_t::kTradeTypeSell
                                                                               : HFSAT::TradeType_t::kTradeTypeNoInfo;
  }
  inline uint32_t GetTradeSize() { return data_.cme_trds_.trd_qty_; }
  uint8_t getContractCode() const {
    switch (msg_) {
      case CME_DELTA:
        return data_.cme_dels_.contract_code_;
        break;
      case CME_TRADE:
        return data_.cme_trds_.contract_code_;
        break;
      default:
        return 0;
    }
  }
  inline const char* getContract() {
    return HFSAT::LsContractInfoMap<HFSAT::FILEPATH::CME_CONTRACT_CODE_FILE>::GetUniqueInstance()
        .GetChar16ExchSymbolFromProductCode(getContractCode());
  }
  inline bool isTradeMsg() { return (CME_TRADE == msg_); }
  inline void SetIntermediate(bool flag) {
    switch (msg_) {
      case CME_DELTA:
        data_.cme_dels_.intermediate_ = flag;
        break;
      case CME_TRADE:
        break;
      default:
        break;
    }
  }
  inline std::string ToString() {
    switch (msg_) {
      case CME_DELTA: {
        std::ostringstream t_temp_oss_;
        t_temp_oss_ << "\n============== CME BookDelta Message ================\n\n";

#if SEND_TIME_OVER_LIVE_SOURCE

        t_temp_oss_ << "Time: " << time_.tv_sec << "." << std::setw(6) << std::setfill('0') << time_.tv_usec << "\n";

#endif
        return (t_temp_oss_.str() + data_.cme_dels_.ToString() +
                "===================================================\n");
      } break;
      case CME_TRADE: {
        std::ostringstream t_temp_oss_;
        t_temp_oss_ << "\n============== CME Trade Message ================\n\n";

#if SEND_TIME_OVER_LIVE_SOURCE

        t_temp_oss_ << "Time: " << time_.tv_sec << "." << std::setw(6) << std::setfill('0') << time_.tv_usec << "\n";

#endif
        return (t_temp_oss_.str() + data_.cme_trds_.ToString() +
                "===================================================\n");
      } break;
      default: {
        std::ostringstream t_temp_oss_;
        t_temp_oss_ << "CMELS: NOT IMPLEMENTED FOR THIS EVENT : " << msg_ << '\n';

        return t_temp_oss_.str();
      } break;
    }
  }
};

struct CMEOBFDeltaStruct {
  char contract[CME_MDS_CONTRACT_TEXT_SIZE];  ///< internal contract name
  uint64_t order_id;
  uint64_t order_priority;
  double price;
  int32_t order_qty;
  uint32_t seqno_;  /// packet based seqno
  char action;
  char type;

  inline void print() { fprintf(stderr, "%s\n", ToString().c_str()); }

  inline std::string ToString() {
    std::ostringstream t_temp_oss;
    t_temp_oss << "Contract      : " << contract << '\n';
    t_temp_oss << "OrderId       : " << order_id << '\n';
    t_temp_oss << "OrderPriority : " << order_priority << '\n';
    t_temp_oss << "Price         : " << std::fixed << std::setprecision(6) << price << '\n';
    t_temp_oss << "OrderQty      : " << order_qty << '\n';
    t_temp_oss << "SeqNo         : " << seqno_ << '\n';

    switch (type) {
      case '1':
        t_temp_oss << "Type           : "
                   << "ASK" << '\n';
        break;
      case '0':
        t_temp_oss << "Type           : "
                   << "BID" << '\n';
        break;
      default:
        t_temp_oss << "Type           : " << type << '\n';
        break;
    }

    switch (action) {
      case '0':
        t_temp_oss << "Action         : "
                   << "NEW" << '\n';
        break;
      case '1':
        t_temp_oss << "Action         : "
                   << "CHANGE" << '\n';
        break;
      case '2':
        t_temp_oss << "Action         : "
                   << "DELETE" << '\n';
        break;
      case '3':
        t_temp_oss << "Action         : "
                   << "DEL_FROM" << '\n';
        break;
      case '4':
        t_temp_oss << "Action         : "
                   << "DEL_THRU" << '\n';
        break;
      default:
        t_temp_oss << "Action         : "
                   << "--------" << '\n';
        break;
    }

    return t_temp_oss.str();
  }
};

struct CMEOBFExecStruct {
  char contract[CME_MDS_CONTRACT_TEXT_SIZE];  ///< internal contract name
  uint64_t order_id;
  int32_t order_qty;
  uint32_t seqno_;  /// packet based seqno

  inline void print() { fprintf(stderr, "%s\n", ToString().c_str()); }

  inline std::string ToString() {
    std::ostringstream t_temp_oss;
    t_temp_oss << "Contract      : " << contract << '\n';
    t_temp_oss << "OrderId       : " << order_id << '\n';
    t_temp_oss << "OrderQty      : " << order_qty << '\n';
    t_temp_oss << "SeqNo         : " << seqno_ << '\n';

    return t_temp_oss.str();
  }
};

struct CMEOBFResetStruct {
  char contract[CME_MDS_CONTRACT_TEXT_SIZE];  ///< internal contract name

  inline void print() { fprintf(stderr, "%s\n", ToString().c_str()); }

  inline std::string ToString() {
    std::ostringstream t_temp_oss;
    t_temp_oss << "Contract      : " << contract << '\n';
    return t_temp_oss.str();
  }
};

struct CMEOBFCommonStruct {
  msgType msg_;
  timeval time_;
  union {
    CMEOBFDeltaStruct cme_dels_;
    CMETradeStruct cme_trds_;
    CMEOBFExecStruct cme_excs_;
    CMEOBFResetStruct cme_reset_;
  } data_;

  inline void print() { fprintf(stderr, "%s\n", ToString().c_str()); }

  const char* getContract() const {
    switch (msg_) {
      case CME_DELTA:
        return data_.cme_dels_.contract;
        break;
      case CME_EXEC:
        return data_.cme_excs_.contract;
        break;
      case CME_TRADE:
        return data_.cme_trds_.contract_;
      case CME_RESET_BEGIN:
        return data_.cme_reset_.contract;
      case CME_RESET_END:
        return data_.cme_reset_.contract;
      default:
        return NULL;
    }
  }

  bool isTradeMsg() const { return msg_ == CME_TRADE; }

  inline std::string ToString() {
    switch (msg_) {
      case CME_DELTA: {
        std::ostringstream t_temp_oss_;
        t_temp_oss_ << "\n============== CME OBF BookDelta Message ================\n\n";
        t_temp_oss_ << "Time: " << time_.tv_sec << "." << std::setw(6) << std::setfill('0') << time_.tv_usec << "\n";

        return (t_temp_oss_.str() + data_.cme_dels_.ToString() +
                "===================================================\n");
      } break;
      case CME_TRADE: {
        std::ostringstream t_temp_oss_;
        t_temp_oss_ << "\n============== CME Trade Message ================\n\n";
        t_temp_oss_ << "Time: " << time_.tv_sec << "." << std::setw(6) << std::setfill('0') << time_.tv_usec << "\n";

        return (t_temp_oss_.str() + data_.cme_trds_.ToString() +
                "===================================================\n");
      } break;
      case CME_EXEC: {
        std::ostringstream t_temp_oss_;
        t_temp_oss_ << "\n============== CME Exec Message ================\n\n";
        t_temp_oss_ << "Time: " << time_.tv_sec << "." << std::setw(6) << std::setfill('0') << time_.tv_usec << "\n";

        return (t_temp_oss_.str() + data_.cme_excs_.ToString() +
                "===================================================\n");
      } break;
      case CME_RESET_BEGIN: {
        std::ostringstream t_temp_oss_;
        t_temp_oss_ << "\n============== CME Reset BEGIN Message ================\n\n";
        t_temp_oss_ << "Time: " << time_.tv_sec << "." << std::setw(6) << std::setfill('0') << time_.tv_usec << "\n";
        t_temp_oss_ << "ACTION      : "
                    << "BEGIN" << '\n';
        return (t_temp_oss_.str() + data_.cme_reset_.ToString() +
                "===================================================\n");
      } break;
      case CME_RESET_END: {
        std::ostringstream t_temp_oss_;
        t_temp_oss_ << "\n============== CME Reset END Message ================\n\n";
        t_temp_oss_ << "Time: " << time_.tv_sec << "." << std::setw(6) << std::setfill('0') << time_.tv_usec << "\n";
        t_temp_oss_ << "ACTION      : "
                    << "END" << '\n';
        return (t_temp_oss_.str() + data_.cme_reset_.ToString() +
                "===================================================\n");
      } break;
      default: {
        std::ostringstream t_temp_oss_;
        t_temp_oss_ << "CME: NOT IMPLEMENTED FOR THIS EVENT : " << msg_ << '\n';

        return t_temp_oss_.str();
      } break;
    }
  }
};

//===================================================================================================================//
}

/// RTS market data messages
namespace RTS_MDS {
#define RTS_MDS_CONTRACT_TEXT_SIZE 12

struct RTSTradeStruct {
  char contract_[RTS_MDS_CONTRACT_TEXT_SIZE];  ///< contract name
  uint32_t trd_qty_;                           ///< quantity in this trade
  uint32_t tot_qty_;                           ///< total quantity
  uint32_t seqno_;                             ///< instrument based seqno .. to order trades and quotes
  double trd_px_;                              ///< trade price
  uint32_t agg_side_;                          ///< 1 - buy, 2-sell
  char flags_[4];                              ///< reserved

  inline void print() { fprintf(stderr, "%s\n", ToString().c_str()); }

  inline std::string ToString() {
    std::ostringstream t_temp_oss_;
    t_temp_oss_ << "Contract:   " << contract_ << '\n';
    t_temp_oss_ << "Agg_Side:   " << agg_side_ << '\n';
    t_temp_oss_ << "Trade_Size: " << trd_qty_ << '\n';
    t_temp_oss_ << "Tot_Qty:    " << tot_qty_ << '\n';
    t_temp_oss_ << "Trade_Px:   " << trd_px_ << '\n';
    t_temp_oss_ << "InstSeqNum: " << seqno_ << '\n';
    return t_temp_oss_.str();
  }
};

struct RTSMktDeltaStruct {
  char contract_[RTS_MDS_CONTRACT_TEXT_SIZE];  ///< internal contract name
  uint32_t level_;                             ///< level >= 1
  int32_t size_;                               ///< size at level
  uint32_t num_ords_;                          ///< number of orders at that level
  double price_;                               ///< price at level
  uint32_t seqno_;                             ///< instrument based seqno .. to order trades and quotes
  char type_;                                  ///< type 0 - Bid , 1 - Offer
  char status_;                                ///< 'A' is normal operation. Not filtered right now
  char action_;                                ///< 0 - New, 1 - Change, 2 - Delete, 3 - Delete Thru , 4 - Delete From
  bool intermediate_;  ///< if this is an intermediate message .. clients should not react to intermediates

  inline void print() { fprintf(stderr, "%s\n", ToString().c_str()); }

  inline std::string ToString() {
    std::ostringstream t_temp_oss_;
    t_temp_oss_ << "Contract:   " << contract_ << '\n';
    t_temp_oss_ << "Level:      " << level_ << '\n';
    t_temp_oss_ << "Size:       " << size_ << '\n';
    t_temp_oss_ << "Num_Ords:   " << num_ords_ << '\n';
    t_temp_oss_ << "Price:      " << std::fixed << std::setprecision(6) << price_ << '\n';
    t_temp_oss_ << "InstSeqNum: " << seqno_ << '\n';

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

    t_temp_oss_ << "Status: " << status_ << '\n';

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

    t_temp_oss_ << "Intermediate: " << (intermediate_ ? "Y" : "N") << '\n';

    return t_temp_oss_.str();
  }
};

/// would include all message types eventually
enum msgType { RTS_DELTA = 3, RTS_TRADE };

/// common struct .. will support all messages eventually
struct RTSCommonStruct {
  msgType msg_;
  timeval time_;
  union {
    RTSTradeStruct rts_trds_;
    RTSMktDeltaStruct rts_dels_;
  } data_;

  inline void print() { fprintf(stderr, "%s\n", ToString().c_str()); }
  inline double GetTradeDoublePrice() { return data_.rts_trds_.trd_px_; }
  inline HFSAT::TradeType_t GetTradeAggressorSide() {
    return (data_.rts_trds_.agg_side_ == 1) ? HFSAT::TradeType_t::kTradeTypeBuy
                                            : (data_.rts_trds_.agg_side_ == 2) ? HFSAT::TradeType_t::kTradeTypeSell
                                                                               : HFSAT::TradeType_t::kTradeTypeNoInfo;
  }
  inline uint32_t GetTradeSize() { return data_.rts_trds_.trd_qty_; }
  inline bool isTradeMsg() { return (RTS_TRADE == msg_); }
  inline void SetIntermediate(bool flag) {
    switch (msg_) {
      case RTS_DELTA:
        data_.rts_dels_.intermediate_ = flag;
        break;
      case RTS_TRADE:
        break;
      default:
        break;
    }
  }
  const char* getContract() const {
    switch (msg_) {
      case RTS_DELTA:
        return data_.rts_dels_.contract_;
        break;
      case RTS_TRADE:
        return data_.rts_trds_.contract_;
        break;
      default:
        return NULL;
    }
  }
  bool isTradeMsg() const { return msg_ == RTS_TRADE; }

  inline std::string ToString() {
    switch (msg_) {
      case RTS_DELTA: {
        std::ostringstream t_temp_oss_;
        t_temp_oss_ << "\n============== RTS BookDelta Message ================\n\n";
        t_temp_oss_ << "Time: " << time_.tv_sec << "." << std::setw(6) << std::setfill('0') << time_.tv_usec << "\n";

        return (t_temp_oss_.str() + data_.rts_dels_.ToString() +
                "===================================================\n");
      } break;
      case RTS_TRADE: {
        std::ostringstream t_temp_oss_;
        t_temp_oss_ << "\n============== RTS Trade Message ================\n\n";
        t_temp_oss_ << "Time: " << time_.tv_sec << "." << std::setw(6) << std::setfill('0') << time_.tv_usec << "\n";

        return (t_temp_oss_.str() + data_.rts_trds_.ToString() +
                "===================================================\n");
      } break;
      default: {
        std::ostringstream t_temp_oss_;
        t_temp_oss_ << "RTS: NOT IMPLEMENTED FOR THIS EVENT : " << msg_ << '\n';

        return t_temp_oss_.str();
      } break;
    }
  }
};

struct RTSNewCommonStruct {
  msgType msg_;
  timeval time_;
  int event_time_;
  union {
    RTSTradeStruct rts_trds_;
    RTSMktDeltaStruct rts_dels_;
  } data_;

  inline void print() { fprintf(stderr, "%s\n", ToString().c_str()); }

  char* getContract() {
    switch (msg_) {
      case RTS_DELTA:
        return data_.rts_dels_.contract_;
        break;
      case RTS_TRADE:
        return data_.rts_trds_.contract_;
        break;
      default:
        return NULL;
    }
  }
  bool isTradeMsg() const { return msg_ == RTS_TRADE; }

  inline std::string ToString() {
    switch (msg_) {
      case RTS_DELTA: {
        std::ostringstream t_temp_oss_;
        t_temp_oss_ << "\n============== RTS BookDelta Message ================\n\n";
        t_temp_oss_ << "Time: " << time_.tv_sec << "." << std::setw(6) << std::setfill('0') << time_.tv_usec << "\n";
        t_temp_oss_ << "EventTS: " << event_time_ << "\n";

        return (t_temp_oss_.str() + data_.rts_dels_.ToString() +
                "===================================================\n");
      } break;
      case RTS_TRADE: {
        std::ostringstream t_temp_oss_;
        t_temp_oss_ << "\n============== RTS Trade Message ================\n\n";
        t_temp_oss_ << "Time: " << time_.tv_sec << "." << std::setw(6) << std::setfill('0') << time_.tv_usec << "\n";
        t_temp_oss_ << "EventTS: " << event_time_ << "\n";

        return (t_temp_oss_.str() + data_.rts_trds_.ToString() +
                "===================================================\n");
      } break;
      default: {
        std::ostringstream t_temp_oss_;
        t_temp_oss_ << "RTSNew: NOT IMPLEMENTED FOR THIS EVENT : " << msg_ << '\n';

        return t_temp_oss_.str();
      } break;
    }
  }
};
}

namespace TMX_MDS {
#define TMX_MDS_CONTRACT_TEXT_SIZE 12
enum msgType { TMX_QUOTE = 7, TMX_TRADE, TMX_BOOK };

struct TMXTradeStruct_v0 {
  char contract_[12];
  int trd_qty_;
  double trd_px_;
  uint32_t seqno_;
  char type_;
  char flags_[11];

  inline void print() { fprintf(stderr, "%s\n", ToString().c_str()); }

  inline std::string ToString() {
    std::ostringstream t_temp_oss_;
    t_temp_oss_ << "Contract:   " << contract_ << "\n";
    t_temp_oss_ << "Trade_Qty:  " << trd_qty_ << "\n";
    t_temp_oss_ << "Trade_Px:   " << trd_px_ << "\n";
    t_temp_oss_ << "Seqno:      " << seqno_ << "\n";
    t_temp_oss_ << "Type:       " << type_ << "\n";
    return t_temp_oss_.str();
  }
};

struct TMXQuoteStruct_v0 {
  char contract_[12];
  double bid_px_;
  int bid_sz_;
  double ask_px_;
  int ask_sz_;
  char status_;
  char flags_[3];

  inline void print() { fprintf(stderr, "%s\n", ToString().c_str()); }

  inline std::string ToString() {
    std::ostringstream t_temp_oss_;
    t_temp_oss_ << "Contract:    " << contract_ << "\n";
    t_temp_oss_ << bid_sz_ << " @ " << bid_px_ << " ----- " << ask_px_ << " @ " << ask_sz_ << "\n";
    t_temp_oss_ << "Status:      " << status_ << "\n";
    return t_temp_oss_.str();
  }
};

struct TMXBookStruct_v0 {
  char contract_[12];
  double bid_pxs_[5];
  int bid_szs_[5];
  int num_bid_ords_[5];
  double ask_pxs_[5];
  int ask_szs_[5];
  int num_ask_ords_[5];
  char status_;
  int num_levels_;
  char flags_[7];

  inline void print() { fprintf(stderr, "%s\n", ToString().c_str()); }

  inline std::string ToString() {
    std::ostringstream t_temp_oss_;
    t_temp_oss_ << "Contract:    " << contract_ << "\n";
    t_temp_oss_ << "Status:      " << status_ << "\n";
    t_temp_oss_ << "Num_Levels:  " << num_levels_ << "\n";
    for (auto i = 0u; i < 5; i++)
      t_temp_oss_ << "( " << num_bid_ords_[i] << " ) " << bid_szs_[i] << " @ " << bid_pxs_[i] << " ---- " << ask_pxs_[i]
                  << " @ " << ask_szs_[i] << " ( " << num_ask_ords_[i] << " )\n";
    return t_temp_oss_.str();
  }
};

struct TMXCommonStruct_v0 {
  msgType msg_;
  timeval time_;
  union {
    TMXTradeStruct_v0 tmx_trds_;
    TMXQuoteStruct_v0 tmx_qts_;
    TMXBookStruct_v0 tmx_books_;
  } data_;

  inline void print() { fprintf(stderr, "%s\n", ToString().c_str()); }

  const char* getContract() const {
    switch (msg_) {
      case TMX_QUOTE:
        return data_.tmx_qts_.contract_;
        break;
      case TMX_TRADE:
        return data_.tmx_trds_.contract_;
        break;
      case TMX_BOOK:
        return data_.tmx_books_.contract_;
        break;
      default:
        return NULL;
    }
  }
  bool isTradeMsg() const { return msg_ == TMX_TRADE; }

  inline std::string ToString() {
    std::ostringstream t_temp_oss_;
    switch (msg_) {
      case TMX_QUOTE: {
        t_temp_oss_ << "\n========================TMX_QUOTE Message ====================\n\n";
        t_temp_oss_ << "Time: " << time_.tv_sec << "." << std::setw(6) << std::setfill('0') << time_.tv_usec << "\n";
        return (t_temp_oss_.str() + data_.tmx_qts_.ToString() +
                "=============================================================\n");
      } break;
      case TMX_TRADE: {
        t_temp_oss_ << "\n========================TMX_TRADE Message ====================\n\n";
        t_temp_oss_ << "Time: " << time_.tv_sec << "." << std::setw(6) << std::setfill('0') << time_.tv_usec << "\n";
        return (t_temp_oss_.str() + data_.tmx_trds_.ToString() +
                "=============================================================\n");
      } break;
      case TMX_BOOK: {
        t_temp_oss_ << "\n========================TMX_BOOK Message ====================\n\n";
        t_temp_oss_ << "Time: " << time_.tv_sec << "." << std::setw(6) << std::setfill('0') << time_.tv_usec << "\n";
        return (t_temp_oss_.str() + data_.tmx_books_.ToString() +
                "=============================================================\n");
      } break;
      default: {
        std::ostringstream t_temp_oss_;
        t_temp_oss_ << "TMX v0: NOT IMPLEMENTED FOR THIS EVENT : " << msg_ << '\n';

        return t_temp_oss_.str();
      } break;
    }
    return t_temp_oss_.str();
  }
};

struct TMXTradeStruct {
  char contract_[30];
  int trd_qty_;
  double trd_px_;
  uint32_t seqno_;
  char type_;
  char flags_[11];

  inline void print() { fprintf(stderr, "%s\n", ToString().c_str()); }

  inline std::string ToString() {
    std::ostringstream t_temp_oss_;
    t_temp_oss_ << "Contract:   " << contract_ << "\n";
    t_temp_oss_ << "Trade_Qty:  " << trd_qty_ << "\n";
    t_temp_oss_ << "Trade_Px:   " << trd_px_ << "\n";
    t_temp_oss_ << "Seqno:      " << seqno_ << "\n";
    t_temp_oss_ << "Type:       " << type_ << "\n";
    return t_temp_oss_.str();
  }
};

struct TMXQuoteStruct {
  char contract_[30];
  double bid_px_;
  int bid_sz_;
  double ask_px_;
  int ask_sz_;
  char status_;
  char flags_[3];

  inline void print() { fprintf(stderr, "%s\n", ToString().c_str()); }

  inline std::string ToString() {
    std::ostringstream t_temp_oss_;
    t_temp_oss_ << "Contract:    " << contract_ << "\n";
    t_temp_oss_ << bid_sz_ << " @ " << bid_px_ << " ----- " << ask_px_ << " @ " << ask_sz_ << "\n";
    t_temp_oss_ << "Status:      " << status_ << "\n";
    return t_temp_oss_.str();
  }
};

struct TMXBookStruct {
  char contract_[30];
  double bid_pxs_[5];
  int bid_szs_[5];
  int num_bid_ords_[5];
  double ask_pxs_[5];
  int ask_szs_[5];
  int num_ask_ords_[5];
  char status_;
  int num_levels_;
  char flags_[7];

  inline void print() { fprintf(stderr, "%s\n", ToString().c_str()); }

  inline std::string ToString() {
    std::ostringstream t_temp_oss_;
    t_temp_oss_ << "Contract:    " << contract_ << "\n";
    t_temp_oss_ << "Status:      " << status_ << "\n";
    t_temp_oss_ << "Num_Levels:  " << num_levels_ << "\n";
    for (auto i = 0u; i < 5; i++)
      t_temp_oss_ << "( " << num_bid_ords_[i] << " ) " << bid_szs_[i] << " @ " << bid_pxs_[i] << " ---- " << ask_pxs_[i]
                  << " @ " << ask_szs_[i] << " ( " << num_ask_ords_[i] << " )\n";
    return t_temp_oss_.str();
  }
};

struct TMXCommonStruct {
  msgType msg_;
  timeval time_;
  union {
    TMXTradeStruct tmx_trds_;
    TMXQuoteStruct tmx_qts_;
    TMXBookStruct tmx_books_;
  } data_;

  inline void print() { fprintf(stderr, "%s\n", ToString().c_str()); }

  const char* getContract() const {
    switch (msg_) {
      case TMX_QUOTE:
        return data_.tmx_qts_.contract_;
        break;
      case TMX_TRADE:
        return data_.tmx_trds_.contract_;
        break;
      case TMX_BOOK:
        return data_.tmx_books_.contract_;
        break;
      default:
        return NULL;
    }
  }

  bool isTradeMsg() const { return msg_ == TMX_TRADE; }

  inline std::string ToString() {
    std::ostringstream t_temp_oss_;
    switch (msg_) {
      case TMX_QUOTE: {
        t_temp_oss_ << "\n========================TMX_QUOTE Message ====================\n\n";
        t_temp_oss_ << "Time: " << time_.tv_sec << "." << std::setw(6) << std::setfill('0') << time_.tv_usec << "\n";
        return (t_temp_oss_.str() + data_.tmx_qts_.ToString() +
                "=============================================================\n");
      } break;
      case TMX_TRADE: {
        t_temp_oss_ << "\n========================TMX_TRADE Message ====================\n\n";
        t_temp_oss_ << "Time: " << time_.tv_sec << "." << std::setw(6) << std::setfill('0') << time_.tv_usec << "\n";
        return (t_temp_oss_.str() + data_.tmx_trds_.ToString() +
                "=============================================================\n");
      } break;
      case TMX_BOOK: {
        t_temp_oss_ << "\n========================TMX_BOOK Message ====================\n\n";
        t_temp_oss_ << "Time: " << time_.tv_sec << "." << std::setw(6) << std::setfill('0') << time_.tv_usec << "\n";
        return (t_temp_oss_.str() + data_.tmx_books_.ToString() +
                "=============================================================\n");
      } break;
      default: {
        std::ostringstream t_temp_oss_;
        t_temp_oss_ << "TMX: NOT IMPLEMENTED FOR THIS EVENT : " << msg_ << '\n';

        return t_temp_oss_.str();
      } break;
    }
    return t_temp_oss_.str();
  }
};

struct TMXLSCommonStruct {
  uint8_t type_;
  uint8_t num_levels_;
  char contract_[14];
  int bid_int_pxs_[5];
  int ask_int_pxs_[5];
  uint16_t bid_szs_[5];
  uint16_t ask_szs_[5];
  uint16_t num_bid_ords_[5];
  uint16_t num_ask_ords_[5];

  inline void print() { fprintf(stderr, "%s\n", ToString().c_str()); }

  char* getContract() { return contract_; }

  bool isTradeMsg() const { return type_ == 1; }

  inline std::string ToString() {
    std::ostringstream t_temp_oss_;

    switch (type_) {
      case 1:  // Trade
      {
        t_temp_oss_ << "\n========================TMX_TRADE Message ====================\n\n";
        t_temp_oss_ << "Contract:   " << contract_ << "\n";
        t_temp_oss_ << "Trade_Qty:  " << bid_szs_[0] << "\n";
        t_temp_oss_ << "TradeIntPx: " << bid_int_pxs_[0] << "\n";
        t_temp_oss_ << "Seqno:      " << 0 << "\n";
        t_temp_oss_ << "Type:       " << (char)num_levels_ << "\n";
      } break;

      case 2:  // Quote
      {
        t_temp_oss_ << "\n========================TMX_QUOTE Message ====================\n\n";
        t_temp_oss_ << "Contract:    " << contract_ << "\n";
        t_temp_oss_ << bid_szs_[0] << " @ " << bid_int_pxs_[0] << " ----- " << ask_int_pxs_[0] << " @ " << ask_szs_[0]
                    << "\n";
        t_temp_oss_ << "Status:      " << (char)num_levels_ << "\n";
      } break;

      case 3:  // Book
      case 4:
      case 5:
      case 6: {
        char status_ = 'T';

        if (type_ == 4)
          status_ = 'Y';
        else if (type_ == 5)
          status_ = 'O';
        else if (type_ == 6)
          status_ = 'E';

        t_temp_oss_ << "\n========================TMX_BOOK Message ====================\n\n";
        t_temp_oss_ << "Contract:    " << contract_ << "\n";
        t_temp_oss_ << "Status:      " << status_ << "\n";
        t_temp_oss_ << "Num_Levels:  " << (int)num_levels_ << "\n";
        for (auto i = 0u; i < 5; i++)
          t_temp_oss_ << "( " << num_bid_ords_[i] << " ) " << bid_szs_[i] << " @ " << bid_int_pxs_[i] << " ---- "
                      << ask_int_pxs_[i] << " @ " << ask_szs_[i] << " ( " << num_ask_ords_[i] << " )\n";
      } break;
    }
    return t_temp_oss_.str();
  }
};

struct TMXOrderBookFeedStruct {
  uint32_t first_seq_of_the_frame;
  struct timeval time;
  double price;
  double prev_price;
  uint32_t size;
  uint32_t prev_size;
  uint64_t order_id;
  uint64_t prev_order_id;
  uint32_t trade_id;
  char contract[14];
  char buy_sell;
  char msg_type;
  uint8_t flags;

  std::string ToString() {
    std::ostringstream t_temp_oss;

    if (0 == trade_id) {
      t_temp_oss << "\n========================TMX_OBF BookMessage====================\n\n";
    } else {
      t_temp_oss << "\n========================TMX_OBF TradeMessage====================\n\n";
    }

    t_temp_oss << "Contract:     " << contract << "\n";
    t_temp_oss << "Time:         " << time.tv_sec << "." << std::setw(6) << std::setfill('0') << time.tv_usec << "\n";
    t_temp_oss << "FirstSeqNum:  " << first_seq_of_the_frame << "\n";
    t_temp_oss << "OrderId:      " << order_id << "\n";
    t_temp_oss << "PrevOrderId:  " << prev_order_id << "\n";
    t_temp_oss << "Price:        " << std::fixed << std::setprecision(6) << price << "\n";
    t_temp_oss << "PrevPrice:    " << std::fixed << std::setprecision(6) << prev_price << "\n";
    t_temp_oss << "Size:         " << size << "\n";
    t_temp_oss << "PrevSize:     " << prev_size << "\n";
    t_temp_oss << "BuySell:      " << buy_sell << "\n";
    t_temp_oss << "MsgType:      " << msg_type << "\n";
    t_temp_oss << "TradeId:      " << trade_id << "\n";
    t_temp_oss << "Flags:        " << (int32_t)flags << "\n";

    return t_temp_oss.str();
  }
};
}

// ====================================   MICEX   =========================

namespace MICEX_MDS {
#define MICEX_MDS_CONTRACT_TEXT_SIZE 14

struct MICEX_Header {
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

struct MICEXTradeStruct {
  char contract_[MICEX_MDS_CONTRACT_TEXT_SIZE];  ///< contract name
  int32_t trd_qty_;                              ///< quantity in this trade
  int32_t tot_qty_;                              ///< Total quantity of in this trade
  uint32_t seqno_;                               ///< instrument based seqno .. to order trades and quotes
  double trd_px_;                                ///< trade price
  uint8_t agg_side_;

  inline void print() { fprintf(stderr, "%s\n", ToString().c_str()); }

  inline std::string ToString() {
    std::ostringstream t_temp_oss_;
    t_temp_oss_ << "Contract:   " << contract_ << '\n';
    t_temp_oss_ << "Trade_Size: " << trd_qty_ << '\n';
    t_temp_oss_ << "Trade_Px:   " << trd_px_ << '\n';
    t_temp_oss_ << "Trade_Value:   " << tot_qty_ << '\n';
    t_temp_oss_ << "InstSeqNum: " << seqno_ << '\n';
    t_temp_oss_ << "AggSide: " << agg_side_ << '\n';

    return t_temp_oss_.str();
  }
};

struct MICEXDeltaStruct {
  char contract_[MICEX_MDS_CONTRACT_TEXT_SIZE];
  double price_;
  uint32_t size_;
  uint32_t level_;
  uint32_t order_count_;
  uint32_t seqno_;
  uint32_t action_;  /// update action
  char type_;        /// entry type
  bool intermediate_;
  char flags[2];

  inline void print() { fprintf(stderr, "%s\n", ToString().c_str()); }

  inline std::string ToString() {
    std::ostringstream t_temp_oss_;
    t_temp_oss_ << "Contract:   " << contract_ << '\n';
    t_temp_oss_ << "Level:       " << level_ << '\n';
    t_temp_oss_ << "Size:       " << size_ << '\n';
    t_temp_oss_ << "Price:      " << std::fixed << std::setprecision(6) << price_ << '\n';
    t_temp_oss_ << "InstSeqNum: " << seqno_ << '\n';
    t_temp_oss_ << "OrderCount:    " << order_count_ << '\n';

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
                    << "DEL_THRU" << '\n';
        break;
      case '4':
        t_temp_oss_ << "Action: "
                    << "DEL_FROM" << '\n';
        break;
      case '5':
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

struct MICEXOrderStruct {
  char contract_[MICEX_MDS_CONTRACT_TEXT_SIZE];
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

enum msgType { MICEX_TRADE, MICEX_DELTA, MICEX_ORDER };

/// common struct .. will support all messages eventually
struct MICEXCommonStruct {
  msgType msg_;
  timeval time_;
  union {
    MICEXTradeStruct micex_trds_;
    MICEXDeltaStruct micex_dels_;
    MICEXOrderStruct micex_ordr_;
  } data_;
  inline bool isTradeMsg() { return (MICEX_TRADE == msg_); }
  inline double GetTradeDoublePrice() { return data_.micex_trds_.trd_px_; }
  inline HFSAT::TradeType_t GetTradeAggressorSide() {
    return (data_.micex_trds_.agg_side_ == 1)
               ? HFSAT::TradeType_t::kTradeTypeBuy
               : (data_.micex_trds_.agg_side_ == 2) ? HFSAT::TradeType_t::kTradeTypeSell
                                                    : HFSAT::TradeType_t::kTradeTypeNoInfo;
  }
  inline uint32_t GetTradeSize() { return data_.micex_trds_.trd_qty_; }
  inline void SetIntermediate(bool flag) {
    switch (msg_) {
      case MICEX_TRADE:
        break;
      case MICEX_DELTA:
        data_.micex_dels_.intermediate_ = flag;
        break;
      case MICEX_ORDER:
        data_.micex_ordr_.intermediate_ = flag;
        break;
      default:
        break;
    }
  }
  const char* getContract() const {
    switch (msg_) {
      case MICEX_TRADE:
        return data_.micex_trds_.contract_;
        break;
      case MICEX_DELTA:
        return data_.micex_dels_.contract_;
        break;
      case MICEX_ORDER:
        return data_.micex_ordr_.contract_;
        break;
      default:
        return NULL;
    }
  }

  bool isTradeMsg() const { return msg_ == MICEX_TRADE; }

  inline void print() { fprintf(stderr, "%s\n", ToString().c_str()); }

  inline std::string ToString() {
    switch (msg_) {
      case MICEX_TRADE: {
        std::ostringstream t_temp_oss_;
        t_temp_oss_ << "\n============== MICEX Trade Message ================\n\n";
        t_temp_oss_ << "Time: " << time_.tv_sec << "." << std::setw(6) << std::setfill('0') << time_.tv_usec << "\n";

        return (t_temp_oss_.str() + data_.micex_trds_.ToString() +
                "===================================================\n");
      } break;
      case MICEX_DELTA: {
        std::ostringstream t_temp_oss_;
        t_temp_oss_ << "\n============== MICEX BookDelta Message ================\n\n";
        t_temp_oss_ << "Time: " << time_.tv_sec << "." << std::setw(6) << std::setfill('0') << time_.tv_usec << "\n";

        return (t_temp_oss_.str() + data_.micex_dels_.ToString() +
                "===================================================\n");
      } break;
      case MICEX_ORDER: {
        std::ostringstream t_temp_oss_;
        t_temp_oss_ << "\n============== MICEX Order Message ================\n\n";
        t_temp_oss_ << "Time: " << time_.tv_sec << "." << std::setw(6) << std::setfill('0') << time_.tv_usec << "\n";

        return (t_temp_oss_.str() + data_.micex_ordr_.ToString() +
                "===================================================\n");
      } break;
      default: {
        std::ostringstream t_temp_oss_;
        t_temp_oss_ << "MICEX: NOT IMPLEMENTED FOR THIS EVENT : " << msg_ << '\n';

        return t_temp_oss_.str();
      } break;
    }
  }
};
}

namespace HKEX_MDS {

#define HKEX_MDS_CONTRACT_TEXT_SIZE 8
enum msgType { HKEX_TRADE = 10, HKEX_BOOK };
struct HKEXTradeStruct {
  char contract_[HKEX_MDS_CONTRACT_TEXT_SIZE];
  int trd_qty_;
  double trd_px_;
  uint32_t no_deals_;
  int tot_vol_;
  bool is_snapshot;
  inline void print() { fprintf(stderr, "%s\n", ToString().c_str()); }

  inline std::string ToString() {
    std::ostringstream t_temp_oss_;
    t_temp_oss_ << "Contract: " << contract_ << "\n";
    t_temp_oss_ << "Trade_Qty:  " << trd_qty_ << "\n";
    t_temp_oss_ << "Trade_Px:   " << trd_px_ << "\n";
    t_temp_oss_ << "total_deals: " << no_deals_ << "\n";
    t_temp_oss_ << "total_volume: " << tot_vol_ << "\n";
    t_temp_oss_ << "Message_Type : " << (is_snapshot ? "Snapshot" : "RealTime") << "\n";
    return t_temp_oss_.str();
  }
};
struct HKEXBookStruct {
  char contract_[HKEX_MDS_CONTRACT_TEXT_SIZE];
#define HK_HALF_BOOK_DEPTH 5
  double pxs_[HK_HALF_BOOK_DEPTH];
  int demand_[HK_HALF_BOOK_DEPTH];
#define HK_HALF_BOOK_SIDE_IS_BID 1
  char side_;
  bool is_snapshot;
  inline void print() { fprintf(stderr, "%s\n", ToString().c_str()); }

  inline std::string ToString() {
    std::ostringstream t_temp_oss_;
    t_temp_oss_ << "Contract:    " << contract_ << "\n";
    t_temp_oss_ << "Side:      " << side_ << "\n";
    for (auto i = 0u; i < 5; i++) t_temp_oss_ << "( " << pxs_[i] << " @ demand of : " << demand_[i] << " )\n";
    t_temp_oss_ << "Message_Type : " << (is_snapshot ? "Snapshot" : "RealTime") << "\n";
    return t_temp_oss_.str();
  }
};

struct HKEXCommonStruct {
  msgType msg_;
  timeval time_;
  union {
    HKEXTradeStruct hkex_trds_;
    HKEXBookStruct hkex_books_;
  } data_;

  inline void print() { fprintf(stderr, "%s\n", ToString().c_str()); }
  inline double GetTradeDoublePrice() { return data_.hkex_trds_.trd_px_; }
  inline HFSAT::TradeType_t GetTradeAggressorSide() { return HFSAT::TradeType_t::kTradeTypeNoInfo; }
  inline uint32_t GetTradeSize() { return data_.hkex_trds_.trd_qty_; }
  const char* getContract() const {
    switch (msg_) {
      case HKEX_TRADE:
        return data_.hkex_trds_.contract_;
        break;
      case HKEX_BOOK:
        return data_.hkex_books_.contract_;
        break;
      default:
        return NULL;
    }
  }

  bool isTradeMsg() const { return msg_ == HKEX_TRADE; }

  inline void SetIntermediate(bool flag) {
    switch (msg_) {
      case HKEX_TRADE: {
      } break;
      case HKEX_BOOK: {
        // No intermediate
      } break;
      default: { } break; }
  }
  inline std::string ToString() {
    std::ostringstream t_temp_oss_;
    switch (msg_) {
      case HKEX_TRADE: {
        t_temp_oss_ << "\n========================HKEX_TRADE Message ====================\n\n";
        t_temp_oss_ << "Time: " << time_.tv_sec << "." << std::setw(6) << std::setfill('0') << time_.tv_usec << "\n";
        return (t_temp_oss_.str() + data_.hkex_trds_.ToString() +
                "=============================================================\n");
      } break;
      case HKEX_BOOK: {
        t_temp_oss_ << "\n========================HKEX_BOOK Message ====================\n\n";
        t_temp_oss_ << "Time: " << time_.tv_sec << "." << std::setw(6) << std::setfill('0') << time_.tv_usec << "\n";
        return (t_temp_oss_.str() + data_.hkex_books_.ToString() +
                "=============================================================\n");
      } break;
      default: {
        std::ostringstream t_temp_oss_;
        t_temp_oss_ << "HKEX: NOT IMPLEMENTED FOR THIS EVENT : " << msg_ << '\n';

        return t_temp_oss_.str();
      } break;
    }
    return t_temp_oss_.str();
  }
};
}

namespace OSE_MDS {

#define OSE_MDS_CONTRACT_TEXT_SIZE 8
#define OSE_NEW_MDS_CONTRACT_TEXT_SIZE 16

struct OSETradeStruct {
  char contract_[OSE_NEW_MDS_CONTRACT_TEXT_SIZE];
  int trd_qty_;
  int trd_px_;
  uint32_t seq_num;
  inline void print() { fprintf(stderr, "%s\n", ToString().c_str()); }

  inline std::string ToString() {
    std::ostringstream t_temp_oss_;
    t_temp_oss_ << "Contract:   " << contract_ << "\n";
    t_temp_oss_ << "Trade_Qty:  " << trd_qty_ << "\n";
    t_temp_oss_ << "Trade_Px:   " << trd_px_ << "\n";
    t_temp_oss_ << "Seq_Num:    " << seq_num << "\n";
    return t_temp_oss_.str();
  }
};

/// Market Delta messages are sent in response to snapshot and market delta events.
struct OSEMktDeltaStruct {
  char contract_[OSE_NEW_MDS_CONTRACT_TEXT_SIZE];  ///< internal contract name
  uint16_t level_;
  int32_t qty_diff_;
  int32_t price_;
  char type_;
  char action_;
  uint64_t order_num;
  uint32_t seq_num;
  inline void print() { fprintf(stderr, "%s\n", ToString().c_str()); }

  inline std::string ToString() {
    std::ostringstream t_temp_oss_;
    t_temp_oss_ << "Contract: " << contract_ << '\n';
    t_temp_oss_ << "Level:    " << level_ << '\n';
    t_temp_oss_ << "quantity difference:     " << qty_diff_ << '\n';
    t_temp_oss_ << "Price:    " << std::fixed << std::setprecision(6) << price_ << '\n';
    switch ((int)type_) {
      case 1:
        t_temp_oss_ << "Type: BUY  " << '\n';
        break;
      case 2:
        t_temp_oss_ << "Type: SELL  " << '\n';
        break;
    }
    switch ((int)action_) {
      case 10:
        t_temp_oss_ << "Action : Snapshot " << '\n';
        break;
      case 0:
        t_temp_oss_ << "Action : ADD ORDER " << '\n';
        break;
      case 1:
        t_temp_oss_ << "Action : DELETE " << '\n';
        break;
      case 2:
        t_temp_oss_ << "Action : MODIFY " << '\n';
        break;
    }

    t_temp_oss_ << "order_num : " << order_num << '\n';
    t_temp_oss_ << "seq_num : " << seq_num << '\n';

    return t_temp_oss_.str();
  };
};

/// would include all message types eventually
enum msgType { OSE_DELTA = 20, OSE_TRADE, OSE_TRADE_DELTA };

/// common struct .. will support all messages eventually
struct OSECommonStruct {
  msgType msg_;
  timeval time_;
  union {
    OSETradeStruct ose_trds_;
    OSEMktDeltaStruct ose_dels_;
  } data_;

  inline void print() { fprintf(stderr, "%s\n", ToString().c_str()); }

  char* getContract() {
    switch (msg_) {
      case OSE_DELTA:
        return data_.ose_dels_.contract_;
        break;
      case OSE_TRADE:
        return data_.ose_trds_.contract_;
        break;
      case OSE_TRADE_DELTA:
        return data_.ose_dels_.contract_;
        break;
      default:
        return NULL;
    }
  }

  bool isTradeMsg() const { return msg_ == OSE_TRADE; }

  inline std::string ToString() {
    switch (msg_) {
      case OSE_DELTA: {
        std::ostringstream t_temp_oss_;
        t_temp_oss_ << "\n============== OSE BookDelta Message ================\n\n";
        t_temp_oss_ << "Time: " << time_.tv_sec << "." << std::setw(6) << std::setfill('0') << time_.tv_usec << "\n";

        return (t_temp_oss_.str() + data_.ose_dels_.ToString() +
                "===================================================\n");
      } break;
      case OSE_TRADE: {
        std::ostringstream t_temp_oss_;
        t_temp_oss_ << "\n============== OSE Trade Message ================\n\n";
        t_temp_oss_ << "Time: " << time_.tv_sec << "." << std::setw(6) << std::setfill('0') << time_.tv_usec << "\n";

        return (t_temp_oss_.str() + data_.ose_trds_.ToString() +
                "===================================================\n");
      } break;
      case OSE_TRADE_DELTA: {
        std::ostringstream t_temp_oss_;
        t_temp_oss_ << "\n============== OSE TradeDelta Message ================\n\n";
        t_temp_oss_ << "Time: " << time_.tv_sec << "." << std::setw(6) << std::setfill('0') << time_.tv_usec << "\n";

        return (t_temp_oss_.str() + data_.ose_dels_.ToString() +
                "===================================================\n");
      } break;
      default: {
        std::ostringstream t_temp_oss_;
        t_temp_oss_ << "OSE: NOT IMPLEMENTED FOR THIS EVENT : " << msg_ << '\n';

        return t_temp_oss_.str();
      } break;
    }
  }
};
}
////
namespace OSE_MDS {
enum l1MSGType { kL1BUY = 0, kL1SELL, kL1TRADE };
struct OSEPLCommonStruct_v0 {
#define OSE_PL_BUY_SELL_TRADE_MASK 3
#define OSE_PL_TRADETYPE_MASK 12
  struct timeval time_;
  char contract_[8];
  uint8_t type_;
  uint32_t price;
  uint32_t size;
  uint32_t order_count_;

  // 0 for buy, 1 for sell, 2 for trade
  void set_buy_sell_trade(uint8_t buy_sell_trade) {
    type_ = type_ & ~OSE_PL_BUY_SELL_TRADE_MASK;
    type_ |= buy_sell_trade;
  }
  void set_buy() {
    type_ = type_ & ~OSE_PL_BUY_SELL_TRADE_MASK;
    // type_ |= 0;
  }
  void set_sell() {
    type_ = type_ & ~OSE_PL_BUY_SELL_TRADE_MASK;
    type_ |= 1;
  }
  void set_trade() {
    type_ = type_ & ~OSE_PL_BUY_SELL_TRADE_MASK;
    type_ |= 2;
  }

  l1MSGType get_buy_sell_trade() const { return (l1MSGType)(type_ & OSE_PL_BUY_SELL_TRADE_MASK); }

  const char* getContract() const { return contract_; }
  bool isTradeMsg() const { return type_ & 2; }
  inline std::string ToString() {
    std::ostringstream t_temp_oss_;
    t_temp_oss_ << "\n============== OSE PLMessage ================\n\n";
    t_temp_oss_ << "Time: " << time_.tv_sec << "." << std::setw(6) << std::setfill('0') << time_.tv_usec << "\n";
    t_temp_oss_ << "Contract:   " << contract_ << "\n";
    t_temp_oss_ << "Buy_Sell:   " << get_buy_sell_trade() << "\n";
    t_temp_oss_ << "Price:      " << std::fixed << std::setprecision(6) << price << "\n";
    t_temp_oss_ << "Size:       " << size << "\n";
    t_temp_oss_ << "OrderCount: " << order_count_ << "\n";
    return t_temp_oss_.str();
  }
};

struct OSEPLCommonStruct_v1 {
  struct timeval time_;
  char contract_[8];
  uint32_t price;
  uint32_t size;
  uint32_t order_count_;
  uint8_t type_;

  // 0 for buy, 1 for sell, 2 for trade
  void set_buy_sell_trade(uint8_t buy_sell_trade) {
    type_ = type_ & ~OSE_PL_BUY_SELL_TRADE_MASK;
    type_ |= buy_sell_trade;
  }
  void set_buy() {
    type_ = type_ & ~OSE_PL_BUY_SELL_TRADE_MASK;
    // type_ |= 0;
  }
  void set_sell() {
    type_ = type_ & ~OSE_PL_BUY_SELL_TRADE_MASK;
    type_ |= 1;
  }
  void set_trade() {
    type_ = type_ & ~OSE_PL_BUY_SELL_TRADE_MASK;
    type_ |= 2;
  }

  l1MSGType get_buy_sell_trade() { return (l1MSGType)(type_ & OSE_PL_BUY_SELL_TRADE_MASK); }

  char* getContract() { return contract_; }
  bool isTradeMsg() const { return type_ & 2; }
  inline std::string ToString() {
    std::ostringstream t_temp_oss_;
    t_temp_oss_ << "\n============== OSE PLMessage ================\n\n";
    t_temp_oss_ << "Time: " << time_.tv_sec << "." << std::setw(6) << std::setfill('0') << time_.tv_usec << "\n";
    t_temp_oss_ << "Contract:   " << contract_ << "\n";
    t_temp_oss_ << "Buy_Sell:   " << get_buy_sell_trade() << "\n";
    t_temp_oss_ << "Price:      " << std::fixed << std::setprecision(6) << price << "\n";
    t_temp_oss_ << "Size:       " << size << "\n";
    t_temp_oss_ << "OrderCount: " << order_count_ << "\n";
    return t_temp_oss_.str();
  }
};

struct OSEPLCommonStruct {
  struct timeval time_;
  char contract_[OSE_NEW_MDS_CONTRACT_TEXT_SIZE];
  uint32_t price;
  uint32_t size;
  uint32_t order_count_;
  uint8_t type_;

  // 0 for buy, 1 for sell, 2 for trade
  void set_buy_sell_trade(uint8_t buy_sell_trade) {
    type_ = type_ & ~OSE_PL_BUY_SELL_TRADE_MASK;
    type_ |= buy_sell_trade;
  }
  void set_buy() {
    type_ = type_ & ~OSE_PL_BUY_SELL_TRADE_MASK;
    // type_ |= 0;
  }
  void set_sell() {
    type_ = type_ & ~OSE_PL_BUY_SELL_TRADE_MASK;
    type_ |= 1;
  }
  void set_trade() {
    type_ = type_ & ~OSE_PL_BUY_SELL_TRADE_MASK;
    type_ |= 2;
  }

  l1MSGType get_buy_sell_trade() const { return (l1MSGType)(type_ & OSE_PL_BUY_SELL_TRADE_MASK); }

  char* getContract() { return contract_; }
  bool isTradeMsg() const { return type_ & 2; }
  inline double GetTradeDoublePrice() { return static_cast<double>(price); }
  inline HFSAT::TradeType_t GetTradeAggressorSide() { return HFSAT::TradeType_t::kTradeTypeNoInfo; }
  inline uint32_t GetTradeSize() { return size; }
  inline void SetIntermediate(bool flag) {
    // No intermediate
  }
  inline std::string ToString() {
    std::ostringstream t_temp_oss_;
    t_temp_oss_ << "\n============== OSE PLMessage ================\n\n";
    t_temp_oss_ << "Time: " << time_.tv_sec << "." << std::setw(6) << std::setfill('0') << time_.tv_usec << "\n";
    t_temp_oss_ << "Contract:   " << contract_ << "\n";
    t_temp_oss_ << "Buy_Sell:   " << get_buy_sell_trade() << "\n";
    t_temp_oss_ << "Price:      " << std::fixed << std::setprecision(6) << price << "\n";
    t_temp_oss_ << "Size:       " << size << "\n";
    t_temp_oss_ << "OrderCount: " << order_count_ << "\n";
    return t_temp_oss_.str();
  }
};

enum OSEPriceFeedMsgType {
  PRICEFEED_NEW = 0,
  PRICEFEED_DELETE,
  PRICEFEED_CHANGE,
  PRICEFEED_TRADE,
  PRICEFEED_TRADE_ORDER,
  PRICEFEED_INVALID
};
enum OSEFeedMsgType { FEED_INVALID = 0, FEED_NEW, FEED_DELETE, FEED_CHANGE, FEED_TRADE, FEED_TRADE_ORDER };

struct OSEPriceFeedCommonStruct_v0 {
  struct timeval time_;
  char contract_[8];
  uint32_t price;
  uint32_t size;
  uint32_t order_count_;
  uint8_t type_;
  uint8_t price_level_;
  OSEPriceFeedMsgType price_feed_msg_type_;

  // 0 for buy, 1 for sell, 2 for trade
  void set_buy_sell_trade(uint8_t buy_sell_trade) {
    type_ = type_ & ~OSE_PL_BUY_SELL_TRADE_MASK;
    type_ |= buy_sell_trade;
  }
  void set_buy() {
    type_ = type_ & ~OSE_PL_BUY_SELL_TRADE_MASK;
    // type_ |= 0;
  }
  void set_sell() {
    type_ = type_ & ~OSE_PL_BUY_SELL_TRADE_MASK;
    type_ |= 1;
  }
  void set_trade() {
    type_ = type_ & ~OSE_PL_BUY_SELL_TRADE_MASK;
    type_ |= 2;
  }

  l1MSGType get_buy_sell_trade() const { return (l1MSGType)(type_ & OSE_PL_BUY_SELL_TRADE_MASK); }

  char* getContract() { return contract_; }
  bool isTradeMsg() const { return type_ & 2; }

  inline std::string ToString() {
    std::ostringstream t_temp_oss_;
    t_temp_oss_ << "\n============== OSE PriceFeed Message ================\n\n";
    t_temp_oss_ << "Time: " << time_.tv_sec << "." << std::setw(6) << std::setfill('0') << time_.tv_usec << "\n";
    t_temp_oss_ << "Contract:   " << contract_ << "\n";
    t_temp_oss_ << "Buy_Sell:   " << get_buy_sell_trade() << "\n";
    t_temp_oss_ << "Price:      " << std::fixed << std::setprecision(6) << price << "\n";
    t_temp_oss_ << "Size:       " << size << "\n";
    t_temp_oss_ << "OrderCount: " << order_count_ << "\n";
    t_temp_oss_ << "Level:      " << (int)price_level_ << "\n";

    switch (price_feed_msg_type_) {
      case PRICEFEED_NEW: {
        t_temp_oss_ << "Action:     "
                    << "NEW"
                    << "\n";

      } break;

      case PRICEFEED_DELETE: {
        t_temp_oss_ << "Action:     "
                    << "DELETE"
                    << "\n";

      } break;

      case PRICEFEED_CHANGE: {
        t_temp_oss_ << "Action:     "
                    << "CHANGE"
                    << "\n";

      } break;

      case PRICEFEED_INVALID: {
        t_temp_oss_ << "Action:     "
                    << "INVALID"
                    << "\n";

      } break;

      default: {
        t_temp_oss_ << "Action:     "
                    << "INVALID"
                    << "\n";

      } break;
    }

    return t_temp_oss_.str();
  }
};

struct OSEPriceFeedCommonStruct {
  struct timeval time_;
  char contract_[OSE_NEW_MDS_CONTRACT_TEXT_SIZE];
  uint64_t exch_order_seq_;
  uint32_t price;
  uint32_t size;
  uint32_t order_count_;
  uint8_t type_;
  uint8_t price_level_;
  OSEPriceFeedMsgType price_feed_msg_type_;

  // 0 for buy, 1 for sell, 2 for trade
  void set_buy_sell_trade(uint8_t buy_sell_trade) {
    type_ = type_ & ~OSE_PL_BUY_SELL_TRADE_MASK;
    type_ |= buy_sell_trade;
  }
  void set_buy() {
    type_ = type_ & ~OSE_PL_BUY_SELL_TRADE_MASK;
    // type_ |= 0;
  }
  void set_sell() {
    type_ = type_ & ~OSE_PL_BUY_SELL_TRADE_MASK;
    type_ |= 1;
  }
  void set_trade() {
    type_ = type_ & ~OSE_PL_BUY_SELL_TRADE_MASK;
    type_ |= 2;
  }

  l1MSGType get_buy_sell_trade() const { return (l1MSGType)(type_ & OSE_PL_BUY_SELL_TRADE_MASK); }

  char* getContract() { return contract_; }
  bool isTradeMsg() const { return type_ & 2; }
  inline double GetTradeDoublePrice() { return static_cast<double>(price); }
  inline HFSAT::TradeType_t GetTradeAggressorSide() { return HFSAT::TradeType_t::kTradeTypeNoInfo; }
  inline uint32_t GetTradeSize() { return size; }
  inline bool isTrade() { return (type_ & 2); }
  inline void SetIntermediate(bool flag) {
    // No intermediate
    switch (price_feed_msg_type_) {
      case PRICEFEED_NEW: {
      } break;

      case PRICEFEED_DELETE: {
      } break;

      case PRICEFEED_CHANGE: {
      } break;

      case PRICEFEED_INVALID: {
      } break;

      default: { } break; }
  }
  inline std::string ToString() {
    std::ostringstream t_temp_oss_;
    t_temp_oss_ << "\n============== OSE PriceFeed Message ================\n\n";
    t_temp_oss_ << "Time: " << time_.tv_sec << "." << std::setw(6) << std::setfill('0') << time_.tv_usec << "\n";
    t_temp_oss_ << "Contract:   " << contract_ << "\n";
    t_temp_oss_ << "ExchSeqd:   " << exch_order_seq_ << "\n";
    t_temp_oss_ << "Buy_Sell:   " << get_buy_sell_trade() << "\n";
    t_temp_oss_ << "Price:      " << std::fixed << std::setprecision(6) << price << "\n";
    t_temp_oss_ << "Size:       " << size << "\n";
    t_temp_oss_ << "OrderCount: " << order_count_ << "\n";
    t_temp_oss_ << "Level:      " << (int)price_level_ << "\n";

    switch (price_feed_msg_type_) {
      case PRICEFEED_NEW: {
        t_temp_oss_ << "Action:     "
                    << "NEW"
                    << "\n";

      } break;

      case PRICEFEED_DELETE: {
        t_temp_oss_ << "Action:     "
                    << "DELETE"
                    << "\n";

      } break;

      case PRICEFEED_CHANGE: {
        t_temp_oss_ << "Action:     "
                    << "CHANGE"
                    << "\n";

      } break;

      case PRICEFEED_TRADE: {
        t_temp_oss_ << "Action:     "
                    << "TRADE"
                    << "\n";

      } break;

      case PRICEFEED_TRADE_ORDER: {
        t_temp_oss_ << "Action:     "
                    << "TRADEOrder"
                    << "\n";

      } break;

      case PRICEFEED_INVALID: {
        t_temp_oss_ << "Action:     "
                    << "INVALID"
                    << "\n";

      } break;

      default: {
        t_temp_oss_ << "Action:     "
                    << "INVALID"
                    << "\n";

      } break;
    }

    return t_temp_oss_.str();
  }
};

struct OSECombinedCommonStruct {
  struct timeval time_;
  char contract_[OSE_NEW_MDS_CONTRACT_TEXT_SIZE];
  uint64_t exch_order_seq_;
  uint64_t message_sequence_;
  uint32_t price_;
  int32_t qty_diff_;
  uint32_t agg_size_;
  uint32_t order_count_;
  uint32_t ose_user_;
  OSEFeedMsgType feed_msg_type_;
  uint32_t order_feed_level_;
  uint8_t price_feed_level_;
  char action_;
  char type_;
  char agg_side_;
  bool is_pricefeed_;

  char* getContract() { return contract_; }
  inline bool isTradeMsg() { return (FEED_TRADE == feed_msg_type_); }
  inline double GetTradeDoublePrice() { return price_; }
  inline HFSAT::TradeType_t GetTradeAggressorSide() {
    return (agg_side_ == 'B') ? HFSAT::TradeType_t::kTradeTypeBuy : (agg_side_ == 'S')
                                                                        ? HFSAT::TradeType_t::kTradeTypeSell
                                                                        : HFSAT::TradeType_t::kTradeTypeNoInfo;
  }
  inline uint32_t GetTradeSize() { return agg_size_; }
  inline void SetIntermediate(bool flag) {
    switch ((int)type_) {
      // No Intermediate Found
      case 1:
        //        t_temp_oss_ << "Type:       BUY" << '\n';
        break;
      case 2:
        //        t_temp_oss_ << "Type:       SELL" << '\n';
        break;
      default:
        //        t_temp_oss_ << "Type:       INVALID" << '\n';
        break;
    }
    switch ((int)action_) {
      case 10:
        //        t_temp_oss_ << "Action:     Snapshot" << '\n';
        break;
      case 0:
        //        t_temp_oss_ << "Action:     ADD" << '\n';
        break;
      case 1:
        //        t_temp_oss_ << "Action:     DELETE" << '\n';
        break;
      case 2:
        //        t_temp_oss_ << "Action:     MODIFY" << '\n';
        break;
      default:
        //        t_temp_oss_ << "Action:     INVALID" << '\n';
        break;
    }
    switch ((int)agg_side_) {
      case 1:
        //        t_temp_oss_ << "AggSide:    BUY" << '\n';
        break;
      case 2:
        //        t_temp_oss_ << "AggSide:    SELL" << '\n';
        break;
      default:
        //        t_temp_oss_ << "AggSide:    INVALID" << '\n';
        break;
    }
    switch (feed_msg_type_) {
      case FEED_NEW: {
      } break;

      case FEED_DELETE: {
      } break;

      case FEED_CHANGE: {
      } break;

      case FEED_TRADE: {
      } break;

      case FEED_TRADE_ORDER: {
      } break;

      case FEED_INVALID: {
      } break;

      default: { } break; }
  }
  inline std::string ToString() {
    std::ostringstream t_temp_oss_;
    t_temp_oss_ << "\n============== OSE CombinedFeed Message ================\n\n";
    t_temp_oss_ << "Time: " << time_.tv_sec << "." << std::setw(6) << std::setfill('0') << time_.tv_usec << "\n";
    t_temp_oss_ << "Contract:   " << contract_ << "\n";
    t_temp_oss_ << "ExchSeqd:   " << exch_order_seq_ << "\n";
    t_temp_oss_ << "Price:      " << std::fixed << std::setprecision(6) << price_ << "\n";
    t_temp_oss_ << "Size:       " << agg_size_ << "\n";
    t_temp_oss_ << "OrderCount: " << order_count_ << "\n";
    t_temp_oss_ << "PriceLevel: " << (int)price_feed_level_ << "\n";
    t_temp_oss_ << "OrderLevel: " << order_feed_level_ << '\n';
    t_temp_oss_ << "Qty Diff:   " << qty_diff_ << '\n';
    t_temp_oss_ << "MsgSeq:     " << message_sequence_ << "\n";

    switch ((int)type_) {
      case 1:
        t_temp_oss_ << "Type:       BUY" << '\n';
        break;
      case 2:
        t_temp_oss_ << "Type:       SELL" << '\n';
        break;
      default:
        t_temp_oss_ << "Type:       INVALID" << '\n';
        break;
    }
    switch ((int)action_) {
      case 10:
        t_temp_oss_ << "Action:     Snapshot" << '\n';
        break;
      case 0:
        t_temp_oss_ << "Action:     ADD" << '\n';
        break;
      case 1:
        t_temp_oss_ << "Action:     DELETE" << '\n';
        break;
      case 2:
        t_temp_oss_ << "Action:     MODIFY" << '\n';
        break;
      default:
        t_temp_oss_ << "Action:     INVALID" << '\n';
        break;
    }
    switch ((int)agg_side_) {
      case 1:
        t_temp_oss_ << "AggSide:    BUY" << '\n';
        break;
      case 2:
        t_temp_oss_ << "AggSide:    SELL" << '\n';
        break;
      default:
        t_temp_oss_ << "AggSide:    INVALID" << '\n';
        break;
    }

    t_temp_oss_ << "OSEUsr:     " << ose_user_ << "\n";

    switch (feed_msg_type_) {
      case FEED_NEW: {
        t_temp_oss_ << "PFType:     "
                    << "NEW"
                    << "\n";

      } break;

      case FEED_DELETE: {
        t_temp_oss_ << "PFType:     "
                    << "DELETE"
                    << "\n";

      } break;

      case FEED_CHANGE: {
        t_temp_oss_ << "PFType:     "
                    << "CHANGE"
                    << "\n";

      } break;

      case FEED_TRADE: {
        t_temp_oss_ << "PFType:     "
                    << "TRADE"
                    << "\n";

      } break;

      case FEED_TRADE_ORDER: {
        t_temp_oss_ << "PFType:     "
                    << "TRADEOrder"
                    << "\n";

      } break;

      case FEED_INVALID: {
        t_temp_oss_ << "PFType:     "
                    << "INVALID"
                    << "\n";

      } break;

      default: {
        t_temp_oss_ << "PFType:     "
                    << "INVALID"
                    << "\n";

      } break;
    }

    t_temp_oss_ << "PF_Trigger: " << (is_pricefeed_ ? "YES" : "NO") << "\n";

    return t_temp_oss_.str();
  }
};
}

/// CSM broadcast structs
namespace CSM_MDS {
#define CSM_MDS_CONTRACT_TEXT_SIZE 32
#define VOLUME_TYPES 4

enum msgType { CSM_DELTA = 1, CSM_TRADE = 2, CSM_TOB = 3 };

struct CSMMktDeltaStruct {
  int32_t size_[VOLUME_TYPES];  ///< size at level
  double price_;                ///< price at level
  uint8_t level_;               ///< level >= 1
  char type_;                   ///< type 0 - Bid , 1 - Offer
  char status_;                 ///< 'A' is normal operation. Not filtered right now
  char action_;                 ///< 0 - New, 1 - Change, 2 - Delete, 3 - Delete Thru , 4 - Delete From  5 - Overlay
  uint8_t security_trading_status_;
  bool intermediate_;  ///< if this is an intermediate message .. clients should not react to intermediates

  inline void print() { fprintf(stderr, "%s\n", ToString().c_str()); }

  inline std::string ToString() {
    std::ostringstream t_temp_oss_;
    t_temp_oss_ << "Level:      " << (int)level_ << '\n';
    t_temp_oss_ << "Size Data : Total Limit -> " << size_[0] << " Customer Limit -> " << size_[1]
                << " Total Contingency -> " << size_[2] << " Customer Contingency -> " << size_[3] << '\n';
    t_temp_oss_ << "Price:      " << std::fixed << std::setprecision(6) << price_ << '\n';

    switch (type_) {
      case '1':
        t_temp_oss_ << "Type:   "
                    << "ASK" << '\n';
        break;
      case '0':
        t_temp_oss_ << "Type:   "
                    << "BID" << '\n';
        break;
      case '2':
        t_temp_oss_ << "Type:   "
                    << "TRD" << '\n';
        break;
      default:
        t_temp_oss_ << "Type:   "
                    << "---" << '\n';
        break;
    }

    t_temp_oss_ << "Status: " << status_ << '\n';

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
      case 5:
        t_temp_oss_ << "Action: "
                    << "OVERLAY" << '\n';
        break;
      case 9:
        t_temp_oss_ << "Action: "
                    << "TRADE" << '\n';
        break;
      default:
        t_temp_oss_ << "Action: "
                    << "--------" << '\n';
        break;
    }

    t_temp_oss_ << "SecTrdStatus: " << (int)(security_trading_status_) << "\n";
    t_temp_oss_ << "Intermediate: " << (intermediate_ ? "Y" : "N") << '\n';

    return t_temp_oss_.str();
  }
};

struct CSMTradeStruct {
  double trd_px_;      ///< trade price
  uint32_t trd_qty_;   ///< quantity in this trade
  uint32_t tot_qty_;   ///< total quantity
  uint32_t agg_side_;  ///< 1 - buy, 2-sell
  char trade_condition[8];

  inline void print() { fprintf(stderr, "%s\n", ToString().c_str()); }

  inline std::string ToString() {
    std::ostringstream t_temp_oss_;
    t_temp_oss_ << "Trade_Size: " << trd_qty_ << '\n';
    t_temp_oss_ << "Tot_Qty:    " << tot_qty_ << '\n';
    t_temp_oss_ << "Trade_Px:   " << trd_px_ << '\n';
    t_temp_oss_ << "Agg_Side:   " << agg_side_ << '\n';
    t_temp_oss_ << "TrdCon:     " << trade_condition << '\n';
    return t_temp_oss_.str();
  }
};

struct CSMCommonStruct {
  msgType msg_;
  timeval time_;
  char contract_[CSM_MDS_CONTRACT_TEXT_SIZE];  ///< internal contract name
  uint32_t seqno_;                             ///< ???
  int32_t message_seq_no_;

  union {
    CSMMktDeltaStruct csm_dels_;
    CSMTradeStruct csm_trds_;

  } data_;

  inline void print() { fprintf(stderr, "%s\n", ToString().c_str()); }
  void SetIntermediate(bool flag) {
    switch (msg_) {
      case CSM_DELTA:
        data_.csm_dels_.intermediate_ = flag;
        break;
      case CSM_TRADE:
        break;
      default:
        break;
    }
  }
  bool isTradeMsg() {
    if (CSM_TRADE == msg_) {
      if (data_.csm_trds_.trade_condition[0] == 'S' && contract_[2] > '9') {
        return true;
      } else if (data_.csm_trds_.trade_condition[0] != 'S') {
        return true;
      }
    }
    return false;
  }

  char* getContract() { return contract_; }

  inline std::string ToString() {
    switch (msg_) {
      case CSM_DELTA: {
        std::ostringstream t_temp_oss_;
        t_temp_oss_ << "\n============== CSM BookDelta Message ================\n\n";
        t_temp_oss_ << "Time: " << time_.tv_sec << "." << std::setw(6) << std::setfill('0') << time_.tv_usec << "\n";
        t_temp_oss_ << "Contract: " << contract_ << "\n";
        t_temp_oss_ << "RptSeq: " << seqno_ << "\n";
        t_temp_oss_ << "MsgSeq: " << message_seq_no_ << "\n";

        return (t_temp_oss_.str() + data_.csm_dels_.ToString() +
                "===================================================\n");

      } break;

      case CSM_TOB: {
        std::ostringstream t_temp_oss_;
        t_temp_oss_ << "\n============== CSM TopOfBoook Message ================\n\n";
        t_temp_oss_ << "Time: " << time_.tv_sec << "." << std::setw(6) << std::setfill('0') << time_.tv_usec << "\n";
        t_temp_oss_ << "Contract: " << contract_ << "\n";
        t_temp_oss_ << "RptSeq: " << seqno_ << "\n";
        t_temp_oss_ << "MsgSeq: " << message_seq_no_ << "\n";

        return (t_temp_oss_.str() + data_.csm_dels_.ToString() +
                "===================================================\n");

      } break;

      case CSM_TRADE: {
        std::ostringstream t_temp_oss_;
        t_temp_oss_ << "\n============== CSM Trade Message ================\n\n";
        t_temp_oss_ << "Time: " << time_.tv_sec << "." << std::setw(6) << std::setfill('0') << time_.tv_usec << "\n";
        t_temp_oss_ << "Contract: " << contract_ << "\n";
        t_temp_oss_ << "RptSeq: " << seqno_ << "\n";
        t_temp_oss_ << "MsgSeq: " << message_seq_no_ << "\n";

        return (t_temp_oss_.str() + data_.csm_trds_.ToString() +
                "===================================================\n");

      } break;

      default: {
        std::ostringstream t_temp_oss_;
        t_temp_oss_ << "CSM: NOT IMPLEMENTED FOR THIS EVENT : " << msg_ << '\n';

        return t_temp_oss_.str();
      } break;
    }
  }
};
}

/// AFLASH broadcast structs
namespace AFLASH_MDS {
#define AFLASH_MDS_CONTRACT_TEXT_SIZE 32
#define VOLUME_TYPES 4

const uint32_t MAX_FIELDS = 4;  // Assumed that max of 4 fields can be there in a single message

enum msgType { AFLASH_DUMMY = 1 };

/*  sturct to encapsulate a datum of an Alpfaflash Message */

struct AFlashDatum {
  uint8_t field_id_;
  // NOT ADDED: datum_type_, assumed that datum_id from the xml_specs uniquely identifies the datum_type
  union {
    double vFloat;
    long int vInt;
    bool vBool;
    // NOT ADDED yet: Enumeration, Yes_no_na, Directional, Time
  } data_;
};
// TODO: fill the struct with requitred fields
struct AFlashCommonStructLive {
  timeval time_;
  int uid_;
  char symbol_[4];  // dummy symbol for logger
  AFlashDatum fields[MAX_FIELDS];
  uint16_t category_id_;
  uint8_t type_;     // AF_MSGSPECS::MsgType
  uint8_t version_;  // AF_MSGSPECS::Message.msg_version_
  uint8_t nfields_;

  AFlashCommonStructLive() { strcpy(symbol_, "AFL"); }

  inline void print() { fprintf(stderr, "%s\n", ToString().c_str()); }

  inline std::string ToString() {
    std::ostringstream t_temp_oss_;
    t_temp_oss_ << "\n============== AFlash Message ================\n\n";
    t_temp_oss_ << "UID: " << uid_ << "\n";
    t_temp_oss_ << "Time: " << time_.tv_sec << "." << std::setw(6) << std::setfill('0') << time_.tv_usec << "\n";

    t_temp_oss_ << "Category: " << category_id_ << "\n";
    t_temp_oss_ << "Msg Type: " << (int)type_ << "\n";
    t_temp_oss_ << "Msg Version: " << (int)version_ << "\n";

    AF_MSGSPECS::AF_MsgParser& af_msgparser_ = AF_MSGSPECS::AF_MsgParser::GetUniqueInstance();
    AF_MSGSPECS::Category* catg_ = af_msgparser_.getCategoryforId(category_id_);
    if (!catg_) {
      t_temp_oss_ << "Error: Message does NOT match the specifications: invalid catg_\n";
      return t_temp_oss_.str();
    }
    AF_MSGSPECS::Message* msg_ = af_msgparser_.getMsgFromCatg(catg_, (short int)type_);
    if (!msg_) {
      t_temp_oss_ << "Error: Message does NOT match the specifications: invalid msg_\n";
      return t_temp_oss_.str();
    }

    for (int i = 0; i < nfields_; i++) {
      AF_MSGSPECS::Field* t_field_ = af_msgparser_.getFieldFromMsg(msg_, fields[i].field_id_);
      if (!t_field_) {
        t_temp_oss_ << "Error: Field does NOT match the specifications: invalid t_field_: "
                    << (int)(fields[i].field_id_) << "\n";
        return t_temp_oss_.str();
      }
      t_temp_oss_ << "Field, " << i << ", id: " << (int)fields[i].field_id_ << ", data: ";
      switch (t_field_->field_type_) {
        case AF_MSGSPECS::kFloat:
        case AF_MSGSPECS::kDouble:
          t_temp_oss_ << fields[i].data_.vFloat;
          break;
        case AF_MSGSPECS::kShort_value_enumeration:
        case AF_MSGSPECS::kLong:
        case AF_MSGSPECS::kShort:
        case AF_MSGSPECS::kInt:
          t_temp_oss_ << fields[i].data_.vInt;
          break;
        case AF_MSGSPECS::kBoolean:
          t_temp_oss_ << fields[i].data_.vBool;
          break;
        default:
          t_temp_oss_ << "Not_Recognized";
          break;
      }
      t_temp_oss_ << "\n";
    }
    return t_temp_oss_.str();
  }

  const char* getContract() const { return symbol_; }
};

struct AFlashCommonStruct {
  int uid_;
  timeval time_;
  char symbol_[4];   // dummy symbol for logger
  uint8_t type_;     // AF_MSGSPECS::MsgType
  uint8_t version_;  // AF_MSGSPECS::Message.msg_version_
  uint8_t nfields_;
  uint16_t category_id_;
  AFlashDatum fields[MAX_FIELDS];

  AFlashCommonStruct() { strcpy(symbol_, "AFL"); }

  inline void print() { fprintf(stderr, "%s\n", ToString().c_str()); }

  inline std::string ToString() {
    std::ostringstream t_temp_oss_;
    t_temp_oss_ << "\n============== AFlash Message ================\n\n";
    t_temp_oss_ << "UID: " << uid_ << "\n";
    t_temp_oss_ << "Time: " << time_.tv_sec << "." << std::setw(6) << std::setfill('0') << time_.tv_usec << "\n";

    t_temp_oss_ << "Category: " << category_id_ << "\n";
    t_temp_oss_ << "Msg Type: " << (int)type_ << "\n";
    t_temp_oss_ << "Msg Version: " << (int)version_ << "\n";

    AF_MSGSPECS::AF_MsgParser& af_msgparser_ = AF_MSGSPECS::AF_MsgParser::GetUniqueInstance();
    AF_MSGSPECS::Category* catg_ = af_msgparser_.getCategoryforId(category_id_);
    if (!catg_) {
      t_temp_oss_ << "Error: Message does NOT match the specifications: invalid catg_\n";
      return t_temp_oss_.str();
    }
    AF_MSGSPECS::Message* msg_ = af_msgparser_.getMsgFromCatg(catg_, (short int)type_);
    if (!msg_) {
      t_temp_oss_ << "Error: Message does NOT match the specifications: invalid msg_\n";
      return t_temp_oss_.str();
    }

    for (int i = 0; i < nfields_; i++) {
      AF_MSGSPECS::Field* t_field_ = af_msgparser_.getFieldFromMsg(msg_, fields[i].field_id_);
      if (!t_field_) {
        t_temp_oss_ << "Error: Field does NOT match the specifications: invalid t_field_: "
                    << (int)(fields[i].field_id_) << "\n";
        return t_temp_oss_.str();
      }
      t_temp_oss_ << "Field, " << i << ", id: " << (int)fields[i].field_id_ << ", data: ";
      switch (t_field_->field_type_) {
        case AF_MSGSPECS::kFloat:
        case AF_MSGSPECS::kDouble:
          t_temp_oss_ << fields[i].data_.vFloat;
          break;
        case AF_MSGSPECS::kShort_value_enumeration:
        case AF_MSGSPECS::kLong:
        case AF_MSGSPECS::kShort:
        case AF_MSGSPECS::kInt:
          t_temp_oss_ << fields[i].data_.vInt;
          break;
        case AF_MSGSPECS::kBoolean:
          t_temp_oss_ << fields[i].data_.vBool;
          break;
        default:
          t_temp_oss_ << "Not_Recognized";
          break;
      }
      t_temp_oss_ << "\n";
    }
    return t_temp_oss_.str();
  }

  const char* getContract() const { return symbol_; }
};
}

/// ICE broadcast structs
namespace ICE_MDS {
#define ICE_MDS_CONTRACT_TEXT_SIZE 32

enum msgType {
  ICE_PL = 1,
  ICE_TRADE = 2,
  ICE_FOD = 3,
  ICE_MS,
  ICE_UNKNOWN,
  ICE_RESET_BEGIN,
  ICE_RESET_END
};  // PL: Price Level, FOD: Full Order Depth, MS: Market Snapshot

struct ICEPriceLevelStruct {
  double price_;  ///< price at level (should be divided by OrderPriceDenominator for the market)
  int32_t size_;  ///< size at level
  int16_t orders_;
  uint8_t level_;  ///< level >= 1
  char action_;    ///< 0 - New, 1 - Change, 2 - Delete, 3 - snapshot
  char side_;      ///< type '1' - Bid , '2' - Offer
  // int32_t implied_size_;
  // int16_t implied_orders_;
  bool intermediate_;  ///< if this is an intermediate message .. clients should not react to intermediates

  inline void print() { fprintf(stderr, "%s\n", ToString().c_str()); }

  inline std::string ToString() {
    std::ostringstream t_temp_oss_;
    t_temp_oss_ << "Level:      " << (int)level_ << '\n';
    t_temp_oss_ << "Size Data : Quantity -> " << size_ << '\n';       // << " Implied_Quantity -> " << implied_size_
    t_temp_oss_ << "Order Data : Num_Orders -> " << orders_ << '\n';  //<< " Num_Implied_Orders -> " << implied_orders_
    t_temp_oss_ << "Price:      " << std::fixed << std::setprecision(6) << price_ << '\n';
    switch (side_) {
      case '1':
        t_temp_oss_ << "Type:   "
                    << "BID" << '\n';
        break;
      case '2':
        t_temp_oss_ << "Type:   "
                    << "ASK" << '\n';
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
                    << "SNAPSHOT" << '\n';
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
struct ICEMSStruct {
  // int32_t volume;
  // int32_t block_volume;
  // int32_t efs_volume;
  // int32_t efp_volume;
  // int32_t open_interest;
  // double opening_price;
  // int64_t high;
  // int64_t low;
  // int64_t vwap;
  double last_trade_price;
  int64_t last_trade_time;
  int32_t num_entries;
  int32_t last_trade_qty;
  int32_t last_msg_seq;
  // char open_interest_date[11];
  int16_t market_type;
  // int64_t spwdpp;
  // double settlement_price;
  // int64_t settle_price_time_;
  // char is_settle_official_;
  int16_t res1_;
  char trading_status;

  inline void print() { fprintf(stderr, "%s\n", ToString().c_str()); }

  inline std::string ToString() {
    std::ostringstream t_temp_oss_;
    t_temp_oss_ << "Trade_Status: " << trading_status << '\n';
    // t_temp_oss_ << "volume: " << volume << ", block_volume: " << block_volume << ", efs_volume: " << efs_volume << ",
    // efp_volume: " << efp_volume << '\n';
    // t_temp_oss_ << "open_interest: " << open_interest << ", opening_price: " << opening_price << '\n';
    // t_temp_oss_ << "high:   " << high << ", low:   " << low << ", vwap:     " << vwap << '\n' ;
    t_temp_oss_ << "num_entries:     " << num_entries << '\n';
    t_temp_oss_ << "last_trade_price:     " << std::fixed << std::setprecision(6) << last_trade_price
                << ", last_trade_qty:     " << last_trade_qty << ", last_trade_time:     " << last_trade_time << '\n';
    // open_interest_date[10] = '\0';
    t_temp_oss_ << "last_msg_seq:     " << last_msg_seq << ", market_type:     " << market_type
                << '\n';  //", open_interest_date:     " << open_interest_date <<
    // t_temp_oss_ << "spwdpp:     " << spwdpp << ", settlement_price:     " << settlement_price << ",
    // settle_price_time_:     " << settle_price_time_
    //    << ", is_settle_official_: " << is_settle_official_ << '\n' ;
    t_temp_oss_ << "res1_: " << res1_ << '\n';
    return t_temp_oss_.str();
  }
};
struct ICETradeStruct {
  int64_t trade_id_;
  int64_t transact_time_;
  double price_;  ///< price at level (should be divided by OrderPriceDenominator for the market)
  int32_t size_;
  char side_;  /// Aggressor side:   ' '-No aggressor, '1' - buy, '2'-sell
  char is_system_priced_leg_;
  char off_market_trade_type_;
  char system_priced_leg_type_;
  char implied_spread_at_mkt_open_;
  char is_adjusted_trade_;
  char is_implied_;
  char is_leg_deal_outside_ipl_;

  inline void print() { fprintf(stderr, "%s\n", ToString().c_str()); }

  inline std::string ToString() {
    std::ostringstream t_temp_oss_;
    t_temp_oss_ << "Trade_ID: " << trade_id_ << '\n';
    t_temp_oss_ << "Trade_Size: " << size_ << '\n';
    t_temp_oss_ << "Trade_Px:   " << std::fixed << std::setprecision(6) << price_ << '\n';
    t_temp_oss_ << "Agg_Side:   " << ((side_ == '1') ? "Buy" : ((side_ == '2') ? "Sell" : "None")) << '\n';
    t_temp_oss_ << "Transact time:   " << transact_time_ << '\n';
    t_temp_oss_ << "is_system_priced_leg_:   " << is_system_priced_leg_ << ", "
                << "off_market_trade_type_:   " << off_market_trade_type_ << "\n"
                << "is_implied_:   " << is_implied_ << '\n';

    return t_temp_oss_.str();
  }
};

struct ICEFODStruct {
  int64_t order_id_;  // Unique order ID per market
  int64_t order_entry_time_;
  double price_;  ///< trade price (should be divided by OrderPriceDenominator for the market)
  int32_t size_;  ///< quantity in this order
  int32_t seq_within_millis_;
  int16_t order_seq;
  uint8_t type_;  // 0-Add, 1-Modify, 2-Delete, 3-Snapshot
  char side_;     ///< '1' - buy, '2'-sell
                  //    char flags_ [2];      //flags[0]: implied order, flags[1]: RFQ orderc
  bool intermediate_;

  inline void print() { fprintf(stderr, "%s\n", ToString().c_str()); }

  inline std::string ToString() {
    std::ostringstream t_temp_oss_;
    t_temp_oss_ << "Order_ID: " << order_id_ << '\n';
    t_temp_oss_ << "Order_Seq: " << order_seq << '\n';
    t_temp_oss_ << "Entrytime: " << order_entry_time_ << '\n';
    t_temp_oss_ << "SeqInMillis: " << seq_within_millis_ << '\n';
    t_temp_oss_ << "Type: ";
    switch (type_) {
      case 0:
        t_temp_oss_ << "Add";
        break;
      case 1:
        t_temp_oss_ << "Modify";
        break;
      case 2:
        t_temp_oss_ << "Delete";
        break;
      case 3:
        t_temp_oss_ << "Snapshot";
        break;
      default:
        t_temp_oss_ << "---";
        break;
    }
    t_temp_oss_ << '\n';
    t_temp_oss_ << "Order_Size: " << size_ << '\n';
    t_temp_oss_ << "Order_Px:   " << std::fixed << std::setprecision(6) << price_ << '\n';
    t_temp_oss_ << "Side:   " << ((side_ == '1') ? "Buy" : ((side_ == '2') ? "Sell" : "None")) << '\n';
    t_temp_oss_ << "Intermediate_: " << (intermediate_ == 0 ? "No" : "Yes") << "\n";
    return t_temp_oss_.str();
  }
};

struct ICECommonStructLive {
  char contract_[ICE_MDS_CONTRACT_TEXT_SIZE];  ///< internal contract name
  timeval time_;
  msgType msg_;
  int32_t seqno_;  /// Packet sequence number

  union {
    ICEPriceLevelStruct ice_pls_;
    ICETradeStruct ice_trds_;
    ICEFODStruct ice_fods_;
    ICEMSStruct ice_ms_;

  } data_;
  inline void SetIntermediate(bool flag) {
    switch (msg_) {
      case ICE_PL: {
        data_.ice_pls_.intermediate_ = flag;
        break;
      }
      case ICE_FOD: {
        data_.ice_fods_.intermediate_ = flag;
        break;
      }
      case ICE_TRADE: {
        break;
      }
      case ICE_MS: {
        break;
      }
      default: { break; }
    }
  }
  inline void print() { fprintf(stderr, "%s\n", ToString().c_str()); }
  inline double GetTradeDoublePrice() { return data_.ice_trds_.price_; }
  inline HFSAT::TradeType_t GetTradeAggressorSide() {
    return (data_.ice_trds_.side_ == '1') ? HFSAT::TradeType_t::kTradeTypeBuy
                                          : (data_.ice_trds_.side_ == '2') ? HFSAT::TradeType_t::kTradeTypeSell
                                                                           : HFSAT::TradeType_t::kTradeTypeNoInfo;
  }
  inline uint32_t GetTradeSize() { return data_.ice_trds_.size_; }
  bool isTradeMsg() { return ICE_TRADE == msg_; }

  char* getContract() { return contract_; }

  inline std::string ToString() {
    switch (msg_) {
      case ICE_PL: {
        std::ostringstream t_temp_oss_;
        t_temp_oss_ << "\n============== ICE Price Level Message ================\n\n";
        t_temp_oss_ << "Time: " << time_.tv_sec << "." << std::setw(6) << std::setfill('0') << time_.tv_usec << "\n";
        t_temp_oss_ << "Contract: " << contract_ << "\n";
        t_temp_oss_ << "Seqnum: " << seqno_ << "\n";

        return (t_temp_oss_.str() + data_.ice_pls_.ToString() +
                "===================================================\n");

      } break;

      case ICE_FOD: {
        std::ostringstream t_temp_oss_;
        t_temp_oss_ << "\n============== ICE FullOrderDepth Message ================\n\n";
        t_temp_oss_ << "Time: " << time_.tv_sec << "." << std::setw(6) << std::setfill('0') << time_.tv_usec << "\n";
        t_temp_oss_ << "Contract: " << contract_ << "\n";
        t_temp_oss_ << "Seqnum: " << seqno_ << "\n";

        return (t_temp_oss_.str() + data_.ice_fods_.ToString() +
                "===================================================\n");

      } break;

      case ICE_TRADE: {
        std::ostringstream t_temp_oss_;
        t_temp_oss_ << "\n============== ICE Trade Message ================\n\n";
        t_temp_oss_ << "Time: " << time_.tv_sec << "." << std::setw(6) << std::setfill('0') << time_.tv_usec << "\n";
        t_temp_oss_ << "Contract: " << contract_ << "\n";
        t_temp_oss_ << "Seqnum: " << seqno_ << "\n";

        return (t_temp_oss_.str() + data_.ice_trds_.ToString() +
                "===================================================\n");

      } break;

      case ICE_MS: {
        std::ostringstream t_temp_oss_;
        t_temp_oss_ << "\n============== ICE Market Snapshot Message ================\n\n";
        t_temp_oss_ << "Time: " << time_.tv_sec << "." << std::setw(6) << std::setfill('0') << time_.tv_usec << "\n";
        t_temp_oss_ << "Contract: " << contract_ << "\n";
        t_temp_oss_ << "Seqnum: " << seqno_ << "\n";

        return (t_temp_oss_.str() + data_.ice_ms_.ToString() + "===================================================\n");

      } break;

      case ICE_RESET_BEGIN: {
        std::ostringstream t_temp_oss_;
        t_temp_oss_ << "\n============== ICE Reset Begin Message ================\n\n";
        t_temp_oss_ << "Time: " << time_.tv_sec << "." << std::setw(6) << std::setfill('0') << time_.tv_usec << "\n";
        t_temp_oss_ << "Contract: " << contract_ << "\n";

        return (t_temp_oss_.str() + "===================================================\n");
      } break;
      case ICE_RESET_END: {
        std::ostringstream t_temp_oss_;
        t_temp_oss_ << "\n============== ICE Reset End Message ================\n\n";
        t_temp_oss_ << "Time: " << time_.tv_sec << "." << std::setw(6) << std::setfill('0') << time_.tv_usec << "\n";
        t_temp_oss_ << "Contract: " << contract_ << "\n";

        return (t_temp_oss_.str() + "===================================================\n");
      } break;

      default: {
        std::ostringstream t_temp_oss_;
        t_temp_oss_ << "ICE: NOT IMPLEMENTED FOR THIS EVENT : " << msg_ << '\n';

        return t_temp_oss_.str();
      } break;
    }
  }
};

struct ICECommonStruct {
  char contract_[ICE_MDS_CONTRACT_TEXT_SIZE];  ///< internal contract name
  msgType msg_;
  timeval time_;
  int32_t seqno_;  /// Packet sequence number

  union {
    ICEPriceLevelStruct ice_pls_;
    ICETradeStruct ice_trds_;
    ICEFODStruct ice_fods_;
    ICEMSStruct ice_ms_;

  } data_;

  inline void print() { fprintf(stderr, "%s\n", ToString().c_str()); }

  bool isTradeMsg() { return ICE_TRADE == msg_; }

  char* getContract() { return contract_; }

  inline std::string ToString() {
    switch (msg_) {
      case ICE_PL: {
        std::ostringstream t_temp_oss_;
        t_temp_oss_ << "\n============== ICE Price Level Message ================\n\n";
        t_temp_oss_ << "Time: " << time_.tv_sec << "." << std::setw(6) << std::setfill('0') << time_.tv_usec << "\n";
        t_temp_oss_ << "Contract: " << contract_ << "\n";
        t_temp_oss_ << "Seqnum: " << seqno_ << "\n";

        return (t_temp_oss_.str() + data_.ice_pls_.ToString() +
                "===================================================\n");

      } break;

      case ICE_FOD: {
        std::ostringstream t_temp_oss_;
        t_temp_oss_ << "\n============== ICE FullOrderDepth Message ================\n\n";
        t_temp_oss_ << "Time: " << time_.tv_sec << "." << std::setw(6) << std::setfill('0') << time_.tv_usec << "\n";
        t_temp_oss_ << "Contract: " << contract_ << "\n";
        t_temp_oss_ << "Seqnum: " << seqno_ << "\n";

        return (t_temp_oss_.str() + data_.ice_fods_.ToString() +
                "===================================================\n");

      } break;

      case ICE_TRADE: {
        std::ostringstream t_temp_oss_;
        t_temp_oss_ << "\n============== ICE Trade Message ================\n\n";
        t_temp_oss_ << "Time: " << time_.tv_sec << "." << std::setw(6) << std::setfill('0') << time_.tv_usec << "\n";
        t_temp_oss_ << "Contract: " << contract_ << "\n";
        t_temp_oss_ << "Seqnum: " << seqno_ << "\n";

        return (t_temp_oss_.str() + data_.ice_trds_.ToString() +
                "===================================================\n");

      } break;

      case ICE_MS: {
        std::ostringstream t_temp_oss_;
        t_temp_oss_ << "\n============== ICE Market Snapshot Message ================\n\n";
        t_temp_oss_ << "Time: " << time_.tv_sec << "." << std::setw(6) << std::setfill('0') << time_.tv_usec << "\n";
        t_temp_oss_ << "Contract: " << contract_ << "\n";
        t_temp_oss_ << "Seqnum: " << seqno_ << "\n";

        return (t_temp_oss_.str() + data_.ice_ms_.ToString() + "===================================================\n");

      } break;

      case ICE_RESET_BEGIN: {
        std::ostringstream t_temp_oss_;
        t_temp_oss_ << "\n============== ICE Reset Begin Message ================\n\n";
        t_temp_oss_ << "Time: " << time_.tv_sec << "." << std::setw(6) << std::setfill('0') << time_.tv_usec << "\n";
        t_temp_oss_ << "Contract: " << contract_ << "\n";

        return (t_temp_oss_.str() + "===================================================\n");
      } break;
      case ICE_RESET_END: {
        std::ostringstream t_temp_oss_;
        t_temp_oss_ << "\n============== ICE Reset End Message ================\n\n";
        t_temp_oss_ << "Time: " << time_.tv_sec << "." << std::setw(6) << std::setfill('0') << time_.tv_usec << "\n";
        t_temp_oss_ << "Contract: " << contract_ << "\n";

        return (t_temp_oss_.str() + "===================================================\n");
      } break;

      default: {
        std::ostringstream t_temp_oss_;
        t_temp_oss_ << "ICE: NOT IMPLEMENTED FOR THIS EVENT : " << msg_ << '\n';

        return t_temp_oss_.str();
      } break;
    }
  }
};

enum ICEPriceFeedType { PL_NEW = 0, PL_CHANGE, PL_DELETE, PL_SNAPSHOT, PL_TRADE, PL_INVALID };
enum ICEOrderFeedType { OL_ADD = 0, OL_MODIFY, OL_DELETE, OL_SNAPSHOT, OL_TRADE, OL_INVALID };

struct ICECombinedCommonStructLive {
  timeval time_;                               // packet time
  char contract_[ICE_MDS_CONTRACT_TEXT_SIZE];  // contract name
  uint64_t order_id_;         // order_id in case of of_type add, modify, delete, trade_id_ in case of a trade
  double price_;              // price
  int32_t seqno_;             // seq. num of packet
  int32_t pl_size_;           // price level size at given price
  int32_t pl_order_count_;    // num orders at given price
  ICEPriceFeedType pf_type_;  // price feed type: new, change, delete, snapshot, trade or invalid
  ICEOrderFeedType of_type_;  // order feed type: add, modify, delete, snapshot, trade or invalid
  int32_t order_size_;        // order_size / trade_size
  int8_t pl_level_;           // price feed level
  char side_;                 // buy_sell side : '1' - Bid , '2' - Offer
  bool has_price_feed_;       // valid price feed values in this struct ?
  bool has_order_feed_;       // valid order feed values in this struct ?
  bool intermediate_;

  char* getContract() { return contract_; }
  inline bool isTradeMsg() { return (PL_TRADE == pf_type_); }
  inline double GetTradeDoublePrice() { return price_; }
  inline HFSAT::TradeType_t GetTradeAggressorSide() { return HFSAT::TradeType_t::kTradeTypeNoInfo; }
  inline uint32_t GetTradeSize() { return order_size_; }
  inline void SetIntermediate(bool flag) { intermediate_ = flag; }
  inline std::string ToString() {
    std::ostringstream t_temp_oss_;
    t_temp_oss_ << "\n============== ICE CombinedFeed Message ================\n\n";
    t_temp_oss_ << "Time: " << time_.tv_sec << "." << std::setw(6) << std::setfill('0') << time_.tv_usec << "\n";
    t_temp_oss_ << "MsgSeq:     " << seqno_ << "\n";
    t_temp_oss_ << "Contract:   " << contract_ << "\n";
    t_temp_oss_ << "Price:      " << std::fixed << std::setprecision(6) << price_ << "\n";
    t_temp_oss_ << "PLSize:     " << pl_size_ << "\n";
    t_temp_oss_ << "PLOrderCount: " << pl_order_count_ << "\n";
    t_temp_oss_ << "PriceLevel: " << (int)pl_level_ << "\n";
    t_temp_oss_ << "Order/TradeID: " << order_id_ << '\n';
    t_temp_oss_ << "OrderSize:   " << order_size_ << '\n';

    switch (side_) {
      case '1':
        t_temp_oss_ << "Side:       BUY" << '\n';
        break;
      case '2':
        t_temp_oss_ << "Side:       SELL" << '\n';
        break;
      default:
        t_temp_oss_ << "Side:       INVALID" << '\n';
        break;
    }
    switch (pf_type_) {
      case PL_NEW:
        t_temp_oss_ << "PFType:     PL_NEW" << '\n';
        break;
      case PL_CHANGE:
        t_temp_oss_ << "PFType:     PL_CHANGE" << '\n';
        break;
      case PL_DELETE:
        t_temp_oss_ << "PFType:     PL_DELETE" << '\n';
        break;
      case PL_TRADE:
        t_temp_oss_ << "PFType:     PL_TRADE" << '\n';
        break;
      case PL_INVALID:
        t_temp_oss_ << "PFType:     PL_INVALID" << '\n';
        break;
      default:
        t_temp_oss_ << "PFType:     PL_INVALID" << '\n';
        break;
    }

    switch (of_type_) {
      case OL_ADD:
        t_temp_oss_ << "OFType:     ORDER_ADD" << '\n';
        break;
      case OL_SNAPSHOT:
        t_temp_oss_ << "OFType:     ORDER_ADD" << '\n';
        break;
      case OL_MODIFY:
        t_temp_oss_ << "OFType:     ORDER_MODIFY" << '\n';
        break;
      case OL_DELETE:
        t_temp_oss_ << "OFType:     ORDER_DELETE" << '\n';
        break;
      case OL_INVALID:
        t_temp_oss_ << "OFType:     ORDER_INVALID" << '\n';
        break;
      case OL_TRADE:
        t_temp_oss_ << "OFType:     ORDER_TRADE" << '\n';
        break;
      default:
        t_temp_oss_ << "OFType:     ORDER_INVALID" << '\n';
        break;
    }

    return t_temp_oss_.str();
  }
};

struct ICECombinedCommonStruct {
  timeval time_;                               // packet time
  char contract_[ICE_MDS_CONTRACT_TEXT_SIZE];  // contract name
  int32_t seqno_;                              // seq. num of packet
  uint64_t order_id_;         // order_id in case of of_type add, modify, delete, trade_id_ in case of a trade
  double price_;              // price
  int32_t pl_size_;           // price level size at given price
  int32_t pl_order_count_;    // num orders at given price
  ICEPriceFeedType pf_type_;  // price feed type: new, change, delete, snapshot, trade or invalid
  ICEOrderFeedType of_type_;  // order feed type: add, modify, delete, snapshot, trade or invalid
  int32_t order_size_;        // order_size / trade_size
  bool has_price_feed_;       // valid price feed values in this struct ?
  bool has_order_feed_;       // valid order feed values in this struct ?
  int8_t pl_level_;           // price feed level
  char side_;                 // buy_sell side : '1' - Bid , '2' - Offer
  bool intermediate_;

  char* getContract() { return contract_; }

  inline std::string ToString() {
    std::ostringstream t_temp_oss_;
    t_temp_oss_ << "\n============== ICE CombinedFeed Message ================\n\n";
    t_temp_oss_ << "Time: " << time_.tv_sec << "." << std::setw(6) << std::setfill('0') << time_.tv_usec << "\n";
    t_temp_oss_ << "MsgSeq:     " << seqno_ << "\n";
    t_temp_oss_ << "Contract:   " << contract_ << "\n";
    t_temp_oss_ << "Price:      " << std::fixed << std::setprecision(6) << price_ << "\n";
    t_temp_oss_ << "PLSize:     " << pl_size_ << "\n";
    t_temp_oss_ << "PLOrderCount: " << pl_order_count_ << "\n";
    t_temp_oss_ << "PriceLevel: " << (int)pl_level_ << "\n";
    t_temp_oss_ << "Order/TradeID: " << order_id_ << '\n';
    t_temp_oss_ << "OrderSize:   " << order_size_ << '\n';

    switch (side_) {
      case '1':
        t_temp_oss_ << "Side:       BUY" << '\n';
        break;
      case '2':
        t_temp_oss_ << "Side:       SELL" << '\n';
        break;
      default:
        t_temp_oss_ << "Side:       INVALID" << '\n';
        break;
    }
    switch (pf_type_) {
      case PL_NEW:
        t_temp_oss_ << "PFType:     PL_NEW" << '\n';
        break;
      case PL_CHANGE:
        t_temp_oss_ << "PFType:     PL_CHANGE" << '\n';
        break;
      case PL_DELETE:
        t_temp_oss_ << "PFType:     PL_DELETE" << '\n';
        break;
      case PL_TRADE:
        t_temp_oss_ << "PFType:     PL_TRADE" << '\n';
        break;
      case PL_INVALID:
        t_temp_oss_ << "PFType:     PL_INVALID" << '\n';
        break;
      default:
        t_temp_oss_ << "PFType:     PL_INVALID" << '\n';
        break;
    }

    switch (of_type_) {
      case OL_ADD:
        t_temp_oss_ << "OFType:     ORDER_ADD" << '\n';
        break;
      case OL_SNAPSHOT:
        t_temp_oss_ << "OFType:     ORDER_ADD" << '\n';
        break;
      case OL_MODIFY:
        t_temp_oss_ << "OFType:     ORDER_MODIFY" << '\n';
        break;
      case OL_DELETE:
        t_temp_oss_ << "OFType:     ORDER_DELETE" << '\n';
        break;
      case OL_INVALID:
        t_temp_oss_ << "OFType:     ORDER_INVALID" << '\n';
        break;
      case OL_TRADE:
        t_temp_oss_ << "OFType:     ORDER_TRADE" << '\n';
        break;
      default:
        t_temp_oss_ << "OFType:     ORDER_INVALID" << '\n';
        break;
    }

    return t_temp_oss_.str();
  }
};
}

namespace MDS_PLAZA2 {

const int kMaxP2ContractSize = 32;

struct P2TradeMsg {
  enum OrdStatus { kExpiration = 0x20, kExpirationSign = 0x80 };

  char contract_[kMaxP2ContractSize];
  double price_;
  int64_t orderno_, seqno_;
  int size_, position_left_;

  OrdStatus status_buy_, status_sell_;

  std::string ToString() {
    std::stringstream ss;
    ss << "Contract:     " << contract_ << '\n';
    ss << "Size:         " << size_ << '\n';
    ss << "Position Left:" << position_left_ << '\n';
    ss << "Price:        " << std::fixed << std::setprecision(6) << price_ << '\n';
    ss << "OrderNum:     " << orderno_ << '\n';
    ss << "InstSeqNum:   " << seqno_ << '\n';
    return ss.str();
  }
};

struct P2Order {
  enum BuySell { kBuy = 1, kSell = 2 };

  enum MsgType { kCancel = 0, kNewOrd = 1, kExec = 2 };

  enum OrdStatus {
    kQoute = 1,
    kCouter = 2,
    kNonSystem = 4,
    kEndOfTransaction = 0x1000,
    kRepoClear = 0x2000,
    kRepo = 0x20000,
    kRegular = 0x40000
  };

  char contract_[kMaxP2ContractSize];
  double price_;
  int size_, size_left_;
  int64_t orderno_, seqno_;
  BuySell buysell_;
  MsgType type_;
  OrdStatus status_;

  bool intermediate() const { return !(status_ & P2Order::kEndOfTransaction); }
  std::string ToString() {
    std::stringstream ss;
    ss << "Contract:   " << contract_ << '\n';
    ss << "Size:       " << size_ << '\n';
    ss << "Size Left:  " << size_left_ << '\n';
    ss << "Price:      " << std::fixed << std::setprecision(6) << price_ << '\n';
    ss << "OrderNum:   " << orderno_ << '\n';
    ss << "InstSeqNum: " << seqno_ << '\n';

    switch (buysell_) {
      case kSell:
        ss << "Type:   "
           << "ASK" << '\n';
        break;
      case kBuy:
        ss << "Type:   "
           << "BID" << '\n';
        break;
      default:
        ss << "Type:   "
           << "---" << '\n';
        break;
    }

    switch (type_) {
      case kCancel:
        ss << "Action: "
           << "CANCEL" << '\n';
        break;
      case kNewOrd:
        ss << "Action: "
           << "NEW" << '\n';
        break;
      case kExec:
        ss << "Action: "
           << "EXEC" << '\n';
        break;
      default:
        ss << "Action: "
           << "--------" << '\n';
        break;
    }
    ss << "Intermediate: " << (intermediate() ? "Y" : "N") << '\n';

    return ss.str();
  }
};

struct P2CommonStruct {
  enum MsgType { kP2Order, kP2Trade } msg_type_;
  timeval time_;
  union {
    P2Order ord_;
    P2TradeMsg trd_;
  } data_;
  std::string ToString() {
    std::stringstream ss;
    switch (msg_type_) {
      case kP2Order: {
        P2Order& o = data_.ord_;
        ss << "\n============== Plaza2 Order Message ================\n";
        ss << "Time: " << time_.tv_sec << "." << std::setw(6) << std::setfill('0') << time_.tv_usec << "\n";
        ss << o.ToString();
        break;
      }
      case kP2Trade: {
        P2TradeMsg& t = data_.trd_;
        ss << "\n============== Plaza2 Trade Message ================\n";
        ss << "Time: " << time_.tv_sec << "." << std::setw(6) << std::setfill('0') << time_.tv_usec << "\n";
        ss << t.ToString();
        break;
      }
    }
    return ss.str();
  }

  const char* getContract() {
    switch (msg_type_) {
      case kP2Order:
        return &data_.ord_.contract_[0];
      case kP2Trade:
        return &data_.trd_.contract_[0];
      default:
        return "---";
    }
  }
};

struct P2NewCommonStruct {
  enum MsgType { kP2Order, kP2Trade } msg_type_;
  timeval time_;
  int event_time_;
  union {
    P2Order ord_;
    P2TradeMsg trd_;
  } data_;
  std::string ToString() {
    std::stringstream ss;
    switch (msg_type_) {
      case kP2Order: {
        P2Order& o = data_.ord_;
        ss << "\n============== Plaza2 Order Message ================\n";
        ss << "Time: " << time_.tv_sec << "." << std::setw(6) << std::setfill('0') << time_.tv_usec << "\n";
        ss << o.ToString();
        ss << "EventTS: " << event_time_ << "\n";
        break;
      }
      case kP2Trade: {
        P2TradeMsg& t = data_.trd_;
        ss << "\n============== Plaza2 Trade Message ================\n";
        ss << "Time: " << time_.tv_sec << "." << std::setw(6) << std::setfill('0') << time_.tv_usec << "\n";
        ss << t.ToString();
        ss << "EventTS: " << event_time_ << "\n";
        break;
      }
    }
    return ss.str();
  }

  const char* getContract() {
    switch (msg_type_) {
      case kP2Order:
        return &data_.ord_.contract_[0];
      case kP2Trade:
        return &data_.trd_.contract_[0];
      default:
        return "---";
    }
  }
};
}

namespace NSE_MDS {

enum OfflineStringMsgType { kInvalid = 0, kDelta, kTrade };

static std::string GetNullPadValue(const char* input, int length) {
  char output_char[length + 1];
  memset(output_char, 0, length + 1);
  memcpy(output_char, input, length);

  return output_char;
}

static std::string GetMonthFromString(std::string month_str) {
  if (std::string("Jan") == month_str)
    return "01";
  else if (std::string("Feb") == month_str)
    return "02";
  else if (std::string("Mar") == month_str)
    return "03";
  else if (std::string("Apr") == month_str)
    return "04";
  else if (std::string("May") == month_str)
    return "05";
  else if (std::string("Jun") == month_str)
    return "06";
  else if (std::string("Jul") == month_str)
    return "07";
  else if (std::string("Aug") == month_str)
    return "08";
  else if (std::string("Sep") == month_str)
    return "09";
  else if (std::string("Oct") == month_str)
    return "10";
  else if (std::string("Nov") == month_str)
    return "11";
  else if (std::string("Dec") == month_str)
    return "12";

  return "IL";
}

struct NSEDotexOfflineDeltaStringStruct {
  char record_indicator_type[2];
  char segment[4];
  char date[8];
  char order_number[8];
  char time_in_jiffies[14];
  char buy_or_sell;
  char activity_type;
  char symbol[10];
  char instrument[6];
  char expiry[9];
  char strike_price_integer[6];
  char strike_price_decimal_point[2];
  char option_type[2];
  char volume_disclosed[8];
  char volume_original[8];
  char limit_price_integer[6];
  char limit_price_decimal_point[2];
  char trigger_price_integer[6];
  char trigger_price_decimal_point[2];
  char mkt_flag;
  char stoploss_indicator;
  char ioc_flag;
  char spread_comb_type;
  char algo_trading_flag;
  char client_identity_flag;
  char dummy;

  std::string GetUnderlyingSymbol() {
    std::string symbol_without_padding = "";
    int i = 0;

    for (i = 0; i < 10; i++) {
      if (symbol[i] != 'b') break;
    }

    while (i < 10) {
      symbol_without_padding += (symbol[i]);
      i++;
    }

    std::string month_str = "";
    for (i = 0; i < 3; i++) {
      month_str += expiry[2 + i];
    }
    month_str = GetMonthFromString(month_str);

    std::string year_str = "";
    for (i = 0; i < 4; i++) {
      year_str += expiry[5 + i];
    }

    return (symbol_without_padding + year_str + month_str);
  }
};

struct NSEDotexOfflineTradeStringStruct {
  char record_indicator_type[2];
  char segment[4];
  char trade_number_date[8];
  char trade_number[8];
  char time_in_jiffies[14];
  char symbol[10];
  char instrument[6];
  char expiry[9];
  char strike_price_integer[6];
  char strike_price_decimal_point[2];
  char option_type[2];
  char trade_price_integer[6];
  char trade_price_decimal_point[2];
  char trade_qty[8];
  char buy_order_date[8];
  char buy_order_number[8];
  char buy_order_algo_indicator;
  char buy_client_identity_flag;
  char sell_order_date[8];
  char sell_order_number[8];
  char sell_order_algo_indicator;
  char sell_client_identity_flag;
  char dummy;

  std::string GetUnderlyingSymbol() {
    std::string symbol_without_padding = "";
    int i = 0;

    for (i = 0; i < 10; i++) {
      if (symbol[i] != 'b') break;
    }

    while (i < 10) {
      symbol_without_padding += (symbol[i]);
      i++;
    }

    std::string month_str = "";
    for (i = 0; i < 3; i++) {
      month_str += expiry[2 + i];
    }
    month_str = GetMonthFromString(month_str);

    std::string year_str = "";
    for (i = 0; i < 4; i++) {
      year_str += expiry[5 + i];
    }

    return (symbol_without_padding + year_str + month_str);
  }
};

struct NSEDotexOfflineDataStringStruct {
  OfflineStringMsgType msg_type;

  union {
    NSEDotexOfflineDeltaStringStruct nse_order;
    NSEDotexOfflineTradeStringStruct nse_trade;

  } data;

  uint64_t GetTimeInJiffies() {
    switch (msg_type) {
      case kDelta: {
        return strtoul(GetNullPadValue(data.nse_order.time_in_jiffies, 14).c_str(), NULL, 0);
      } break;

      case kTrade: {
        return strtoul(GetNullPadValue(data.nse_trade.time_in_jiffies, 14).c_str(), NULL, 0);

      } break;

      default: { return 0; } break;
    }

    return 0;
  }

  std::string GetUnderlyingSymbol() {
    switch (msg_type) {
      case kDelta: {
        return data.nse_order.GetUnderlyingSymbol();
      } break;
      case kTrade: {
        return data.nse_trade.GetUnderlyingSymbol();
      } break;
      default: { return "INVALID"; } break;
    }

    return "INVALID";
  }

  std::string GetInstrument() {
    switch (msg_type) {
      case kDelta: {
        return data.nse_order.instrument;
      } break;
      case kTrade: {
        return data.nse_trade.instrument;
      } break;
      default: { return "INVALID"; } break;
    }

    return "INVALID";
  }

  std::string GetOptionType() {
    switch (msg_type) {
      case kDelta: {
        return data.nse_order.option_type;
      } break;
      case kTrade: {
        return data.nse_trade.option_type;
      } break;
      default: { return "INVALID"; } break;
    }

    return "INVALID";
  }
};

#define NSE_DOTEX_OFFLINE_SYMBOL_LENGTH 48

enum class MsgType {
  kInvalid = 0,
  kNSEOrderDelta,
  kNSETrade,
  kNSETradeDelta,
  kNSEOrderSpreadDelta,
  kNSESpreadTrade,
  kNSETradeExecutionRange
};

static std::string GetClientIdString(char client_id_flag) {
  switch (client_id_flag) {
    case '1':
      return "Client - CP";
    case '2':
      return "Proprietary";
    case '3':
      return "Non CP - Non Prop";

    default:
      return "Invalid";
  }

  return "Invalid";
}

static std::string GetAlgoIndicatorString(char algo_indicator) {
  switch (algo_indicator) {
    case '0':
      return "Algo Order";
    case '1':
      return "Non-Algo Order";
    case '2':
      return "Algo SOR Order";
    case '3':
      return "Non-Algo SOR Order";

    default:
      return "Invalid";
  }

  return "Invalid";
}

static std::string GetCombTypeString(char comb_type) {
  switch (comb_type) {
    case 'S':
      return "SpreadOrder";
    case '2':
      return "2 Leg Order";
    case '3':
      return "3 Leg Order";

    default:
      return "NoLegOrder";
  }

  return "NoLegOrder";
}

static std::string GetActivityStr(char activity_type) {
  switch (activity_type) {
    case '1':
      return "OrderAdd";
    case '3':
      return "OrderDelete";
    case '4':
      return "OrderModify";
    default:
      return "Invalid";
  }

  return "Invalid";
}

struct NSEDotexTrade {
  double trade_price;
  uint64_t buy_order_number;
  uint64_t sell_order_number;
  int32_t bid_size_remaining;
  int32_t ask_size_remaining;
  uint32_t trade_quantity;
  char buy_algo_indicator;
  char sell_algo_indicator;
  char buy_client_id_flag;
  char sell_client_id_flag;

  std::string ToString() {
    std::ostringstream t_temp_oss;
    t_temp_oss << "TradePrice:      " << std::fixed << std::setprecision(6) << trade_price << "\n";
    t_temp_oss << "TradeQty:        " << trade_quantity << "\n";
    t_temp_oss << "BuyOrderNum:     " << buy_order_number << "\n";
    t_temp_oss << "BuyLeftOver:     " << bid_size_remaining << "\n";
    t_temp_oss << "BuyAlgo:         " << GetAlgoIndicatorString(buy_algo_indicator) << "\n";
    t_temp_oss << "BuyClientId:     " << GetClientIdString(buy_client_id_flag) << "\n";
    t_temp_oss << "SellOrderNum:    " << sell_order_number << "\n";
    t_temp_oss << "SellLeftOver:    " << ask_size_remaining << "\n";
    t_temp_oss << "SellAlgo:        " << GetAlgoIndicatorString(sell_algo_indicator) << "\n";
    t_temp_oss << "SellClientId:    " << GetClientIdString(sell_client_id_flag) << "\n";

    return t_temp_oss.str();
  }
};

struct NSEDotexOrderDelta {
  double order_price;
  double old_price;
  //    uint32_t aggregate_size ;
  uint32_t volume_original;
  uint32_t old_size;
  //    uint16_t number_of_orders ;
  //    uint8_t price_level ;
  char buysell;
  char activity_type;
  char spread_comb_type;
  char algo_indicator;
  char client_id_flag;
  //    char is_pricefeed_event ;
  //    char price_level_activity ;
  //    char is_orderfeed_event ;

  std::string ToString() {
    std::ostringstream t_temp_oss;
    t_temp_oss << "OrderPrice:      " << std::fixed << std::setprecision(6) << order_price << "\n";
    t_temp_oss << "OldPrice:        " << std::fixed << std::setprecision(6) << old_price << "\n";
    //      t_temp_oss << "AggSize:         " << ( uint32_t ) aggregate_size << "\n" ;
    t_temp_oss << "VolumeOrig:      " << (uint32_t)volume_original << "\n";
    t_temp_oss << "OldVolume:       " << (uint32_t)old_size << "\n";
    //      t_temp_oss << "NumberOfOrders:  " << ( uint32_t ) number_of_orders << "\n" ;
    t_temp_oss << "BuySell:         " << buysell << "\n";
    t_temp_oss << "EntryType:       " << GetActivityStr(activity_type) << "\n";
    t_temp_oss << "SpreadType:      " << GetCombTypeString(spread_comb_type) << "\n";
    t_temp_oss << "AlgoIndicator:   " << GetAlgoIndicatorString(algo_indicator) << "\n";
    t_temp_oss << "ClientId:        " << GetClientIdString(client_id_flag) << "\n";
    //      t_temp_oss << "IsPriceFeed:     " << ( is_pricefeed_event ? "Y" : "N" ) << "\n" ;
    //      t_temp_oss << "PLEntryType:     " << GetActivityStr ( price_level_activity ) << "\n" ;
    //      t_temp_oss << "PriceLevel:      " << ( uint32_t ) price_level << "\n" ;

    return t_temp_oss.str();
  }
};

struct NNSEDotexTradeExecRange {
  double high_exec_band;
  double low_exec_band;

  std::string ToString() {
    std::ostringstream t_temp_oss;
    t_temp_oss << "HighExecBand:       " << std::fixed << std::setprecision(6) << high_exec_band << "\n";
    t_temp_oss << "LowExecBand:        " << std::fixed << std::setprecision(6) << low_exec_band << "\n";
    return t_temp_oss.str();
  }
};

struct NSEDotexOfflineCommonStruct {
  struct timeval source_time;
  double strike_price;
  char symbol[NSE_DOTEX_OFFLINE_SYMBOL_LENGTH];
  uint64_t order_number;
  MsgType msg_type;
  char option_type[2];

  union {
    NSEDotexOrderDelta nse_dotex_order_delta;
    NSEDotexTrade nse_dotex_trade;
    NNSEDotexTradeExecRange nse_dotex_trade_range;

  } data;

  std::string ToString() {
    std::ostringstream t_temp_oss;

    switch (msg_type) {
      case MsgType::kNSEOrderDelta: {
        t_temp_oss << "\n=================== NSEBookDelta ===================\n\n";
        t_temp_oss << "Time:            " << source_time.tv_sec << "." << std::setw(6) << std::setfill('0')
                   << source_time.tv_usec << "\n";
        t_temp_oss << "OrderNum:        " << order_number << "\n";
        t_temp_oss << "Symbol:          " << symbol << "\n";
        t_temp_oss << "StrikePrice:     " << strike_price << "\n";
        t_temp_oss << "OptionType:      " << option_type << "\n";

        t_temp_oss << data.nse_dotex_order_delta.ToString() << "===================================================\n";

      } break;

      case MsgType::kNSETrade: {
        t_temp_oss << "\n=================== NSETrade ===================\n\n";
        t_temp_oss << "Time:            " << source_time.tv_sec << "." << std::setw(6) << std::setfill('0')
                   << source_time.tv_usec << "\n";
        t_temp_oss << "TradeNumber:     " << order_number << "\n";
        t_temp_oss << "Symbol:          " << symbol << "\n";
        t_temp_oss << "StrikePrice:     " << strike_price << "\n";
        t_temp_oss << "OptionType:      " << option_type << "\n";

        t_temp_oss << data.nse_dotex_trade.ToString() << "===================================================\n";

      } break;

      case MsgType::kNSETradeExecutionRange: {
        t_temp_oss << "\n=================== NSE Trade Execution Range ===================\n\n";
        t_temp_oss << "Time:            " << source_time.tv_sec << "." << std::setw(6) << std::setfill('0')
                   << source_time.tv_usec << "\n";
        t_temp_oss << "Symbol:          " << symbol << "\n";

        t_temp_oss << data.nse_dotex_trade_range.ToString() << "===================================================\n";

      } break;

      default: { t_temp_oss << "NOT_IMPLEMENTED_FOR_THIS_EVENT\n"; } break;
    }

    return t_temp_oss.str();
  }
};

struct NSETBTBookDelta {
  uint64_t order_id;
  int32_t order_price;
  int32_t order_qty;
  HFSAT::TradeType_t buysell;

  std::string ToString() {
    std::ostringstream t_temp_oss;
    t_temp_oss << "Orderid:             " << order_id << "\n";
    t_temp_oss << "Price:               " << std::fixed << std::setprecision(6) << order_price << "\n";
    t_temp_oss << "Qty:                 " << order_qty << "\n";
    t_temp_oss << "BuySell:             " << HFSAT::GetTradeTypeChar(buysell) << "\n";

    return t_temp_oss.str();
  }
};

struct NSETBTTrade {
  uint64_t buy_order_id;
  uint64_t sell_order_id;
  int32_t trade_price;
  int32_t trade_qty;

  std::string ToString() {
    std::ostringstream t_temp_oss;
    t_temp_oss << "BuyOrderId:         " << buy_order_id << "\n";
    t_temp_oss << "SellOrderId:        " << sell_order_id << "\n";
    t_temp_oss << "TradePrice:         " << std::fixed << std::setprecision(6) << trade_price << "\n";
    t_temp_oss << "TradeQty:           " << trade_qty << "\n";

    return t_temp_oss.str();
  }
};

struct NSETBTTradeExecRange {
  uint64_t high_exec_band;
  uint64_t low_exec_band;

  std::string ToString() {
    std::ostringstream t_temp_oss;
    t_temp_oss << "HighExecBand:       " << high_exec_band << "\n";
    t_temp_oss << "LowExecBand:        " << low_exec_band << "\n";
    return t_temp_oss.str();
  }
};

struct NSETBTDataCommonStruct {
  struct timeval source_time;
  int32_t token;
  int32_t msg_seq;
  MsgType msg_type;

  union {
    NSETBTBookDelta nse_order;
    NSETBTTrade nse_trade;
    NSETBTTradeExecRange nse_trade_range;
  } data;

  char activity_type;
  char segment_type;
  inline bool isTradeMsg() { return (MsgType::kNSESpreadTrade == msg_type || MsgType::kNSETrade == msg_type); }
  inline double GetTradeDoublePrice() { return data.nse_trade.trade_price; }
  inline HFSAT::TradeType_t GetTradeAggressorSide() { return HFSAT::TradeType_t::kTradeTypeNoInfo; }
  inline uint32_t GetTradeSize() { return data.nse_trade.trade_qty; }
  inline void SetIntermediate(bool flag) {
    // No intermediate here
  }
  std::string ToString() {
    std::ostringstream t_temp_oss;

    switch (msg_type) {
      case MsgType::kNSEOrderDelta: {
        t_temp_oss << "\n=================== NSEBookDelta ===================\n\n";
        t_temp_oss << "Time:            " << source_time.tv_sec << "." << std::setw(6) << std::setfill('0')
                   << source_time.tv_usec << "\n";
        t_temp_oss << "Symbol:          "
                   << HFSAT::Utils::NSEDailyTokenSymbolHandler::GetUniqueInstance(-1)
                          .GetInternalSymbolFromToken(token, segment_type) << "\n";
        t_temp_oss << "msgSeq :         " << msg_seq << "\n";
        t_temp_oss << "Activity :       " << activity_type << "\n";
        t_temp_oss << "Segment  :       " << segment_type << "\n";

        t_temp_oss << data.nse_order.ToString() << "===================================================\n";

      } break;

      case MsgType::kNSETrade: {
        t_temp_oss << "\n=================== NSETrade ===================\n\n";
        t_temp_oss << "Time:            " << source_time.tv_sec << "." << std::setw(6) << std::setfill('0')
                   << source_time.tv_usec << "\n";
        t_temp_oss << "Symbol:          "
                   << HFSAT::Utils::NSEDailyTokenSymbolHandler::GetUniqueInstance(-1)
                          .GetInternalSymbolFromToken(token, segment_type) << "\n";
        t_temp_oss << "msgSeq :         " << msg_seq << "\n";
        t_temp_oss << "Activity :       " << activity_type << "\n";
        t_temp_oss << "Segment  :       " << segment_type << "\n";

        t_temp_oss << data.nse_trade.ToString() << "===================================================\n";

      } break;

      case MsgType::kNSETradeExecutionRange: {
        t_temp_oss << "\n=================== NSETradeExecRange ===================\n\n";
        t_temp_oss << "Time:            " << source_time.tv_sec << "." << std::setw(6) << std::setfill('0')
                   << source_time.tv_usec << "\n";
        t_temp_oss << "Symbol:          "
                   << HFSAT::Utils::NSEDailyTokenSymbolHandler::GetUniqueInstance(-1)
                          .GetInternalSymbolFromToken(token, segment_type) << "\n";
        t_temp_oss << data.nse_trade_range.ToString() << "===================================================\n";
      } break;

      default: { t_temp_oss << "NOT_IMPLEMENTED_FOR_THIS_EVENT : " << (int32_t)msg_type << "\n"; } break;
    }

    return t_temp_oss.str();
  }

  // Assumption : Since there is not date information available here, It's not possible to extract symbol name from
  // token
  // However Assuming one might already have called a static function, It's time to take advantage of the fact that the
  // function
  // will anyways be called only once and hence we can pass any date but we'll have the same result
  char const* getContract() {
    return HFSAT::Utils::NSEDailyTokenSymbolHandler::GetUniqueInstance(-1)
        .GetInternalSymbolFromToken(token, segment_type);
  }
};
}

#endif  // BASE_CDEF_STORED_MARKET_DATA_COMMON_MESSAGE_STRUCTS_HPP
