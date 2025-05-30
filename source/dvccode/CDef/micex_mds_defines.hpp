/**
 \file dvccode/CDef/micex_mds_defines.hpp

 \author: (c) Copyright Two Roads Technological Solutions Pvt Ltd 2011
 Address:
 Suite 217, Level 2, Prestige Omega,
 No 104, EPIP Zone, Whitefield,
 Bangalore - 560066
 India
 +91 80 4060 0717
 */
#pragma once

#include <string>
#include <sstream>
#include <inttypes.h>
#include <sys/time.h>
#include <math.h>
#include "dvccode/CDef/defines.hpp"

/// MICEX market data messages
namespace MICEX_OF_MDS {
#define MICEX_MDS_CONTRACT_TEXT_SIZE 14

enum class MICEXOFMsgType {
  kMICEXAdd,
  kMICEXModify,
  kMICEXDelete,
  kMICEXExec,
  kMICEXBestLevelUpdate,
  kMICEXLastTradeInfo,
  kMICEXResetBegin,
  kMICEXResetEnd
};

struct MicexOFOrderInfo {
  double price;
  int size;
  uint8_t side;
  bool best_level_updated_;

  MicexOFOrderInfo() : price(0.0), size(-1), side('-'), best_level_updated_(false) {}

  MicexOFOrderInfo(double t_price, int t_size, uint8_t t_side, bool t_need_masking) {
    price = t_price;
    size = t_size;
    side = t_side;
    best_level_updated_ = t_need_masking;
  }
};

struct MicexOFLiveOrderInfo {
  double price;
  int size;
  uint8_t side;
  uint64_t msg_seq_number;
  uint64_t order_id;

  MicexOFLiveOrderInfo() : price(0.0), size(-1), side('-'), msg_seq_number(0), order_id(0) {}

  MicexOFLiveOrderInfo(double t_price, int t_size, uint8_t t_side, uint64_t t_msg_seq_number, uint64_t t_order_id) {
    price = t_price;
    size = t_size;
    side = t_side;
    msg_seq_number = t_msg_seq_number;
    order_id = t_order_id;
  }
};

struct MICEXBestLevelUpdate {
  int best_bid_size;
  int best_ask_size;
  int last_trade_size;
  double best_bid_price;
  double best_ask_price;
  double last_trade_price;

  bool HasL1BidSideInfo() { return (best_bid_size != -1); }

  bool HasL1AskSideInfo() { return (best_ask_size != -1); }

  bool HasLastTradePriceInfo() { return (last_trade_size != -1); }

  inline std::string ToString() const {
    std::ostringstream temp_oss;
    if (best_bid_size != -1) {
      temp_oss << "BestBidSize : " << best_bid_size << "\n";
      temp_oss << "BestBidPrice : " << std::fixed << std::setprecision(6) << best_bid_price << "\n";
    }
    if (best_ask_size != -1) {
      temp_oss << "BestAskSize : " << best_ask_size << "\n";
      temp_oss << "BestAskPrice : " << std::fixed << std::setprecision(6) << best_ask_price << "\n";
    }
    if (last_trade_size != -1) {
      temp_oss << "LastTradeSize : " << last_trade_size << "\n";
      temp_oss << "LastTradePrice : " << std::fixed << std::setprecision(6) << last_trade_price << "\n";
    }
    return temp_oss.str();
  }

  void Reset() {
    best_bid_size = -1;
    best_ask_size = -1;
    last_trade_size = -1;
    best_bid_price = -1.0;
    best_ask_price = -1.0;
    last_trade_price = -1.0;
  }
};

struct MICEXReset {
  int64_t packet_num_;
  int64_t seq_num;
};

struct MICEXAdd {
  int64_t packet_num_;
  int64_t seq_num;
  uint64_t order_id;
  double price;
  uint32_t size;
  char side;

  inline std::string ToString() const {
    std::ostringstream temp_oss;
    temp_oss << "OrderId: " << order_id << "\n";
    temp_oss << "Price: " << std::fixed << std::setprecision(6) << price << "\n";
    temp_oss << "Size: " << size << "\n";
    temp_oss << "Side: " << side << "\n";
    return temp_oss.str();
  }
};

struct MICEXModify {
  int64_t packet_num_;
  int64_t seq_num;
  uint64_t order_id;
  double price;
  uint32_t size;
  char side;

  inline std::string ToString() const {
    std::ostringstream temp_oss;
    temp_oss << "OrderId: " << order_id << "\n";
    temp_oss << "Price: " << std::fixed << std::setprecision(6) << price << "\n";
    temp_oss << "Size: " << size << "\n";
    temp_oss << "Side: " << side << "\n";
    return temp_oss.str();
  }
};

struct MICEXDelete {
  int64_t packet_num_;
  int64_t seq_num;
  uint64_t order_id;
  char side;

  inline std::string ToString() const {
    std::ostringstream temp_oss;
    temp_oss << "OrderId: " << order_id << "\n";
    temp_oss << "Side: " << side << "\n";
    return temp_oss.str();
  }
};

struct MICEXExec {
  int64_t packet_num_;
  int64_t seq_num;
  uint64_t order_id;
  uint32_t size_exec;
  double price;
  char side;

  inline std::string ToString() const {
    std::ostringstream temp_oss;
    temp_oss << "OrderId: " << order_id << "\n";
    temp_oss << "SizeExec: " << size_exec << "\n";
    temp_oss << "Price: " << std::fixed << std::setprecision(6) << price << "\n";
    temp_oss << "Side: " << side << "\n";
    return temp_oss.str();
  }
};

struct MICEXOFCommonStruct {
  timeval time_;
  MICEXOFMsgType msg_type;
  bool intermediate_;
  bool is_slower_;
  uint64_t exchange_time_stamp;
  char contract[MICEX_MDS_CONTRACT_TEXT_SIZE];
  union {
    MICEXAdd add;
    MICEXModify mod;
    MICEXDelete del;
    MICEXExec exec;
    MICEXBestLevelUpdate best_level_update;
    MICEXReset reset;
  } data;

  const char* getContract() const { return contract; }

  int64_t getMsgSequenceNumber() const {
    switch (msg_type) {
      case MICEX_OF_MDS::MICEXOFMsgType::kMICEXAdd: {
        return data.add.seq_num;
        break;
      }
      case MICEX_OF_MDS::MICEXOFMsgType::kMICEXDelete: {
        return data.del.seq_num;
        break;
      }
      case MICEX_OF_MDS::MICEXOFMsgType::kMICEXModify: {
        return data.mod.seq_num;
        break;
      }
      case MICEX_OF_MDS::MICEXOFMsgType::kMICEXExec: {
        return data.exec.seq_num;
        break;
      }
      case MICEX_OF_MDS::MICEXOFMsgType::kMICEXBestLevelUpdate: {
        return -1;
      } break;
      case MICEX_OF_MDS::MICEXOFMsgType::kMICEXResetBegin: {
        return data.reset.seq_num;
      } break;
      case MICEX_OF_MDS::MICEXOFMsgType::kMICEXResetEnd: {
        return data.reset.seq_num;
      } break;
      default: {
        return -1;
        break;
      }
    }
  }

  inline void print() { fprintf(stderr, "%s\n", ToString().c_str()); }

  bool isMSRMsg() {
    return ((msg_type == MICEXOFMsgType::kMICEXBestLevelUpdate) || (msg_type == MICEXOFMsgType::kMICEXLastTradeInfo));
  }

  bool isDeltaMsg() {
    return ((msg_type == MICEXOFMsgType::kMICEXAdd) || (msg_type == MICEXOFMsgType::kMICEXDelete) ||
            (msg_type == MICEXOFMsgType::kMICEXModify));
  }

  bool isTradeMsg() const { return (msg_type == MICEXOFMsgType::kMICEXExec); }

  inline double GetTradeDoublePrice() { return data.exec.price; }
  inline HFSAT::TradeType_t GetTradeAggressorSide() {
    return (data.exec.side == 'B') ? HFSAT::TradeType_t::kTradeTypeBuy : (data.exec.side == 'S')
                                                                             ? HFSAT::TradeType_t::kTradeTypeSell
                                                                             : HFSAT::TradeType_t::kTradeTypeNoInfo;
  }
  inline uint32_t GetTradeSize() { return data.exec.size_exec; }
  inline std::string ToString() const {
    uint64_t ten_power_6 = 1000000LL;
    std::ostringstream temp_oss;
    temp_oss << "\n============== MICEX Message ================\n\n";
    temp_oss << "OurTimeStamp: " << time_.tv_sec << "." << std::setw(6) << std::setfill('0') << time_.tv_usec << "\n";
    temp_oss << "ExchTimeStamp: " << (exchange_time_stamp / ten_power_6) << "." << std::setw(6) << std::setfill('0')
             << (exchange_time_stamp % ten_power_6) << "\n";
    temp_oss << "Contract: " << contract << '\n';
    temp_oss << "Intermediate: " << intermediate_ << '\n';
    temp_oss << "Is_Slower:" << is_slower_ << '\n';

    switch (msg_type) {
      case MICEXOFMsgType::kMICEXAdd: {
        temp_oss << "PacketNum: " << data.add.packet_num_ << '\n';
        temp_oss << "MsgSeqNum: " << data.add.seq_num << '\n';
        temp_oss << "MSGType: ADD\n";
        temp_oss << data.add.ToString();
        break;
      }
      case MICEXOFMsgType::kMICEXDelete: {
        temp_oss << "PacketNum: " << data.del.packet_num_ << '\n';
        temp_oss << "MsgSeqNum: " << data.del.seq_num << '\n';
        temp_oss << "MSGType: DELETE\n";
        temp_oss << data.del.ToString();
        break;
      }
      case MICEXOFMsgType::kMICEXModify: {
        temp_oss << "PacketNum: " << data.mod.packet_num_ << '\n';
        temp_oss << "MsgSeqNum: " << data.mod.seq_num << '\n';
        temp_oss << "MSGType: MODIFY\n";
        temp_oss << data.mod.ToString();
        break;
      }
      case MICEXOFMsgType::kMICEXExec: {
        temp_oss << "PacketNum: " << data.exec.packet_num_ << '\n';
        temp_oss << "MsgSeqNum: " << data.exec.seq_num << '\n';
        temp_oss << "MSGType: EXEC\n";
        temp_oss << data.exec.ToString();
        break;
      }
      case MICEXOFMsgType::kMICEXBestLevelUpdate: {
        temp_oss << "MSGType: BEST_LEVEL_UPDATE\n";
        temp_oss << data.best_level_update.ToString();
      } break;
      case MICEXOFMsgType::kMICEXResetBegin: {
        temp_oss << "PacketNum: " << data.reset.packet_num_ << '\n';
        temp_oss << "MsgSeqNum: " << data.reset.seq_num << '\n';
        temp_oss << "MSGType: RESET_BEGIN\n";
        break;
      }
      case MICEXOFMsgType::kMICEXResetEnd: {
        temp_oss << "PacketNum: " << data.reset.packet_num_ << '\n';
        temp_oss << "MsgSeqNum: " << data.reset.seq_num << '\n';
        temp_oss << "MSGType: RESET_END\n";
        break;
      }

      default: { temp_oss << "msg_type:" << static_cast<int>(msg_type) << " not found.\n"; }
    }
    temp_oss << "===================================================\n";
    return temp_oss.str();
  }
};
}
