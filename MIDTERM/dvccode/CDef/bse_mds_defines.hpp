#ifndef _BSE_MDS_DEFINES_
#define _BSE_MDS_DEFINES_

#include <iomanip>
#include <inttypes.h>
#include <sstream>
#include <string>
#include <sys/time.h>

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
#endif
