#ifndef _BSE_MDS_DEFINES_
#define _BSE_MDS_DEFINES_

#include <iomanip>
#include <inttypes.h>
#include <sstream>
#include <string>
#include <sys/time.h>
#include "dvccode/CDef/defines.hpp"
#define DEBUG 0

namespace BSE_MDS {

#define BSE_MDS_CONTRACT_TEXT_SIZE 12
#define BSE_DATA_FIELDS_COUNT 14
#define ISTAdjustmentFactor 19800  // IST is +5:30 UTC

enum BSEOrderSide { kBSEBid = 'B', kBSEAsk = 'S', kBSESideUnknown = 'U' };
enum BSEOrderKind { kLimit = 'L', kStopLoss = 'P', kMarket = 'G' };
enum BSEOrderResponse {
  kSuccessfull = 0,
  kDeleteForSelfTrade1 = 229,
  kDeleteForSelfTrade2 = 230,
  kDeleteForSelfTrade3 = 231,
  kStopLossTrigger = 288,
  kProvisionalAccept = 784,
  kMarketToLimit = 788,
  kIOCCancel = 789
};
enum class BSEOrderType {
  kAdd = 'A',
  kModify = 'U',
  kDelete = 'D',
  kTrade = 'T',
  kSystem = 'M',
  kComplexInstTrade = 'S'
};

struct BSEOrder {
 public:
  inline std::string ToString() {
    std::ostringstream temp_oss;
    temp_oss << "\n================= BSE Message =====================\n\n";
    temp_oss << "Date :\t\t" << date_ << "\n";
    temp_oss << "Time :\t\t" << time_.tv_sec << "." << std::setw(6) << std::setfill('0') << time_.tv_usec << "\n";
#if DEBUG
    temp_oss << "Time :\t\t" << time << "\n";
#endif
    temp_oss << "Order Id :\t" << order_id_ << "\n";
    temp_oss << "Instrument :\t" << instrument_code_ << "\n";
    switch (static_cast<BSEOrderSide>(side_)) {
      case BSEOrderSide::kBSEBid:
        temp_oss << "Side : \t\t"
                 << "B"
                 << "\n";
        break;
      case BSEOrderSide::kBSEAsk:
        temp_oss << "Side : \t\t"
                 << "S"
                 << "\n";
        break;
      default:
        temp_oss << "Side : \t\t"
                 << "Unknown"
                 << "\n";
        break;
    }
    temp_oss << "Price : \t" << std::fixed << std::setprecision(6) << price_ << "\n";
    temp_oss << "Size : \t\t" << total_size_ << "\n";
    temp_oss << "Revealed Size :\t" << revealed_size_ << "\n";
    temp_oss << "Pending Size :\t" << pending_size_ << "\n";
    temp_oss << "Cancelled Size : " << cancelled_size_ << "\n";
    switch (order_kind_) {
      case BSEOrderKind::kLimit:
        temp_oss << "Type : \t\t"
                 << "Limit"
                 << "\n";
        break;
      case BSEOrderKind::kMarket:
        temp_oss << "Type : \t\t"
                 << "Market"
                 << "\n";
        break;
      case BSEOrderKind::kStopLoss:
        temp_oss << "Type : \t\t"
                 << "Stop-Loss"
                 << "\n";
        break;
      default:
        temp_oss << "Type : \t\t"
                 << "Unknown"
                 << "\n";
        break;
    }
    switch (static_cast<BSEOrderType>(type_)) {
      case BSEOrderType::kAdd:
        temp_oss << "Action : \t"
                 << "ADD"
                 << "\n";
        break;
      case BSEOrderType::kModify:
        temp_oss << "Action : \t"
                 << "MODIFY"
                 << "\n";
        break;
      case BSEOrderType::kDelete:
        temp_oss << "Action : \t"
                 << "DELETE"
                 << "\n";
        break;
      case BSEOrderType::kTrade:
        temp_oss << "Action : \t"
                 << "TRADE"
                 << "\n";
        break;
      case BSEOrderType::kSystem:
        temp_oss << "Action : \t"
                 << "SYSTEM GENERATED RECORD"
                 << "\n";
        break;
      case BSEOrderType::kComplexInstTrade:
        temp_oss << "Action : \t"
                 << "COMPLEX INSTRUMENT TRADE"
                 << "\n";
        break;
      default:
        temp_oss << "Action : \t"
                 << "UNKNOWN"
                 << "\n";
        break;
    }
    switch (validity_) {
      case 0:
        temp_oss << "Validity : \t"
                 << "Good For Day"
                 << "\n";
        break;
      case 3:
        temp_oss << "Validity : \t"
                 << "IOC"
                 << "\n";
        break;
      case 4:
        temp_oss << "Validity : \t"
                 << "Good For Session"
                 << "\n";
        break;
      default:
        temp_oss << "Validity : \t"
                 << "Unknown"
                 << "\n";
        break;
    }
    temp_oss << "Response Code :\t" << response_code_ << "\n";
    temp_oss << "===================================================\n";
    return temp_oss.str();
  }
  char date_[11];
#if DEBUG
  char time[17];
#endif
  timeval time_;
  int instrument_code_;
  char side_;
  char order_kind_;
  double price_;
  int total_size_;
  int revealed_size_;
  int cancelled_size_;
  int pending_size_;
  int validity_;
  char type_;
  char order_id_[20];

  short int response_code_;
};

/**************************************************************************************************
 * PriceFeed Structs
 **************************************************************************************************/

enum PriceFeedmsgType { BSE_PF_DELTA = 1, BSE_PF_TRADE = 2 };

struct BSEPFDeltaStruct {
  char contract_[BSE_MDS_CONTRACT_TEXT_SIZE];
  double price_;
  uint64_t quantity_;
  int32_t num_orders_;
  uint32_t msg_seq_num_;
  uint32_t level_;
  uint8_t action_;  // 0 -> New, 1 -> Change, 2 -> Delete, 3 -> Reset, 4 -> Clear
  char side_;
  bool intermediate_;

  inline std::string ToString() {
    std::ostringstream temp_oss;
    temp_oss << "Seq. No: " << msg_seq_num_ << "\n";
    temp_oss << "Contract: " << contract_ << "\n";
    switch (static_cast<BSEOrderSide>(side_)) {
      case kBSEBid:
        temp_oss << "Side: B"
                 << "\n";
        break;
      case kBSEAsk:
        temp_oss << "Side: S"
                 << "\n";
        break;
      default:
        temp_oss << "Side: U"
                 << "\n";
        break;
    }
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

    if (intermediate_) {
      temp_oss << "Intermediate: Yes "
               << "\n";
    } else {
      temp_oss << "Intermediate: No "
               << "\n";
    }

    return temp_oss.str();
  }
};

struct BSEPFTradeStruct {
  char contract_[BSE_MDS_CONTRACT_TEXT_SIZE];
  double price_;
  uint64_t quantity_;
  uint32_t msg_seq_num_;
  char side_;
  uint8_t deal_type_;
  uint16_t deal_info_;
  bool intermediate_;

  inline std::string ToString() {
    std::ostringstream t_temp_oss_;
    t_temp_oss_ << "Seq. No: " << msg_seq_num_ << "\n";
    t_temp_oss_ << "Contract: " << contract_ << "\n";
    switch (static_cast<BSEOrderSide>(side_)) {
      case kBSEBid:
        t_temp_oss_ << "Side: B"
                    << "\n";
        break;
      case kBSEAsk:
        t_temp_oss_ << "Side: S"
                    << "\n";
        break;
      default:
        t_temp_oss_ << "Side: U"
                    << "\n";
        break;
    }
    t_temp_oss_ << "Price: " << std::fixed << std::setprecision(6) << price_ << "\n";
    t_temp_oss_ << "Quantity: " << quantity_ << "\n";
    t_temp_oss_ << "Deal_type: " << int(deal_type_) << "\n";
    t_temp_oss_ << "Deal_info_: " << int(deal_info_) << "\n";
    if (intermediate_) {
      t_temp_oss_ << "Intermediate: Yes "
                  << "\n";
    } else {
      t_temp_oss_ << "Intermediate: No "
                  << "\n";
    }

    return t_temp_oss_.str();
  }
};

struct BSEPFCommonStruct {
  PriceFeedmsgType msg_;
  timeval time_;
  union {
    BSEPFDeltaStruct delta_;
    BSEPFTradeStruct trade_;
  } data_;

  char* getContract() {
    switch (msg_) {
      case BSE_PF_DELTA:
        return data_.delta_.contract_;
        break;
      case BSE_PF_TRADE:
        return data_.trade_.contract_;
        break;
      default:
        return NULL;
    }
    return NULL;
  }

  inline std::string ToString() {
    switch (msg_) {
      case BSE_PF_DELTA: {
        std::ostringstream temp_oss;
        temp_oss << "\n========= BSE Delta Message =========\n\n";
        temp_oss << "Time: " << time_.tv_sec << "." << std::setw(6) << std::setfill('0') << time_.tv_usec << "\n";
        return (temp_oss.str() + data_.delta_.ToString() + "=======================================================\n");
      } break;
      case BSE_PF_TRADE: {
        std::ostringstream temp_oss;
        temp_oss << "\n========== BSE Trade Message =============\n\n";
        temp_oss << "Time: " << time_.tv_sec << "." << std::setw(6) << std::setfill('0') << time_.tv_usec << "\n";
        return (temp_oss.str() + data_.trade_.ToString() + "=======================================================\n");
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

namespace BSE_UDP_MDS {

struct BSERefData {
  int32_t token;
  int64_t expiry;
  double strike_price;
  char instrument[7];
  char symbol[11];
  char option_type[3];
  char exchange_symbol[32];
  double lower_price_range;
  double upper_price_range;
  double price_multiplier;
  HFSAT::ExchSource_t exch_source;

  BSERefData() : token(0), expiry(0), strike_price(-1), lower_price_range(0), upper_price_range(0) {
    memset(instrument, 0, sizeof(instrument));
    memset(symbol, 0, sizeof(symbol));
    memset(option_type, 0, sizeof(option_type));
    memset(exchange_symbol, 0, sizeof(exchange_symbol));
  }

  void Init(int32_t token_, int64_t expiry_, double strike_price_, const char *instrument_, const char *symbol_,
            const char *option_type_, const char *exchange_symbol_, double price_mul, HFSAT::ExchSource_t ex_src,
            double lower_px = 0, double upper_px = 0) {
    BSERefData();
    token = token_;
    expiry = expiry_;
    strike_price = strike_price_;
    memcpy(instrument, instrument_, sizeof(instrument) - 1);
    memcpy(symbol, symbol_, strlen(symbol_));
    memcpy(option_type, option_type_, sizeof(option_type) - 1);
    memcpy(exchange_symbol, exchange_symbol_, sizeof(exchange_symbol) - 1);
    price_multiplier = price_mul;
    exch_source = ex_src;
    lower_price_range = lower_px;
    upper_price_range = upper_px;
  }

  inline std::string ToString() {
    std::ostringstream t_temp_oss_;
    t_temp_oss_ << "token: " << token << "\n";
    t_temp_oss_ << "expiry: " << expiry << "\n";
    t_temp_oss_ << "strike_price: " << strike_price << "\n";
    t_temp_oss_ << "instrument: " << instrument << "\n";
    t_temp_oss_ << "symbol: " << symbol << "\n";
    t_temp_oss_ << "option_type: " << option_type << "\n";
    t_temp_oss_ << "exchange_symbol: " << exchange_symbol << "\n";
    t_temp_oss_ << "lower_price_range_: " << lower_price_range << "\n";
    t_temp_oss_ << "upper_price_range_: " << upper_price_range << "\n";
    return t_temp_oss_.str();
  }
};

#define BSE_BOOK_DEPTH 5
#define BSE_CONTRACT_LENGTH 32

enum PriceFeedmsgType {
  BSE_BOOK = 1,
  BSE_TRADE = 2,
  BSE_GENERAL = 3,
};

struct BSEBookStruct {
  double bid_prices[BSE_BOOK_DEPTH];
  int32_t bid_sizes[BSE_BOOK_DEPTH];
  int32_t bid_orders[BSE_BOOK_DEPTH];

  double ask_prices[BSE_BOOK_DEPTH];
  int32_t ask_sizes[BSE_BOOK_DEPTH];
  int32_t ask_orders[BSE_BOOK_DEPTH];
  int32_t closing_price;
  int32_t open_price;
  int32_t high_price;
  int32_t low_price;

  inline std::string ToString() {
    std::ostringstream t_temp_oss_;
    t_temp_oss_ << "CloseingPrice: " << closing_price << "\n";
    t_temp_oss_ << "OpeningPrice: " << open_price << "\n";
    t_temp_oss_ << "HighPrice: " << high_price << "\n";
    t_temp_oss_ << "LowPrice: " << low_price << "\n";

    uint32_t total_bid_size = 0;
    uint32_t total_bid_orders = 0;
    uint32_t total_ask_size = 0;
    uint32_t total_ask_orders = 0;

    for (unsigned int level_counter_ = 0; level_counter_ < BSE_BOOK_DEPTH; level_counter_++) {
      total_bid_size += bid_sizes[level_counter_];
      total_bid_orders += bid_orders[level_counter_];
      total_ask_size += ask_sizes[level_counter_];
      total_ask_orders += ask_orders[level_counter_];
    }

    t_temp_oss_ << "CumulativeBidData : " << total_bid_orders << " : " << total_bid_size << "\n";
    t_temp_oss_ << "CumulativeAskData : " << total_ask_orders << " : " << total_ask_size << "\n\n";

    for (unsigned int level_counter_ = 0; level_counter_ < BSE_BOOK_DEPTH; level_counter_++)
      t_temp_oss_ << "( " << bid_orders[level_counter_] << " ) "
                  << "( " << bid_sizes[level_counter_] << " ) "
                  << " @ " << bid_prices[level_counter_] << " ---- " << ask_prices[level_counter_] << " @ "
                  << " ( " << ask_sizes[level_counter_] << " ) "
                  << " ( " << ask_orders[level_counter_] << " )\n";

    return t_temp_oss_.str();
  }
};

struct BSETradeStruct {
  double price;
  int32_t size;

  inline std::string ToString() {
    std::ostringstream t_temp_oss_;
    t_temp_oss_ << "Price: " << std::fixed << std::setprecision(6) << price << "\n";
    t_temp_oss_ << "Size: " << size << "\n";
    return t_temp_oss_.str();
  }
};

struct BSEGeneralStruct {
  int16_t branch_num;
  char broker_num[6];
  char action_code[4];
  char bcast_msg[240];

  inline std::string ToString() {
    std::ostringstream t_temp_oss_;
    t_temp_oss_ << "Branch Num: " << branch_num << "\n";
    t_temp_oss_ << "Broker Num: " << broker_num << "\n";
    t_temp_oss_ << "Action Code: " << action_code << "\n";
    t_temp_oss_ << "Bcast Msg: " << bcast_msg << "\n";
    return t_temp_oss_.str();
  }
};

struct BSECommonStruct {
  PriceFeedmsgType msg_;
  int32_t seq_no;
  timeval source_time;
  int16_t status;
  int32_t token;
  int32_t token_2;  // second leg in case of spread
  char symbol[BSE_CONTRACT_LENGTH];

  union {
    BSEBookStruct book;
    BSETradeStruct trade;
    BSEGeneralStruct general;
  } data;

  const char *getContract() const {
    // These changes are for logging the files as "token" instead of file names
    std::ostringstream oss;
    oss << token;
    if (token_2 > 0) {
      oss << "_" << token_2;
    }
    return oss.str().c_str();
    // return std::string ( contract ).c_str();
  }

  inline std::string ToString() {
    std::ostringstream t_temp_oss_;
    switch (msg_) {
      case BSE_BOOK: {
        t_temp_oss_ << "\n========= BSE Book Message ==============\n";
        t_temp_oss_ << "SeqNum : " << seq_no << "\n";
        t_temp_oss_ << "Time : " << source_time.tv_sec << "." << source_time.tv_usec << "\n";
        t_temp_oss_ << "Status : " << status << "\n";
        t_temp_oss_ << "Token : " << token << "\n";
        t_temp_oss_ << "Token2 : " << token_2 << "\n";
        t_temp_oss_ << "Contract : " << symbol << "\n";
        return (t_temp_oss_.str() + data.book.ToString() + "=======================================================\n");
      } break;
      case BSE_TRADE: {
        t_temp_oss_ << "\n========== BSE Trade Message =============\n";
        t_temp_oss_ << "SeqNum : " << seq_no << "\n";
        t_temp_oss_ << "Time : " << source_time.tv_sec << "." << source_time.tv_usec << "\n";
        t_temp_oss_ << "Status : " << status << "\n";
        t_temp_oss_ << "Token : " << token << "\n";
        t_temp_oss_ << "Contract : " << symbol << "\n";
        return (t_temp_oss_.str() + data.trade.ToString() +
                "=======================================================\n");
      } break;
      case BSE_GENERAL: {
        t_temp_oss_ << "\n========== BSE General Message =============\n";
        t_temp_oss_ << "SeqNum : " << seq_no << "\n";
        t_temp_oss_ << "Time : " << source_time.tv_sec << "." << source_time.tv_usec << "\n";
        return (t_temp_oss_.str() + data.general.ToString() +
                "=======================================================\n");
      } break;
      default: {
        t_temp_oss_ << "NOT IMPLEMENTEND FOR this EVENT: " << msg_ << "\n";
        return t_temp_oss_.str();
      } break;
    }
    return " ";
  }
};
}


#endif
