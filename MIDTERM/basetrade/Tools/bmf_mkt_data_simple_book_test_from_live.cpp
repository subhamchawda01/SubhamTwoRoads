#include <iostream>
#include <stdio.h>
#include <stdlib.h>
#include <vector>

#include "dvccode/CDef/defines.hpp"
#include "dvccode/CDef/stored_market_data_common_message_defines.hpp"

// #include "dvccode/Utils/multicast_receiver_socket.hpp"
#include "dvccode/Utils/udp_receiver_socket.hpp"

struct BMFMapOrder {
  int order_id_;
  int int_price_;
  double price_double_;
  int size_;
  HFSAT::TradeType_t buysell_;

  BMFMapOrder() {}

  BMFMapOrder(int t_order_id_, int t_int_price_, double t_price_double_, int t_size_, HFSAT::TradeType_t t_buysell_)
      : order_id_(t_order_id_),
        int_price_(t_int_price_),
        price_double_(t_price_double_),
        size_(t_size_),
        buysell_(t_buysell_) {}

  void set(int t_order_id_, int t_int_price_, double t_price_double_, int t_size_, HFSAT::TradeType_t t_buysell_) {
    order_id_ = t_order_id_;
    int_price_ = t_int_price_;
    price_double_ = t_price_double_;
    size_ = t_size_;
    buysell_ = t_buysell_;
  }

  std::string ToString() {
    std::ostringstream t_temp_oss_;

    t_temp_oss_.width(6);
    t_temp_oss_ << order_id_;
    t_temp_oss_.width(6);
    t_temp_oss_ << int_price_;
    t_temp_oss_.width(10);
    t_temp_oss_ << price_double_;
    t_temp_oss_.width(5);
    t_temp_oss_ << size_;
    t_temp_oss_.width(3);
    t_temp_oss_ << ((buysell_ == HFSAT::kTradeTypeBuy) ? 'B' : 'S');

    return t_temp_oss_.str();
  }
};

/// This class holds all the info for 1 security
struct BMFOrderMarketView {
  BMFOrderMarketView() {
    bid_order_depth_book_.clear();
    ask_order_depth_book_.clear();
  }

 public:
  // These structures maintain the order depth book for the bid side and the ask side.
  std::vector<BMFMapOrder> bid_order_depth_book_;
  std::vector<BMFMapOrder> ask_order_depth_book_;

  void Process(BMF_MDS::BMFCommonStruct& next_event_) {
    HFSAT::TradeType_t _buysell_ = HFSAT::TradeType_t(next_event_.data_.bmf_ordr_.type_ - '0');

    if (next_event_.data_.bmf_ordr_.num_ords_ == 111) {
      next_event_.data_.bmf_ordr_.num_ords_ = 1;
      if (bid_order_depth_book_.size() < next_event_.data_.bmf_ordr_.level_) {
        bid_order_depth_book_.resize(next_event_.data_.bmf_ordr_.level_);
      }

      switch (next_event_.data_.bmf_ordr_.action_) {
        case '0': {
          if (_buysell_ == HFSAT::kTradeTypeBuy) {
            bid_order_depth_book_[next_event_.data_.bmf_ordr_.level_ - 1].set(
                next_event_.data_.bmf_ordr_.order_id_, (int)(next_event_.data_.bmf_ordr_.price_ * 1000),
                next_event_.data_.bmf_ordr_.price_, next_event_.data_.bmf_ordr_.size_, _buysell_);
          } else {
            ask_order_depth_book_[next_event_.data_.bmf_ordr_.level_ - 1].set(
                next_event_.data_.bmf_ordr_.order_id_, (int)(next_event_.data_.bmf_ordr_.price_ * 1000),
                next_event_.data_.bmf_ordr_.price_, next_event_.data_.bmf_ordr_.size_, _buysell_);
          }
        } break;
        case '1': {
        } break;
        case '2': {
        } break;
      }
    } else {
      switch (next_event_.data_.bmf_ordr_.action_) {
        case '0': {
          BMFMapOrder b_m_o_(next_event_.data_.bmf_ordr_.order_id_, (int)(next_event_.data_.bmf_ordr_.price_ * 1000),
                             next_event_.data_.bmf_ordr_.price_, next_event_.data_.bmf_ordr_.size_, _buysell_);
          if (_buysell_ == HFSAT::kTradeTypeBuy) {
            if (bid_order_depth_book_.size() < next_event_.data_.bmf_ordr_.level_) {
              bid_order_depth_book_.resize(next_event_.data_.bmf_ordr_.level_);
            }
            std::vector<BMFMapOrder>::iterator bmoviter = bid_order_depth_book_.begin();
            for (int i = 0; i < next_event_.data_.bmf_ordr_.level_ - 1; i++) {
              bmoviter++;
            }
            bid_order_depth_book_.insert(bmoviter, b_m_o_);
          } else {
            if (ask_order_depth_book_.size() < next_event_.data_.bmf_ordr_.level_) {
              ask_order_depth_book_.resize(next_event_.data_.bmf_ordr_.level_);
            }
            std::vector<BMFMapOrder>::iterator bmoviter = ask_order_depth_book_.begin();
            for (int i = 0; i < next_event_.data_.bmf_ordr_.level_ - 1; i++) {
              bmoviter++;
            }
            ask_order_depth_book_.insert(bmoviter, b_m_o_);
          }
        } break;
        case '1': {
          BMFMapOrder b_m_o_(next_event_.data_.bmf_ordr_.order_id_, (int)(next_event_.data_.bmf_ordr_.price_ * 1000),
                             next_event_.data_.bmf_ordr_.price_, next_event_.data_.bmf_ordr_.size_, _buysell_);
          if (_buysell_ == HFSAT::kTradeTypeBuy) {
            if (bid_order_depth_book_.size() < next_event_.data_.bmf_ordr_.level_) {
              bid_order_depth_book_.resize(next_event_.data_.bmf_ordr_.level_);
            }
            std::vector<BMFMapOrder>::iterator bmoviter = bid_order_depth_book_.begin();
            for (int i = 0; i < next_event_.data_.bmf_ordr_.level_ - 1; i++) {
              bmoviter++;
            }
            *bmoviter = b_m_o_;
          } else {
            if (ask_order_depth_book_.size() < next_event_.data_.bmf_ordr_.level_) {
              ask_order_depth_book_.resize(next_event_.data_.bmf_ordr_.level_);
            }
            std::vector<BMFMapOrder>::iterator bmoviter = ask_order_depth_book_.begin();
            for (int i = 0; i < next_event_.data_.bmf_ordr_.level_ - 1; i++) {
              bmoviter++;
            }
            *bmoviter = b_m_o_;
          }
        } break;
        case '2': {
          if (_buysell_ == HFSAT::kTradeTypeBuy) {
            if (bid_order_depth_book_.size() >= next_event_.data_.bmf_ordr_.level_) {
              std::vector<BMFMapOrder>::iterator bmoviter = bid_order_depth_book_.begin();
              for (int i = 0; i < next_event_.data_.bmf_ordr_.level_ - 1; i++) {
                bmoviter++;
              }
              bid_order_depth_book_.erase(bmoviter);
            }
          } else {
            if (ask_order_depth_book_.size() >= next_event_.data_.bmf_ordr_.level_) {
              std::vector<BMFMapOrder>::iterator bmoviter = ask_order_depth_book_.begin();
              for (int i = 0; i < next_event_.data_.bmf_ordr_.level_ - 1; i++) {
                bmoviter++;
              }
              ask_order_depth_book_.erase(bmoviter);
            }
          }
        } break;

        case '9': {
          // Special book reset msg. Not part of the BMF spec. This is sent before processing a snapshot.
        } break;

        default: {
          fprintf(stderr, "Weird message type in BMFLiveDataSource::ProcessAllEvents BMF_ORDER %d | action_ %c\n",
                  (int)next_event_.msg_, next_event_.data_.bmf_ordr_.action_);
        } break;
      }
    }

    for (size_t i = 0; (i < bid_order_depth_book_.size()) & (i < ask_order_depth_book_.size()); i++) {
      std::cout << bid_order_depth_book_[i].ToString() << " X " << ask_order_depth_book_[i].ToString() << std::endl;
    }
  }
};

int main(int argc, char** argv) {
  if (argc < 4) {
    std::cerr << "Usage: " << argv[0] << " mcast_ip  mcast_port bmf_symbol_1 bmf_symbol_2 ... " << std::endl;
    exit(0);
  }

  std::vector<std::string> symbol_vec_;
  std::vector<BMFOrderMarketView> bmf_book_vec_;

  std::string md_udp_ip = argv[1];
  int md_udp_port = atoi(argv[2]);
  for (int i = 0; i < argc; i++) {
    symbol_vec_.push_back(argv[3]);
  }

  //    HFSAT::MulticastReceiverSocket multicast_receiver_socket_;
  HFSAT::UDPReceiverSocket multicast_receiver_socket_(md_udp_ip, md_udp_port);

  BMF_MDS::BMFCommonStruct next_event_;

  while (1) {
    int num_bytes = multicast_receiver_socket_.ReadN(sizeof(BMF_MDS::BMFCommonStruct), (void*)&next_event_);

    std::cout << "DEBUG " << next_event_.ToString();
    if (num_bytes < (int)sizeof(BMF_MDS::BMFCommonStruct)) {
      break;
    }
    std::cout << next_event_.ToString();

    if (next_event_.msg_ == BMF_MDS::BMF_ORDER) {
      for (size_t i = 0; i < symbol_vec_.size(); i++) {
        if (symbol_vec_[i].compare(next_event_.data_.bmf_ordr_.contract_) == 0) {
          if (bmf_book_vec_.size() <= i) {
            bmf_book_vec_.resize(i + 1);
          }
          bmf_book_vec_[i].Process(next_event_);
        }
      }
    }

    if (next_event_.msg_ == BMF_MDS::BMF_TRADE) {
      std::cout << next_event_.ToString();
    }
  }

  return 0;
}
