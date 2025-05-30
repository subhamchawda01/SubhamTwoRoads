#ifndef _SGX_MDS_DEFINES_
#define _SGX_MDS_DEFINES_

#include <cstring>
#include <iomanip>
#include <inttypes.h>
#include <sstream>
#include <string>
#include <sys/time.h>
#include "dvccode/CDef/defines.hpp"

namespace SGX_MDS {

#define SGX_ITCH_MDS_CONTRACT_TEXT_SIZE 16
#define SGX_MDS_CONTRACT_TEXT_SIZE 16
#define SGX_DATA_FIELDS_COUNT 14

enum SGXOrderType { kSGXAdd = 0, kSGXDelete = 1, kSGXChange = 2 };
enum SGXOrderSide { kSGXBid = 1, kSGXAsk = 2, kSGXSideUnknown = 3 };

class SGXOrder {
 public:
  SGXOrder()
      : seq_num(-1),
        priority(-1),
        diff_size(-1),
        order_type(-1),
        change_reason(-1),
        price(-1),
        side(-1),
        is_intermediate(false) {}

  SGXOrder(const SGXOrder& other) {
    date = other.date;
    std::strcpy(series, other.series);
    series[SGX_MDS_CONTRACT_TEXT_SIZE - 1] = '\0';
    time = other.time;
    seq_num = other.seq_num;
    priority = other.priority;
    diff_size = other.diff_size;
    order_type = other.order_type;
    change_reason = other.change_reason;
    order_id = other.order_id;
    price = other.price;
    side = other.side;  // Bid (1), Ask(2)
    is_intermediate = other.is_intermediate;
  }

  char* getContract() { return series; }

  inline std::string ToString() {
    std::ostringstream temp_oss;
    temp_oss << "\n================= SGX Message =====================\n\n";
    temp_oss << "Time:\t\t" << time.tv_sec << "." << std::setw(6) << std::setfill('0') << time.tv_usec << "\n";
    temp_oss << "Order Id:\t" << order_id << "\n";

    // temp_oss << "Date:\t" << date << "\n";
    temp_oss << "Seq Number:\t" << seq_num << "\n";
    temp_oss << "Series:\t\t" << series << "\n";
    switch (order_type) {
      case kSGXAdd:
        temp_oss << "Msge Type:\t"
                 << "ADD"
                 << "\n";
        break;
      case kSGXDelete:
        temp_oss << "Msge Type:\t"
                 << "DELETE"
                 << "\n";
        break;
      case kSGXChange:
        temp_oss << "Msge Type:\t"
                 << "CHANGE"
                 << "\n";
        break;
      default:
        temp_oss << "Msge Type:\t"
                 << "Unknown"
                 << "\n";
        break;
    }
    switch (side) {
      case kSGXBid:
        temp_oss << "Side:\t\t"
                 << "B"
                 << "\n";
        break;
      case kSGXAsk:
        temp_oss << "Side:\t\t"
                 << "S"
                 << "\n";
        break;
      default: {
        temp_oss << "Side:\t\t"
                 << "Unknown"
                 << "\n";
        break;
      }
    }
    temp_oss << "Price:\t\t" << std::fixed << std::setprecision(6) << price << "\n";
    temp_oss << "Size Increase:\t" << diff_size << "\n";
    switch (change_reason) {
      case 1:
        temp_oss << "Reason:\t\t"
                 << "DELETED"
                 << "\n";
        break;
      case 3:
        temp_oss << "Reason:\t\t"
                 << "TRADED"
                 << "\n";
        break;
      case 5:
        temp_oss << "Reason:\t\t"
                 << "AMENDED"
                 << "\n";
        break;
      case 6:
        temp_oss << "Reason:\t\t"
                 << "ADDED or ACTIVATED"
                 << "\n";
        break;
      case 19:
        temp_oss << "Reason:\t\t"
                 << "Good-For-Day Orders Deleted"
                 << "\n";
        break;
      case 30:
        temp_oss << "Reason:\t\t"
                 << "Good-To-Cancel Orders Reloaded"
                 << "\n";
        break;
      default:
        temp_oss << "Reason:\t\t"
                 << "Unknown"
                 << "\n";
        break;
    }
    temp_oss << "Priority:\t" << priority << "\n";
    temp_oss << "Intermediate:\t" << is_intermediate << "\n";
    temp_oss << "===================================================\n";
    return temp_oss.str();
  }

  uint32_t date;
  char series[SGX_MDS_CONTRACT_TEXT_SIZE];
  timeval time;
  uint32_t seq_num;
  uint32_t priority;
  int32_t diff_size;
  uint8_t order_type;
  uint8_t change_reason;
  uint64_t order_id;
  double price;
  uint8_t side;  // Bid (1), Ask(2)
  bool is_intermediate;
};

/**************************************************************************************************
 * PriceFeed Structs
 **************************************************************************************************/

enum PriceFeedmsgType { SGX_PF_DELTA = 1, SGX_PF_TRADE = 2, SGX_PF_RESET_BEGIN = 3, SGX_PF_RESET_END = 4 };

struct SGXPFDeltaStruct {
  char contract_[SGX_MDS_CONTRACT_TEXT_SIZE];
  double price_;
  uint64_t quantity_;
  int32_t num_orders_;
  uint32_t msg_seq_num_;
  uint32_t level_;
  uint8_t action_;  // 0 -> New, 1 -> Change, 2 -> Delete, 3 -> Reset, 4 -> Clear
  uint8_t side_;
  bool intermediate_;
  inline void SetIntermediate(bool int_flag) { intermediate_ = int_flag; }
  inline std::string ToString() {
    std::ostringstream temp_oss;
    temp_oss << "Seq. No: " << msg_seq_num_ << "\n";
    temp_oss << "Contract: " << contract_ << "\n";
    switch (side_) {
      case kSGXBid:
        temp_oss << "Side: B"
                 << "\n";
        break;
      case kSGXAsk:
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

struct SGXPFTradeStruct {
  char contract_[SGX_MDS_CONTRACT_TEXT_SIZE];
  double price_;
  uint64_t quantity_;
  uint32_t msg_seq_num_;
  uint8_t side_;
  bool intermediate_;

  inline std::string ToString() {
    std::ostringstream t_temp_oss_;
    t_temp_oss_ << "Seq. No: " << msg_seq_num_ << "\n";
    t_temp_oss_ << "Contract: " << contract_ << "\n";
    switch (side_) {
      case kSGXBid:
        t_temp_oss_ << "Side: B"
                    << "\n";
        break;
      case kSGXAsk:
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

struct SGXPFResetStruct {
  char contract_[SGX_MDS_CONTRACT_TEXT_SIZE];

  std::string ToString() {
    std::ostringstream t_temp_oss_;
    t_temp_oss_ << "Contract: " << contract_ << "\n";
    return t_temp_oss_.str();
  }
};

struct SGXPFCommonStruct {
  PriceFeedmsgType msg_;
  timeval time_;
  union {
    SGXPFDeltaStruct delta_;
    SGXPFTradeStruct trade_;
    SGXPFResetStruct reset_;
  } data_;
  inline void SetIntermediate(bool flag) {
    switch (msg_) {
      case SGX_PF_DELTA:
        data_.delta_.intermediate_ = flag;
        break;
      case SGX_PF_TRADE:
        break;
      case SGX_PF_RESET_BEGIN:
      case SGX_PF_RESET_END:
        break;
      default:
        break;
    }
  }
  inline double GetTradeDoublePrice() { return data_.trade_.price_; }
  inline HFSAT::TradeType_t GetTradeAggressorSide() {
    return (data_.trade_.side_ == SGXOrderSide::kSGXBid)
               ? HFSAT::TradeType_t::kTradeTypeBuy
               : (data_.trade_.side_ == SGXOrderSide::kSGXAsk) ? HFSAT::TradeType_t::kTradeTypeSell
                                                               : HFSAT::TradeType_t::kTradeTypeNoInfo;
  }
  inline uint32_t GetTradeSize() { return data_.trade_.quantity_; }
  inline bool isTradeMsg() { return (SGX_PF_TRADE == msg_); }
  char* getContract() {
    switch (msg_) {
      case SGX_PF_DELTA:
        return data_.delta_.contract_;
        break;
      case SGX_PF_TRADE:
        return data_.trade_.contract_;
        break;
      case SGX_PF_RESET_BEGIN:
      case SGX_PF_RESET_END:
        return data_.reset_.contract_;
        break;
      default:
        return NULL;
    }
    return NULL;
  }

  inline std::string ToString() {
    switch (msg_) {
      case SGX_PF_DELTA: {
        std::ostringstream temp_oss;
        temp_oss << "\n========= SGX Delta Message =========\n\n";
        temp_oss << "Time: " << time_.tv_sec << "." << std::setw(6) << std::setfill('0') << time_.tv_usec << "\n";
        return (temp_oss.str() + data_.delta_.ToString() + "=======================================================\n");
      } break;
      case SGX_PF_TRADE: {
        std::ostringstream temp_oss;
        temp_oss << "\n========== SGX Trade Message =============\n\n";
        temp_oss << "Time: " << time_.tv_sec << "." << std::setw(6) << std::setfill('0') << time_.tv_usec << "\n";
        return (temp_oss.str() + data_.trade_.ToString() + "=======================================================\n");
      } break;
      case SGX_PF_RESET_BEGIN: {
        std::ostringstream temp_oss;
        temp_oss << "\n========== SGX Reset Begin Message =============\n\n";
        temp_oss << "Time: " << time_.tv_sec << "." << std::setw(6) << std::setfill('0') << time_.tv_usec << "\n";
        return (temp_oss.str() + data_.reset_.ToString() + "=======================================================\n");
      } break;
      case SGX_PF_RESET_END: {
        std::ostringstream temp_oss;
        temp_oss << "\n========== SGX Reset End Message =============\n\n";
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

namespace SGX_ITCH_MDS {
enum SGXOrderType {
  kAdd = 'A',
  kDelete = 'D',
  kExec = 'E',
  kExecWithPrice = 'C',
  kResetBegin = '0',
  kResetEnd = '1',

  kTradingStatus = 'O',

  kInvalid = 'i'
};

class SGXItchOrderAdd {
 public:
  SGXItchOrderAdd() : size(0), price(0) {}
  SGXItchOrderAdd(uint64_t t_size, double t_price) { Set(t_size, t_price); }

  void Set(uint64_t t_size, double t_price) {
    t_size = size;
    t_price = price;
  }

  std::string ToString() {
    std::ostringstream temp_oss;
    temp_oss << "Price:\t\t" << std::fixed << std::setprecision(6) << price << "\n";
    temp_oss << "Size:\t\t" << size << "\n";
    return temp_oss.str();
  }

  uint64_t size;
  double price;
};

class SGXItchOrderExec {
 public:
  SGXItchOrderExec() : size(0) {}
  SGXItchOrderExec(uint64_t t_size) { Set(t_size); }

  void Set(uint64_t t_size) { t_size = size; }
  std::string ToString() {
    std::ostringstream temp_oss;
    temp_oss << "Size:\t\t" << size << "\n";
    return temp_oss.str();
  }

  uint64_t size;
};

class SGXItchOrderExecWithPrice {
 public:
  SGXItchOrderExecWithPrice() : size(0), price(0) {}
  SGXItchOrderExecWithPrice(uint64_t t_size, double t_price) { Set(t_size, t_price); }

  void Set(uint64_t t_size, double t_price) {
    t_size = size;
    t_price = price;
  }
  std::string ToString() {
    std::ostringstream temp_oss;
    temp_oss << "Price:\t\t" << std::fixed << std::setprecision(6) << price << "\n";
    temp_oss << "Size:\t\t" << size << "\n";
    return temp_oss.str();
  }

  uint64_t size;
  double price;
};

class SGXItchBookState {
 public:
  SGXItchBookState(const SGXItchBookState& book_state) { std::memcpy(state, book_state.state, 21); }
  void operator=(const SGXItchBookState& book_state) { memcpy(state, book_state.state, 21); }
  std::string ToString() {
    std::ostringstream temp_oss;
    temp_oss << "State:\t\t" << state << "\n";
    return temp_oss.str();
  }
  char state[21];
};

class SGXItchOrder {
 public:
  SGXItchOrder()
      : time{0, 0}, order_id(0), seq_num(0), order_type(SGXOrderType::kInvalid), side(0), is_intermediate(false) {}
  SGXItchOrder(const SGXItchOrder& order) {
    std::memcpy(series, order.series, SGX_ITCH_MDS_CONTRACT_TEXT_SIZE);
    time = order.time;
    order_id = order.order_id;
    seq_num = order.seq_num;
    order_type = order.order_type;
    side = order.side;
    is_intermediate = order.is_intermediate;

    switch (order_type) {
      case kAdd:
        add = order.add;
        break;
      case kDelete:

        break;
      case kExec:
        exec = order.exec;
        break;
      case kExecWithPrice:
        exec_px = order.exec_px;
        break;
      case kResetBegin:

        break;
      case kResetEnd:

        break;
      case kTradingStatus:
        state = order.state;
        break;
      default:
        break;
    }
  }

  void operator=(const SGXItchOrder& order) {
    std::memcpy(series, order.series, SGX_ITCH_MDS_CONTRACT_TEXT_SIZE);
    time = order.time;
    order_id = order.order_id;
    seq_num = order.seq_num;
    order_type = order.order_type;
    side = order.side;
    is_intermediate = order.is_intermediate;

    switch (order_type) {
      case kAdd:
        add = order.add;
        break;
      case kDelete:

        break;
      case kExec:
        exec = order.exec;
        break;
      case kExecWithPrice:
        exec_px = order.exec_px;
        break;
      case kResetBegin:

        break;
      case kResetEnd:

        break;
      case kTradingStatus:
        state = order.state;
        break;
      default:
        break;
    }
  }

  char* getContract() { return series; }

  std::string ToString() {
    std::ostringstream temp_oss;
    temp_oss << "\n================= SGX Message =====================\n\n";
    temp_oss << "Time:\t\t" << time.tv_sec << "." << std::setw(6) << std::setfill('0') << time.tv_usec << "\n";
    temp_oss << "Order Id:\t" << order_id << "\n";

    temp_oss << "Seq Number:\t" << seq_num << "\n";
    temp_oss << "Series:\t\t" << series << "\n";
    switch (order_type) {
      case kAdd:
        temp_oss << "Msge Type:\t"
                 << "ADD"
                 << "\n";
        temp_oss << add.ToString();
        break;
      case kDelete:
        temp_oss << "Msge Type:\t"
                 << "DELETE"
                 << "\n";
        break;
      case kExec:
        temp_oss << "Msge Type:\t"
                 << "Exec"
                 << "\n";
        temp_oss << exec.ToString();
        break;
      case kExecWithPrice:
        temp_oss << "Msge Type:\t"
                 << "Exec With Price"
                 << "\n";
        temp_oss << exec_px.ToString();
        break;
      case kResetBegin:
        temp_oss << "Msge Type:\t"
                 << "Reset Begin"
                 << "\n";
        break;
      case kResetEnd:
        temp_oss << "Msge Type:\t"
                 << "Reset End"
                 << "\n";
        break;
      case kTradingStatus:
        temp_oss << "Msge Type:\t"
                 << "Book State"
                 << "\n";
        temp_oss << state.ToString();
        break;
      default:
        temp_oss << "Msge Type:\t"
                 << "Unknown"
                 << "\n";
        break;
    }
    switch (side) {
      case SGX_MDS::kSGXBid:
        temp_oss << "Side:\t\t"
                 << "B"
                 << "\n";
        break;
      case SGX_MDS::kSGXAsk:
        temp_oss << "Side:\t\t"
                 << "S"
                 << "\n";
        break;
      default: {
        temp_oss << "Side:\t\t"
                 << "Unknown"
                 << "\n";
        break;
      }
    }
    temp_oss << "Intermediate:\t" << is_intermediate << "\n";
    temp_oss << "===================================================\n";
    return temp_oss.str();
  }

  char series[SGX_ITCH_MDS_CONTRACT_TEXT_SIZE];
  timeval time;
  uint64_t order_id;
  uint32_t seq_num;
  SGXOrderType order_type;
  uint8_t side;  // Bid (1), Ask(2)
  bool is_intermediate;

  union {
    SGXItchOrderAdd add;
    SGXItchOrderExec exec;
    SGXItchOrderExecWithPrice exec_px;
    SGXItchBookState state;
  };
};
}
#endif
