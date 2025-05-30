// =====================================================================================
//
//       Filename:  ors_conf_market_data_delay_computer.hpp
//
//    Description:
//
//        Version:  1.0
//        Created:  12/05/2012 09:35:20 AM
//       Revision:  none
//       Compiler:  g++
//
//         Author:  (c) Copyright Two Roads Technological Solutions Pvt Ltd 2011
//
//        Address:  Suite No 353, Evoma, #14, Bhattarhalli,
//                  Old Madras Road, Near Garden City College,
//                  KR Puram, Bangalore 560049, India
//          Phone:  +91 80 4190 3551
//
// =====================================================================================

#ifndef BASE_ORSMESSAGES_ORS_CONF_MARKET_DELAY_STATS_COMPUTER_H
#define BASE_ORSMESSAGES_ORS_CONF_MARKET_DELAY_STATS_COMPUTER_H

#include <iostream>
#include <stdio.h>
#include <stdlib.h>
#include <fstream>
#include <signal.h>
#include <getopt.h>
#include <map>
#include <algorithm>

#include "dvccode/CDef/debug_logger.hpp"
#include "dvccode/CDef/file_utils.hpp"  // To create the directory
#include "dvccode/CDef/stored_market_data_common_message_defines.hpp"
#include "dvccode/CDef/trading_location_manager.hpp"
#include "dvccode/CDef/exchange_symbol_manager.hpp"
#include "dvccode/CDef/defines.hpp"
#include "dvccode/CDef/ttime.hpp"
#include "dvccode/CDef/ors_defines.hpp"
#include "dvccode/CDef/security_definitions.hpp"

#include "dvccode/Utils/multicast_receiver_socket.hpp"
#include "dvccode/Utils/bulk_file_reader.hpp"
#include "dvccode/CDef/ors_messages.hpp"

#include "baseinfra/LoggedSources/cme_logged_message_filenamer.hpp"
#include "baseinfra/LoggedSources/eurex_logged_message_filenamer.hpp"
#include "baseinfra/LoggedSources/ntp_logged_message_filenamer.hpp"
#include "baseinfra/LoggedSources/liffe_logged_message_filenamer.hpp"
#include "baseinfra/LoggedSources/tmx_logged_message_filenamer.hpp"
#include "baseinfra/LoggedSources/hkex_logged_message_filenamer.hpp"
#include "baseinfra/LoggedSources/ose_pricefeed_logged_message_filenamer.hpp"
#include "baseinfra/MarketAdapter/shortcode_security_market_view_map.hpp"
#define MAX_DELAY_LIMIT 12000
#define MAX_CXL_DELAY_LIMIT 20000
#define MIN_DELAY_LIMIT 300

#define DEFAULT_DELAY 3000  // 2msec, TODO load this from config for each product

namespace HFSAT {

struct DeltaInfo {
  double delta_price_;
  HFSAT::TradeType_t buysell_;
  int delta_qty_;
  int delta_order_count_;  // not used for LIFFE
};

class ORSConfMarketDelayComputer {
 private:
  std::vector<unsigned long> mds_delta_events_time_;
  std::vector<struct DeltaInfo> mds_delta_packet_;
  int last_vector_index_for_mds_event_;
  int last_vector_index_for_cxl_mds_event_;
  std::vector<unsigned long> delta_mktupdate_delay_vec_;
  std::map<HFSAT::ttime_t, int> trade_time_to_int_price_map_;

 public:
  ORSConfMarketDelayComputer(const std::string& _shortcode_, const int& _yyyymmdd_)
      : mds_delta_events_time_(),
        mds_delta_packet_(),
        last_vector_index_for_mds_event_(0),
        last_vector_index_for_cxl_mds_event_(0),
        delta_mktupdate_delay_vec_() {
    HFSAT::ExchangeSymbolManager::SetUniqueInstance(_yyyymmdd_);
    const char* t_exchange_symbol_ = HFSAT::ExchangeSymbolManager::GetExchSymbol(_shortcode_);

    HFSAT::ExchSource_t exch_src_ = HFSAT::SecurityDefinitions::GetContractExchSource(_shortcode_, _yyyymmdd_);
    HFSAT::TradingLocation_t trading_location_file_read_ =
        HFSAT::TradingLocationUtils::GetTradingLocationExch(exch_src_);  // only primary location makes sense

    HFSAT::BulkFileReader bulk_file_reader_;
    SecurityMarketView& this_smv_ = *ShortcodeSecurityMarketViewMap::StaticGetSecurityMarketView(_shortcode_);
    // TODO currently only looking to implement for liffe
    switch (exch_src_) {
      case HFSAT::kExchSourceLIFFE: {
        std::string t_liffe_filename_ =
            HFSAT::LIFFELoggedMessageFileNamer::GetName(t_exchange_symbol_, _yyyymmdd_, trading_location_file_read_);

        LIFFE_MDS::LIFFECommonStruct next_event_;

        bulk_file_reader_.open(t_liffe_filename_);

        if (!bulk_file_reader_.is_open()) {
          std::cerr << " Could not open : " << t_liffe_filename_ << " to compute Accurate Conf to ORS Data \n";
          return;
        }

        while (true) {
          size_t available_len_ = bulk_file_reader_.read(&next_event_, sizeof(LIFFE_MDS::LIFFECommonStruct));

          if (available_len_ >= sizeof(LIFFE_MDS::LIFFECommonStruct)) {  // data not found in file

            if (next_event_.msg_ == LIFFE_MDS::LIFFE_DELTA) {
              unsigned long this_time_ = next_event_.time_.tv_sec * 1000000 + next_event_.time_.tv_usec;
              mds_delta_events_time_.push_back(this_time_);

              struct DeltaInfo this_delta_info_;
              this_delta_info_.delta_price_ = next_event_.data_.liffe_dels_.price_;
              this_delta_info_.delta_qty_ = next_event_.data_.liffe_dels_.size_;
              this_delta_info_.buysell_ =
                  ('2' == next_event_.data_.liffe_dels_.type_) ? HFSAT::kTradeTypeBuy : HFSAT::kTradeTypeSell;

              mds_delta_packet_.push_back(this_delta_info_);

            } else if (next_event_.msg_ == LIFFE_MDS::LIFFE_TRADE) {
              trade_time_to_int_price_map_[next_event_.time_] =
                  this_smv_.GetIntPx(next_event_.data_.liffe_trds_.trd_px_);
            }
          }

          else {
            break;
          }
        }

      } break;
      case HFSAT::kExchSourceHONGKONG: {
        std::string t_hkex_file_name_ =
            HKEXLoggedMessageFileNamer::GetName(t_exchange_symbol_, _yyyymmdd_, trading_location_file_read_);
        HKEX_MDS::HKEXCommonStruct next_event_;
        bulk_file_reader_.open(t_hkex_file_name_);
        if (!bulk_file_reader_.is_open()) {
          std::cerr << " Could not open : " << t_hkex_file_name_ << " to Compute Accurate Conf to ORS data \n";
          return;
        }
        while (true) {
          size_t available_len_ = bulk_file_reader_.read(&next_event_, sizeof(HKEX_MDS::HKEXCommonStruct));
          if (available_len_ >= sizeof(HKEX_MDS::HKEXCommonStruct)) {
            if (next_event_.msg_ == HKEX_MDS::HKEX_BOOK) {
              unsigned long this_time_ = next_event_.time_.tv_sec * 1000000 + next_event_.time_.tv_usec;
              mds_delta_events_time_.push_back(this_time_);
              struct DeltaInfo this_delta_info_;
              this_delta_info_.delta_price_ = next_event_.data_.hkex_books_.pxs_[0];
              this_delta_info_.delta_qty_ = next_event_.data_.hkex_books_.demand_[0];
              this_delta_info_.buysell_ =
                  (1 == next_event_.data_.hkex_books_.side_) ? HFSAT::kTradeTypeBuy : HFSAT::kTradeTypeSell;
              mds_delta_packet_.push_back(this_delta_info_);
            }
            if (next_event_.msg_ == HKEX_MDS::HKEX_TRADE) {
              trade_time_to_int_price_map_[next_event_.time_] =
                  this_smv_.GetIntPx(next_event_.data_.hkex_trds_.trd_px_);
            }
          } else {
            break;
          }
        }

      } break;
        {
          std::string t_jpy_file_name_ =
              OSEPriceFeedLoggedMessageFileNamer::GetName(t_exchange_symbol_, _yyyymmdd_, trading_location_file_read_);
          // at primary locastion i.e, tok we have pricefeed data, check if we ever need L! data int this case
          OSE_MDS::OSEPriceFeedCommonStruct next_event_;
          bulk_file_reader_.open(t_jpy_file_name_);
          if (!bulk_file_reader_.is_open()) {
            std::cerr << " Could not open: " << t_jpy_file_name_ << " to compute mkt_delays \n";
            return;
          }
          while (true) {
            size_t available_len_ = bulk_file_reader_.read(&next_event_, sizeof(OSE_MDS::OSEPriceFeedCommonStruct));
            if (available_len_ >= sizeof(OSE_MDS::OSEPriceFeedCommonStruct)) {
              if (next_event_.get_buy_sell_trade() == 0 || next_event_.get_buy_sell_trade() == 1) {
                // this is book message
                unsigned long this_time_ = next_event_.time_.tv_sec * 1000000 + next_event_.time_.tv_usec;
                mds_delta_events_time_.push_back(this_time_);
                struct DeltaInfo this_delta_info_;
                this_delta_info_.delta_price_ = next_event_.price;
                this_delta_info_.delta_qty_ = next_event_.size;
                this_delta_info_.buysell_ =
                    (next_event_.get_buy_sell_trade() == 0) ? HFSAT::kTradeTypeBuy : HFSAT::kTradeTypeSell;
                mds_delta_packet_.push_back(this_delta_info_);
              }
              if (next_event_.get_buy_sell_trade() == 2) {
                trade_time_to_int_price_map_[next_event_.time_] = this_smv_.GetIntPx(next_event_.price);
              }
            } else {
              break;
            }
          }
        }
        break;

      default: { } break; }
  }

  std::map<HFSAT::ttime_t, int> GetTradeToPriceMap() { return trade_time_to_int_price_map_; }

  HFSAT::ttime_t getAccurateConfToMarketUpdateDelay(const HFSAT::ttime_t& conf_time_, const double& delta_price_,
                                                    const HFSAT::TradeType_t& buysell_, const int& qty_) {
    int this_unmatched_index_ = 0;
    bool unmatched_ = false;
    int this_qty_diff_ = 0;
    double last_delta_price_ = 0.0;
    int last_delta_size_ = 0;

    for (; last_vector_index_for_mds_event_ >= 0 && last_vector_index_for_mds_event_ < (int)mds_delta_packet_.size() &&
           last_vector_index_for_mds_event_ < (int)mds_delta_events_time_.size();
         last_vector_index_for_mds_event_++) {
      int this_tsec_ = conf_time_.tv_sec;
      int this_usec_ = conf_time_.tv_usec;

      if (buysell_ != mds_delta_packet_[last_vector_index_for_mds_event_].buysell_) continue;

      if ((last_delta_price_ == 0) ||
          (last_delta_price_ != mds_delta_packet_[last_vector_index_for_mds_event_].delta_price_)) {
        // earlier level completely deleted or first time
        last_delta_price_ = mds_delta_packet_[last_vector_index_for_mds_event_].delta_price_;
        last_delta_size_ = 0;  // earlier level completely deleted
      }

      this_qty_diff_ = mds_delta_packet_[last_vector_index_for_mds_event_].delta_qty_ - last_delta_size_;

      if (this_qty_diff_ < 0) {
        last_delta_price_ = mds_delta_packet_[last_vector_index_for_mds_event_].delta_price_;
        last_delta_size_ = mds_delta_packet_[last_vector_index_for_mds_event_].delta_qty_;
        continue;
      }

      unsigned long this_delta_time_ = ((unsigned long)this_tsec_ * 1000000) + this_usec_;

      if (this_delta_time_ > mds_delta_events_time_[last_vector_index_for_mds_event_]) {
        last_delta_price_ = mds_delta_packet_[last_vector_index_for_mds_event_].delta_price_;
        last_delta_size_ = mds_delta_packet_[last_vector_index_for_mds_event_].delta_qty_;
        continue;
      }

      if (unmatched_) {
        if (mds_delta_events_time_[last_vector_index_for_mds_event_] - this_delta_time_ > MAX_DELAY_LIMIT) {
          last_vector_index_for_mds_event_ = this_unmatched_index_;
          HFSAT::ttime_t this_delay_(0, DEFAULT_DELAY);
          return this_delay_;
        }
      }

      if (((int)(mds_delta_packet_[last_vector_index_for_mds_event_].delta_price_ * 1000) ==
           (int)(delta_price_ * 1000)) &&
          (mds_delta_packet_[last_vector_index_for_mds_event_].delta_qty_ >= qty_) && (this_qty_diff_ >= qty_) &&
          (buysell_ == mds_delta_packet_[last_vector_index_for_mds_event_].buysell_)) {
        unsigned long delta_market_update_time_ =
            mds_delta_events_time_[last_vector_index_for_mds_event_] - this_delta_time_;
        unsigned int delay_usec_ = delta_market_update_time_;

        if (delay_usec_ < MIN_DELAY_LIMIT) continue;

        mds_delta_packet_[last_vector_index_for_mds_event_].delta_qty_ -= qty_;

        int sec_delay_ = delay_usec_ / 1000000;
        int usec_delay_ = delay_usec_ % 1000000;

        HFSAT::ttime_t this_delay_(sec_delay_, usec_delay_);
        delta_mktupdate_delay_vec_.push_back(delta_market_update_time_);
        return this_delay_;
      } else {
        if (!unmatched_) {
          unmatched_ = true;
          this_unmatched_index_ = last_vector_index_for_mds_event_;
        }
      }
      last_delta_price_ = mds_delta_packet_[last_vector_index_for_mds_event_].delta_price_;
      last_delta_size_ = mds_delta_packet_[last_vector_index_for_mds_event_].delta_qty_;
    }

    HFSAT::ttime_t this_delay_(0, DEFAULT_DELAY);

    return this_delay_;
  }

  HFSAT::ttime_t getAccurateCxlToMarketUpdateDelay(const HFSAT::ttime_t& cxl_time_, const double& delta_price_,
                                                   const HFSAT::TradeType_t& buysell_, const int& qty_) {
    int this_unmatched_index_ = 0;
    bool unmatched_ = false;
    int this_qty_diff_ = 0;
    double last_delta_price_ = 0.0;
    int last_delta_size_ = 0;

    for (; last_vector_index_for_cxl_mds_event_ >= 0 &&
           last_vector_index_for_cxl_mds_event_ < (int)mds_delta_packet_.size() &&
           last_vector_index_for_cxl_mds_event_ < (int)mds_delta_events_time_.size();
         last_vector_index_for_cxl_mds_event_++) {
      int this_tsec_ = cxl_time_.tv_sec;
      int this_usec_ = cxl_time_.tv_usec;

      if (buysell_ != mds_delta_packet_[last_vector_index_for_cxl_mds_event_].buysell_) continue;

      if (last_delta_price_ == 0 ||
          (last_delta_price_ != mds_delta_packet_[last_vector_index_for_cxl_mds_event_].delta_price_)) {
        last_delta_price_ = mds_delta_packet_[last_vector_index_for_cxl_mds_event_].delta_price_;
        last_delta_size_ = 0;
      }

      this_qty_diff_ = mds_delta_packet_[last_vector_index_for_cxl_mds_event_].delta_qty_ - last_delta_size_;

      if (this_qty_diff_ < 0) {
        last_delta_price_ = mds_delta_packet_[last_vector_index_for_cxl_mds_event_].delta_price_;
        last_delta_size_ = mds_delta_packet_[last_vector_index_for_cxl_mds_event_].delta_qty_;
        continue;
      }

      unsigned long this_delta_time_ = ((unsigned long)this_tsec_ * 1000000) + this_usec_;

      if (this_delta_time_ > mds_delta_events_time_[last_vector_index_for_cxl_mds_event_]) continue;

      if (unmatched_) {
        if (mds_delta_events_time_[last_vector_index_for_cxl_mds_event_] - this_delta_time_ > MAX_CXL_DELAY_LIMIT) {
          last_vector_index_for_cxl_mds_event_ = this_unmatched_index_;
          HFSAT::ttime_t this_delay_(0, DEFAULT_DELAY);
          return this_delay_;
        }
      }

      if (((int)(mds_delta_packet_[last_vector_index_for_cxl_mds_event_].delta_price_ * 1000) ==
           (int)(delta_price_ * 1000)) &&
          (this_qty_diff_ >= qty_) && (buysell_ == mds_delta_packet_[last_vector_index_for_cxl_mds_event_].buysell_)) {
        unsigned long delta_market_update_time_ =
            mds_delta_events_time_[last_vector_index_for_cxl_mds_event_] - this_delta_time_;
        unsigned int delay_usec_ = delta_market_update_time_;
        mds_delta_packet_[last_vector_index_for_cxl_mds_event_].delta_qty_ -= qty_;
        int sec_delay_ = delay_usec_ / 1000000;
        int usec_delay_ = delay_usec_ % 1000000;
        HFSAT::ttime_t this_delay_(sec_delay_, usec_delay_);

        delta_mktupdate_delay_vec_.push_back(delta_market_update_time_);
        return this_delay_;
      } else {
        if (!unmatched_) {
          unmatched_ = true;
          this_unmatched_index_ = last_vector_index_for_cxl_mds_event_;
        }
      }

      last_delta_price_ = mds_delta_packet_[last_vector_index_for_cxl_mds_event_].delta_price_;
      last_delta_size_ = mds_delta_packet_[last_vector_index_for_cxl_mds_event_].delta_qty_;
    }

    HFSAT::ttime_t this_delay_(0, DEFAULT_DELAY);

    return this_delay_;
  }
};
}

#endif
