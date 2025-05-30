/*
 * order_feed_to_price_feed.cpp
 *
 *  Created on: 07-Aug-2014
 *      Author: diwakar
 */

#include "dvccode/Utils/bulk_file_reader.hpp"
#include "dvccode/Utils/bulk_file_writer.hpp"
#include "infracore/HKOMDD/hkomd_order_book.hpp"
#include "dvccode/CDef/hkomd_mds_defines.hpp"

#include <string>
#include <bitset>

typedef struct OrderInfo {
  double price_;
  int size_;
  uint8_t side_;
} OrderInfo;

std::map<uint64_t, OrderInfo> bidside_order_id_to_order_info_;
std::map<uint64_t, OrderInfo> askside_order_id_to_order_info_;
bool last_message_was_trade_;
uint64_t last_trade_id_;
std::string contract_name;

void ProcessStruct(HKOMD_MDS::HKOMDPFCommonStruct* struct_, HFSAT::BulkFileWriter& writer_, timeval& time_) {
  // timeval time_ ;
  // gettimeofday(& time_, NULL ) ;
  if (struct_ == NULL) {
    std::cerr << "Could not create Struct." << std::endl;
    return;
  }
  struct_->time_ = time_;
  strcpy(struct_->data_.delta_.contract_, contract_name.c_str());
  strcpy(struct_->data_.trade_.contract_, contract_name.c_str());
  writer_.Write(struct_, sizeof(HKOMD_MDS::HKOMDPFCommonStruct));
  writer_.CheckToFlushBuffer();
}

void ConvertToPrice(HFSAT::BulkFileReader& reader_, HFSAT::BulkFileWriter& writer_, std::string t_shortcode_) {
  HFSAT::HKOMDOrderBook* order_book_ = new HFSAT::HKOMDOrderBook(t_shortcode_, 0);

  HKOMD_MDS::HKOMDCommonStruct of_next_event_;
  HKOMD_MDS::HKOMDPFCommonStruct pf_next_event_;
  if (reader_.is_open()) {
    while (true) {
      size_t len_ = reader_.read(&of_next_event_, sizeof(HKOMD_MDS::HKOMDCommonStruct));
      if (len_ < sizeof(of_next_event_)) break;
      HKOMD_MDS::msgType msg_type_;

      std::cout << of_next_event_.time_.tv_sec << "." << std::setw(6) << std::setfill('0')
                << of_next_event_.time_.tv_usec << " ";
      //      std::cout << "ToString: " << of_next_event_.ToString() << "\n";
      //      std::cout << "Contract: " << of_next_event_.getContract() << "\n";

      strcpy(pf_next_event_.data_.delta_.contract_, of_next_event_.getContract());
      strcpy(pf_next_event_.data_.trade_.contract_, of_next_event_.getContract());

      contract_name = of_next_event_.getContract();

      switch (of_next_event_.msg_) {
        case HKOMD_MDS::HKOMD_ORDER: {
          int side_ = of_next_event_.data_.order_.side_;
          double price_ = of_next_event_.data_.order_.price_;
          int level_ = of_next_event_.data_.order_.level_;
          int size_ = of_next_event_.data_.order_.quantity_;
          char intermediate_ = of_next_event_.data_.order_.intermediate_;
          if (last_message_was_trade_) {
            ProcessStruct(order_book_->FlushTrades(), writer_, of_next_event_.time_);
            ProcessStruct(order_book_->MarketUpdateForFlushedTrades(), writer_, of_next_event_.time_);
          }
          switch (of_next_event_.data_.order_.action_) {
            case '0': {
              OrderInfo this_order_info_;
              this_order_info_.size_ = size_;
              this_order_info_.price_ = price_;
              this_order_info_.side_ = side_;
              if (side_ == 0) {
                bidside_order_id_to_order_info_[of_next_event_.data_.order_.order_id_] = this_order_info_;
              } else {
                askside_order_id_to_order_info_[of_next_event_.data_.order_.order_id_] = this_order_info_;
              }

              ProcessStruct(order_book_->OrderAdd(side_, price_, size_, level_, intermediate_ == 'Y'), writer_,
                            of_next_event_.time_);
            } break;
            case '1': {
              OrderInfo this_order_info_;
              this_order_info_.size_ = size_;
              this_order_info_.price_ = price_;
              this_order_info_.side_ = side_;

              if (side_ == 0) {
                bidside_order_id_to_order_info_[of_next_event_.data_.order_.order_id_] = this_order_info_;
              } else {
                askside_order_id_to_order_info_[of_next_event_.data_.order_.order_id_] = this_order_info_;
              }
            } break;
            case '2': {
              if (side_ == 0) {
                if (bidside_order_id_to_order_info_.find(of_next_event_.data_.order_.order_id_) ==
                    bidside_order_id_to_order_info_.end()) {
                  std::cerr << " Bid Order not found to delete " << of_next_event_.data_.order_.order_id_ << std::endl;
                } else {
                  std::cout << " Calling delete with side: " << side_ << std::endl;
                  ProcessStruct(
                      order_book_->OrderDelete(
                          side_, bidside_order_id_to_order_info_[of_next_event_.data_.order_.order_id_].price_,
                          bidside_order_id_to_order_info_[of_next_event_.data_.order_.order_id_].size_, true,
                          intermediate_ == 'Y'),
                      writer_, of_next_event_.time_);
                }
              } else if (side_ == 1) {
                if (askside_order_id_to_order_info_.find(of_next_event_.data_.order_.order_id_) ==
                    askside_order_id_to_order_info_.end()) {
                  std::cerr << " Ask Order not found to delete " << of_next_event_.data_.order_.order_id_ << std::endl;
                } else {
                  bool delete_ = false;
                  int size1_ = askside_order_id_to_order_info_[of_next_event_.data_.order_.order_id_].size_;
                  if (size1_ == size_) {
                    delete_ = true;
                  }
                  std::cout << " Calling ask delete with side: " << side_ << std::endl;
                  ProcessStruct(
                      order_book_->OrderDelete(
                          side_, askside_order_id_to_order_info_[of_next_event_.data_.order_.order_id_].price_,
                          askside_order_id_to_order_info_[of_next_event_.data_.order_.order_id_].size_, true,
                          intermediate_),
                      writer_, of_next_event_.time_);
                }
              }
            }
            default:
              break;
          }
          last_message_was_trade_ = false;
        } break;
        case HKOMD_MDS::HKOMD_TRADE: {
          uint8_t side_ = of_next_event_.data_.trade_.side_;
          double price_ = of_next_event_.data_.trade_.price_;
          int size_ = of_next_event_.data_.trade_.quantity_;
          uint16_t deal_info_ = of_next_event_.data_.trade_.deal_info_;
          uint8_t deal_type_ = of_next_event_.data_.trade_.deal_type_;
          int qty_ = 0;
          if (of_next_event_.data_.trade_.order_id_ == 0) {
            break;
          }
          if (!(deal_type_ & (1 << 0))) {
            break;
          }

          if ((last_trade_id_ != of_next_event_.data_.trade_.trade_id_ ||
               order_book_->CheckToFlushTrades(side_, price_))) {
            ProcessStruct(order_book_->FlushTrades(), writer_, of_next_event_.time_);
            ProcessStruct(order_book_->MarketUpdateForFlushedTrades(), writer_, of_next_event_.time_);
          }

          if (side_ == 0) {
            std::cout << " Side 0 " << std::endl;
            if (bidside_order_id_to_order_info_.find(of_next_event_.data_.order_.order_id_) !=
                bidside_order_id_to_order_info_.end()) {
              side_ = bidside_order_id_to_order_info_[of_next_event_.data_.order_.order_id_].side_;
              qty_ = bidside_order_id_to_order_info_[of_next_event_.data_.order_.order_id_].size_;
            }
            std::cout << " side : " << side_ << std::endl;
            if (askside_order_id_to_order_info_.find(of_next_event_.data_.order_.order_id_) !=
                askside_order_id_to_order_info_.end()) {
              side_ = askside_order_id_to_order_info_[of_next_event_.data_.order_.order_id_].side_;
              qty_ = askside_order_id_to_order_info_[of_next_event_.data_.order_.order_id_].size_;
            }
            std::cout << " side: " << side_ << std::endl;
          }

          else if (side_ == 2) {
            qty_ = bidside_order_id_to_order_info_[of_next_event_.data_.trade_.order_id_].size_;
          } else if (side_ == 3) {
            qty_ = askside_order_id_to_order_info_[of_next_event_.data_.trade_.order_id_].size_;
          }

          order_book_->Trade(side_, price_, size_, qty_, deal_info_, deal_type_);

          // Reduce the remaining size of the order
          if (side_ == 2) {
            bidside_order_id_to_order_info_[of_next_event_.data_.trade_.order_id_].size_ -= size_;
          } else if (side_ == 3) {
            askside_order_id_to_order_info_[of_next_event_.data_.trade_.order_id_].size_ -= size_;
          }

          // ProcessStruct( order_book_->FlushTrades(), writer_, of_next_event_.time_ ) ;
          // ProcessStruct( order_book_->MarketUpdateForFlushedTrades(), writer_, of_next_event_.time_ );
          last_trade_id_ = of_next_event_.data_.trade_.trade_id_;
          last_message_was_trade_ = true;
        }

        break;
      }

      if (last_message_was_trade_) {
        ProcessStruct(order_book_->FlushTrades(), writer_, of_next_event_.time_);
        ProcessStruct(order_book_->MarketUpdateForFlushedTrades(), writer_, of_next_event_.time_);
        last_message_was_trade_ = false;
      }
    }
  }
  reader_.close();
  writer_.Close();
}
int main(int argc, char** argv) {
  if (argc < 3) {
    printf(" USAGE: source_of_file dest_pf_file_  shc_ \n");
    exit(0);
  }
  HFSAT::BulkFileReader reader_;
  reader_.open(argv[1]);

  HFSAT::BulkFileWriter writer_;
  writer_.Open(argv[2]);
  std::string shortcode_ = std::string(argv[3]);
  ConvertToPrice(reader_, writer_, shortcode_);
}
