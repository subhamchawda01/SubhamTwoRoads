// =====================================================================================
//
//       Filename:  get_pnl_for_shortcode_bytime_stats.cpp
//
//    Description:
//
//        Version:  1.0
//        Created:  11/30/2012 12:42:20 PM
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

#include <stdio.h>
#include <stdlib.h>
#include <fstream>
#include <signal.h>
#include <getopt.h>
#include <map>
#include <vector>
#include <algorithm>

#include "dvccode/CDef/debug_logger.hpp"
#include "dvccode/CDef/file_utils.hpp"  // To create the directory
#include "dvccode/CDef/stored_market_data_common_message_defines.hpp"
#include "dvccode/CDef/trading_location_manager.hpp"
#include "dvccode/CDef/exchange_symbol_manager.hpp"
#include "dvccode/CDef/defines.hpp"
#include "dvccode/CDef/ttime.hpp"
#include "dvccode/CDef/currency_convertor.hpp"
#include "dvccode/CDef/ors_defines.hpp"
#include "dvccode/CDef/security_definitions.hpp"
#include "dvccode/CommonTradeUtils/date_time.hpp"

#include "dvccode/Utils/multicast_receiver_socket.hpp"
#include "dvccode/Utils/bulk_file_reader.hpp"
#include "dvccode/CDef/ors_messages.hpp"

#define LOGGED_DATA_PREFIX "/NAS1/data/ORSData/"
#define TRADES_DATA_PREFIX "/NAS1/logs/ORSTrades/"

bool print_verbose_ = false;

class Commission {
 public:
  static double GetCommissionForShortCode(std::string shortcode_, unsigned int volume_) {
    if (shortcode_ == "FGBM_0")
      return (0.22 * HFSAT::CurrencyConvertor::Convert(HFSAT::kCurrencyEUR, HFSAT::kCurrencyUSD));
    if (shortcode_ == "FESX_0")
      return (0.32 * HFSAT::CurrencyConvertor::Convert(HFSAT::kCurrencyEUR, HFSAT::kCurrencyUSD));
    if (shortcode_ == "FBTP_0")
      return (0.22 * HFSAT::CurrencyConvertor::Convert(HFSAT::kCurrencyEUR, HFSAT::kCurrencyUSD));
    if (shortcode_ == "FBTS_0")
      return (0.22 * HFSAT::CurrencyConvertor::Convert(HFSAT::kCurrencyEUR, HFSAT::kCurrencyUSD));
    if (shortcode_ == "FGBL_0")
      return (0.22 * HFSAT::CurrencyConvertor::Convert(HFSAT::kCurrencyEUR, HFSAT::kCurrencyUSD));
    if (shortcode_ == "FDAX_0")
      return (0.22 * HFSAT::CurrencyConvertor::Convert(HFSAT::kCurrencyEUR, HFSAT::kCurrencyUSD));
    if (shortcode_ == "FOAT_0")
      return (0.22 * HFSAT::CurrencyConvertor::Convert(HFSAT::kCurrencyEUR, HFSAT::kCurrencyUSD));
    if (shortcode_ == "FGBS_0")
      return (0.22 * HFSAT::CurrencyConvertor::Convert(HFSAT::kCurrencyEUR, HFSAT::kCurrencyUSD));
    if (shortcode_ == "FGBX_0")
      return (0.22 * HFSAT::CurrencyConvertor::Convert(HFSAT::kCurrencyEUR, HFSAT::kCurrencyUSD));

    if (shortcode_ == "ZN_0") return (0.13);
    if (shortcode_ == "ZF_0") return (0.13);
    if (shortcode_ == "ZB_0") return (0.13);
    if (shortcode_ == "UB_0") return (0.13);

    if (shortcode_ == "CGB_0")
      return (0.15 * HFSAT::CurrencyConvertor::Convert(HFSAT::kCurrencyCAD, HFSAT::kCurrencyUSD));
    if (shortcode_ == "CGF_0")
      return (0.15 * HFSAT::CurrencyConvertor::Convert(HFSAT::kCurrencyCAD, HFSAT::kCurrencyUSD));
    if (shortcode_ == "CGZ_0")
      return (0.15 * HFSAT::CurrencyConvertor::Convert(HFSAT::kCurrencyCAD, HFSAT::kCurrencyUSD));
    if (shortcode_ == "BAX_0")
      return (0.15 * HFSAT::CurrencyConvertor::Convert(HFSAT::kCurrencyCAD, HFSAT::kCurrencyUSD));
    if (shortcode_ == "BAX_1")
      return (0.15 * HFSAT::CurrencyConvertor::Convert(HFSAT::kCurrencyCAD, HFSAT::kCurrencyUSD));
    if (shortcode_ == "BAX_2")
      return (0.15 * HFSAT::CurrencyConvertor::Convert(HFSAT::kCurrencyCAD, HFSAT::kCurrencyUSD));
    if (shortcode_ == "BAX_3")
      return (0.15 * HFSAT::CurrencyConvertor::Convert(HFSAT::kCurrencyCAD, HFSAT::kCurrencyUSD));
    if (shortcode_ == "BAX_4")
      return (0.15 * HFSAT::CurrencyConvertor::Convert(HFSAT::kCurrencyCAD, HFSAT::kCurrencyUSD));
    if (shortcode_ == "BAX_5")
      return (0.15 * HFSAT::CurrencyConvertor::Convert(HFSAT::kCurrencyCAD, HFSAT::kCurrencyUSD));
    if (shortcode_ == "BAX_6")
      return (0.15 * HFSAT::CurrencyConvertor::Convert(HFSAT::kCurrencyCAD, HFSAT::kCurrencyUSD));
    if (shortcode_ == "SXF_0")
      return (0.15 * HFSAT::CurrencyConvertor::Convert(HFSAT::kCurrencyCAD, HFSAT::kCurrencyUSD));

    if (shortcode_ == "LFR_0")
      return (0.25 * HFSAT::CurrencyConvertor::Convert(HFSAT::kCurrencyGBP, HFSAT::kCurrencyUSD));
    if (shortcode_ == "LFZ_0")
      return (0.31 * HFSAT::CurrencyConvertor::Convert(HFSAT::kCurrencyGBP, HFSAT::kCurrencyUSD));
    if (shortcode_ == "YFEBM_0")
      return (1.03 * HFSAT::CurrencyConvertor::Convert(HFSAT::kCurrencyGBP, HFSAT::kCurrencyUSD));
    if (shortcode_ == "YFEBM_1")
      return (1.30 * HFSAT::CurrencyConvertor::Convert(HFSAT::kCurrencyGBP, HFSAT::kCurrencyUSD));

    if (shortcode_ == "JFFCE_0")
      return (0.30 * HFSAT::CurrencyConvertor::Convert(HFSAT::kCurrencyEUR, HFSAT::kCurrencyUSD));
    if (shortcode_ == "KFFTI_0")
      return (0.48 * HFSAT::CurrencyConvertor::Convert(HFSAT::kCurrencyEUR, HFSAT::kCurrencyUSD));
    if (shortcode_ == "LFI_0")
      return (0.37 * HFSAT::CurrencyConvertor::Convert(HFSAT::kCurrencyEUR, HFSAT::kCurrencyUSD));
    if (shortcode_ == "LFI_1")
      return (0.37 * HFSAT::CurrencyConvertor::Convert(HFSAT::kCurrencyEUR, HFSAT::kCurrencyUSD));
    if (shortcode_ == "LFI_2")
      return (0.37 * HFSAT::CurrencyConvertor::Convert(HFSAT::kCurrencyEUR, HFSAT::kCurrencyUSD));
    if (shortcode_ == "LFI_3")
      return (0.37 * HFSAT::CurrencyConvertor::Convert(HFSAT::kCurrencyEUR, HFSAT::kCurrencyUSD));
    if (shortcode_ == "LFI_4")
      return (0.37 * HFSAT::CurrencyConvertor::Convert(HFSAT::kCurrencyEUR, HFSAT::kCurrencyUSD));
    if (shortcode_ == "LFI_5")
      return (0.37 * HFSAT::CurrencyConvertor::Convert(HFSAT::kCurrencyEUR, HFSAT::kCurrencyUSD));
    if (shortcode_ == "LFI_6")
      return (0.37 * HFSAT::CurrencyConvertor::Convert(HFSAT::kCurrencyEUR, HFSAT::kCurrencyUSD));
    if (shortcode_ == "LFI_7")
      return (0.37 * HFSAT::CurrencyConvertor::Convert(HFSAT::kCurrencyEUR, HFSAT::kCurrencyUSD));
    if (shortcode_ == "LFI_8")
      return (0.37 * HFSAT::CurrencyConvertor::Convert(HFSAT::kCurrencyEUR, HFSAT::kCurrencyUSD));
    if (shortcode_ == "LFI_9")
      return (0.37 * HFSAT::CurrencyConvertor::Convert(HFSAT::kCurrencyEUR, HFSAT::kCurrencyUSD));
    if (shortcode_ == "LFI_10")
      return (0.37 * HFSAT::CurrencyConvertor::Convert(HFSAT::kCurrencyEUR, HFSAT::kCurrencyUSD));
    if (shortcode_ == "LFI_11")
      return (0.37 * HFSAT::CurrencyConvertor::Convert(HFSAT::kCurrencyEUR, HFSAT::kCurrencyUSD));
    if (shortcode_ == "LFL_0")
      return (0.31 * HFSAT::CurrencyConvertor::Convert(HFSAT::kCurrencyGBP, HFSAT::kCurrencyUSD));
    if (shortcode_ == "LFL_1")
      return (0.31 * HFSAT::CurrencyConvertor::Convert(HFSAT::kCurrencyGBP, HFSAT::kCurrencyUSD));
    if (shortcode_ == "LFL_2")
      return (0.31 * HFSAT::CurrencyConvertor::Convert(HFSAT::kCurrencyGBP, HFSAT::kCurrencyUSD));
    if (shortcode_ == "LFL_3")
      return (0.31 * HFSAT::CurrencyConvertor::Convert(HFSAT::kCurrencyGBP, HFSAT::kCurrencyUSD));
    if (shortcode_ == "LFL_4")
      return (0.31 * HFSAT::CurrencyConvertor::Convert(HFSAT::kCurrencyGBP, HFSAT::kCurrencyUSD));
    if (shortcode_ == "LFL_5")
      return (0.31 * HFSAT::CurrencyConvertor::Convert(HFSAT::kCurrencyGBP, HFSAT::kCurrencyUSD));
    if (shortcode_ == "LFL_6")
      return (0.31 * HFSAT::CurrencyConvertor::Convert(HFSAT::kCurrencyGBP, HFSAT::kCurrencyUSD));
    if (shortcode_ == "LFL_7")
      return (0.31 * HFSAT::CurrencyConvertor::Convert(HFSAT::kCurrencyGBP, HFSAT::kCurrencyUSD));
    if (shortcode_ == "LFL_8")
      return (0.31 * HFSAT::CurrencyConvertor::Convert(HFSAT::kCurrencyGBP, HFSAT::kCurrencyUSD));
    if (shortcode_ == "LFL_9")
      return (0.31 * HFSAT::CurrencyConvertor::Convert(HFSAT::kCurrencyGBP, HFSAT::kCurrencyUSD));
    if (shortcode_ == "LFL_10")
      return (0.31 * HFSAT::CurrencyConvertor::Convert(HFSAT::kCurrencyGBP, HFSAT::kCurrencyUSD));
    if (shortcode_ == "LFL_11")
      return (0.31 * HFSAT::CurrencyConvertor::Convert(HFSAT::kCurrencyGBP, HFSAT::kCurrencyUSD));

    if (shortcode_ == "BR_DOL_0")
      return (0.506 * HFSAT::CurrencyConvertor::Convert(HFSAT::kCurrencyBRL, HFSAT::kCurrencyUSD));
    if (shortcode_ == "BR_WIN_0")
      return (0.05 * HFSAT::CurrencyConvertor::Convert(HFSAT::kCurrencyBRL, HFSAT::kCurrencyUSD));
    if (shortcode_ == "BR_IND_0")
      return (0.316 * HFSAT::CurrencyConvertor::Convert(HFSAT::kCurrencyBRL, HFSAT::kCurrencyUSD));
    if (shortcode_ == "DI1F15")
      return (0.346 * HFSAT::CurrencyConvertor::Convert(HFSAT::kCurrencyBRL, HFSAT::kCurrencyUSD));
    if (shortcode_ == "DI1F16")
      return (0.346 * HFSAT::CurrencyConvertor::Convert(HFSAT::kCurrencyBRL, HFSAT::kCurrencyUSD));
    if (shortcode_ == "DI1F17")
      return (0.346 * HFSAT::CurrencyConvertor::Convert(HFSAT::kCurrencyBRL, HFSAT::kCurrencyUSD));
    if (shortcode_ == "DI1F18")
      return (0.346 * HFSAT::CurrencyConvertor::Convert(HFSAT::kCurrencyBRL, HFSAT::kCurrencyUSD));
    if (shortcode_ == "DI1F19")
      return (0.346 * HFSAT::CurrencyConvertor::Convert(HFSAT::kCurrencyBRL, HFSAT::kCurrencyUSD));
    if (shortcode_ == "DI1F20")
      return (0.346 * HFSAT::CurrencyConvertor::Convert(HFSAT::kCurrencyBRL, HFSAT::kCurrencyUSD));
    if (shortcode_ == "DI1F21")
      return (0.346 * HFSAT::CurrencyConvertor::Convert(HFSAT::kCurrencyBRL, HFSAT::kCurrencyUSD));
    if (shortcode_ == "DI1F22")
      return (0.346 * HFSAT::CurrencyConvertor::Convert(HFSAT::kCurrencyBRL, HFSAT::kCurrencyUSD));
    if (shortcode_ == "DI1F23")
      return (0.346 * HFSAT::CurrencyConvertor::Convert(HFSAT::kCurrencyBRL, HFSAT::kCurrencyUSD));
    if (shortcode_ == "DI1N14")
      return (0.346 * HFSAT::CurrencyConvertor::Convert(HFSAT::kCurrencyBRL, HFSAT::kCurrencyUSD));

    if (shortcode_ == "HHI_0")
      return (5.85 * HFSAT::CurrencyConvertor::Convert(HFSAT::kCurrencyHKD, HFSAT::kCurrencyUSD));
    if (shortcode_ == "HSI_0")
      return (12.35 * HFSAT::CurrencyConvertor::Convert(HFSAT::kCurrencyHKD, HFSAT::kCurrencyUSD));
    if (shortcode_ == "MHI_0")
      return (4.62 * HFSAT::CurrencyConvertor::Convert(HFSAT::kCurrencyHKD, HFSAT::kCurrencyUSD));
    if (shortcode_ == "MCH_0")
      return (2.62 * HFSAT::CurrencyConvertor::Convert(HFSAT::kCurrencyHKD, HFSAT::kCurrencyUSD));

    if (shortcode_ == "NK_0") return (86 * HFSAT::CurrencyConvertor::Convert(HFSAT::kCurrencyJPY, HFSAT::kCurrencyUSD));
    if (shortcode_ == "NKM_0")
      return (11 * HFSAT::CurrencyConvertor::Convert(HFSAT::kCurrencyJPY, HFSAT::kCurrencyUSD));

    return 1000000.0;  // return huge commish for unknown shortcodes
  }
};

struct ors_trade_line {
  std::string ex_symbol_;
  int buysell_;
  int size_;
  double tr_price_;
  int saos_id_;

  ors_trade_line() : ex_symbol_(""), buysell_(-1), size_(-1), tr_price_(-100000), saos_id_(9999999) {}
  ors_trade_line(const ors_trade_line& tr_line_)
      : ex_symbol_(tr_line_.ex_symbol_),
        buysell_(tr_line_.buysell_),
        size_(tr_line_.size_),
        tr_price_(tr_line_.tr_price_),
        saos_id_(tr_line_.saos_id_) {}
  ors_trade_line(ors_trade_line& tr_line_)
      : ex_symbol_(tr_line_.ex_symbol_),
        buysell_(tr_line_.buysell_),
        size_(tr_line_.size_),
        tr_price_(tr_line_.tr_price_),
        saos_id_(tr_line_.saos_id_) {}
};

class PNLStatsComputer {
 private:
  std::string shortcode_;

  int date_;
  int num_of_days_;
  unsigned int trades_start_time_;
  unsigned int trades_end_time_;
  bool output_format_summary_;

  std::map<int, double> date_to_pnl_map_;
  std::map<int, int> date_to_volume_map_;

 public:
  PNLStatsComputer(std::string _shortcode_, int _yyyymmdd_, int _num_of_days_, unsigned int start_time_,
                   unsigned int end_time_, bool is_summary_)
      : shortcode_(_shortcode_),
        date_(_yyyymmdd_),
        num_of_days_(_num_of_days_),
        trades_start_time_(start_time_),
        trades_end_time_(end_time_),
        output_format_summary_(is_summary_) {}

  std::map<int, int> getSAOSids(std::string ors_log_filename_, unsigned int start_time_, unsigned int end_time_) {
    std::map<int, int> saos_ids_;
    HFSAT::BulkFileReader bulk_file_reader_;
    bulk_file_reader_.open(ors_log_filename_.c_str());

    if (!bulk_file_reader_.is_open()) {
      std::cerr << " Could not open ORS Data file: " << ors_log_filename_ << " check permissions \n";
      exit(-1);
    }

    // std::cout << " DEBUG start_time " << start_time_ << " end_time " << end_time_ << " ORS_LOG_FILE "  <<
    // ors_log_filename_ << "\n" ;
    while (true) {
      HFSAT::GenericORSReplyStruct reply_struct;
      size_t read_length_ =
          bulk_file_reader_.read(reinterpret_cast<char*>(&reply_struct), sizeof(HFSAT::GenericORSReplyStruct));

      if (read_length_ < sizeof(HFSAT::GenericORSReplyStruct)) {
        break;
      }

      if (reply_struct.orr_type_ == HFSAT::kORRType_Exec) {
        unsigned int this_msg_timestamp_ = reply_struct.time_set_by_server_.tv_sec;
        if (this_msg_timestamp_ < start_time_ || this_msg_timestamp_ > end_time_) {
          // std::cerr << "there are trades outside this period, size :: " << reply_struct.size_executed_ << "\n";
          continue;
        }
        saos_ids_[reply_struct.server_assigned_order_sequence_] = 1;
      }
    }

    // std::cerr << " saos ids map has " << saos_ids_.size( ) << " items for this date \n" ;
    return saos_ids_;
  }

  int ComputePNLForDay(std::string ors_log_filename_, int yyyymmdd_, unsigned int start_time_, unsigned int end_time_) {
    std::map<int, int> saos_ids_ = getSAOSids(ors_log_filename_, start_time_, end_time_);
    if (saos_ids_.size() <= 0) {
      std::cerr << " saos ids map has no items for this date " << yyyymmdd_ << "\n";
      return -1;
    }

    // get ors trades file name
    HFSAT::ExchangeSymbolManager::SetUniqueInstance(yyyymmdd_);
    const char* t_exchange_symbol_ = HFSAT::ExchangeSymbolManager::GetExchSymbol(shortcode_);

    std::ostringstream t_temp_oss_;
    t_temp_oss_ << yyyymmdd_;
    std::string sdate_ = t_temp_oss_.str();

    std::stringstream ff;
    ff << "grep -l " << t_exchange_symbol_ << " " << TRADES_DATA_PREFIX << "*/*/" << sdate_.substr(0, 4) << "/"
       << sdate_.substr(4, 2) << "/" << sdate_.substr(6, 2) << "/"
       << "trades." << sdate_ << " 2>/dev/null ";
    std::string command_ = ff.str();
    FILE* fp = popen(command_.c_str(), "r");
    char buf[1024];
    std::string ors_trades_filename_ = "NA";
    while (fgets(buf, 1024, fp)) {
      ors_trades_filename_ = buf;
      ors_trades_filename_.erase(std::remove(ors_trades_filename_.begin(), ors_trades_filename_.end(), '\n'),
                                 ors_trades_filename_.end());
    }
    if (!HFSAT::FileUtils::exists(ors_trades_filename_)) {
      std::cerr << " there is no ors_trades_ file for this date " << ors_trades_filename_ << "\n";
      return -1;
    }
    // end of ors trades file name

    // DEBUG //
    /*
      std::map < int , int > ::iterator it_ = saos_ids_.begin ( ) ;
      std::cout << ors_trades_filename_ << "\n" ;
      while ( it_ != saos_ids_.end ( ) )
      {
      std::cout << it_-> first << "\t" ;
      it_ ++ ;
      }
    */

    // get only relevant trades
    std::ifstream file_stream_;
    file_stream_.open(ors_trades_filename_.c_str(), std::ifstream::in);

    std::map<int, ors_trade_line*> saos_trade_line_;

    if (file_stream_.is_open()) {
      const int kBufferLen = 1024;
      char readline_buffer_[kBufferLen];
      bzero(readline_buffer_, kBufferLen);

      while (file_stream_.good()) {
        int tkno_ = 0;
        ors_trade_line temp_tr_line_;
        std::ostringstream t_temp_oss_;

        bzero(readline_buffer_, kBufferLen);
        file_stream_.getline(readline_buffer_, kBufferLen);
        std::string this_full_line_(readline_buffer_);
        // std::cerr << this_full_line_ << "\t" ;

        for (unsigned int i = 0; i < this_full_line_.size(); i++) {
          if (this_full_line_[i] != (char)1) {
            t_temp_oss_ << this_full_line_[i];
            //		    std::cout << this_full_line_[ i ] ;
          } else {
            if (tkno_ == 0) {
              tkno_++;
              temp_tr_line_.ex_symbol_ = t_temp_oss_.str();
              if (temp_tr_line_.ex_symbol_.compare(t_exchange_symbol_) != 0) {
                break;
              }
              t_temp_oss_.str("");
              t_temp_oss_.clear();
            } else if (tkno_ == 1) {
              tkno_++;
              temp_tr_line_.buysell_ = atoi(t_temp_oss_.str().c_str());
              t_temp_oss_.str("");
              t_temp_oss_.clear();
            } else if (tkno_ == 2) {
              tkno_++;
              temp_tr_line_.size_ = atoi(t_temp_oss_.str().c_str());
              t_temp_oss_.str("");
              t_temp_oss_.clear();
            } else if (tkno_ == 3) {
              tkno_++;
              temp_tr_line_.tr_price_ = atof(t_temp_oss_.str().c_str());
              t_temp_oss_.str("");
              t_temp_oss_.clear();
            }
          }
        }
        if (tkno_ == 4) {
          temp_tr_line_.saos_id_ = atoi(t_temp_oss_.str().c_str());
          if (saos_ids_.find(temp_tr_line_.saos_id_) == saos_ids_.end()) {
            // std::cerr << " out side the range \n" ;
          } else if (saos_trade_line_.find(temp_tr_line_.saos_id_) != saos_trade_line_.end()) {
            saos_trade_line_[temp_tr_line_.saos_id_]->size_ += temp_tr_line_.size_;
          } else {
            saos_trade_line_[temp_tr_line_.saos_id_] = new ors_trade_line(temp_tr_line_);
          }

          t_temp_oss_.str("");
          t_temp_oss_.clear();
        }
        /*std::cerr << " KP clearing :: "
                  << temp_tr_line_.ex_symbol_ << " "
                  << temp_tr_line_.buysell_ << " "
                  << temp_tr_line_.size_ << " "
                  << temp_tr_line_.tr_price_ << " "
                  << temp_tr_line_.saos_id_ << "\n" ;*/
      }
      file_stream_.close();
    }

    // std::cerr << saos_trade_line_.size ( ) << "\n";
    int volume_ = 0;
    int bvol_ = 0;
    int svol_ = 0;
    double pnl_ = 0;
    double this_day_commission_ = Commission::GetCommissionForShortCode(shortcode_, 1);
    double number_to_dollars_ = HFSAT::SecurityDefinitions::GetContractNumbersToDollars(shortcode_, yyyymmdd_);

    double last_btr_price_ = -10000000;  // to close out open positions
    double last_str_price_ = -10000000;  // to close out open positions

    for (std::map<int, ors_trade_line*>::iterator it = saos_trade_line_.begin(); it != saos_trade_line_.end(); it++) {
      volume_ += (it->second)->size_;
      if ((it->second)->buysell_ == 0) {
        bvol_ += (it->second)->size_;
        pnl_ -= (it->second)->size_ * (it->second)->tr_price_ * number_to_dollars_;
        pnl_ -= (it->second)->size_ * this_day_commission_;
        last_btr_price_ = (it->second)->tr_price_;
      } else if ((it->second)->buysell_ == 1) {
        svol_ += (it->second)->size_;
        pnl_ += (it->second)->size_ * (it->second)->tr_price_ * number_to_dollars_;
        pnl_ -= (it->second)->size_ * this_day_commission_;
        last_str_price_ = (it->second)->tr_price_;
      }
    }

    pnl_ += (bvol_ - svol_) * (last_btr_price_ + last_str_price_) / 2 * number_to_dollars_;
    pnl_ -= abs((bvol_ - svol_) * this_day_commission_);

    date_to_pnl_map_[yyyymmdd_] = pnl_;
    date_to_volume_map_[yyyymmdd_] = volume_;

    //    printf ( " volume %d buy_size %d sell_size %d open_positions %d pnl %f \n" , volume_ , bvol_ , svol_ , abs (
    //    bvol_ - svol_) , pnl_ ) ;
    return 1;
  }

  void GeneratePNLStats() {
    int start_date_ = date_;
    int MAX_ATTEMPTS = 2 * num_of_days_;

    for (int days_counter_ = 0, max_attempt_counter_ = 0;
         days_counter_ < num_of_days_ && max_attempt_counter_ < MAX_ATTEMPTS; max_attempt_counter_++) {
      max_attempt_counter_++;
      // get ors log file name
      HFSAT::ExchangeSymbolManager::SetUniqueInstance(start_date_);
      const char* t_exchange_symbol_ = HFSAT::ExchangeSymbolManager::GetExchSymbol(shortcode_);
      std::string location_ =
          HFSAT::TradingLocationUtils::GetTradingLocationName(HFSAT::TradingLocationUtils::GetTradingLocationExch(
              HFSAT::SecurityDefinitions::GetContractExchSource(shortcode_, start_date_)));

      std::ostringstream t_temp_oss_;
      t_temp_oss_ << start_date_;
      std::string date_ = t_temp_oss_.str();

      std::stringstream ff;
      ff << LOGGED_DATA_PREFIX << location_ << "/" << date_.substr(0, 4) << "/" << date_.substr(4, 2) << "/"
         << date_.substr(6, 2) << "/" << t_exchange_symbol_ << "_" << start_date_;
      std::string ors_log_filename_ = ff.str();
      while (ors_log_filename_.find(" ") != std::string::npos) {  // Liffe naming issues
        ors_log_filename_.replace(ors_log_filename_.find(" "), 1, "~");
      }
      if (!HFSAT::FileUtils::exists(ors_log_filename_) && !HFSAT::FileUtils::exists(ors_log_filename_ + ".gz")) {
        start_date_ = HFSAT::DateTime::CalcPrevWeekDay(start_date_);
        continue;
      }
      // end ors_log_file

      // get date_to_pnl map record
      days_counter_++;

      struct tm timeinfo = {0};
      timeinfo.tm_year = (start_date_ / 10000) - 1900;
      timeinfo.tm_mon = (start_date_ / 100) % 100 - 1;
      timeinfo.tm_mday = (start_date_ % 100);
      time_t unixtime_start_ = mktime(&timeinfo);
      time_t unixtime_end_ = mktime(&timeinfo);
      unixtime_start_ += (trades_start_time_);
      unixtime_end_ += (trades_end_time_);

      if (ComputePNLForDay(ors_log_filename_, start_date_, unixtime_start_, unixtime_end_) == -1) {
        days_counter_--;
      }
      start_date_ = HFSAT::DateTime::CalcPrevWeekDay(start_date_);
    }

    if (!output_format_summary_) {
      std::map<int, double>::iterator pnl_itr_ = date_to_pnl_map_.begin();

      while (pnl_itr_ != date_to_pnl_map_.end()) {
        printf("Date : %d TotalVolume : %d TotalPnl : %lf\n", pnl_itr_->first, date_to_volume_map_[pnl_itr_->first],
               pnl_itr_->second);
        pnl_itr_++;
      }
    }
    {
      double sum_pnl_ = 0.0;
      double sum_pnl_square_ = 0.0;
      unsigned int sum_volume_ = 0;

      double mean_pnl_ = 0.0;
      double std_pnl_ = 0.0;
      double sharpe_pnl_ = 0.0;
      double max_pnl_cum_sum_ = 0.0;
      double max_pnl_drawdown_ = 0.0;
      double sum_pnl_losses_only_ = 0.0;
      double pnl_gain_to_pain_ratio_ = 0.0;
      double mean_volume_ = 0.0;
      double temp_var_ = 0.0;

      std::map<int, double>::iterator pnl_itr_ = date_to_pnl_map_.begin();

      if (pnl_itr_ != date_to_pnl_map_.end()) {
        while (pnl_itr_ != date_to_pnl_map_.end()) {
          temp_var_ = pnl_itr_->second;
          sum_pnl_ += temp_var_;
          sum_pnl_square_ += (temp_var_) * (temp_var_);
          max_pnl_cum_sum_ = std::max(max_pnl_cum_sum_, sum_pnl_);
          max_pnl_drawdown_ = std::max(max_pnl_drawdown_, max_pnl_cum_sum_ - sum_pnl_);
          sum_volume_ += date_to_volume_map_[pnl_itr_->first];

          if (temp_var_ < 0) {
            sum_pnl_losses_only_ += -temp_var_;
          }
          pnl_itr_++;
        }
        mean_pnl_ = sum_pnl_ / date_to_pnl_map_.size();
        std_pnl_ = std::sqrt((sum_pnl_square_ - (mean_pnl_ * mean_pnl_)) /
                             (std::max(1, (int)date_to_pnl_map_.size() - 1)));  // sample stdev
        sharpe_pnl_ = mean_pnl_ / std_pnl_;
        mean_volume_ = sum_volume_ / date_to_volume_map_.size();

        if (sum_pnl_losses_only_ > 0.0001) {
          pnl_gain_to_pain_ratio_ = sum_pnl_ / sum_pnl_losses_only_;
        }
        if (print_verbose_) {
          printf("StartDate: %d LoopBackDays: %d TotalPNL: %lf TotalVolume: %u AvgPNL: %lf AvgVol: %lf \n", date_,
                 (int)date_to_pnl_map_.size(), sum_pnl_, sum_volume_, mean_pnl_, mean_volume_);
          printf("%s  \t  %s  \t  %s  \t  %s  \t  %s  \t  %s  \t  %s  \t  %s  \t  %s\n", "SHORTCODE", "PNL_SUM",
                 "PNL_MEAN", "PNL_STD", "PNL_SHARPE", "PNL_VOL", "MAX_DD", "MEAN_PNL/MAX_DD", "PNL_GPR");
          printf("%s  \t  %.3f  \t  %.3f  \t  %.3f  \t  %.3f   \t  %.3f  \t  %.3f  \t  %.3f   \t\t  %.3f\n",
                 shortcode_.c_str(), sum_pnl_, mean_pnl_, std_pnl_, sharpe_pnl_, mean_volume_, max_pnl_drawdown_,
                 mean_pnl_ / max_pnl_drawdown_, pnl_gain_to_pain_ratio_);
        } else {
          // WARNING :: this particular format is being used in performance_summary script, so if you wish to change
          // print format use a flag to do so.
          printf("%s | %.3f | %.3f | %.3f | %.3f | %.3f | %.3f | %.3f | %.3f", shortcode_.c_str(), sum_pnl_, mean_pnl_,
                 std_pnl_, sharpe_pnl_, mean_volume_, max_pnl_drawdown_, mean_pnl_ / max_pnl_drawdown_,
                 pnl_gain_to_pain_ratio_);
        }
        // shortcode_ sum_pnl_ mean_pnl_ std_pnl_ sharpe_pnl_ mean_volume_ max_pnl_drawdown_ mean_pnl_ / max_drawdown_
        // pnl_gain_to_pain_ratio_
      }
    }
  }
};

int main(int argc, char** argv) {
  std::string dir = "";
  std::string shortcode_ = "";
  int yyyymmdd_ = 0;
  int num_of_days_ = 0;
  int utc_start_time_ = 0;
  int utc_end_time_ = 0;
  bool output_format_summary_ = true;

  std::string hours_start_str_ = "";
  std::string hours_end_str_ = "";
  std::string timezone_ = "";

  if (argc < 7) {
    std::cout << "Usage : SHORTCODE YYYYMMDD NUM_OF_DAYS START_TIME END_TIME SUMMARY/DETAILS" << std::endl;
    std::cout << "Example : HSI_0 20130214 30 HKT_800 HKT_1500 DETAILS" << std::endl;
    exit(0);
  } else {
    shortcode_ = std::string(argv[1]);
    yyyymmdd_ = atoi(argv[2]);
    num_of_days_ = atoi(argv[3]);

    if ((strncmp(argv[4], "EST_", 4) == 0) || (strncmp(argv[4], "CST_", 4) == 0) ||
        (strncmp(argv[4], "CET_", 4) == 0) || (strncmp(argv[4], "BRT_", 4) == 0) ||
        (strncmp(argv[4], "UTC_", 4) == 0) || (strncmp(argv[4], "KST_", 4) == 0) ||
        (strncmp(argv[4], "HKT_", 4) == 0) || (strncmp(argv[4], "IST_", 4) == 0) ||
        (strncmp(argv[4], "JST_", 4) == 0) || (strncmp(argv[4], "BST_", 4) == 0)) {
      utc_start_time_ = HFSAT::DateTime::GetUTCHHMMFromTZHHMM(yyyymmdd_, atoi(argv[4] + 4), argv[4]);
    }

    if ((strncmp(argv[5], "EST_", 4) == 0) || (strncmp(argv[5], "CST_", 4) == 0) ||
        (strncmp(argv[5], "CET_", 4) == 0) || (strncmp(argv[5], "BRT_", 4) == 0) ||
        (strncmp(argv[5], "UTC_", 4) == 0) || (strncmp(argv[5], "KST_", 4) == 0) ||
        (strncmp(argv[5], "HKT_", 4) == 0) || (strncmp(argv[5], "IST_", 4) == 0) ||
        (strncmp(argv[5], "JST_", 4) == 0) || (strncmp(argv[5], "BST_", 4) == 0)) {
      utc_end_time_ = HFSAT::DateTime::GetUTCHHMMFromTZHHMM(yyyymmdd_, atoi(argv[5] + 4), argv[5]);
    }

    output_format_summary_ = (!strcmp(argv[6], "SUMMARY")) ? true : false;

    if (argc > 7) {
      print_verbose_ = (!strcmp(argv[7], "V") || !strcmp(argv[7], "v"));
    }
  }

  // std::cerr << " DEBUG utc_start_time_ "  << utc_start_time_ << " utc_end_time_ "  << utc_end_time_ << "\n";
  utc_start_time_ = (utc_start_time_ / 100) * 3600 + (utc_start_time_ % 100) * 60;
  utc_end_time_ = (utc_end_time_ / 100) * 3600 + (utc_end_time_ % 100) * 60;

  /*
     for each date
        check if volume is greater than zero
        check if we have ORSTrades file
        get all exec SAOS numbers from ORSlog file within start & end time
        make trades_file using ORSTrades files conditioning on the SAOS number found in step3
        calculate pnl
     end for
     generate pnl stats
  */

  PNLStatsComputer pnl_status_computer_(shortcode_, yyyymmdd_, num_of_days_, utc_start_time_, utc_end_time_,
                                        output_format_summary_);
  pnl_status_computer_.GeneratePNLStats();
  return 0;
}
