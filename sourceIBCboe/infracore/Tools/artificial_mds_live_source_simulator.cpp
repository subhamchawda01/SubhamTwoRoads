/**
   \file Tools/live_source_simulator.cpp

   \author: (c) Copyright Two Roads Technological Solutions Pvt Ltd 2011
   Address:
   Suite 217, Level 2, Prestige Omega,
   No 104, EPIP Zone, Whitefield,
   Bangalore - 560066
   India
   +91 80 4060 0717
*/

#include <iostream>
#include <stdlib.h>

#include "dvccode/CDef/defines.hpp"
#include "dvccode/CDef/ttime.hpp"
#include "dvccode/CDef/stored_market_data_common_message_defines.hpp"

#include "dvccode/CommonTradeUtils/date_time.hpp"
#include "dvccode/CommonDataStructures/perishable_string_tokenizer.hpp"
#include "dvccode/CDef/exchange_symbol_manager.hpp"

#include "dvccode/Utils/bulk_file_reader.hpp"
#include "dvccode/Utils/multicast_sender_socket.hpp"

void LoadBasePrice(double& bid_price_, double& ask_price_) {
  std::string base_price_filename_ = "/home/ravi/base_price.txt";

  std::ifstream base_price_file_;

  base_price_file_.open(base_price_filename_.c_str());

  if (!base_price_file_.is_open()) {
    std::cout << " Can't open Base Price File \n";
    exit(1);
  }

  char buffer[1024];

  memset(buffer, 0, 1024);

  while (base_price_file_.good()) {
    base_price_file_.getline(buffer, 1024);

    std::string this_temp_str_ = buffer;

    if (this_temp_str_.length() <= 1) break;

    bid_price_ = atof(buffer);

    memset(buffer, 0, 1024);

    base_price_file_.getline(buffer, 1024);

    this_temp_str_ = buffer;

    if (this_temp_str_.length() <= 1) break;

    ask_price_ = atof(buffer);
  }

  base_price_file_.close();
}

void ReadMDSStructs(HFSAT::BulkFileReader& bulk_file_reader_, const HFSAT::ExchSource_t& exch, const int& st,
                    const int& et, const char* bcast_ip_, const int bcast_port_, std::string interface_) {
  HFSAT::MulticastSenderSocket mcast_sender_(bcast_ip_, bcast_port_, interface_);

  unsigned long last_packet_send_time_ = 0;

  if (bulk_file_reader_.is_open()) {
    switch (exch) {
      case HFSAT::kExchSourceBMF: {
        NTP_MDS::NTPCommonStruct next_event_;
        while (true) {
          size_t available_len_ = bulk_file_reader_.read(&next_event_, sizeof(next_event_));

          if (last_packet_send_time_ != 0) {
            unsigned int wait_time_ =
                (next_event_.time_.tv_sec * 1000000 + next_event_.time_.tv_usec) - last_packet_send_time_;

            std::cerr << " Wait Time : " << wait_time_ << "\n";

            int sec = (wait_time_ / 1000000);
            int usec = (wait_time_ % 1000000);

            if (sec || usec) {
              for (; sec > 0; sec--) {
                usleep(99999);
              }

              usleep(usec);
            }
          }

          last_packet_send_time_ = next_event_.time_.tv_sec * 1000000 + next_event_.time_.tv_usec;

          if (available_len_ < sizeof(next_event_)) break;
          HFSAT::ttime_t next_event_timestamp_ = next_event_.time_;
          if (et < next_event_timestamp_.tv_sec || next_event_timestamp_.tv_sec < st)  // time filter
            continue;

          gettimeofday(&next_event_.time_, NULL);
          mcast_sender_.WriteN(sizeof(next_event_), &next_event_);
        }
      } break;
      case HFSAT::kExchSourceCME: {
        CME_MDS::CMECommonStruct next_event_;
        while (true) {
          size_t available_len_ = bulk_file_reader_.read(&next_event_, sizeof(next_event_));

          if (available_len_ < sizeof(next_event_)) break;

          if (last_packet_send_time_ != 0) {
            unsigned int wait_time_ =
                (next_event_.time_.tv_sec * 1000000 + next_event_.time_.tv_usec) - last_packet_send_time_;

            std::cerr << " Wait Time : " << wait_time_ << "\n";

            int sec = (wait_time_ / 1000000);
            int usec = (wait_time_ % 1000000);

            if (sec || usec) {
              for (; sec > 0; sec--) {
                usleep(99999);
              }

              usleep(usec);
            }
          }

          last_packet_send_time_ = next_event_.time_.tv_sec * 1000000 + next_event_.time_.tv_usec;

          HFSAT::ttime_t next_event_timestamp_ = next_event_.time_;

          if (et < next_event_timestamp_.tv_sec || next_event_timestamp_.tv_sec < st)  // time filter
            continue;

          gettimeofday(&next_event_.time_, NULL);
          mcast_sender_.WriteN(sizeof(next_event_), &next_event_);
        }
      } break;
      case HFSAT::kExchSourceEUREX: {
#if USE_LOW_BANDWIDTH_EUREX_MDS_STRUCTS

        // For Reducing the bandwidth consumption & jittery latency
        //

        std::map<uint8_t, bool> live_source_products_list_;
        std::map<std::string, std::string> exchange_symbol_to_shortcode_map_;
        std::map<std::string, uint8_t> shortcode_to_product_code_map_;

        std::ifstream livesource_contractcode_shortcode_mapping_file_;

        livesource_contractcode_shortcode_mapping_file_.open(DEF_LS_PRODUCTCODE_SHORTCODE_);

        if (!livesource_contractcode_shortcode_mapping_file_.is_open()) {
          std::cerr << " Couldn't open broadcast product list file : " << DEF_LS_PRODUCTCODE_SHORTCODE_ << "\n";
          exit(1);
        }

        std::cerr << " Read Cntract Code To Shortcode List File : " << DEF_LS_PRODUCTCODE_SHORTCODE_ << "\n";

        char productcode_shortcode_line_[1024];

        int tradingdate_ = HFSAT::DateTime::GetCurrentIsoDateLocal();
        HFSAT::ExchangeSymbolManager::SetUniqueInstance(tradingdate_);

        while (livesource_contractcode_shortcode_mapping_file_.good()) {
          livesource_contractcode_shortcode_mapping_file_.getline(productcode_shortcode_line_, 1024);

          std::string check_for_comment_ = productcode_shortcode_line_;

          if (check_for_comment_.find("#") != std::string::npos) continue;  // comments

          HFSAT::PerishableStringTokenizer st_(productcode_shortcode_line_, 1024);
          const std::vector<const char*>& tokens_ = st_.GetTokens();

          if (tokens_.size() != 2) continue;  // mal formatted

          uint8_t this_contract_code_ = uint8_t(atoi(tokens_[0]));
          std::string this_shortcode_ = tokens_[1];
          std::string this_exch_symbol_ = HFSAT::ExchangeSymbolManager::GetExchSymbol(this_shortcode_);

          if (live_source_products_list_.find(this_contract_code_) == live_source_products_list_.end()) {
            live_source_products_list_[this_contract_code_] = true;
          }

          // don't really need two maps, can use only one
          if (exchange_symbol_to_shortcode_map_.find(this_exch_symbol_) == exchange_symbol_to_shortcode_map_.end()) {
            exchange_symbol_to_shortcode_map_[this_exch_symbol_] = this_shortcode_;

            if (shortcode_to_product_code_map_.find(this_shortcode_) == shortcode_to_product_code_map_.end()) {
              shortcode_to_product_code_map_[this_shortcode_] = this_contract_code_;
            }
          }
        }

        livesource_contractcode_shortcode_mapping_file_.close();

        EUREX_MDS::EUREXCommonStruct next_event_;

        struct timeval this_global_time_;

        gettimeofday(&this_global_time_, NULL);

        unsigned long long last_global_time_ = this_global_time_.tv_sec * 1000000 + this_global_time_.tv_usec;

        double base_bid_price_ = 0.0;
        double base_ask_price_ = 0.0;

        unsigned int send_sequnece_ = 1;

        while (true) {
          size_t available_len_ = bulk_file_reader_.read(&next_event_, sizeof(next_event_));

          if (available_len_ < sizeof(next_event_)) break;

          gettimeofday(&this_global_time_, NULL);

          unsigned long long this_global_t_ = this_global_time_.tv_sec * 1000000 + this_global_time_.tv_usec;

          if (this_global_t_ - last_global_time_ > 10000000) {
            LoadBasePrice(base_bid_price_, base_ask_price_);

            std::cout << " Time : " << this_global_t_ << " Base Bid : " << base_bid_price_
                      << " Base Ask : " << base_ask_price_ << "\n";
            last_global_time_ = this_global_t_;
          }

          if (last_packet_send_time_ != 0) {
            unsigned int wait_time_ =
                (next_event_.time_.tv_sec * 1000000 + next_event_.time_.tv_usec) - last_packet_send_time_;

            std::cerr << " Wait Time : " << wait_time_ << "\n";

            int sec = (wait_time_ / 1000000);
            int usec = (wait_time_ % 1000000);

            sec = 0;
            usec = 100000;

            if (sec || usec) {
              for (; sec > 0; sec--) {
                usleep(99999);
              }

              usleep(usec);
            }
          }

          last_packet_send_time_ = next_event_.time_.tv_sec * 1000000 + next_event_.time_.tv_usec;

          HFSAT::ttime_t next_event_timestamp_ = next_event_.time_;
          if (et < next_event_timestamp_.tv_sec || next_event_timestamp_.tv_sec < st)  // time filter
            continue;

          EUREX_MDS::EUREXLSCommonStruct eurex_ls_event_;

#if SEND_TIME_OVER_LIVE_SOURCE

          gettimeofday(&eurex_ls_event_.time_, NULL);

#endif
          eurex_ls_event_.msg_ = next_event_.msg_;

          //==============================   Add sequence ======================= //

          eurex_ls_event_.msg_sequence_ = send_sequnece_;
          send_sequnece_++;

          //=====================================================================//

          if (next_event_.msg_ == EUREX_MDS::EUREX_DELTA) {
            std::string this_exch_symbol_ = next_event_.data_.eurex_dels_.contract_;

            // stricter check, can be removed safely assuming files were loaded correctly
            if (exchange_symbol_to_shortcode_map_.find(this_exch_symbol_) != exchange_symbol_to_shortcode_map_.end()) {
              if (shortcode_to_product_code_map_.find(exchange_symbol_to_shortcode_map_[this_exch_symbol_]) !=
                  shortcode_to_product_code_map_.end()) {
                eurex_ls_event_.data_.eurex_dels_.contract_code_ =
                    shortcode_to_product_code_map_[exchange_symbol_to_shortcode_map_[this_exch_symbol_]];
              }
            }

            eurex_ls_event_.data_.eurex_dels_.level_ = (uint8_t)next_event_.data_.eurex_dels_.level_;

            eurex_ls_event_.data_.eurex_dels_.size_ = next_event_.data_.eurex_dels_.size_;

            HFSAT::TradeType_t _buysell_ = HFSAT::TradeType_t('2' - next_event_.data_.eurex_dels_.type_);

            if (_buysell_ == HFSAT::kTradeTypeBuy) {
              eurex_ls_event_.data_.eurex_dels_.price_ =
                  base_bid_price_ - (eurex_ls_event_.data_.eurex_dels_.level_ - 1);

            } else if (_buysell_ == HFSAT::kTradeTypeSell) {
              eurex_ls_event_.data_.eurex_dels_.price_ =
                  base_ask_price_ + (eurex_ls_event_.data_.eurex_dels_.level_ - 1);

            } else {
              continue;
            }

            eurex_ls_event_.data_.eurex_dels_.type_ = next_event_.data_.eurex_dels_.type_;
            eurex_ls_event_.data_.eurex_dels_.action_ = next_event_.data_.eurex_dels_.action_;
            eurex_ls_event_.data_.eurex_dels_.intermediate_ = next_event_.data_.eurex_dels_.intermediate_;
            eurex_ls_event_.data_.eurex_dels_.num_ords_ = next_event_.data_.eurex_dels_.num_ords_;

          } else if (next_event_.msg_ == EUREX_MDS::EUREX_TRADE) {
            continue;

            std::string this_exch_symbol_ = next_event_.data_.eurex_trds_.contract_;

            // stricter check, can be removed safely assuming files were loaded correctly
            if (exchange_symbol_to_shortcode_map_.find(this_exch_symbol_) != exchange_symbol_to_shortcode_map_.end()) {
              if (shortcode_to_product_code_map_.find(exchange_symbol_to_shortcode_map_[this_exch_symbol_]) !=
                  shortcode_to_product_code_map_.end()) {
                eurex_ls_event_.data_.eurex_trds_.contract_code_ =
                    shortcode_to_product_code_map_[exchange_symbol_to_shortcode_map_[this_exch_symbol_]];
              }
            }

            eurex_ls_event_.data_.eurex_trds_.trd_qty_ = next_event_.data_.eurex_trds_.trd_qty_;
            eurex_ls_event_.data_.eurex_trds_.agg_side_ = next_event_.data_.eurex_trds_.agg_side_;
            eurex_ls_event_.data_.eurex_trds_.trd_px_ = next_event_.data_.eurex_trds_.trd_px_;

          } else {
            std::cout << " Invalid Msg Type : \n";
          }

          mcast_sender_.WriteN(sizeof(eurex_ls_event_), &eurex_ls_event_);
        }

#else
        EUREX_MDS::EUREXCommonStruct next_event_;
        while (true) {
          size_t available_len_ = bulk_file_reader_.read(&next_event_, sizeof(next_event_));

          if (available_len_ < sizeof(next_event_)) break;

          if (last_packet_send_time_ != 0) {
            unsigned int wait_time_ =
                (next_event_.time_.tv_sec * 1000000 + next_event_.time_.tv_usec) - last_packet_send_time_;

            std::cerr << " Wait Time : " << wait_time_ << "\n";

            int sec = (wait_time_ / 1000000);
            int usec = (wait_time_ % 1000000);

            if (sec || usec) {
              for (; sec > 0; sec--) {
                usleep(99999);
              }

              usleep(usec);
            }
          }

          last_packet_send_time_ = next_event_.time_.tv_sec * 1000000 + next_event_.time_.tv_usec;

          HFSAT::ttime_t next_event_timestamp_ = next_event_.time_;
          if (et < next_event_timestamp_.tv_sec || next_event_timestamp_.tv_sec < st)  // time filter
            continue;

          gettimeofday(&next_event_.time_, NULL);
          mcast_sender_.WriteN(sizeof(next_event_), &next_event_);
        }
#endif
      } break;
      case HFSAT::kExchSourceTMX: {
        TMX_MDS::TMXCommonStruct next_event_;
        while (true) {
          size_t available_len_ = bulk_file_reader_.read(&next_event_, sizeof(next_event_));
          if (available_len_ < sizeof(next_event_)) break;

          if (last_packet_send_time_ != 0) {
            unsigned int wait_time_ =
                (next_event_.time_.tv_sec * 1000000 + next_event_.time_.tv_usec) - last_packet_send_time_;

            std::cerr << " Wait Time : " << wait_time_ << "\n";

            int sec = (wait_time_ / 1000000);
            int usec = (wait_time_ % 1000000);

            if (sec || usec) {
              for (; sec > 0; sec--) {
                usleep(99999);
              }

              usleep(usec);
            }
          }

          last_packet_send_time_ = next_event_.time_.tv_sec * 1000000 + next_event_.time_.tv_usec;

          HFSAT::ttime_t next_event_timestamp_ = next_event_.time_;

          if (et < next_event_timestamp_.tv_sec || next_event_timestamp_.tv_sec < st)  // time filter
            continue;

          gettimeofday(&next_event_.time_, NULL);
          mcast_sender_.WriteN(sizeof(next_event_), &next_event_);
        }
      } break;
      case HFSAT::kExchSourceHONGKONG: {
        HKEX_MDS::HKEXCommonStruct next_event_;
        while (true) {
          size_t available_len_ = bulk_file_reader_.read(&next_event_, sizeof(next_event_));
          if (available_len_ < sizeof(next_event_)) break;

          if (last_packet_send_time_ != 0) {
            unsigned int wait_time_ =
                (next_event_.time_.tv_sec * 1000000 + next_event_.time_.tv_usec) - last_packet_send_time_;

            std::cerr << " Wait Time : " << wait_time_ << "\n";

            int sec = (wait_time_ / 1000000);
            int usec = (wait_time_ % 1000000);

            if (sec || usec) {
              for (; sec > 0; sec--) {
                usleep(99999);
              }

              usleep(usec);
            }
          }

          last_packet_send_time_ = next_event_.time_.tv_sec * 1000000 + next_event_.time_.tv_usec;

          HFSAT::ttime_t next_event_timestamp_ = next_event_.time_;

          if (et < next_event_timestamp_.tv_sec || next_event_timestamp_.tv_sec < st)  // time filter
            continue;

          gettimeofday(&next_event_.time_, NULL);
          mcast_sender_.WriteN(sizeof(next_event_), &next_event_);
        }
      } break;
      default:
        std::cout << "exchange not implemented for\n";
        break;
    }
  }
}

int main(int argc, char** argv) {
  if (argc != 6 && argc != 8) {
    std::cout << " USAGE: EXEC <exchange> <file_path> <bcast_ip> <bcast_port> <interface> "
                 "<start-time-optional-unix-time-sec> <end-time-optional-unix-time-sec>"
              << std::endl;
    exit(0);
  }

  std::string exch = argv[1];
  HFSAT::ExchSource_t exch_src = HFSAT::kExchSourceMAX;
  if (exch == "EUREX")
    exch_src = HFSAT::kExchSourceEUREX;
  else if (exch == "CME")
    exch_src = HFSAT::kExchSourceCME;
  else if (exch == "BMF")
    exch_src = HFSAT::kExchSourceBMF;
  else if (exch == "TMX")
    exch_src = HFSAT::kExchSourceTMX;
  else if (exch == "HKEX")
    exch_src = HFSAT::kExchSourceHONGKONG;

  HFSAT::BulkFileReader reader;
  reader.open(argv[2]);

  std::string interface_ = argv[5];

  int st = 0;
  int et = 0x7fffffff;
  if (argc == 7) {
    st = atol(argv[6]);
    et = atol(argv[7]);
  }

  ReadMDSStructs(reader, exch_src, st, et, argv[3], atoi(argv[4]), interface_);
  return 0;
}
