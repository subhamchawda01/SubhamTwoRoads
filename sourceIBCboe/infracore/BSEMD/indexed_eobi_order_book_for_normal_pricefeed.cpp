/**
   \file EobiD/indexed_eobi_order_book.cpp

   \author: (c) Copyright Two Roads Technological Solutions Pvt Ltd 2011
   Address:
   Suite 217, Level 2, Prestige Omega,
   No 104, EPIP Zone, Whitefield,
   Bangalore - 560066, India
   Phone: +91 80 4060 0717
 */

#include "infracore/BSEMD/indexed_eobi_order_book_for_normal_pricefeed.hpp"

namespace HFSAT {

EobiOrderBookForNPF::EobiOrderBookForNPF(std::string &t_shortcode_, std::string _exchange_symbol_,
                                         double min_price_inc) {
  memset((void *)exchange_symbol_, 0, EUREX_MDS_CONTRACT_TEXT_SIZE);
  memcpy((void *)exchange_symbol_, (void *)_exchange_symbol_.c_str(), _exchange_symbol_.length());

  bid_levels_.resize(EOBID_ORDER_BOOK_SIZE, EobiOrderLevelForNPF(0, 0));
  ask_levels_.resize(EOBID_ORDER_BOOK_SIZE, EobiOrderLevelForNPF(0, 0));

  last_msg_seq_num_ = 0;
  last_security_id_ = 0;

  eurex_mds_ = new EUREX_MDS::EUREXCommonStruct();

  price_adjustment_ = 0;
  bid_side_trade_size_ = 0;
  ask_side_trade_size_ = 0;

  // The default case when min_px isn't given as input
  if (min_price_inc < 0) {
    fast_price_convertor_ = new HFSAT::FastPriceConvertor(HFSAT::SecurityDefinitions::GetContractMinPriceIncrement(
        t_shortcode_, HFSAT::DateTime::GetCurrentIsoDateLocal()));
  } else {
    fast_price_convertor_ = new HFSAT::FastPriceConvertor(min_price_inc);
  }

  price_adjustment_set_ = false;

  top_bid_index_ = -1;
  top_ask_index_ = -1;
}

EUREX_MDS::EUREXCommonStruct *EobiOrderBookForNPF::OrderAdd(uint8_t t_side_, double t_price_, int t_size_,
                                                            bool t_intermediate_) {
  int int_price_ = fast_price_convertor_->GetFastIntPx(t_price_);

  if (!price_adjustment_set_) {
    price_adjustment_ = int_price_ - EOBID_START_INDEX;

    price_adjustment_set_ = true;
  }

  memset(eurex_mds_, 0, sizeof(EUREX_MDS::EUREXCommonStruct));
  eurex_mds_->msg_ = EUREX_MDS::EUREX_DELTA;
  memcpy((void *)(eurex_mds_->data_.eurex_dels_.contract_), (void *)exchange_symbol_, EUREX_MDS_CONTRACT_TEXT_SIZE);
  eurex_mds_->data_.eurex_dels_.intermediate_ = t_intermediate_;
  eurex_mds_->data_.eurex_dels_.level_ = 0;
  eurex_mds_->data_.eurex_dels_.price_ = t_price_;
  eurex_mds_->data_.eurex_dels_.type_ = t_side_;

  switch (t_side_) {
    case '2': {
      bid_side_trade_size_ = 0;

      int index_ = int_price_ - price_adjustment_;

      // Skip the order if the index_ is invalid
      if (index_ < 0 || index_ >= EOBID_ORDER_BOOK_SIZE) {
        return NULL;
      }

      if (index_ > top_bid_index_) {
        top_bid_index_ = index_;
      }

      // Sanitization code. The while loop shouldn't get executed unless there is packet drop.
      if (top_ask_index_ >= 0) {
        while (int_price_ >= (EOBID_ORDER_BOOK_SIZE + price_adjustment_ - top_ask_index_)) {
          ask_levels_[top_ask_index_].order_count_ = 0;
          ask_levels_[top_ask_index_].size_ = 0;
          top_ask_index_--;
        }
      }

      if (bid_levels_[index_].size_ <= 0) {
        bid_levels_[index_].size_ = t_size_;
        bid_levels_[index_].order_count_ = 1;
        eurex_mds_->data_.eurex_dels_.action_ = '1';  // Price level new
      } else {
        bid_levels_[index_].size_ += t_size_;
        bid_levels_[index_].order_count_++;

        eurex_mds_->data_.eurex_dels_.action_ = '2';  // Price level change
      }

      eurex_mds_->data_.eurex_dels_.size_ = bid_levels_[index_].size_;
      eurex_mds_->data_.eurex_dels_.num_ords_ = bid_levels_[index_].order_count_;
    } break;
    case '1': {
      ask_side_trade_size_ = 0;

      int index_ = EOBID_ORDER_BOOK_SIZE - (int_price_ - price_adjustment_);

      // Skip the order if the index_ is invalid
      if (index_ < 0 || index_ >= EOBID_ORDER_BOOK_SIZE) {
        return NULL;
      }

      if (index_ > top_ask_index_) {
        top_ask_index_ = index_;
      }

      // Sanitization code. The while loop shouldn't get executed unless there is packet drop.
      if (top_bid_index_ >= 0) {
        while (int_price_ <= (top_bid_index_ + price_adjustment_)) {
          bid_levels_[top_bid_index_].order_count_ = 0;
          bid_levels_[top_bid_index_].size_ = 0;
          top_bid_index_--;
        }
      }

      if (ask_levels_[index_].size_ <= 0) {
        ask_levels_[index_].size_ = t_size_;
        ask_levels_[index_].order_count_ = 1;

        eurex_mds_->data_.eurex_dels_.action_ = '1';  // Price level new
      } else {
        ask_levels_[index_].size_ += t_size_;
        ask_levels_[index_].order_count_++;

        eurex_mds_->data_.eurex_dels_.action_ = '2';  // Price level change
      }

      eurex_mds_->data_.eurex_dels_.size_ = ask_levels_[index_].size_;
      eurex_mds_->data_.eurex_dels_.num_ords_ = ask_levels_[index_].order_count_;
    } break;
    default:
      break;
  }

  return eurex_mds_;
}

EUREX_MDS::EUREXCommonStruct *EobiOrderBookForNPF::OrderDelete(uint8_t t_side_, double t_price_, int t_size_,
                                                               bool t_delete_order_, bool t_intermediate_) {
  int int_price_ = fast_price_convertor_->GetFastIntPx(t_price_);

  if (!price_adjustment_set_) {
    price_adjustment_ = int_price_ - EOBID_START_INDEX;

    price_adjustment_set_ = true;
  }

  memset((eurex_mds_), 0, sizeof(EUREX_MDS::EUREXCommonStruct));
  eurex_mds_->msg_ = EUREX_MDS::EUREX_DELTA;
  memcpy((void *)(eurex_mds_->data_.eurex_dels_.contract_), (void *)exchange_symbol_, EUREX_MDS_CONTRACT_TEXT_SIZE);
  eurex_mds_->data_.eurex_dels_.intermediate_ = t_intermediate_;
  eurex_mds_->data_.eurex_dels_.level_ = 0;
  eurex_mds_->data_.eurex_dels_.price_ = t_price_;
  eurex_mds_->data_.eurex_dels_.type_ = t_side_;

  switch (t_side_) {
    case '2': {
      int index_ = int_price_ - price_adjustment_;

      // Skip the order if the index_ is invalid
      if (index_ < 0 || index_ >= EOBID_ORDER_BOOK_SIZE) {
        return NULL;
      }

      bid_levels_[index_].size_ -= t_size_;

      if (t_delete_order_) {
        bid_levels_[index_].order_count_--;
      }

      // If the size or order count falls below one, make sure that order count and size are reset to 0.
      if (bid_levels_[index_].size_ <= 0 || bid_levels_[index_].order_count_ <= 0) {
        if (index_ == top_bid_index_) {
          // Decrease the top_bid_index_ if top level is getting deleted
          top_bid_index_--;
          while (top_bid_index_ >= 0) {
            if (bid_levels_[top_bid_index_].size_ > 0 && bid_levels_[top_bid_index_].order_count_ > 0) {
              // We have found the next top bid index
              break;
            }

            bid_levels_[top_bid_index_].size_ = 0;
            bid_levels_[top_bid_index_].order_count_ = 0;
            top_bid_index_--;
          }
        }

        bid_levels_[index_].size_ = 0;
        bid_levels_[index_].order_count_ = 0;
        eurex_mds_->data_.eurex_dels_.action_ = '3';  // Price level delete
      } else {
        eurex_mds_->data_.eurex_dels_.action_ = '2';  // Price level change
      }

      eurex_mds_->data_.eurex_dels_.size_ = bid_levels_[index_].size_;
      eurex_mds_->data_.eurex_dels_.num_ords_ = bid_levels_[index_].order_count_;
    } break;
    case '1': {
      int index_ = EOBID_ORDER_BOOK_SIZE - (int_price_ - price_adjustment_);

      // Skip the order if the index_ is invalid
      if (index_ < 0 || index_ >= EOBID_ORDER_BOOK_SIZE) {
        return NULL;
      }

      ask_levels_[index_].size_ -= t_size_;

      if (t_delete_order_) {
        ask_levels_[index_].order_count_--;
      }

      // If the size or order count falls below one, make sure that order count and size are reset to 0.
      if (ask_levels_[index_].size_ <= 0 || ask_levels_[index_].order_count_ <= 0) {
        if (index_ == top_ask_index_) {
          // Decrease the top_ask_index_ if top level is getting deleted
          top_ask_index_--;
          while (top_ask_index_ >= 0) {
            if (ask_levels_[top_ask_index_].size_ > 0 && ask_levels_[top_ask_index_].order_count_ > 0) {
              // We have found the next top ask index
              break;
            }

            ask_levels_[top_ask_index_].size_ = 0;
            ask_levels_[top_ask_index_].order_count_ = 0;
            top_ask_index_--;
          }
        }

        ask_levels_[index_].size_ = 0;
        ask_levels_[index_].order_count_ = 0;
        eurex_mds_->data_.eurex_dels_.action_ = '3';  // Price level delete
      } else {
        eurex_mds_->data_.eurex_dels_.action_ = '2';  // Price level change
      }

      eurex_mds_->data_.eurex_dels_.size_ = ask_levels_[index_].size_;
      eurex_mds_->data_.eurex_dels_.num_ords_ = ask_levels_[index_].order_count_;
    } break;
    default:
      break;
  }

  return eurex_mds_;
}

EUREX_MDS::EUREXCommonStruct *EobiOrderBookForNPF::OrderMassDelete() {
  memset(eurex_mds_, 0, sizeof(EUREX_MDS::EUREXCommonStruct));
  eurex_mds_->msg_ = EUREX_MDS::EUREX_DELTA;
  memcpy((void *)(eurex_mds_->data_.eurex_dels_.contract_), (void *)exchange_symbol_, EUREX_MDS_CONTRACT_TEXT_SIZE);
  eurex_mds_->data_.eurex_dels_.action_ = '5';

  bid_levels_.clear();
  ask_levels_.clear();

  bid_levels_.resize(EOBID_ORDER_BOOK_SIZE, EobiOrderLevelForNPF(0, 0));
  ask_levels_.resize(EOBID_ORDER_BOOK_SIZE, EobiOrderLevelForNPF(0, 0));

  top_bid_index_ = -1;
  top_ask_index_ = -1;

  return eurex_mds_;
}

EUREX_MDS::EUREXCommonStruct *EobiOrderBookForNPF::FullOrderExecution(uint8_t t_side_, double t_price_, int t_size_) {
  // Marking this delta for trade intermediate as per request
  EUREX_MDS::EUREXCommonStruct *eurex_ls_ = OrderDelete(t_side_, t_price_, t_size_, true, true);

  if (eurex_ls_ == NULL) {
    return NULL;
  }

  switch (t_side_) {
    case '2': {
      if (bid_side_trade_size_ > 0) {
        bid_side_trade_size_ -= t_size_;

        // Skip intermediate updates by returning NULL
        if (bid_side_trade_size_ > 0) {
          if (eurex_ls_->data_.eurex_dels_.action_ == '3') {
            return eurex_ls_;
          } else {
            return NULL;
          }
        }
      }
    } break;
    case '1': {
      if (ask_side_trade_size_ > 0) {
        ask_side_trade_size_ -= t_size_;

        if (ask_side_trade_size_ > 0) {
          if (eurex_ls_->data_.eurex_dels_.action_ == '3') {
            return eurex_ls_;
          } else {
            return NULL;
          }
        }
      }
    } break;
    default:
      break;
  }

  return eurex_ls_;
}

EUREX_MDS::EUREXCommonStruct *EobiOrderBookForNPF::PartialOrderExecution(uint8_t t_side_, double t_price_,
                                                                         int t_size_) {
  switch (t_side_) {
    case '2':
      bid_side_trade_size_ = 0;
      break;
    case '1':
      ask_side_trade_size_ = 0;
      break;
    default:
      break;
  }

  // Marking this delta for trade intermediate as per request
  return OrderDelete(t_side_, t_price_, t_size_, false, true);
}

EUREX_MDS::EUREXCommonStruct *EobiOrderBookForNPF::ExecutionSummary(uint8_t t_side_, double t_price_, int t_size_) {
  memset((eurex_mds_), 0, sizeof(EUREX_MDS::EUREXCommonStruct));
  eurex_mds_->msg_ = EUREX_MDS::EUREX_TRADE;
  memcpy((void *)(eurex_mds_->data_.eurex_dels_.contract_), (void *)exchange_symbol_, EUREX_MDS_CONTRACT_TEXT_SIZE);
  eurex_mds_->data_.eurex_trds_.agg_side_ = t_side_;
  eurex_mds_->data_.eurex_trds_.trd_px_ = t_price_;
  eurex_mds_->data_.eurex_trds_.trd_qty_ = t_size_;

  switch (t_side_) {
    case '2': {
      // Aggressive buy
      ask_side_trade_size_ = t_size_;
    } break;
    case '1': {
      // Aggressive sell
      bid_side_trade_size_ = t_size_;
    } break;
    default:
      break;
  }

  return eurex_mds_;
}
}
