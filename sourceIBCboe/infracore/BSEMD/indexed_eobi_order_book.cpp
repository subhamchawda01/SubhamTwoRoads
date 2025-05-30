/**
   \file EobiD/indexed_eobi_order_book.cpp

   \author: (c) Copyright Two Roads Technological Solutions Pvt Ltd 2011
   Address:
   Suite 217, Level 2, Prestige Omega,
   No 104, EPIP Zone, Whitefield,
   Bangalore - 560066, India
   Phone: +91 80 4060 0717
 */

#include "infracore/BSEMD/indexed_eobi_order_book.hpp"

namespace HFSAT {

EobiOrderBook::EobiOrderBook(std::string& t_shortcode_, uint8_t t_contract_code_) {
  contract_code_ = t_contract_code_;

  bid_levels_.resize(EOBID_ORDER_BOOK_SIZE, EobiOrderLevel(0, 0));
  ask_levels_.resize(EOBID_ORDER_BOOK_SIZE, EobiOrderLevel(0, 0));

  last_msg_seq_num_ = 0;
  last_security_id_ = 0;

  eurex_mds_ = new EUREX_MDS::EUREXLSCommonStruct();

  price_adjustment_ = 0;
  bid_side_trade_size_ = 0;
  ask_side_trade_size_ = 0;

  fast_price_convertor_ = new HFSAT::FastPriceConvertor(HFSAT::SecurityDefinitions::GetContractMinPriceIncrement(
      t_shortcode_, HFSAT::DateTime::GetCurrentIsoDateLocal()));

  price_adjustment_set_ = false;

  top_bid_index_ = -1;
  top_ask_index_ = -1;

  eobid_order_book_size_ = EOBID_ORDER_BOOK_SIZE;
  eobid_start_index_ = EOBID_START_INDEX;
}

EUREX_MDS::EUREXLSCommonStruct* EobiOrderBook::OrderAdd(uint8_t t_side_, double t_price_, int t_size_,
                                                        bool t_intermediate_) {
  int int_price_ = fast_price_convertor_->GetFastIntPx(t_price_);

  if (!price_adjustment_set_) {
    price_adjustment_ = int_price_ - EOBID_START_INDEX;

    price_adjustment_set_ = true;
  }

  memset(eurex_mds_, 0, sizeof(EUREX_MDS::EUREXLSCommonStruct));
  eurex_mds_->msg_ = EUREX_MDS::EUREX_DELTA;
  eurex_mds_->data_.eurex_dels_.contract_code_ = contract_code_;
  eurex_mds_->data_.eurex_dels_.intermediate_ = t_intermediate_;
  eurex_mds_->data_.eurex_dels_.level_ = 0;
  eurex_mds_->data_.eurex_dels_.price_ = t_price_;
  eurex_mds_->data_.eurex_dels_.type_ = t_side_;

  switch (t_side_) {
    case '2': {
      bid_side_trade_size_ = 0;

      int index_ = int_price_ - price_adjustment_;

      if (index_ < 0 || index_ >= eobid_order_book_size_) {
        // Skip the order if the index_ is still invalid
        return nullptr;
      }

      if (index_ > top_bid_index_) {
        top_bid_index_ = index_;
      }

      // Sanitization code. The while loop shouldn't get executed unless there is packet drop.
      if (top_ask_index_ >= 0) {
        while (int_price_ >= (eobid_order_book_size_ + price_adjustment_ - top_ask_index_)) {
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

      int index_ = eobid_order_book_size_ - (int_price_ - price_adjustment_);

      if (index_ < 0 || index_ >= eobid_order_book_size_) {
        // Skip the order if the index_ is still invalid
        return nullptr;
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

EUREX_MDS::EUREXLSCommonStruct* EobiOrderBook::OrderDelete(uint8_t t_side_, double t_price_, int t_size_,
                                                           bool t_delete_order_, bool t_intermediate_) {
  int int_price_ = fast_price_convertor_->GetFastIntPx(t_price_);

  if (!price_adjustment_set_) {
    price_adjustment_ = int_price_ - EOBID_START_INDEX;

    price_adjustment_set_ = true;
  }

  memset((eurex_mds_), 0, sizeof(EUREX_MDS::EUREXLSCommonStruct));
  eurex_mds_->msg_ = EUREX_MDS::EUREX_DELTA;
  eurex_mds_->data_.eurex_dels_.contract_code_ = contract_code_;
  eurex_mds_->data_.eurex_dels_.intermediate_ = t_intermediate_;
  eurex_mds_->data_.eurex_dels_.level_ = 0;
  eurex_mds_->data_.eurex_dels_.price_ = t_price_;
  eurex_mds_->data_.eurex_dels_.type_ = t_side_;

  switch (t_side_) {
    case '2': {
      int index_ = int_price_ - price_adjustment_;

      if (index_ < 0 || index_ >= eobid_order_book_size_) {
        // Skip the order if the index_ is still invalid
        return nullptr;
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
      int index_ = eobid_order_book_size_ - (int_price_ - price_adjustment_);

      if (index_ < 0 || index_ >= eobid_order_book_size_) {
        // Skip the order if the index_ is still invalid
        return nullptr;
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

EUREX_MDS::EUREXLSCommonStruct* EobiOrderBook::OrderMassDelete() {
  memset(eurex_mds_, 0, sizeof(EUREX_MDS::EUREXLSCommonStruct));
  eurex_mds_->msg_ = EUREX_MDS::EUREX_DELTA;
  eurex_mds_->data_.eurex_dels_.contract_code_ = contract_code_;
  eurex_mds_->data_.eurex_dels_.action_ = '5';

  bid_levels_.clear();
  ask_levels_.clear();

  bid_levels_.resize(eobid_order_book_size_, EobiOrderLevel(0, 0));
  ask_levels_.resize(eobid_order_book_size_, EobiOrderLevel(0, 0));

  top_bid_index_ = -1;
  top_ask_index_ = -1;

  return eurex_mds_;
}

EUREX_MDS::EUREXLSCommonStruct* EobiOrderBook::FullOrderExecution(uint8_t t_side_, double t_price_, int t_size_) {
  // Marking this delta for trade intermediate as per request
  EUREX_MDS::EUREXLSCommonStruct* eurex_ls_ = OrderDelete(t_side_, t_price_, t_size_, true, true);

  if (eurex_ls_ == nullptr) {
    return nullptr;
  }

  switch (t_side_) {
    case '2': {
      if (bid_side_trade_size_ > 0) {
        bid_side_trade_size_ -= t_size_;

        // Skip intermediate updates by returning nullptr
        if (bid_side_trade_size_ > 0) {
          if (eurex_ls_->data_.eurex_dels_.action_ == '3') {
            return eurex_ls_;
          } else {
            return nullptr;
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
            return nullptr;
          }
        }
      }
    } break;
    default:
      break;
  }

  return eurex_ls_;
}

EUREX_MDS::EUREXLSCommonStruct* EobiOrderBook::PartialOrderExecution(uint8_t t_side_, double t_price_, int t_size_) {
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

EUREX_MDS::EUREXLSCommonStruct* EobiOrderBook::ExecutionSummary(uint8_t t_side_, double t_price_, int t_size_) {
  memset((eurex_mds_), 0, sizeof(EUREX_MDS::EUREXLSCommonStruct));
  eurex_mds_->msg_ = EUREX_MDS::EUREX_TRADE;
  eurex_mds_->data_.eurex_trds_.contract_code_ = contract_code_;
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

void EobiOrderBook::RecenterBook() {
  // doubling the allowed price-limits
  int old_eobid_order_book_size = eobid_order_book_size_;
  int old_eobid_start_index = eobid_start_index_;

  eobid_order_book_size_ *= 2;
  eobid_start_index_ *= 2;

  auto temp_bid_levels = bid_levels_;
  auto temp_ask_levels = ask_levels_;

  bid_levels_.resize(eobid_order_book_size_, EobiOrderLevel(0, 0));
  ask_levels_.resize(eobid_order_book_size_, EobiOrderLevel(0, 0));

  for (auto i = 0; i < old_eobid_order_book_size; i++) {
    bid_levels_[eobid_start_index_ - (old_eobid_start_index - i)] = temp_bid_levels[i];
  }

  for (auto i = 0; i < old_eobid_order_book_size; i++) {
    ask_levels_[eobid_start_index_ - (old_eobid_start_index - i)] = temp_ask_levels[i];
  }

  price_adjustment_ -= (eobid_start_index_ - old_eobid_start_index);
}
}
