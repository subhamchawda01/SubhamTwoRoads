// =====================================================================================
//
//       Filename:  retail_data_defines.hpp
//
//    Description:
//
//        Version:  1.0
//        Created:  05/05/2014 08:55:40 AM
//       Revision:  none
//       Compiler:  g++
//
//         Author:  (c) Copyright Two Roads Technological Solutions Pvt Ltd 2011
//
//        Address:  Suite No 162, Evoma, #14, Bhattarhalli,
//                  Old Madras Road, Near Garden City College,
//                  KR Puram, Bangalore 560049, India
//          Phone:  +91 80 4190 3551
//
// =====================================================================================

#ifndef BASE_CDEF_RETAIL_DATA_DEFINES_H
#define BASE_CDEF_RETAIL_DATA_DEFINES_H

#include <array>
#include <inttypes.h>
#include <sstream>
#include <cstring>

#include "dvccode/CDef/math_utils.hpp"

#define MAX_RETAIL_SHORTCODE_LENGTH 16
#define MAX_RETAIL_SYMBOL_LENGTH 32

#define RETAIL_UPDATE_IP "239.23.0.95"
#define RETAIL_UPDATE_PORT 33333

#define MAX_INVALID_PRICE 999999.0
#define MIN_INVALID_PRICE -999999.0
#define RETAIL_UPDATE_TIMEOUT_MSECS 5000
#define OFFER_COOLOFF_MSECS_AFTER_GUI_EXEC 30000

#define MAX_LEGS 5

namespace HFSAT {
namespace CDef {

typedef enum {
  knormal = 1,          // normal update
  kecotime,             // during eco time
  kafterendtime,        // after query ended
  kbeforestarttime,     // before query started
  kstartnotgiven,       // pls. give start
  kquerynotready,       // can be the case sometimes, if(bid<tgt<ask) is not satisfied
  kexternalgetflat,     // pls. dont getflat on retail
  kmaxpos,              // we are too long/short
  kunstablemarket,      // book is too thin, we can't show much size due to our positions etc.
  kquerynotshowingall,  // can't offer best price
  kSubBestBid,
  kSubBestAsk,
  kSubBestBidAsk,
  kother  // other things like max_loss etc., again should be very rare
} RetailUpdateType;

typedef enum {  // sub_product_ count
  normal = 0,   // Don't Store any info
  spread = 2,
  butterfly = 3
} SplitType;

inline std::string RetailUpdateTypeToString(RetailUpdateType _retail_update_type_) {
  switch (_retail_update_type_) {
    case (knormal): {
      return "normal";
    }
    case (kecotime): {
      return "ecotime";
    }
    case (kafterendtime): {
      return "afterendtime";
    }
    case (kbeforestarttime): {
      return "beforestarttime";
    }
    case (kstartnotgiven): {
      return "startnotgiven";
    }
    case (kquerynotready): {
      return "querynotready";
    }
    case (kexternalgetflat): {
      return "externalgetflat";
    }
    case (kmaxpos): {
      return "maxpos";
    }
    case (kunstablemarket): {
      return "unstablemarket";
    }
    case (kquerynotshowingall): {
      return "querynotshowingall";
    }
    case (kSubBestBid): {
      return "SubBestBid";
    }
    case (kSubBestAsk): {
      return "SubBestAsk";
    }
    case (kSubBestBidAsk): {
      return "SubBestBidAsk";
    }
    case (kother): {
      return "other";
    }
    default:
      return "unknown";
  }
}

struct RetailSubProduct {  // Structure For Storing Individual SubProduct details
  char shortcode_[MAX_RETAIL_SHORTCODE_LENGTH];
  char security_name_[MAX_RETAIL_SYMBOL_LENGTH];
  double product_price_;
  int product_size_;
  TradeType_t buysell_;
  inline std::string ToString() const {
    std::ostringstream temp_oss_;
    temp_oss_ << " Shortcode : " << shortcode_ << " Security : " << security_name_
              << " Product Price : " << product_price_ << " Product Size : " << product_size_;
    return temp_oss_.str();
  }
  void operator=(const RetailSubProduct& rhs_) {
    this->product_price_ = rhs_.product_price_;
    this->product_size_ = rhs_.product_size_;
    this->buysell_ = rhs_.buysell_;
    memset((void*)(this->shortcode_), 0, MAX_RETAIL_SHORTCODE_LENGTH);
    memcpy((void*)this->shortcode_, (void*)rhs_.shortcode_, MAX_RETAIL_SHORTCODE_LENGTH);
    memset((void*)(this->security_name_), 0, MAX_RETAIL_SYMBOL_LENGTH);
    memcpy((void*)this->security_name_, (void*)rhs_.security_name_, MAX_RETAIL_SYMBOL_LENGTH);
  }
};

struct ProductSplitDetails {  // Structure For Storing Split details
  SplitType product_type_;
  std::array<RetailSubProduct, MAX_LEGS> sub_product_bid_;
  std::array<RetailSubProduct, MAX_LEGS> sub_product_ask_;

  inline std::string ToString() const {
    std::ostringstream temp_oss_;
    temp_oss_ << "[ "
              << "Legs Count: " << product_type_ << " ]\n";
    for (int i = 0; i < product_type_; i++) {
      temp_oss_ << "(Bid :" << sub_product_bid_[i].ToString() << " )";
      temp_oss_ << "(Ask :" << sub_product_ask_[i].ToString() << " )\n";
    }
    return temp_oss_.str();
  }
  void operator=(const ProductSplitDetails& rhs_) {
    product_type_ = rhs_.product_type_;
    sub_product_bid_ = rhs_.sub_product_bid_;
    sub_product_ask_ = rhs_.sub_product_ask_;
  }
};

struct RetailOffer {
  double offered_bid_price_;
  double offered_ask_price_;
  int offered_bid_size_;
  int offered_ask_size_;
  RetailUpdateType retail_update_type_;
  ProductSplitDetails product_split_;

  void operator=(const RetailOffer& rhs_) {
    offered_bid_price_ = rhs_.offered_bid_price_;
    offered_ask_price_ = rhs_.offered_ask_price_;
    offered_bid_size_ = rhs_.offered_bid_size_;
    offered_ask_size_ = rhs_.offered_ask_size_;
    retail_update_type_ = rhs_.retail_update_type_;
    product_split_ = rhs_.product_split_;
  }
  inline std::string ToString() const {
    std::ostringstream temp_oss_;
    temp_oss_ << "[ " << offered_bid_size_ << " " << offered_bid_price_ << " * " << offered_ask_price_ << " "
              << offered_ask_size_ << " ] ( " << RetailUpdateTypeToString(retail_update_type_) << " )"
              << product_split_.ToString();
    return temp_oss_.str();
  }

  inline std::vector<double> get_bid_leg_prices() {
    std::vector<double> _leg_price_vec_(product_split_.product_type_, 0.0);
    for (auto i = 0u; i < _leg_price_vec_.size(); i++) {
      _leg_price_vec_[i] = product_split_.sub_product_bid_[i].product_price_;
    }
    return _leg_price_vec_;
  }

  inline std::vector<double> get_ask_leg_prices() {
    std::vector<double> _leg_price_vec_(product_split_.product_type_, 0.0);
    for (auto i = 0u; i < _leg_price_vec_.size(); i++) {
      _leg_price_vec_[i] = product_split_.sub_product_ask_[i].product_price_;
    }
    return _leg_price_vec_;
  }

  inline std::vector<int> get_leg_sizes() {
    std::vector<int> _leg_size_vec_(product_split_.product_type_, 0.0);
    for (auto i = 0u; i < _leg_size_vec_.size(); i++) {
      _leg_size_vec_[i] = product_split_.sub_product_bid_[i].product_size_;
    }
    return _leg_size_vec_;
  }

  // assumption 0.0001 << min_price_increment - instead use ( min_price_increament - 0.001 ) ?
  bool operator==(const RetailOffer& rhs_) {
    return ((MathUtils::DblPxCompare(rhs_.offered_bid_price_, offered_bid_price_, 0.0001)) &&
            (MathUtils::DblPxCompare(rhs_.offered_ask_price_, offered_ask_price_, 0.0001)) &&
            (rhs_.offered_bid_size_ == offered_bid_size_) && (rhs_.offered_ask_size_ == offered_ask_size_) &&
            (rhs_.retail_update_type_ == retail_update_type_));
  }

  bool operator!=(const RetailOffer& rhs_) const {
    return ((!MathUtils::DblPxCompare(rhs_.offered_bid_price_, offered_bid_price_, 0.0001)) ||
            (!MathUtils::DblPxCompare(rhs_.offered_ask_price_, offered_ask_price_, 0.0001)) ||
            (rhs_.offered_bid_size_ != offered_bid_size_) || (rhs_.offered_ask_size_ != offered_ask_size_) ||
            (rhs_.retail_update_type_ != retail_update_type_));
  }
};

struct RetailDataStruct {
  char shortcode_[MAX_RETAIL_SHORTCODE_LENGTH];
  char security_name_[MAX_RETAIL_SYMBOL_LENGTH];

  RetailOffer retail_offer_;
  char extra_[64];

  std::string ToString() {
    std::ostringstream t_temp_oss;
    t_temp_oss << " Shortcode : " << shortcode_ << " Security : " << security_name_ << " " << retail_offer_.ToString();

    return t_temp_oss.str();
  }
};
}
}

#endif  // BASE_CDEF_RETAIL_DATA_DEFINES_H
