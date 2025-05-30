// =====================================================================================
//
//       Filename:  trade_ratio_calculator.cpp
//
//    Description:  Used to calculate trade ratio for a product on given day
//                  Which is further used in calculation of qA and qB
//
//        Version:  1.0
//        Created:  Tuesday 10 September 2013 06:56:36  GMT
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
#include "dvccode/CDef/stored_market_data_common_message_defines.hpp"
#include "dvccode/Utils/bulk_file_reader.hpp"
#include "dvccode/CDef/trading_location_manager.hpp"

#include "baseinfra/LoggedSources/eurex_logged_message_filenamer.hpp"
#include "baseinfra/LoggedSources/hkex_logged_message_filenamer.hpp"
#include "baseinfra/LoggedSources/liffe_logged_message_filenamer.hpp"
#include "baseinfra/LoggedSources/micex_logged_message_filenamer.hpp"
#include "baseinfra/LoggedSources/ntp_logged_message_filenamer.hpp"
#include "baseinfra/LoggedSources/ose_logged_message_filenamer.hpp"
#include "baseinfra/LoggedSources/tmx_logged_message_filenamer.hpp"
#include "baseinfra/LoggedSources/ose_l1_logged_message_filenamer.hpp"
#include "baseinfra/LoggedSources/common_logged_message_filenamer.hpp"
#include "baseinfra/LoggedSources/hkomd_logged_message_filenamer.hpp"

#include "baseinfra/SimMarketMaker/trade_ratio_calculator.hpp"

#define LEVELS_TO_CHECK 1
namespace HFSAT {
void TradeRatioCalculator::BidAskTradeRatio(const SecurityMarketView& dep_market_view_, const Watch& watch_,
                                            const double& percentile_, double& rw_bid_tr_ratio_,
                                            double& rw_ask_tr_ratio_) {
  HFSAT::BulkFileReader reader_;
  std::vector<double> bid_sort_vec_;
  std::vector<double> ask_sort_vec_;
  std::string file_name_ = "";
  uint32_t quantity = 0;
  uint32_t total_size_ = 0;
  double price_ = 0.0;
  HFSAT::TradingLocation_t trading_loc_ = TradingLocationUtils::GetTradingLocationExch(
      HFSAT::SecurityDefinitions::GetContractExchSource(dep_market_view_.shortcode(), watch_.YYYYMMDD()));

  switch (HFSAT::SecurityDefinitions::GetContractExchSource(dep_market_view_.shortcode(), watch_.YYYYMMDD())) {
    case HFSAT::kExchSourceCME: {
      file_name_ = CommonLoggedMessageFileNamer::GetName(kExchSourceCME, dep_market_view_.secname(), watch_.YYYYMMDD(),
                                                         trading_loc_);
      reader_.open(file_name_);
      if (reader_.is_open()) {
        bool trade_recv = false;
        CME_MDS::CMECommonStruct next_event_;
        bool quote_recv_ = false;
        // trade before quote
        while (true) {
          size_t available_len_ = reader_.read(&next_event_, sizeof(CME_MDS::CMECommonStruct));
          if (available_len_ < sizeof(next_event_)) break;
          if (next_event_.msg_ == CME_MDS::CME_TRADE) {
            trade_recv = true;
            // agg = next_event_.data_.cme_trds_.agg_side_;
            price_ = next_event_.data_.cme_trds_.trd_px_;
            if (!quote_recv_)
              quantity += next_event_.data_.cme_trds_.trd_qty_;  // consecutive trades
            else
              quantity = next_event_.data_.cme_trds_.trd_qty_;
            quote_recv_ = false;
          }

          if (next_event_.msg_ == CME_MDS::CME_DELTA) quote_recv_ = true;

          if (trade_recv && (next_event_.msg_ == CME_MDS::CME_DELTA)) {
            if ((next_event_.data_.cme_dels_.type_ == '1')) {
              if (dep_market_view_.DblPxCompare(price_, next_event_.data_.cme_dels_.price_)) {
                total_size_ = next_event_.data_.cme_dels_.size_;
                trade_recv = false;
                if (quantity == 0) {
                  ask_sort_vec_.push_back(0);  // sanity check
                } else {
                  double ratio_ = double(quantity) / double(total_size_ + quantity);
                  ask_sort_vec_.push_back(ratio_);
                }
              } else if (price_ < next_event_.data_.cme_dels_.price_) {
                if (next_event_.data_.cme_dels_.level_ == 1)  // the last level reduced to zero
                {
                  ask_sort_vec_.push_back(1.0);
                  trade_recv = false;
                }
              } else {
                // trade at non-best level, assuming that market update right after trade is only related to trade,
                // this would happen only if that level has ended
                // Not considering trades at non best levels for now
                // ask_sort_vec_.push_back(1.0);
                trade_recv = false;
              }
            }
            if ((next_event_.data_.cme_dels_.type_ == '0')) {
              if (dep_market_view_.DblPxCompare(price_, next_event_.data_.cme_dels_.price_)) {
                total_size_ = next_event_.data_.cme_dels_.size_;
                trade_recv = false;
                if (quantity == 0)
                  bid_sort_vec_.push_back(0);
                else {
                  double ratio_ = double(quantity) / double(total_size_ + quantity);
                  bid_sort_vec_.push_back(ratio_);
                }
              }
              if (price_ > next_event_.data_.cme_dels_.price_) {
                if (next_event_.data_.cme_dels_.level_ == 1) {
                  // earlier best level was completely erased
                  bid_sort_vec_.push_back(1.0);
                  trade_recv = false;
                }
              } else {
                // trade at non best level, this will happen only if that level was completely erased
                // Not Considering trades at non best levels
                // bid_sort_vec_.push_back(1.0);
                trade_recv = false;
              }
            }
          }
        }
      }
      break;
    }
    case HFSAT::kExchSourceEUREX: {
      file_name_ = CommonLoggedMessageFileNamer::GetName(kExchSourceEUREX, dep_market_view_.secname(),
                                                         watch_.YYYYMMDD(), trading_loc_);
      reader_.open(file_name_);
      if (reader_.is_open()) {
        EUREX_MDS::EUREXCommonStruct next_event_;
        char agg = ' ';
        bool trade_recv = false;
        bool quote_recv_ = false;
        while (true) {
          size_t available_len_ = reader_.read(&next_event_, sizeof(EUREX_MDS::EUREXCommonStruct));
          if (available_len_ < sizeof(next_event_)) break;
          if (next_event_.msg_ == EUREX_MDS::EUREX_TRADE) {
            trade_recv = true;

            agg = next_event_.data_.eurex_trds_.agg_side_;
            price_ = next_event_.data_.eurex_trds_.trd_px_;
            if (!quote_recv_)
              quantity += next_event_.data_.eurex_trds_.trd_qty_;
            else
              quantity = next_event_.data_.eurex_trds_.trd_qty_;
            quote_recv_ = false;
          }
          if ((next_event_.msg_ == EUREX_MDS::EUREX_DELTA)) quote_recv_ = true;

          if (trade_recv && (next_event_.msg_ == EUREX_MDS::EUREX_DELTA)) {
            if ((agg == 'B') && (next_event_.data_.eurex_dels_.type_ == '1')) {
              if (dep_market_view_.DblPxCompare(price_, next_event_.data_.eurex_dels_.price_)) {
                total_size_ = next_event_.data_.eurex_dels_.size_;
                trade_recv = false;
                if (quantity == 0) {
                  ask_sort_vec_.push_back(0);
                } else {
                  double ratio_ = double(quantity) / double(total_size_ + quantity);
                  ask_sort_vec_.push_back(ratio_);
                }
              } else if (price_ < next_event_.data_.eurex_dels_.price_) {
                if (next_event_.data_.eurex_dels_.level_ == 1) {
                  // last best level was completely erased
                  ask_sort_vec_.push_back(1.0);
                  trade_recv = false;
                }
              } else {
                // trade at non _best _level
                // ignoring trades there trades for now
                // ask_sort_vec_.push_back(1.0);
                trade_recv = false;
              }
            }
            if ((agg == 'S') && (next_event_.data_.eurex_dels_.type_ == '2'))  // bid
            {
              if (dep_market_view_.DblPxCompare(price_, next_event_.data_.eurex_dels_.price_)) {
                total_size_ = next_event_.data_.eurex_dels_.size_;
                trade_recv = false;
                if (quantity == 0) {
                  bid_sort_vec_.push_back(0);
                } else {
                  double ratio_ = double(quantity) / double(total_size_ + quantity);
                  bid_sort_vec_.push_back(ratio_);
                }
              } else if (price_ > next_event_.data_.eurex_dels_.price_) {
                if (next_event_.data_.eurex_dels_.level_ == 1) {
                  // last best level completely removed
                  bid_sort_vec_.push_back(1.0);
                  trade_recv = false;
                }
              } else {
                // trade at non best level
                // igonoring these trades for now
                // bid_sort_vec_.push_back(1.0);
                trade_recv = false;
              }
            }
          }
        }
      }
      break;
    }
    case HFSAT::kExchSourceLIFFE: {
      file_name_ = LIFFELoggedMessageFileNamer::GetName(dep_market_view_.secname(), watch_.YYYYMMDD(), trading_loc_);
      reader_.open(file_name_);
      if (reader_.is_open()) {
        LIFFE_MDS::LIFFECommonStruct next_event_;
        double ask_price_ = 0.0, bid_price_ = 0.0;
        int ask_size_ = 0, bid_size_ = 0;
        // quote before trade system
        // also we get seperate information about best level
        // only best level trades considered
        while (true) {
          if (reader_.is_open()) {
            size_t available_len_ = reader_.read(&next_event_, sizeof(LIFFE_MDS::LIFFECommonStruct));
            if (available_len_ < sizeof(next_event_)) break;
          }
          if ((next_event_.msg_ == LIFFE_MDS::LIFFE_TRADE)) {
            quantity = next_event_.data_.liffe_trds_.trd_qty_;
            if (dep_market_view_.DblPxCompare(next_event_.data_.liffe_trds_.trd_px_, ask_price_)) {
              if (quantity == 0) {
                ask_sort_vec_.push_back(0.0);
              } else {
                double ratio_ = double(quantity) / double(quantity + ask_size_);
                ask_sort_vec_.push_back(ratio_);
              }
            } else if (dep_market_view_.DblPxCompare(next_event_.data_.liffe_trds_.trd_px_, bid_price_)) {
              if (quantity == 0) {
                bid_sort_vec_.push_back(0.0);
              } else {
                double ratio_ = double(quantity) / double(quantity + bid_size_);
                bid_sort_vec_.push_back(ratio_);
              }
            }
          }
          if ((next_event_.msg_ == LIFFE_MDS::LIFFE_DELTA)) {
            if ((next_event_.data_.liffe_dels_.type_ == '2') && (next_event_.data_.liffe_dels_.level_ == 1)) {
              bid_price_ = next_event_.data_.liffe_dels_.price_;
              bid_size_ = next_event_.data_.liffe_dels_.size_;
            }
            if ((next_event_.data_.liffe_dels_.type_ == '1') && (next_event_.data_.liffe_dels_.level_ == 1)) {
              ask_price_ = next_event_.data_.liffe_dels_.price_;
              ask_size_ = next_event_.data_.liffe_dels_.size_;
            }
          }
        }
      }
      break;
    }
    case HFSAT::kExchSourceBMFEQ:
    case HFSAT::kExchSourceBMF:
    case HFSAT::kExchSourceNTP: {
      bool is_bmf_equity = dep_market_view_.exch_source() == kExchSourceBMFEQ;
      file_name_ = NTPLoggedMessageFileNamer::GetName(dep_market_view_.secname(), watch_.YYYYMMDD(), trading_loc_,
                                                      false, is_bmf_equity);
      reader_.open(file_name_);
      NTP_MDS::NTPCommonStruct next_event_;
      if (reader_.is_open()) {
        bool trade_recv = false;
        double trade_price_ = 0.0;
        bool quote_recv_ = false;
        while (true) {
          size_t available_len_ = reader_.read(&next_event_, sizeof(NTP_MDS::NTPCommonStruct));
          if (available_len_ < sizeof(next_event_)) break;
          if (next_event_.msg_ == NTP_MDS::NTP_TRADE) {
            trade_recv = true;
            trade_price_ = next_event_.data_.ntp_trds_.trd_px_;
            if (!quote_recv_)
              quantity += next_event_.data_.ntp_trds_.trd_qty_;
            else
              quantity = next_event_.data_.ntp_trds_.trd_qty_;
            quote_recv_ = false;
          }

          if ((next_event_.msg_ == NTP_MDS::NTP_DELTA) && trade_recv) {
            if (dep_market_view_.DblPxCompare(next_event_.data_.ntp_dels_.price_, trade_price_)) {
              trade_recv = false;
              if ((next_event_.data_.ntp_dels_.type_ == '0') && (next_event_.data_.ntp_dels_.level_ == 1)) {
                if (quantity == 0) {
                  bid_sort_vec_.push_back(0);
                } else {
                  double ratio_ = double(quantity) / double(quantity + next_event_.data_.ntp_dels_.size_);
                  bid_sort_vec_.push_back(ratio_);
                }
                trade_recv = false;
              } else if ((next_event_.data_.ntp_dels_.type_ == '1') && (next_event_.data_.ntp_dels_.level_ == 1)) {
                if (quantity == 0) {
                  ask_sort_vec_.push_back(0);
                } else {
                  double ratio_ = double(quantity) / double(quantity + next_event_.data_.ntp_dels_.size_);
                  ask_sort_vec_.push_back(ratio_);
                }
                trade_recv = false;
              }
            }
          }
          if (next_event_.msg_ == NTP_MDS::NTP_DELTA) quote_recv_ = true;
        }
      }
      break;
    }
    case HFSAT::kExchSourceTMX: {
      if (watch_.YYYYMMDD() >= USING_TMX_OBF_FROM) {
        file_name_ = CommonLoggedMessageFileNamer::GetName(kExchSourceTMX, dep_market_view_.secname(),
                                                           watch_.YYYYMMDD(), trading_loc_);
        reader_.open(file_name_);
        if (reader_.is_open()) {
          TMX_OBF_MDS::TMXPFCommonStruct next_event_;
          bool trade_recv = false;
          double trade_price_ = 0.0;
          bool quote_recv_ = false;
          quantity = 0;
          while (true) {
            size_t available_len_ = reader_.read(&next_event_, sizeof(TMX_OBF_MDS::TMXPFCommonStruct));
            if (available_len_ < sizeof(next_event_)) break;
            if (next_event_.msg_ == TMX_OBF_MDS::TMXPriceFeedmsgType::TMX_PF_TRADE) {
              trade_recv = true;
              trade_price_ = next_event_.data_.trade_.price_;
              if (!quote_recv_)
                quantity += next_event_.data_.trade_.quantity_;
              else
                quantity = next_event_.data_.trade_.quantity_;

              quote_recv_ = false;
            }
            if (next_event_.msg_ == TMX_OBF_MDS::TMXPriceFeedmsgType::TMX_PF_DELTA) {
              quote_recv_ = true;
              if (trade_recv) {
                if (next_event_.data_.delta_.side_ == 'B' && next_event_.data_.delta_.level_ == 1) {
                  if (dep_market_view_.DblPxCompare(next_event_.data_.delta_.price_, trade_price_)) {
                    if (quantity == 0)
                      bid_sort_vec_.push_back(0);
                    else {
                      double ratio_ = double(quantity) / double(quantity + next_event_.data_.delta_.quantity_);
                      bid_sort_vec_.push_back(ratio_);
                    }
                    trade_recv = false;
                  }
                } else if (next_event_.data_.delta_.side_ == 'S' && next_event_.data_.delta_.level_ == 1) {
                  if (dep_market_view_.DblPxCompare(next_event_.data_.delta_.price_, trade_price_)) {
                    if (quantity == 0) {
                      ask_sort_vec_.push_back(0);
                    } else {
                      double ratio_ = double(quantity) / double(quantity + next_event_.data_.delta_.quantity_);
                      ask_sort_vec_.push_back(ratio_);
                    }
                    trade_recv = false;
                  }
                }
              }
            }
          }
        }
      } else {
        file_name_ = TMXLoggedMessageFileNamer::GetName(dep_market_view_.secname(), watch_.YYYYMMDD(), trading_loc_);
        reader_.open(file_name_);
        TMX_MDS::TMXCommonStruct next_event_;
        if (reader_.is_open()) {
          bool trade_recv = false;
          double trade_price_ = 0.0;
          bool quote_recv_ = false;
          std::vector<double> last_bid_price_vec_;  // to keep track of erased bid level
          std::vector<double> last_ask_price_vec_;  // to keep track of erased ask level
          bool ratio_calculated_ = false;
          for (int i = 0; i < LEVELS_TO_CHECK; i++) {
            last_bid_price_vec_.push_back(0.0);
            last_ask_price_vec_.push_back(0.0);
          }

          while (true) {
            size_t available_len_ = reader_.read(&next_event_, sizeof(TMX_MDS::TMXCommonStruct));
            if (available_len_ < sizeof(next_event_)) break;
            if (next_event_.msg_ == TMX_MDS::TMX_TRADE) {
              trade_recv = true;
              trade_price_ = next_event_.data_.tmx_trds_.trd_px_;
              if (!quote_recv_)
                quantity += next_event_.data_.tmx_trds_.trd_qty_;
              else
                quantity = next_event_.data_.tmx_trds_.trd_qty_;
              quote_recv_ = false;
            }

            if (trade_recv && next_event_.msg_ == TMX_MDS::TMX_BOOK) {
              for (int i = 0; i < LEVELS_TO_CHECK; i++) {
                if (dep_market_view_.DblPxCompare(trade_price_, next_event_.data_.tmx_books_.bid_pxs_[i])) {
                  trade_recv = false;
                  if (quantity == 0)
                    bid_sort_vec_.push_back(0);
                  else {
                    double ratio_ = double(quantity) / double(quantity + next_event_.data_.tmx_books_.bid_szs_[i]);
                    bid_sort_vec_.push_back(ratio_);
                  }
                  ratio_calculated_ = true;
                  break;
                }
              }
              for (int i = 0; i < LEVELS_TO_CHECK; i++) {
                if (dep_market_view_.DblPxCompare(trade_price_, next_event_.data_.tmx_books_.ask_pxs_[i])) {
                  trade_recv = false;
                  if (quantity == 0)
                    ask_sort_vec_.push_back(0);
                  else {
                    double ratio_ = double(quantity) / double(quantity + next_event_.data_.tmx_books_.bid_szs_[i]);
                    ask_sort_vec_.push_back(ratio_);
                  }
                  ratio_calculated_ = true;
                  break;
                }
              }
              if (!ratio_calculated_) {
                // either the level was completely erased or trade at non best level
                // trades at non best levels are ignored
                for (int i = 0; i < LEVELS_TO_CHECK; i++) {
                  if (dep_market_view_.DblPxCompare(trade_price_, last_bid_price_vec_[i])) {
                    bid_sort_vec_.push_back(1.0);
                    trade_recv = false;
                  }
                }
                for (int i = 0; i < LEVELS_TO_CHECK; i++) {
                  if (dep_market_view_.DblPxCompare(trade_price_, last_ask_price_vec_[i])) {
                    ask_sort_vec_.push_back(1.0);
                    trade_recv = false;
                  }
                }
              }
            }
            if (next_event_.msg_ == TMX_MDS::TMX_BOOK) {
              quote_recv_ = true;
              for (int i = 0; i < LEVELS_TO_CHECK; i++) {
                last_bid_price_vec_[i] = (next_event_.data_.tmx_books_.bid_pxs_[i]);
                last_ask_price_vec_[i] = (next_event_.data_.tmx_books_.ask_pxs_[i]);
              }
            }
          }
        }
      }
      break;
    }
    case HFSAT::kExchSourceHONGKONG:
    case HFSAT::kExchSourceHKOMDCPF: {
      file_name_ = HKOMDCPFLoggedMessagefileNamer::GetName(dep_market_view_.secname(), watch_.YYYYMMDD(), trading_loc_);
      reader_.open(file_name_);
      if (reader_.is_open()) {
        HKOMD_MDS::HKOMDPFCommonStruct next_event_;
        bool trade_recv = false;
        double trade_price_ = 0.0;
        bool quote_recv_ = false;
        quantity = 0;
        while (true) {
          size_t available_len_ = reader_.read(&next_event_, sizeof(HKOMD_MDS::HKOMDPFCommonStruct));
          if (available_len_ < sizeof(next_event_)) break;
          if (next_event_.msg_ == HKOMD_MDS::HKOMD_PF_TRADE) {
            trade_recv = true;
            trade_price_ = next_event_.data_.trade_.price_;
            if (!quote_recv_)
              quantity += next_event_.data_.trade_.quantity_;
            else
              quantity = next_event_.data_.trade_.quantity_;

            quote_recv_ = false;
          }
          if (next_event_.msg_ == HKOMD_MDS::HKOMD_PF_DELTA) {
            quote_recv_ = true;
            if (trade_recv) {
              if (next_event_.data_.delta_.side_ == 0 && next_event_.data_.delta_.level_ == 1) {
                if (dep_market_view_.DblPxCompare(next_event_.data_.delta_.price_, trade_price_)) {
                  if (quantity == 0)
                    bid_sort_vec_.push_back(0);
                  else {
                    double ratio_ = double(quantity) / double(quantity + next_event_.data_.delta_.quantity_);
                    bid_sort_vec_.push_back(ratio_);
                  }
                  trade_recv = false;
                }
              } else if (next_event_.data_.delta_.side_ == 1 && next_event_.data_.delta_.level_ == 1) {
                if (dep_market_view_.DblPxCompare(next_event_.data_.delta_.price_, trade_price_)) {
                  if (quantity == 0) {
                    ask_sort_vec_.push_back(0);
                  } else {
                    double ratio_ = double(quantity) / double(quantity + next_event_.data_.delta_.quantity_);
                    ask_sort_vec_.push_back(ratio_);
                  }
                  trade_recv = false;
                }
              }
            }
          }
        }
      }
      break;
    }
    case HFSAT::kExchSourceRTS:
    case HFSAT::kExchSourceMICEX:
    case HFSAT::kExchSourceMICEX_EQ:
    case HFSAT::kExchSourceMICEX_CR:
    case HFSAT::kExchSourceNASDAQ:
    case HFSAT::kExchSourceJPY: {
      break;
    }
    case HFSAT::kExchSourceTSE: {
      break;
    }
    case HFSAT::kExchSourceMEFF:
    default: { break; } break;
  }

  sort(bid_sort_vec_.begin(), bid_sort_vec_.end());
  sort(ask_sort_vec_.begin(), ask_sort_vec_.end());
  if (bid_sort_vec_.size() < 1) {
    rw_bid_tr_ratio_ = 1000000000;
  } else {
    rw_bid_tr_ratio_ = (bid_sort_vec_[int(bid_sort_vec_.size() * percentile_)]);
  }
  if (ask_sort_vec_.size() < 1) {
    rw_ask_tr_ratio_ = 1000000000;
  } else {
    rw_ask_tr_ratio_ = (ask_sort_vec_[int(ask_sort_vec_.size() * percentile_)]);
  }
}
}
