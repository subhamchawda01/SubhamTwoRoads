/**
 \file dvccode/CDef/rts_mds_defines.hpp

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

/// RTS market data messages
namespace RTS_MDS {
#define RTS_MDS_CONTRACT_TEXT_SIZE 12

enum class RTSOFMsgType { kRTSAdd, kRTSDelete, kRTSExec, kRTSDeleteAll, kRTSResetBegin, kRTSResetEnd };

struct RTSAdd {
  uint64_t order_id;
  double price;
  uint32_t size;
  char side;

  inline std::string ToString() {
    std::ostringstream temp_oss;
    temp_oss << "OrderId: " << order_id << "\n";
    temp_oss << "Price: " << std::fixed << std::setprecision(6) << price << "\n";
    temp_oss << "Size: " << size << "\n";
    temp_oss << "Side: " << side << "\n";
    return temp_oss.str();
  }
};

struct RTSDelete {
  uint64_t order_id;
  char side;

  inline std::string ToString() {
    std::ostringstream temp_oss;
    temp_oss << "OrderId: " << order_id << "\n";
    temp_oss << "Side: " << side << "\n";
    return temp_oss.str();
  }
};

struct RTSExec {
  uint64_t order_id;
  uint32_t size_exec;
  double price;
  char side;

  inline std::string ToString() {
    std::ostringstream temp_oss;
    temp_oss << "OrderId: " << order_id << "\n";
    temp_oss << "SizeExec: " << size_exec << "\n";
    temp_oss << "Price: " << std::fixed << std::setprecision(6) << price << "\n";
    temp_oss << "Side: " << side << "\n";
    return temp_oss.str();
  }
};

struct RTSOFCommonStruct {
  timeval time_;
  RTSOFMsgType msg_type;
  int64_t seq_num;
  int64_t md_flags;
  char contract[RTS_MDS_CONTRACT_TEXT_SIZE];
  union {
    RTSAdd add;
    RTSDelete del;
    RTSExec exec;
  } data;

  char* getContract() { return contract; }

  inline void print() { fprintf(stderr, "%s\n", ToString().c_str()); }

  bool isTradeMsg() const { return (msg_type == RTSOFMsgType::kRTSExec); }

  inline std::string ToString() {
    std::string mdflag_str("");

    if (md_flags & 0x01) {
      mdflag_str += "Day";
    }
    if (md_flags & 0x02) {
      mdflag_str += ", IOC";
    }
    if (md_flags & 0x04) {
      mdflag_str += ", OTC";
    }
    if (md_flags & 0x8) {
      mdflag_str += ", PosTransferTrade";
    }
    if (md_flags & 0x10) {
      mdflag_str += ", Collateral";
    }
    if (md_flags & 0x20) {
      mdflag_str += ", OptionExerciseTrade";
    }
    if (md_flags & 0x80) {
      mdflag_str += ", InstExpiration";
    }
    if (md_flags & 0x1000) {
      mdflag_str += ", EndOfTransaction";
    }
    if (md_flags & 0x20000) {
      mdflag_str += ", RepoTrade";
    }
    if (md_flags & 0x40000) {
      mdflag_str += ", SeriesOfTrade";
    }
    if (md_flags & 0x100000) {
      mdflag_str += ", DueToModify";
    }
    if (md_flags & 0x200000) {
      mdflag_str += ", DueToCancel";
    }
    if (md_flags & 0x400000) {
      mdflag_str += ", DueToMassCancel";
    }
    if (md_flags & 0x800000) {
      mdflag_str += ", OptionExpirationTrade";
    }
    if (md_flags & 0x2000000) {
      mdflag_str += ", ClearingSessionTrade";
    }
    if (md_flags & 0x4000000) {
      mdflag_str += ", NegotiatedTrade";
    }
    if (md_flags & 0x8000000) {
      mdflag_str += ", MultilegTrade";
    }
    if (md_flags & 0x10000000) {
      mdflag_str += ", TradeOnNonDelivery";
    }
    if (md_flags & 0x20000000) {
      mdflag_str += ", CancellingDueToCrossTrade";
    }
    if (md_flags & 0x00080000) {
      mdflag_str += ", FOK";
    }
    if (md_flags & 0x100000000) {
      mdflag_str += ", COD";
    }
    if (md_flags & 0x40000000) {
      mdflag_str += ", FuturesExerciseTrade";
    }

    std::ostringstream temp_oss;
    temp_oss << "\n============== RTS Message ================\n\n";
    temp_oss << "Time: " << time_.tv_sec << "." << std::setw(6) << std::setfill('0') << time_.tv_usec << "\n";
    temp_oss << "Contract: " << contract << '\n';
    temp_oss << "SeqNum: " << seq_num << '\n';
    temp_oss << "MDFlags: " << md_flags << " " << mdflag_str << '\n';

    switch (msg_type) {
      case RTSOFMsgType::kRTSAdd: {
        temp_oss << "MSGType: ADD\n";
        temp_oss << data.add.ToString();
        break;
      }
      case RTSOFMsgType::kRTSDelete: {
        temp_oss << "MSGType: DELETE\n";
        temp_oss << data.del.ToString();
        break;
      }
      case RTSOFMsgType::kRTSExec: {
        temp_oss << "MSGType: EXEC\n";
        temp_oss << data.exec.ToString();
        break;
      }
      case RTSOFMsgType::kRTSDeleteAll: {
        temp_oss << "MSGType: DELALL\n";
        break;
      }
      default: { temp_oss << "msg_type:" << static_cast<int>(msg_type) << " not found.\n"; }
    }
    temp_oss << "===================================================\n";
    return temp_oss.str();
  }
};

struct RTSOFCommonStructv2 {
  timeval time_;
  RTSOFMsgType msg_type;
  int64_t seq_num;
  int64_t packet_num;
  int64_t md_flags;
  char contract[RTS_MDS_CONTRACT_TEXT_SIZE];
  uint64_t order_id;
  double price;
  uint32_t size;
  char side;
  bool is_full_exec;  // False means partial exec
  bool is_intermediate;

  char* getContract() { return contract; }

  inline void print() { fprintf(stderr, "%s\n", ToString().c_str()); }

  bool isTradeMsg() const { return (msg_type == RTSOFMsgType::kRTSExec); }
  inline double GetTradeDoublePrice() { return price; }
  inline HFSAT::TradeType_t GetTradeAggressorSide() {
    return (side == 'B') ? HFSAT::TradeType_t::kTradeTypeBuy : (side == 'S') ? HFSAT::TradeType_t::kTradeTypeSell
                                                                             : HFSAT::TradeType_t::kTradeTypeNoInfo;
  }
  inline uint32_t GetTradeSize() { return size; }
  inline void SetIntermediate(bool flag) {
    switch (msg_type) {
      case RTSOFMsgType::kRTSAdd: {
        break;
      }
      case RTSOFMsgType::kRTSDelete: {
        break;
      }
      case RTSOFMsgType::kRTSExec: {
        break;
      }
      case RTSOFMsgType::kRTSDeleteAll: {
        break;
      }
      default:
        break;
    }
  }
  inline std::string ToString() {
    std::string mdflag_str("");

    if (md_flags & 0x01) {
      mdflag_str += "Day";
    }
    if (md_flags & 0x02) {
      mdflag_str += ", IOC";
    }
    if (md_flags & 0x04) {
      mdflag_str += ", OTC";
    }
    if (md_flags & 0x8) {
      mdflag_str += ", PosTransferTrade";
    }
    if (md_flags & 0x10) {
      mdflag_str += ", Collateral";
    }
    if (md_flags & 0x20) {
      mdflag_str += ", OptionExerciseTrade";
    }
    if (md_flags & 0x80) {
      mdflag_str += ", InstExpiration";
    }
    if (md_flags & 0x1000) {
      mdflag_str += ", EndOfTransaction";
    }
    if (md_flags & 0x20000) {
      mdflag_str += ", RepoTrade";
    }
    if (md_flags & 0x40000) {
      mdflag_str += ", SeriesOfTrade";
    }
    if (md_flags & 0x100000) {
      mdflag_str += ", DueToModify";
    }
    if (md_flags & 0x200000) {
      mdflag_str += ", DueToCancel";
    }
    if (md_flags & 0x400000) {
      mdflag_str += ", DueToMassCancel";
    }
    if (md_flags & 0x800000) {
      mdflag_str += ", OptionExpirationTrade";
    }
    if (md_flags & 0x2000000) {
      mdflag_str += ", ClearingSessionTrade";
    }
    if (md_flags & 0x4000000) {
      mdflag_str += ", NegotiatedTrade";
    }
    if (md_flags & 0x8000000) {
      mdflag_str += ", MultilegTrade";
    }
    if (md_flags & 0x10000000) {
      mdflag_str += ", TradeOnNonDelivery";
    }
    if (md_flags & 0x20000000) {
      mdflag_str += ", CancellingDueToCrossTrade";
    }
    if (md_flags & 0x00080000) {
      mdflag_str += ", FOK";
    }
    if (md_flags & 0x100000000) {
      mdflag_str += ", COD";
    }
    if (md_flags & 0x40000000) {
      mdflag_str += ", FuturesExerciseTrade";
    }

    std::ostringstream temp_oss;
    temp_oss << "\n============== RTS Message ================\n\n";
    temp_oss << "Time: " << time_.tv_sec << "." << std::setw(6) << std::setfill('0') << time_.tv_usec << "\n";
    temp_oss << "Contract: " << contract << '\n';
    temp_oss << "PacketNum: " << packet_num << '\n';
    temp_oss << "SeqNum: " << seq_num << '\n';
    temp_oss << "MDFlags: " << md_flags << " " << mdflag_str << '\n';

    switch (msg_type) {
      case RTSOFMsgType::kRTSAdd: {
        temp_oss << "MSGType: ADD\n";
        break;
      }
      case RTSOFMsgType::kRTSDelete: {
        temp_oss << "MSGType: DELETE\n";
        break;
      }
      case RTSOFMsgType::kRTSExec: {
        temp_oss << "MSGType: EXEC\n";
        break;
      }
      case RTSOFMsgType::kRTSDeleteAll: {
        temp_oss << "MSGType: DELALL\n";
        break;
      }
      case RTSOFMsgType::kRTSResetBegin: {
        temp_oss << "MSGType: RESET_BEGIN\n";
        break;
      }
      case RTSOFMsgType::kRTSResetEnd: {
        temp_oss << "MSGType: RESET_END\n";
        break;
      }
      default: { temp_oss << "msg_type:" << static_cast<int>(msg_type) << " not found.\n"; }
    }
    temp_oss << "OrderId: " << order_id << "\n";
    temp_oss << "Size: " << size << "\n";
    temp_oss << "Price: " << price << "\n";
    temp_oss << "Side: " << side << "\n";
    temp_oss << "IsFullExec: " << is_full_exec << "\n";
    temp_oss << "Intermediate: " << is_intermediate << '\n';
    temp_oss << "===================================================\n";
    return temp_oss.str();
  }
};
}
