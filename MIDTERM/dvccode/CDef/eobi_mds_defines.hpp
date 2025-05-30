/**
   \file dvccode/CDef/eobi_mds_defines.hpp

   \author: (c) Copyright Two Roads Technological Solutions Pvt Ltd 2011
   Address:
   Suite 217, Level 2, Prestige Omega,
   No 104, EPIP Zone, Whitefield,
   Bangalore - 560066
   India
   +91 80 4060 0717
 */
#ifndef EOBI_MDS_DEFINES
#define EOBI_MDS_DEFINES

#include <inttypes.h>
#include <sys/time.h>
#include <string>
#include <cstring>
#include <sstream>
#include <iostream>
#include <iomanip>
#include "dvccode/CDef/defines.hpp"
namespace EOBI_MDS {

#define EOBI_MDS_CONTRACT_TEXT_SIZE 12

enum msgType { EOBI_ORDER = 1 };

// Each EOBI order can be uniquely identified using instrument id, side and priority timestamp
struct EOBIOrderOld {
  uint32_t msg_seq_num_;
  char contract_[EOBI_MDS_CONTRACT_TEXT_SIZE];
  uint8_t side;
  uint64_t priority_ts;
  uint32_t action_;  // 0 - New, 1 - Change, 2 - Delete, 3 - Mass Delete, 4 - Partial Order Execution, 5 - Full Order
                     // Execution, 6 - execution summary
  double price;
  int32_t size;
  double prev_price;          // Used in OrderModify
  int32_t prev_size;          // Used in OrderModify
  uint64_t prev_priority_ts;  // Used in OrderModify
  bool intermediate_;         ///< if this is an intermediate message .. clients should not react to intermediates

  inline std::string ToString() {
    std::ostringstream t_temp_oss_;
    t_temp_oss_ << "Seq. No: " << msg_seq_num_ << '\n';
    t_temp_oss_ << "Contract: " << contract_ << '\n';

    if (action_ == '6') {
      t_temp_oss_ << "Aggressor Side: " << side << '\n';
    } else {
      t_temp_oss_ << "Side: " << side << '\n';
    }

    if (action_ == '1') {
      t_temp_oss_ << "Prev_Priority_TS: " << prev_priority_ts << "\n";
    }

    t_temp_oss_ << "Prority_TS: " << priority_ts << '\n';

    switch (action_) {
      case '0':
        t_temp_oss_ << "Action: "
                    << "ADD" << '\n';
        break;
      case '1':
        t_temp_oss_ << "Action: "
                    << "MODIFY" << '\n';
        break;
      case '2':
        t_temp_oss_ << "Action: "
                    << "DELETE" << '\n';
        break;
      case '3':
        t_temp_oss_ << "Action: "
                    << "MASS_DEL" << '\n';
        break;
      case '4':
        t_temp_oss_ << "Action: "
                    << "PARTIAL_EXEC" << '\n';
        break;
      case '5':
        t_temp_oss_ << "Action: "
                    << "FULL_EXEC" << '\n';
        break;
      case '6':
        t_temp_oss_ << "Action: "
                    << "EXEC_SUMMARY" << '\n';
        break;
      default:
        t_temp_oss_ << "Action: "
                    << "--------" << '\n';
        break;
    }

    t_temp_oss_ << "Price: " << std::fixed << std::setprecision(6) << price << '\n';
    t_temp_oss_ << "Size: " << size << '\n';

    if (action_ == '1') {
      t_temp_oss_ << "Prev Price: " << std::fixed << std::setprecision(6) << prev_price << '\n';
      t_temp_oss_ << "Prev Size: " << prev_size << '\n';
    } else if (action_ == '6') {
      t_temp_oss_ << "Synthetic_Match:" << prev_size << '\n';
    }

    t_temp_oss_ << "Intermediate: " << (intermediate_ ? "Y" : "N") << '\n';

    return t_temp_oss_.str();
  }
};

struct EOBICommonStructOld {
  msgType msg_;
  timeval time_;

  union {
    EOBIOrderOld order_;
  } data_;

  char* getContract() {
    switch (msg_) {
      case EOBI_ORDER:
        return data_.order_.contract_;
        break;
      default:
        return NULL;
    }
  }

  inline void print() {}

  bool isTradeMsg() const { return data_.order_.action_ == '6'; }

  inline std::string ToString() {
    switch (msg_) {
      case EOBI_ORDER: {
        std::ostringstream t_temp_oss_;
        t_temp_oss_ << "\n============== EOBI Order Message ================\n\n";
        t_temp_oss_ << "Time: " << time_.tv_sec << "." << std::setw(6) << std::setfill('0') << time_.tv_usec << "\n";

        return (t_temp_oss_.str() + data_.order_.ToString() + "===================================================\n");
      } break;
      default: {
        std::ostringstream t_temp_oss_;
        t_temp_oss_ << "NOT IMPLEMENTED FOR THIS EVENT : " << msg_ << '\n';

        return t_temp_oss_.str();
      } break;
    }
    return "";
  }
};

// Each EOBI order can be uniquely identified using instrument id, side and priority timestamp
struct EOBIOrder {
  uint32_t msg_seq_num_;
  char contract_[EOBI_MDS_CONTRACT_TEXT_SIZE];  // eurex security name
  uint8_t side;
  uint64_t priority_ts;  // Priority timestamp
  uint32_t action_;  // 0 - New, 1 - Change, 2 - Delete, 3 - Mass Delete, 4 - Partial Order Execution, 5 - Full Order
                     // Execution, 6 - execution summary
  double price;
  int32_t size;
  double prev_price;          // Used in OrderModify
  int32_t prev_size;          // Used in OrderModify
  uint64_t prev_priority_ts;  // Used in OrderModify
  bool intermediate_;         ///< if this is an intermediate message .. clients should not react to intermediates
  uint64_t trd_reg_ts;        // Matching engine In timestamp

  inline std::string ToString() {
    std::ostringstream t_temp_oss_;
    t_temp_oss_ << "Seq. No: " << msg_seq_num_ << '\n';
    t_temp_oss_ << "Contract: " << contract_ << '\n';

    if (action_ == '6') {
      t_temp_oss_ << "Aggressor Side: " << side << '\n';
    } else {
      t_temp_oss_ << "Side: " << side << '\n';
    }

    if (action_ == '1') {
      t_temp_oss_ << "Prev_Priority_TS: " << prev_priority_ts << "\n";
    }

    t_temp_oss_ << "Prority_TS: " << priority_ts << '\n';
    t_temp_oss_ << "MatchingEngineTs: " << trd_reg_ts << '\n';

    switch (action_) {
      case '0':
        t_temp_oss_ << "Action: "
                    << "ADD" << '\n';
        break;
      case '1':
        t_temp_oss_ << "Action: "
                    << "MODIFY" << '\n';
        break;
      case '2':
        t_temp_oss_ << "Action: "
                    << "DELETE" << '\n';
        break;
      case '3':
        t_temp_oss_ << "Action: "
                    << "MASS_DEL" << '\n';
        break;
      case '4':
        t_temp_oss_ << "Action: "
                    << "PARTIAL_EXEC" << '\n';
        break;
      case '5':
        t_temp_oss_ << "Action: "
                    << "FULL_EXEC" << '\n';
        break;
      case '6':
        t_temp_oss_ << "Action: "
                    << "EXEC_SUMMARY" << '\n';
        break;
      default:
        t_temp_oss_ << "Action: "
                    << "--------" << '\n';
        break;
    }

    t_temp_oss_ << "Price: " << std::fixed << std::setprecision(6) << price << '\n';
    t_temp_oss_ << "Size: " << size << '\n';

    if (action_ == '1') {
      t_temp_oss_ << "Prev Price: " << std::fixed << std::setprecision(6) << prev_price << '\n';
      t_temp_oss_ << "Prev Size: " << prev_size << '\n';
    } else if (action_ == '6') {
      t_temp_oss_ << "Synthetic_Match:" << prev_size << '\n';
    }

    t_temp_oss_ << "Intermediate: " << (intermediate_ ? "Y" : "N") << '\n';

    return t_temp_oss_.str();
  }
};

struct EOBICommonStruct {
  msgType msg_;
  timeval time_;

  union {
    EOBIOrder order_;
  } data_;

  char* getContract() {
    switch (msg_) {
      case EOBI_ORDER:
        return data_.order_.contract_;
        break;
      default:
        return NULL;
    }
  }

  inline void print() {}

  bool isTradeMsg() const { return data_.order_.action_ == '6'; }

  inline std::string ToString() {
    switch (msg_) {
      case EOBI_ORDER: {
        std::ostringstream t_temp_oss_;
        t_temp_oss_ << "\n============== EOBI Order Message ================\n\n";
        t_temp_oss_ << "Time: " << time_.tv_sec << "." << std::setw(6) << std::setfill('0') << time_.tv_usec << "\n";

        return (t_temp_oss_.str() + data_.order_.ToString() + "===================================================\n");
      } break;
      default: {
        std::ostringstream t_temp_oss_;
        t_temp_oss_ << "NOT IMPLEMENTED FOR THIS EVENT : " << msg_ << '\n';

        return t_temp_oss_.str();
      } break;
    }
    return "";
  }
};

// Each EOBI order can be uniquely identified using instrument id, side and priority timestamp
struct EOBICompactOrder {
  char contract_[EOBI_MDS_CONTRACT_TEXT_SIZE];  // eurex security name
  struct timeval time_;
  uint64_t priority_ts;  // Priority timestamp
  double price;
  double prev_price;          // Used in OrderModify
  uint64_t prev_priority_ts;  // Used in OrderModify
  uint64_t trd_reg_ts;        // Matching engine In timestamp
  uint32_t action_;  // 0 - New, 1 - Change, 2 - Delete, 3 - Mass Delete, 4 - Partial Order Execution, 5 - Full Order
  uint32_t msg_seq_num_;
  // Execution, 6 - execution summary
  int32_t size;
  int32_t prev_size;   // Used in OrderModify
  bool intermediate_;  ///< if this is an intermediate message .. clients should not react to intermediates
  uint8_t side;

  inline void SetIntermediate(bool flag) {
    if (!isTradeMsg()) intermediate_ = flag;
  }
  char* getContract() { return contract_; }

  inline void print() {}
  inline double GetTradeDoublePrice() { return price; }
  inline HFSAT::TradeType_t GetTradeAggressorSide() {
    return (side == 'B') ? HFSAT::TradeType_t::kTradeTypeBuy : (side == 'S') ? HFSAT::TradeType_t::kTradeTypeSell
                                                                             : HFSAT::TradeType_t::kTradeTypeNoInfo;
  }
  inline uint32_t GetTradeSize() { return size; }
  bool isTradeMsg() const { return action_ == '6'; }

  inline std::string ToString() {
    std::ostringstream t_temp_oss_;

    t_temp_oss_ << "\n============== EOBI Compact Order Message ================\n\n";
    t_temp_oss_ << "Time: " << time_.tv_sec << "." << std::setw(6) << std::setfill('0') << time_.tv_usec << "\n";

    t_temp_oss_ << "Seq. No: " << msg_seq_num_ << '\n';
    t_temp_oss_ << "Contract: " << contract_ << '\n';

    if (action_ == '6') {
      t_temp_oss_ << "Aggressor Side: " << side << '\n';
    } else {
      t_temp_oss_ << "Side: " << side << '\n';
    }

    if (action_ == '1') {
      t_temp_oss_ << "Prev_Priority_TS: " << prev_priority_ts << "\n";
    }

    t_temp_oss_ << "Prority_TS: " << priority_ts << '\n';
    t_temp_oss_ << "MatchingEngineTs: " << trd_reg_ts << '\n';

    switch (action_) {
      case '0':
        t_temp_oss_ << "Action: "
                    << "ADD" << '\n';
        break;
      case '1':
        t_temp_oss_ << "Action: "
                    << "MODIFY" << '\n';
        break;
      case '2':
        t_temp_oss_ << "Action: "
                    << "DELETE" << '\n';
        break;
      case '3':
        t_temp_oss_ << "Action: "
                    << "MASS_DEL" << '\n';
        break;
      case '4':
        t_temp_oss_ << "Action: "
                    << "PARTIAL_EXEC" << '\n';
        break;
      case '5':
        t_temp_oss_ << "Action: "
                    << "FULL_EXEC" << '\n';
        break;
      case '6':
        t_temp_oss_ << "Action: "
                    << "EXEC_SUMMARY" << '\n';
        break;
      default:
        t_temp_oss_ << "Action: "
                    << "--------" << '\n';
        break;
    }

    t_temp_oss_ << "Price: " << std::fixed << std::setprecision(6) << price << '\n';
    t_temp_oss_ << "Size: " << size << '\n';

    if (action_ == '1') {
      t_temp_oss_ << "Prev Price: " << std::fixed << std::setprecision(6) << prev_price << '\n';
      t_temp_oss_ << "Prev Size: " << prev_size << '\n';
    } else if (action_ == '6') {
      t_temp_oss_ << "Synthetic_Match:" << prev_size << '\n';
    }

    t_temp_oss_ << "Intermediate: " << (intermediate_ ? "Y" : "N") << '\n';

    return t_temp_oss_.str();
  }
};

inline void ConvertOrderEventToCompactEvent(EOBICommonStruct& eobi_order, EOBICompactOrder& eobi_compact_order) {
  eobi_compact_order.time_ = eobi_order.time_;
  eobi_compact_order.msg_seq_num_ = eobi_order.data_.order_.msg_seq_num_;
  memcpy((void*)eobi_compact_order.contract_, (void*)eobi_order.data_.order_.contract_, EOBI_MDS_CONTRACT_TEXT_SIZE);
  eobi_compact_order.priority_ts = eobi_order.data_.order_.priority_ts;
  eobi_compact_order.action_ = eobi_order.data_.order_.action_;
  eobi_compact_order.price = eobi_order.data_.order_.price;
  eobi_compact_order.size = eobi_order.data_.order_.size;
  eobi_compact_order.prev_price = eobi_order.data_.order_.prev_price;
  eobi_compact_order.prev_size = eobi_order.data_.order_.prev_size;
  eobi_compact_order.prev_priority_ts = eobi_order.data_.order_.prev_priority_ts;
  eobi_compact_order.trd_reg_ts = eobi_order.data_.order_.trd_reg_ts;
  eobi_compact_order.intermediate_ = eobi_order.data_.order_.intermediate_;
  eobi_compact_order.side = eobi_order.data_.order_.side;
}
}

#endif
