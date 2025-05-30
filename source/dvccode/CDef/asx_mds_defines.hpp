/**
 \file dvccode/CDef/asx_mds_defines.hpp

 \author: (c) Copyright Two Roads Technological Solutions Pvt Ltd 2011
 Address:
 Suite 217, Level 2, Prestige Omega,
 No 104, EPIP Zone, Whitefield,
 Bangalore - 560066
 India
 +91 80 4060 0717
 */
#pragma once

#include <cstring>
#include <iomanip>
#include <inttypes.h>
#include <sstream>
#include <string>
#include <sys/time.h>
#include "dvccode/CDef/defines.hpp"
namespace ASX_MDS {

#define ASX_MDS_CONTRACT_TEXT_SIZE 12

enum ASXMsgType {
  kASXAdd = 'A',
  kASXModify = 'U',
  kASXVolDel = 'X',
  kASXDelete = 'D',
  kASXExec = 'E',
  kASXExecPx = 'C',

  kASXImpAdd = 'j',
  kASXImpModify = 'l',
  kASXImpDelete = 'k',
  kASXSpExec = 'e',
  kASXSpExecChain = 'P',

  kASXMktStatus = 'O',
  kASXEquiPx = 'Z',
  kASXOpenHighLow = 't',
  kASXSettlement = 'Y',
  kASXThreshold = 'W',
  kASXOpenInterest = 'V',
  kASXLoginAccepted = 'A',
  kASXLoginRejected = 'J',
  kASXRecoveryComplete = 'G'
};

struct ASXAdd {
  uint64_t order_id;
  double price;
  uint32_t size;
  uint32_t priority;
  char side;
  char flags[3];

  inline std::string ToString() {
    std::ostringstream temp_oss;
    temp_oss << "OrderId: " << order_id << "\n";
    temp_oss << "Price: " << std::fixed << std::setprecision(6) << price << "\n";
    temp_oss << "Size: " << size << "\n";
    temp_oss << "Priority: " << priority << "\n";
    temp_oss << "Side: " << side << "\n";
    return temp_oss.str();
  }
};

struct ASXModify {
  uint64_t order_id;
  double price;
  uint32_t size;
  uint32_t priority;
  char side;
  char flags[3];

  inline std::string ToString() {
    std::ostringstream temp_oss;
    temp_oss << "OrderId: " << order_id << "\n";
    temp_oss << "Price: " << std::fixed << std::setprecision(6) << price << "\n";
    temp_oss << "Size: " << size << "\n";
    temp_oss << "Priority: " << priority << "\n";
    temp_oss << "Side: " << side << "\n";
    return temp_oss.str();
  }
};

struct ASXVolumeDelete {
  uint64_t order_id;
  uint32_t size;
  char side;
  char flags[3];

  inline std::string ToString() {
    std::ostringstream temp_oss;
    temp_oss << "OrderId: " << order_id << "\n";
    temp_oss << "Size: " << size << "\n";
    temp_oss << "Side: " << side << "\n";
    return temp_oss.str();
  }
};

struct ASXDelete {
  uint64_t order_id;
  char side;
  char flags[3];

  inline std::string ToString() {
    std::ostringstream temp_oss;
    temp_oss << "OrderId: " << order_id << "\n";
    temp_oss << "Side: " << side << "\n";
    return temp_oss.str();
  }
};

struct ASXExec {
  uint64_t order_id;
  double exec_price;
  uint32_t size_exec;
  uint32_t size_remaining;
  uint32_t deal_num;
  char side;
  char trade_type;
  char flags[2];

  inline std::string ToString() {
    std::ostringstream temp_oss;
    temp_oss << "OrderId: " << order_id << "\n";
    temp_oss << "Price: " << std::fixed << std::setprecision(6) << exec_price << "\n";
    temp_oss << "SizeExec: " << size_exec << "\n";
    temp_oss << "SizeRemaining: " << size_remaining << "\n";
    temp_oss << "DealNum: " << deal_num << "\n";
    temp_oss << "Side: " << side << "\n";
    temp_oss << "TradeType: " << trade_type << "\n";
    return temp_oss.str();
  }
};

struct ASXExecWithPx {
  uint64_t buy_order_id;
  uint64_t sell_order_id;
  double exec_price;
  uint32_t size_exec;
  uint32_t buy_size_left;
  uint32_t sell_size_left;
  uint32_t deal_num;
  char trade_type;
  char flags[3];

  inline std::string ToString() {
    std::ostringstream temp_oss;
    temp_oss << "BuyOrderId: " << buy_order_id << "\n";
    temp_oss << "SellOrderId: " << sell_order_id << "\n";
    temp_oss << "Price: " << std::fixed << std::setprecision(6) << exec_price << "\n";
    temp_oss << "SizeExec: " << size_exec << "\n";
    temp_oss << "BuySizeLeft: " << buy_size_left << "\n";
    temp_oss << "SellSizeLeft: " << sell_size_left << "\n";
    temp_oss << "DealNum: " << deal_num << "\n";
    temp_oss << "TradeType: " << trade_type << "\n";
    return temp_oss.str();
  }
};

struct ASXSpExec {
  uint64_t order_id;
  double exec_price;
  uint32_t size_exec;
  uint32_t size_remaining;
  uint32_t deal_num;
  uint32_t traded_contract_number;
  double sp_trade_px;
  char side;
  char trade_type;
  char leg_trade_side;
  char printable;

  bool Printable() const { return printable == 'Y'; }

  inline std::string ToString() {
    std::ostringstream temp_oss;
    temp_oss << "OrderId: " << order_id << "\n";
    temp_oss << "Price: " << std::fixed << std::setprecision(6) << exec_price << "\n";
    temp_oss << "SizeExec: " << size_exec << "\n";
    temp_oss << "SizeRemaining: " << size_remaining << "\n";
    temp_oss << "DealNum: " << deal_num << "\n";
    temp_oss << "Side: " << side << "\n";
    temp_oss << "TradeType: " << trade_type << "\n";
    temp_oss << "TradedContract: " << traded_contract_number << "\n";
    temp_oss << "SpTradePx: " << sp_trade_px << "\n";
    temp_oss << "LegTradeSide: " << leg_trade_side << "\n";
    temp_oss << "Printable: " << printable << "\n";
    return temp_oss.str();
  }
};

struct ASXTradingStatus {
  char state;
  char flags[3];

  inline std::string ToString() {
    std::ostringstream temp_oss;
    temp_oss << "TradingStatus: " << state << "\n";
    return temp_oss.str();
  }
};

struct ASXEquilibriumPx {
  double price;
  double bid_price;
  double ask_price;
  uint32_t bid_qty;
  uint32_t ask_qty;

  inline std::string ToString() {
    std::ostringstream temp_oss;
    temp_oss << "Price: " << std::fixed << std::setprecision(6) << price << "\n";
    temp_oss << "BestBidPrice: " << std::fixed << std::setprecision(6) << bid_price << "\n";
    temp_oss << "BestAskPrice: " << std::fixed << std::setprecision(6) << ask_price << "\n";
    temp_oss << "BestBidQty: " << bid_qty << "\n";
    temp_oss << "BestAskQty: " << ask_qty << "\n";
    return temp_oss.str();
  }
};

struct ASXOpenHighLow {
  double open_price;
  double high_price;
  double low_price;
  double last_price;
  uint32_t last_volume;
  uint32_t total_volume;
  uint32_t total_trades;
  char market_updates;
  char flags[3];

  inline std::string ToString() {
    std::ostringstream temp_oss;
    temp_oss << "OpenPrice: " << open_price << "\n";
    temp_oss << "HighPrice: " << high_price << "\n";
    temp_oss << "LowPrice: " << low_price << "\n";
    temp_oss << "LastPrice: " << last_price << "\n";
    temp_oss << "LastVolume: " << last_volume << "\n";
    temp_oss << "TotalVolume: " << total_volume << "\n";
    temp_oss << "TotalTrades: " << total_trades << "\n";
    temp_oss << "MarketUpdates: " << market_updates << "\n";
    return temp_oss.str();
  }
};

struct ASXSettlement {
  double settlement_price;
  double volatility;
  char settlement_type;
  char flags[3];

  inline std::string ToString() {
    std::ostringstream temp_oss;
    temp_oss << "SettlementPrice: " << settlement_price << "\n";
    temp_oss << "Volatility: " << volatility << "\n";
    temp_oss << "SettlementType: " << settlement_type << "\n";
    return temp_oss.str();
  }
};

struct ASXThreshold {
  double aot_price;
  double aot_upper_price;
  double aot_lower_price;
  double etr_price;
  double etr_upper_price;
  double etr_lower_price;

  inline std::string ToString() {
    std::ostringstream temp_oss;
    temp_oss << "AOTPrice: " << aot_price << "\n";
    temp_oss << "AOTUpperPrice: " << aot_upper_price << "\n";
    temp_oss << "AOTLowerPrice: " << aot_lower_price << "\n";
    temp_oss << "ETRPrice: " << etr_price << "\n";
    temp_oss << "ETRUpperPrice: " << etr_upper_price << "\n";
    temp_oss << "ETRLowerPrice: " << etr_lower_price << "\n";
    return temp_oss.str();
  }
};

struct ASXVolOpenInterest {
  uint32_t total_volume;
  uint32_t open_interest;
  uint32_t date;

  inline std::string ToString() {
    std::ostringstream temp_oss;
    temp_oss << "TotalVolume: " << total_volume << "\n";
    temp_oss << "OpenInterest: " << open_interest << "\n";
    temp_oss << "Date: " << (int)date << "\n";
    return temp_oss.str();
  }
};

struct ASXCommonStruct {
  timeval time_;
  ASXMsgType msg_type;
  char contract[ASX_MDS_CONTRACT_TEXT_SIZE];
  union {
    ASXAdd add;
    ASXModify mod;
    ASXVolumeDelete vol_del;
    ASXDelete del;
    ASXExec exec;
    ASXExecWithPx exec_px;
    ASXAdd implied_add;
    ASXModify implied_mod;
    ASXDelete implied_del;
    ASXSpExec spread_exec;
    ASXSpExec exec_ch;  // Should be same as Exec
    ASXTradingStatus status;
    ASXEquilibriumPx equi_px;
    ASXOpenHighLow open_high_low;
    ASXSettlement settlement;
    ASXThreshold threshold;
    ASXVolOpenInterest open_interest;
  } data;

  char* getContract() { return contract; }

  inline void print() { fprintf(stderr, "%s\n", ToString().c_str()); }

  bool isTradeMsg() const {
    return (msg_type == kASXExec || msg_type == kASXExecPx ||
            (msg_type == kASXSpExec && data.spread_exec.Printable()) ||
            (msg_type == kASXSpExecChain && data.exec_ch.Printable()));
  }

  inline std::string ToString() {
    std::ostringstream temp_oss;
    temp_oss << "\n============== ASX Message ================\n\n";
    temp_oss << "Time: " << time_.tv_sec << "." << std::setw(6) << std::setfill('0') << time_.tv_usec << "\n";
    temp_oss << "Contract: " << contract << '\n';

    switch (msg_type) {
      case kASXAdd: {
        temp_oss << "MSGType: ADD\n";
        temp_oss << data.add.ToString();
        break;
      }
      case kASXModify: {
        temp_oss << "MSGType: MODIFY\n";
        temp_oss << data.mod.ToString();
        break;
      }
      case kASXVolDel: {
        temp_oss << "MSGType: VOLUME_DELETE\n";
        temp_oss << data.vol_del.ToString();
        break;
      }
      case kASXDelete: {
        temp_oss << "MSGType: DELETE\n";
        temp_oss << data.del.ToString();
        break;
      }
      case kASXExec: {
        temp_oss << "MSGType: EXEC\n";
        temp_oss << data.exec.ToString();
        break;
      }
      case kASXExecPx: {
        temp_oss << "MSGType: EXEC_WITH_PX_CHANGE\n";
        temp_oss << data.exec_px.ToString();
        break;
      }
      case kASXImpAdd: {
        temp_oss << "MSGType: IMP_ADD\n";
        temp_oss << data.implied_add.ToString();
        break;
      }
      case kASXImpModify: {
        temp_oss << "MSGType: IMP_MODIFY\n";
        temp_oss << data.implied_mod.ToString();
        break;
      }
      case kASXImpDelete: {
        temp_oss << "MSGType: IMP_DELETE\n";
        temp_oss << data.implied_del.ToString();
        break;
      }
      case kASXSpExec: {
        temp_oss << "MSGType: IMP_SP_EXEC\n";
        temp_oss << data.spread_exec.ToString();
        break;
      }
      case kASXSpExecChain: {
        temp_oss << "MSGType: SP_EXEC_CHAIN\n";
        temp_oss << data.exec_ch.ToString();
        break;
      }
      case kASXMktStatus: {
        temp_oss << "MSGType: TRADING_STATUS\n";
        temp_oss << data.status.ToString();
        break;
      }
      case kASXEquiPx: {
        temp_oss << "MSGType: EQUILIBRIUM_PRICE\n";
        temp_oss << data.equi_px.ToString();
        break;
      }
      case kASXOpenHighLow: {
        temp_oss << "MSGType: OPEN_HIGH_LOW\n";
        temp_oss << data.open_high_low.ToString();
        break;
      }
      case kASXSettlement: {
        temp_oss << "MSGType: SETTLEMENT\n";
        temp_oss << data.settlement.ToString();
        break;
      }
      case kASXThreshold: {
        temp_oss << "MSGType: THRESHOLD\n";
        temp_oss << data.threshold.ToString();
        break;
      }
      case kASXOpenInterest: {
        temp_oss << "MSGType: OPEN_INTEREST\n";
        temp_oss << data.open_interest.ToString();
        break;
      }
      default: { temp_oss << "msg_type:" << msg_type << " not found.\n"; }
    }

    temp_oss << "===================================================\n";
    return temp_oss.str();
  }
};

/**************************************************************************************************
 * PriceFeed Structs
 **************************************************************************************************/

enum PriceFeedmsgType { ASX_PF_DELTA = 1, ASX_PF_TRADE = 2, ASX_PF_RESET_BEGIN = 3, ASX_PF_RESET_END = 4 };

struct ASXPFDeltaStruct {
  char contract_[ASX_MDS_CONTRACT_TEXT_SIZE];
  double price_;
  uint64_t quantity_;
  int32_t num_orders_;
  uint32_t msg_seq_num_;
  uint32_t level_;
  uint8_t action_;  // 0 -> New, 1 -> Change, 2 -> Delete, 3 -> Reset, 4 -> Clear
  uint8_t side_;
  char intermediate_;

  inline std::string ToString() {
    std::ostringstream temp_oss;
    temp_oss << "Seq. No: " << msg_seq_num_ << "\n";
    temp_oss << "Contract: " << contract_ << "\n";
    temp_oss << "Side: " << char(side_) << "\n";
    switch (action_) {
      case '0':
        temp_oss << "Action: "
                 << "NEW"
                 << "\n";
        break;
      case '1':
        temp_oss << "Action: "
                 << "CHANGE"
                 << "\n";
        break;
      case '2':
        temp_oss << "Action: "
                 << "DELETE"
                 << "\n";
        break;
      case '3':
        temp_oss << "Action: "
                 << "RESET"
                 << "\n";
        break;
      case '4':
        temp_oss << "Action: "
                 << "CLEAR"
                 << "\n";
        break;
      default:
        temp_oss << "Action: "
                 << "------"
                 << "\n";
        break;
    }

    temp_oss << "Price: " << std::fixed << std::setprecision(6) << price_ << "\n";
    temp_oss << "Size: " << quantity_ << "\n";
    temp_oss << "Num Ord: " << num_orders_ << "\n";
    temp_oss << "Level: " << int(level_) << "\n";

    switch (intermediate_) {
      case 'Y':
        temp_oss << "Intermediate: Yes "
                 << "\n";
        break;
      case 'N':
        temp_oss << "Intermediate: No "
                 << "\n";
        break;
      case 'I':
        temp_oss << "Intermediate: I"
                 << "\n";
        break;
      default:
        temp_oss << "Intermediate: " << int(intermediate_) << "\n";
        break;
    }

    return temp_oss.str();
  }
};

struct ASXPFTradeStruct {
  char contract_[ASX_MDS_CONTRACT_TEXT_SIZE];
  double price_;
  uint64_t quantity_;
  uint32_t msg_seq_num_;
  uint8_t side_;
  uint8_t deal_type_;
  uint16_t deal_info_;

  inline std::string ToString() {
    std::ostringstream t_temp_oss_;
    t_temp_oss_ << "Seq. No: " << msg_seq_num_ << "\n";
    t_temp_oss_ << "Contract: " << contract_ << "\n";
    t_temp_oss_ << "Side: " << char(side_) << "\n";
    t_temp_oss_ << "Price: " << std::fixed << std::setprecision(6) << price_ << "\n";
    t_temp_oss_ << "Quantity: " << quantity_ << "\n";
    t_temp_oss_ << "Deal_type: " << int(deal_type_) << "\n";
    t_temp_oss_ << "Deal_info_: " << int(deal_info_) << "\n";

    return t_temp_oss_.str();
  }
};

struct ASXPFResetStruct {
  char contract_[ASX_MDS_CONTRACT_TEXT_SIZE];

  std::string ToString() {
    std::ostringstream t_temp_oss_;
    t_temp_oss_ << "Contract: " << contract_ << "\n";
    return t_temp_oss_.str();
  }
};

struct ASXPFCommonStruct {
  PriceFeedmsgType msg_;
  timeval time_;
  union {
    ASXPFDeltaStruct delta_;
    ASXPFTradeStruct trade_;
    ASXPFResetStruct reset_;
  } data_;
  inline void SetIntermediate(bool flag) {
    switch (msg_) {
      case ASX_PF_DELTA:
        data_.delta_.intermediate_ = flag;
        break;
      case ASX_PF_TRADE:
        break;
      case ASX_PF_RESET_BEGIN:
      case ASX_PF_RESET_END:
        break;
      default:
        break;
    }
  }
  char* getContract() {
    switch (msg_) {
      case ASX_PF_DELTA:
        return data_.delta_.contract_;
        break;
      case ASX_PF_TRADE:
        return data_.trade_.contract_;
        break;
      case ASX_PF_RESET_BEGIN:
      case ASX_PF_RESET_END:
        return data_.reset_.contract_;
        break;
      default:
        return NULL;
    }
    return NULL;
  }
  inline bool isTradeMsg() { return (ASX_PF_TRADE == msg_); }
  inline double GetTradeDoublePrice() { return data_.trade_.price_; }
  inline HFSAT::TradeType_t GetTradeAggressorSide() {
    return (data_.trade_.side_ == '1') ? HFSAT::TradeType_t::kTradeTypeBuy : (data_.trade_.side_ == '2')
                                                                                 ? HFSAT::TradeType_t::kTradeTypeSell
                                                                                 : HFSAT::TradeType_t::kTradeTypeNoInfo;
  }
  inline uint32_t GetTradeSize() { return data_.trade_.quantity_; }
  inline std::string ToString() {
    switch (msg_) {
      case ASX_PF_DELTA: {
        std::ostringstream temp_oss;
        temp_oss << "\n========= ASX Delta Message =========\n\n";
        temp_oss << "Time: " << time_.tv_sec << "." << std::setw(6) << std::setfill('0') << time_.tv_usec << "\n";
        return (temp_oss.str() + data_.delta_.ToString() + "=======================================================\n");
      } break;
      case ASX_PF_TRADE: {
        std::ostringstream temp_oss;
        temp_oss << "\n========== ASX Trade Message =============\n\n";
        temp_oss << "Time: " << time_.tv_sec << "." << std::setw(6) << std::setfill('0') << time_.tv_usec << "\n";
        return (temp_oss.str() + data_.trade_.ToString() + "=======================================================\n");
      } break;
      case ASX_PF_RESET_BEGIN: {
        std::ostringstream temp_oss;
        temp_oss << "\n========== ASX Reset Begin Message =============\n\n";
        temp_oss << "Time: " << time_.tv_sec << "." << std::setw(6) << std::setfill('0') << time_.tv_usec << "\n";
        return (temp_oss.str() + data_.reset_.ToString() + "=======================================================\n");
      } break;
      case ASX_PF_RESET_END: {
        std::ostringstream temp_oss;
        temp_oss << "\n========== ASX Reset End Message =============\n\n";
        temp_oss << "Time: " << time_.tv_sec << "." << std::setw(6) << std::setfill('0') << time_.tv_usec << "\n";
        return (temp_oss.str() + data_.reset_.ToString() + "=======================================================\n");

      } break;
      default: {
        std::ostringstream temp_oss;
        temp_oss << "NOT IMPLEMENTEND FOR this EVENT: " << msg_ << "\n";
        return temp_oss.str();
      } break;
    }
    return " ";
  }
};
}

namespace ASX_ITCH_MDS {
#define ASX_ITCH_MDS_CONTRACT_TEXT_SIZE 12

enum ASXOrderSide { kASXBid = 'B', kASXAsk = 'S', kASXSideUnknown = 'U' };

enum ASXItchMsgType {
  kASXAdd = 'A',
  kASXVolDel = 'X',
  kASXDelete = 'D',
  kASXExec = 'E',
  kASXExecPx = 'C',
  kASXCombExec = 'e',
  kASXResetBegin = '0',
  kASXResetEnd = '1',

  kASXImpliedAdd = 'j',
  kASXImpliedReplaced = 'l',
  kASXImpliedDelete = 'k',

  kASXMktStatus = 'O',
  kASXLoginAccepted = 'A',
  kASXLoginRejected = 'J',
  kASXRecoveryComplete = 'G',

  kASXInvalid = '7'
};

class ASXItchAdd {
 public:
  std::string ToString() const {
    std::ostringstream temp_oss;
    temp_oss << "OrderId: " << order_id << "\n";
    temp_oss << "Price: " << std::fixed << std::setprecision(6) << price << "\n";
    temp_oss << "Size: " << size << "\n";
    temp_oss << "Priority: " << priority << "\n";
    temp_oss << "Side: " << (char)side << "\n";
    return temp_oss.str();
  }

  uint64_t order_id;
  double price;
  uint32_t size;
  uint64_t priority;
  uint8_t side;
};

class ASXItchVolumeDelete {
 public:
  std::string ToString() const {
    std::ostringstream temp_oss;
    temp_oss << "OrderId: " << order_id << "\n";
    temp_oss << "Size: " << size << "\n";
    temp_oss << "Side: " << (char)side << "\n";
    return temp_oss.str();
  }

  uint64_t order_id;
  uint32_t size;
  uint8_t side;
};

class ASXItchDelete {
 public:
  std::string ToString() const {
    std::ostringstream temp_oss;
    temp_oss << "OrderId: " << order_id << "\n";
    temp_oss << "Side: " << (char)side << "\n";
    return temp_oss.str();
  }

  uint64_t order_id;
  uint8_t side;
};

class ASXItchExec {
 public:
  std::string ToString() const {
    std::ostringstream temp_oss;
    temp_oss << "OrderId: " << order_id << "\n";
    temp_oss << "Price: " << std::fixed << std::setprecision(6) << exec_price << "\n";
    temp_oss << "SizeExec: " << size_exec << "\n";
    temp_oss << "SizeRemaining: " << size_remaining << "\n";
    temp_oss << "DealNum: " << deal_num << "\n";
    temp_oss << "Side: " << (char)side << "\n";
    temp_oss << "TradeType: " << trade_type << "\n";
    temp_oss << "CombDealNum: " << combination_deal_num << "\n";
    return temp_oss.str();
  }

  uint64_t order_id;
  double exec_price;
  uint32_t size_exec;
  uint32_t size_remaining;
  uint64_t deal_num;
  uint64_t combination_deal_num;
  uint8_t side;
  char trade_type;
};

class ASXItchExecWithPx {
 public:
  std::string ToString() const {
    std::ostringstream temp_oss;
    temp_oss << "OrderId: " << order_id << "\n";
    temp_oss << "OppositeOrderId: " << opposite_order_id << "\n";
    temp_oss << "Side: " << (char)side << "\n";
    temp_oss << "Price: " << std::fixed << std::setprecision(6) << exec_price << "\n";
    temp_oss << "SizeExec: " << size_exec << "\n";
    temp_oss << "SizeLeft: " << size_remaining << "\n";
    temp_oss << "DealNum: " << deal_num << "\n";
    temp_oss << "TradeType: " << trade_type << "\n";
    return temp_oss.str();
  }

  uint64_t order_id;
  uint64_t opposite_order_id;
  uint64_t deal_num;
  double exec_price;
  uint32_t size_remaining;
  uint32_t size_exec;
  uint8_t side;
  char trade_type;
};

class ASXItchCombinationExec {
 public:
  std::string ToString() const {
    std::ostringstream temp_oss;
    temp_oss << "OrderId: " << order_id << "\n";
    temp_oss << "Price: " << std::fixed << std::setprecision(6) << exec_price << "\n";
    temp_oss << "SizeExec: " << size_exec << "\n";
    temp_oss << "SizeRemaining: " << size_remaining << "\n";
    temp_oss << "DealNum: " << deal_num << "\n";
    temp_oss << "Side: " << (char)side << "\n";
    temp_oss << "TradeType: " << trade_type << "\n";
    temp_oss << "CombDealNum: " << combination_deal_num << "\n";
    temp_oss << "Opposite OrderId: " << opposite_order_id << "\n";
    temp_oss << "Opposite Side: : " << (int)opposite_side << "\n";
    return temp_oss.str();
  }

  uint64_t order_id;
  uint64_t opposite_order_id;
  double exec_price;
  uint32_t size_exec;
  uint32_t size_remaining;
  uint64_t deal_num;
  uint64_t combination_deal_num;
  uint8_t side;
  uint8_t opposite_side;
  char trade_type;
};

class ASXItchTradingStatus {
 public:
  ASXItchTradingStatus(const ASXItchTradingStatus& status) { state = status.state; }
  void operator=(const ASXItchTradingStatus& status) { state = status.state; }

  std::string ToString() const {
    std::ostringstream temp_oss;
    temp_oss << "TradingStatus: " << state << "\n";
    return temp_oss.str();
  }

  char state;
};

class ASXItchOrder {
 public:
  ASXItchOrder() : time{0, 0}, seq_num(0), msg_type(kASXInvalid), is_intermediate(false) {}

  ASXItchOrder(const ASXItchOrder& order) {
    time = order.time;
    seq_num = order.seq_num;
    msg_type = order.msg_type;
    memcpy(contract, order.contract, ASX_ITCH_MDS_CONTRACT_TEXT_SIZE);

    switch (msg_type) {
      case kASXImpliedAdd:
      case kASXImpliedReplaced:
      case kASXAdd: {
        add = order.add;
        break;
      }

      case kASXVolDel: {
        vol_del = order.vol_del;
        break;
      }
      case kASXImpliedDelete:
      case kASXDelete: {
        del = order.del;
        break;
      }
      case kASXExec: {
        exec = order.exec;
        break;
      }
      case kASXExecPx: {
        exec_px = order.exec_px;
        break;
      }
      case kASXCombExec: {
        sp_exec = order.sp_exec;
        break;
      }

      case kASXMktStatus: {
        status = order.status;
        break;
      }
      default:
        break;
    }

    is_intermediate = order.is_intermediate;
  }

  void operator=(const ASXItchOrder& order) {
    time = order.time;
    seq_num = order.seq_num;
    msg_type = order.msg_type;
    memcpy(contract, order.contract, ASX_ITCH_MDS_CONTRACT_TEXT_SIZE);

    switch (msg_type) {
      case kASXImpliedAdd:
      case kASXImpliedReplaced:
      case kASXAdd: {
        add = order.add;
        break;
      }

      case kASXVolDel: {
        vol_del = order.vol_del;
        break;
      }
      case kASXImpliedDelete:
      case kASXDelete: {
        del = order.del;
        break;
      }
      case kASXExec: {
        exec = order.exec;
        break;
      }
      case kASXExecPx: {
        exec_px = order.exec_px;
        break;
      }

      case kASXCombExec: {
        sp_exec = order.sp_exec;
        break;
      }

      case kASXMktStatus: {
        status = order.status;
        break;
      }
      default:
        break;
    }

    is_intermediate = order.is_intermediate;
  }

  std::string ToString() const {
    std::ostringstream temp_oss;
    temp_oss << "\n============== ASX Message ================\n\n";
    temp_oss << "Time: " << time.tv_sec << "." << std::setw(6) << std::setfill('0') << time.tv_usec << "\n";
    temp_oss << "Contract: " << contract << '\n';
    temp_oss << "SeqNum: " << seq_num << '\n';

    switch (msg_type) {
      case kASXAdd: {
        temp_oss << "MSGType: ADD\n";
        temp_oss << add.ToString();
        break;
      }

      case kASXVolDel: {
        temp_oss << "MSGType: VOLUME_DELETE\n";
        temp_oss << vol_del.ToString();
        break;
      }
      case kASXDelete: {
        temp_oss << "MSGType: DELETE\n";
        temp_oss << del.ToString();
        break;
      }
      case kASXExec: {
        temp_oss << "MSGType: EXEC\n";
        temp_oss << exec.ToString();
        break;
      }
      case kASXExecPx: {
        temp_oss << "MSGType: EXEC_WITH_PX_CHANGE\n";
        temp_oss << exec_px.ToString();
        break;
      }

      case kASXCombExec: {
        temp_oss << "MSGType: COMBINATION_EXEC\n";
        temp_oss << sp_exec.ToString();
        break;
      }

      case kASXImpliedAdd: {
        temp_oss << "MSGType: IMPLIED ADD\n";
        temp_oss << add.ToString();
        break;
      }
      case kASXImpliedReplaced: {
        temp_oss << "MSGType: IMPLIED REPLACED\n";
        temp_oss << add.ToString();
        break;
      }
      case kASXImpliedDelete: {
        temp_oss << "MSGType: IMPLIED DELETE\n";
        temp_oss << del.ToString();
        break;
      }
      case kASXMktStatus: {
        temp_oss << "MSGType: TRADING_STATUS\n";
        temp_oss << status.ToString();
        break;
      }
      case kASXResetBegin: {
        temp_oss << "MSGType: RESET_BEGIN\n";
        break;
      }

      case kASXResetEnd: {
        temp_oss << "MSGType: RESET_END\n";
        break;
      }

      default: { temp_oss << "msg_type:" << msg_type << " not found.\n"; }
    }

    temp_oss << "Intermediate: " << is_intermediate << '\n';
    temp_oss << "===================================================\n";
    return temp_oss.str();
  }

  const char* getContract() const { return contract; }

  timeval time;
  uint64_t seq_num;
  ASXItchMsgType msg_type;
  char contract[ASX_ITCH_MDS_CONTRACT_TEXT_SIZE];
  union {
    ASXItchAdd add;
    ASXItchVolumeDelete vol_del;
    ASXItchDelete del;
    ASXItchExec exec;
    ASXItchExecWithPx exec_px;
    ASXItchCombinationExec sp_exec;
    ASXItchTradingStatus status;
  };

  bool is_intermediate;
};
}
