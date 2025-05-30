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

bool print_verbose_ = false;

struct PnlStats {
  double pnl_;
  double realized_pnl_;
  double opentrade_unrealized_pnl_;
  double total_pnl_;
  double min_pnl_;
  double max_pnl_;
  double max_drawdown_;
  int total_uts_;
};

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
    if (shortcode_ == "FOAM_0")
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
      return (0.26 * HFSAT::CurrencyConvertor::Convert(HFSAT::kCurrencyGBP, HFSAT::kCurrencyUSD));
    if (shortcode_ == "LFZ_0")
      return (0.31 * HFSAT::CurrencyConvertor::Convert(HFSAT::kCurrencyGBP, HFSAT::kCurrencyUSD));
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

class BasePnlClass {
 protected:
  double pnl_;           ///< without accounting for position ... cash accouting
  double realized_pnl_;  ///< the pnl recorded when the last time we were flat or we changed signs of position
  double opentrade_unrealized_pnl_;  ///< the unrealized pnl of this trade
  double total_pnl_;  ///< realized_pnl_ + opentrade_unrealized_pnl_ ( of this trade ) ... best estimate of current pnl
  double last_closing_trade_pnl_;  ///< the pnl of only the last closed trade
  int position_;                   ///< current position
  double commish_dollars_per_unit_;
  double numbers_to_dollars_;

  double current_price_;

  double min_pnl_;
  double max_pnl_;
  double max_drawdown_;

 public:
  BasePnlClass(double t_commish_dollars_per_unit_, double n2d_)
      : pnl_(0),
        realized_pnl_(0),
        opentrade_unrealized_pnl_(0),
        total_pnl_(0),
        last_closing_trade_pnl_(0),
        position_(0),
        commish_dollars_per_unit_(t_commish_dollars_per_unit_),
        numbers_to_dollars_(n2d_),
        current_price_(0),
        min_pnl_(10000.0),
        max_pnl_(-10000.0),
        max_drawdown_(0) {}

  ~BasePnlClass() {}

  void OnExec(int r_new_position_, const int exec_size_, const HFSAT::TradeType_t _buysell_,
              const double this_trade_price_) {
    if (_buysell_ == HFSAT::kTradeTypeBuy)
      r_new_position_ = position_ + exec_size_;
    else
      r_new_position_ = position_ - exec_size_;

    int abs_change_position_ = abs(r_new_position_ - position_);
    current_price_ =
        this_trade_price_;  // on a trade use this trade price to compute unrealized pnl and not mid_price ? why ?

    if (_buysell_ == HFSAT::kTradeTypeBuy) {  // buy was filled
      if ((position_ < 0) &&
          (position_ * r_new_position_ <= 0)) {  // we were short and position and new position are of differetn sign
        // hence we are changing sign

        // break trade into two parts
        // first part to close out earlier position
        int trade1size = -1 * position_;
        pnl_ -= (trade1size * this_trade_price_ * numbers_to_dollars_);
        pnl_ -= (trade1size * commish_dollars_per_unit_);

        last_closing_trade_pnl_ = pnl_ - realized_pnl_;
        realized_pnl_ = pnl_;

        // remaining part of the trade
        int trade2size = (abs_change_position_ - trade1size);
        if (trade2size == 0) total_pnl_ = pnl_;
        if (trade2size > 0) {
          pnl_ -= (trade2size * this_trade_price_ * numbers_to_dollars_);
          pnl_ -= (trade2size * commish_dollars_per_unit_);

          total_pnl_ = pnl_ + (r_new_position_ * current_price_ * numbers_to_dollars_);
          opentrade_unrealized_pnl_ = total_pnl_ - realized_pnl_;
        }
      } else {
        pnl_ -= (abs_change_position_ * this_trade_price_ * numbers_to_dollars_);
        pnl_ -= (abs_change_position_ * commish_dollars_per_unit_);

        total_pnl_ = pnl_ + (r_new_position_ * current_price_ * numbers_to_dollars_);
        opentrade_unrealized_pnl_ = total_pnl_ - realized_pnl_;
      }
    } else {  // _buysell_ == TradeTypeSell ... sell trade
      if ((position_ > 0) &&
          (position_ * r_new_position_ <= 0)) {  // we were long and position and new position are of differetn sign
        // hence we are changing sign

        // break trade into two parts
        // first part to close out earlier position
        int trade1size = position_;
        pnl_ += (trade1size * this_trade_price_ * numbers_to_dollars_);
        pnl_ -= (trade1size * commish_dollars_per_unit_);

        // since trade closed update realized pnl estimates
        last_closing_trade_pnl_ = pnl_ - realized_pnl_;
        realized_pnl_ = pnl_;

        // remaining part of the trade
        int trade2size = (abs_change_position_ - trade1size);
        if (trade2size == 0) total_pnl_ = pnl_;
        if (trade2size > 0) {
          pnl_ += (trade2size * this_trade_price_ * numbers_to_dollars_);
          pnl_ -= (trade2size * commish_dollars_per_unit_);

          total_pnl_ = pnl_ + (r_new_position_ * current_price_ * numbers_to_dollars_);
          opentrade_unrealized_pnl_ = total_pnl_ - realized_pnl_;
        }
      } else {
        pnl_ += (abs_change_position_ * this_trade_price_ * numbers_to_dollars_);
        pnl_ -= (abs_change_position_ * commish_dollars_per_unit_);

        total_pnl_ = pnl_ + (r_new_position_ * current_price_ * numbers_to_dollars_);
        opentrade_unrealized_pnl_ = total_pnl_ - realized_pnl_;
      }
    }

    position_ = r_new_position_;
    if (total_pnl_ < min_pnl_) min_pnl_ = total_pnl_;
    if (total_pnl_ > max_pnl_) max_pnl_ = total_pnl_;
    if (max_pnl_ - total_pnl_ > max_drawdown_) max_drawdown_ = max_pnl_ - total_pnl_;
  }

  /*    void OnMarketUpdate ( const unsigned int _security_id_, const MarketUpdateInfo & _market_update_info_ )
      {
        current_price_ = _market_update_info_.mkt_size_weighted_price_;
        total_pnl_ = pnl_ + ( position_ * current_price_ * numbers_to_dollars_ ) ;
        opentrade_unrealized_pnl_ = total_pnl_ - realized_pnl_ ;
      }
  */

  inline double pnl() const { return pnl_; }
  inline double realized_pnl() const { return realized_pnl_; }
  inline double opentrade_unrealized_pnl() const { return opentrade_unrealized_pnl_; }
  inline double total_pnl() const { return total_pnl_; }
  inline double min_pnl() const { return min_pnl_; }
  inline double max_pnl() const { return max_pnl_; }
  inline double max_drawdown() const { return max_drawdown_; }
  inline double last_closing_trade_pnl() const { return last_closing_trade_pnl_; }
  inline int position() const { return position_; }
  inline double commish_dollars_per_unit() const { return commish_dollars_per_unit_; }
  inline double numbers_to_dollars() const { return numbers_to_dollars_; }
  inline double current_price() const { return current_price_; }
  inline double ReportConservativeTotalPNL() const { return total_pnl_; }
};

class PNLStatsComputer {
 private:
  std::string shortcode_;
  int date_;
  int num_of_days_;
  unsigned int trades_start_time_;
  unsigned int trades_end_time_;
  bool output_format_summary_;

  std::map<int, int> saos_to_size_exec_;       // meant to store the total volume for day
  std::map<int, int> saos_to_last_size_exec_;  // meant to compute pnls for incremental size exec

  std::map<int, double> date_to_pnl_map_;
  std::map<int, int> date_to_volume_map_;
  std::map<int, PnlStats> date_to_pnlstats_map_;

  std::map<int, int> conf_saos_size_map_;
  std::map<int, int> exec_saos_size_map_;
  std::map<int, unsigned long> filter_imatch_trades_saos_;
  std::map<int, unsigned long> exec_last_timestamp_;
  std::map<int, bool> filter_exec_after_cxld_;
  std::map<int, std::vector<int> > saci_to_order_size_vec_map_;

 public:
  PNLStatsComputer(std::string _shortcode_, int _yyyymmdd_, int _num_of_days_, unsigned int start_time_,
                   unsigned int end_time_, bool is_summary_)
      : shortcode_(_shortcode_),
        date_(_yyyymmdd_),
        num_of_days_(_num_of_days_),
        trades_start_time_(start_time_),
        trades_end_time_(end_time_),
        output_format_summary_(is_summary_),
        saos_to_size_exec_(),
        saos_to_last_size_exec_(),
        filter_imatch_trades_saos_(),
        exec_last_timestamp_(),
        filter_exec_after_cxld_()

  {}

  void InitAllMembers() {
    saos_to_size_exec_.clear();
    saos_to_last_size_exec_.clear();
    conf_saos_size_map_.clear();
    exec_saos_size_map_.clear();
    filter_imatch_trades_saos_.clear();
    exec_last_timestamp_.clear();
    filter_exec_after_cxld_.clear();
    saci_to_order_size_vec_map_.clear();
  }

  unsigned int PreComputeTotalVolume(std::string filename_to_read, unsigned int start_time_, unsigned int end_time_) {
    unsigned int total_volume_ = 0;

    HFSAT::BulkFileReader bulk_file_reader_;
    bulk_file_reader_.open(filename_to_read.c_str());

    if (!bulk_file_reader_.is_open()) {
      std::cerr << " Could not open ORS Data file: " << filename_to_read << " check permissions \n";
      exit(-1);
    }

    while (true) {
      HFSAT::GenericORSReplyStruct reply_struct;

      size_t read_length_ =
          bulk_file_reader_.read(reinterpret_cast<char*>(&reply_struct), sizeof(HFSAT::GenericORSReplyStruct));

      if (read_length_ < sizeof(HFSAT::GenericORSReplyStruct)) break;

      char pp[6] = {'\0'};
      sprintf(pp, "%.6f", reply_struct.price_);

      if (reply_struct.orr_type_ == HFSAT::kORRType_Conf) {
        conf_saos_size_map_[reply_struct.server_assigned_order_sequence_] = reply_struct.size_remaining_;
      }

      if ((reply_struct.orr_type_ == HFSAT::kORRType_Exec) || (reply_struct.orr_type_ == HFSAT::kORRType_IntExec)) {
        if (conf_saos_size_map_.find(reply_struct.server_assigned_order_sequence_) == conf_saos_size_map_.end()) {
          filter_imatch_trades_saos_[reply_struct.server_assigned_order_sequence_] =
              reply_struct.time_set_by_server_.tv_sec * 1000000 + reply_struct.time_set_by_server_.tv_usec;
        }

        unsigned int this_msg_timestamp_ = reply_struct.time_set_by_server_.tv_sec;

        if (this_msg_timestamp_ < start_time_ || this_msg_timestamp_ > end_time_) continue;

        exec_last_timestamp_[reply_struct.server_assigned_order_sequence_] =
            reply_struct.time_set_by_server_.tv_sec * 1000000 + reply_struct.time_set_by_server_.tv_usec;

        saos_to_size_exec_[reply_struct.server_assigned_order_sequence_] = reply_struct.size_executed_;

        exec_saos_size_map_[reply_struct.server_assigned_order_sequence_] = reply_struct.size_executed_;
      }

      if (reply_struct.orr_type_ == HFSAT::kORRType_Cxld) {
        if (reply_struct.size_remaining_ == conf_saos_size_map_[reply_struct.server_assigned_order_sequence_]) {
          filter_exec_after_cxld_[reply_struct.server_assigned_order_sequence_] = true;
        }

        if (exec_saos_size_map_.find(reply_struct.server_assigned_order_sequence_) != exec_saos_size_map_.end()) {
          if (exec_saos_size_map_[reply_struct.server_assigned_order_sequence_] ==
              conf_saos_size_map_[reply_struct.server_assigned_order_sequence_])
            filter_imatch_trades_saos_[reply_struct.server_assigned_order_sequence_] =
                exec_last_timestamp_[reply_struct.server_assigned_order_sequence_];
        }
      }
    }

    std::map<int, int>::iterator saos_to_size_exec_itr_ = saos_to_size_exec_.begin();

    while (saos_to_size_exec_itr_ != saos_to_size_exec_.end()) {
      total_volume_ += (saos_to_size_exec_itr_->second);

      saos_to_size_exec_itr_++;
    }

    return total_volume_;
  }

  void ComputePNLForDay(std::string filename_to_read, int yyyymmdd_, unsigned int start_time_, unsigned int end_time_) {
    unsigned int this_day_volume_ = PreComputeTotalVolume(filename_to_read, start_time_, end_time_);
    double this_day_commission_ = Commission::GetCommissionForShortCode(shortcode_, this_day_volume_);
    double number_to_dollars_ = HFSAT::SecurityDefinitions::GetContractNumbersToDollars(shortcode_, yyyymmdd_);

    BasePnlClass PnlObj(this_day_commission_, number_to_dollars_);

    // int last_global_exec_position_ = -1000 ;

    int total_buy_ = 0;
    int total_sell_ = 0;

    HFSAT::BulkFileReader bulk_file_reader_;
    bulk_file_reader_.open(filename_to_read.c_str());

    if (!bulk_file_reader_.is_open()) {
      std::cerr << " Could not open ORS Data file , check permissions " << filename_to_read << "\n";
      exit(-1);
    }

    double unreal_pnl_ = 0;
    int volume_ = 0;
    double last_trade_price_ = 0.0;

    while (true) {
      HFSAT::GenericORSReplyStruct reply_struct;

      size_t read_length_ =
          bulk_file_reader_.read(reinterpret_cast<char*>(&reply_struct), sizeof(HFSAT::GenericORSReplyStruct));

      if (read_length_ < sizeof(HFSAT::GenericORSReplyStruct)) break;

      char pp[6] = {'\0'};
      sprintf(pp, "%.6f", reply_struct.price_);

      if (filter_exec_after_cxld_.find(reply_struct.server_assigned_order_sequence_) != filter_exec_after_cxld_.end())
        continue;

      if (filter_imatch_trades_saos_.find(reply_struct.server_assigned_order_sequence_) !=
          filter_imatch_trades_saos_.end()) {
        unsigned long this_time_ =
            reply_struct.time_set_by_server_.tv_sec * 1000000 + reply_struct.time_set_by_server_.tv_usec;

        if (filter_imatch_trades_saos_[reply_struct.server_assigned_order_sequence_] == this_time_) continue;
      }

      if (reply_struct.orr_type_ == HFSAT::kORRType_Seqd) {
        unsigned int this_msg_timestamp_ = reply_struct.time_set_by_server_.tv_sec;
        if (this_msg_timestamp_ < start_time_ || this_msg_timestamp_ > end_time_) continue;

        if (saci_to_order_size_vec_map_.find(reply_struct.server_assigned_client_id_) !=
            saci_to_order_size_vec_map_.end()) {
          saci_to_order_size_vec_map_[reply_struct.server_assigned_client_id_].push_back(reply_struct.size_remaining_);
        } else {
          std::vector<int> temp_vector;
          saci_to_order_size_vec_map_[reply_struct.server_assigned_client_id_] = temp_vector;
          saci_to_order_size_vec_map_[reply_struct.server_assigned_client_id_].push_back(reply_struct.size_remaining_);
        }
      }

      if ((reply_struct.orr_type_ == HFSAT::kORRType_Exec) || (reply_struct.orr_type_ == HFSAT::kORRType_IntExec)) {
        unsigned int this_msg_timestamp_ = reply_struct.time_set_by_server_.tv_sec;

        if (this_msg_timestamp_ < start_time_ || this_msg_timestamp_ > end_time_) continue;

        int size_exec_ = 0;

        if (!reply_struct.size_executed_) continue;

        if (saos_to_last_size_exec_.find(reply_struct.server_assigned_order_sequence_) !=
            saos_to_last_size_exec_.end()) {
          size_exec_ =
              reply_struct.size_executed_ - saos_to_last_size_exec_[reply_struct.server_assigned_order_sequence_];

        } else {
          size_exec_ = reply_struct.size_executed_;
        }

        // PnlObj.OnExec( reply_struct.global_position_, reply_struct.buysell_, reply_struct.price_ ) ;

        if (!size_exec_ || size_exec_ < 0) continue;

        //        if ( last_global_exec_position_ == -1000 ) last_global_exec_position_ = reply_struct.global_position_
        //        ;
        //        else if ( reply_struct.global_position_ == last_global_exec_position_ ) continue ;

        if (conf_saos_size_map_[reply_struct.server_assigned_order_sequence_] < size_exec_) {
          size_exec_ = conf_saos_size_map_[reply_struct.server_assigned_order_sequence_];
        }

        HFSAT::TradeType_t this_trade_type_ = reply_struct.buysell_;

        last_trade_price_ = reply_struct.price_;

        PnlObj.OnExec(reply_struct.global_position_, size_exec_, reply_struct.buysell_, reply_struct.price_);

        if (this_trade_type_ == HFSAT::kTradeTypeBuy) {
          unreal_pnl_ -= size_exec_ * reply_struct.price_ * number_to_dollars_;

          //	  std::cerr << reply_struct.symbol_ << "\001" << "0" << "\001" << size_exec_ << "\001" << pp << "\001"
          //<< reply_struct.server_assigned_order_sequence_ << "\n" ;

          total_buy_ += size_exec_;

        } else {
          unreal_pnl_ += size_exec_ * reply_struct.price_ * number_to_dollars_;

          //	  std::cerr << reply_struct.symbol_ << "\001" << "1" << "\001" << size_exec_ << "\001" << pp << "\001"
          //<< reply_struct.server_assigned_order_sequence_ << "\n" ;
          total_sell_ += size_exec_;
        }

        unreal_pnl_ -= (size_exec_ * this_day_commission_);

        //        std::cerr << unreal_pnl_ << "\t" << PnlObj.opentrade_unrealized_pnl() << "\t" << PnlObj.realized_pnl()
        //        <<"\n" ;

        volume_ += size_exec_;

        saos_to_last_size_exec_[reply_struct.server_assigned_order_sequence_] = reply_struct.size_executed_;
      }
    }

    int position_ = total_buy_ - total_sell_;

    if (position_) {
      std::cerr << "DEBUG:: the position is not flat within the period specified. "
                << "OPEN POSITIONS:: " << position_ << "\n";
    }

    //    std::cerr << "DEBUG :: volume :: " << volume_ << "\n" ;

    unreal_pnl_ += (position_)*last_trade_price_ * number_to_dollars_ - abs(position_) * this_day_commission_;

    date_to_pnl_map_[yyyymmdd_] = unreal_pnl_;
    date_to_volume_map_[yyyymmdd_] = volume_;

    int total_uts = 0;
    int skip_check = 0;
    if ((shortcode_ == "LFI_0") || (shortcode_ == "LFI_1") || (shortcode_ == "LFI_2") || (shortcode_ == "LFI_3") ||
        (shortcode_ == "LFI_4") || (shortcode_ == "LFI_5") || (shortcode_ == "LFI_6") || (shortcode_ == "BAX_1") ||
        (shortcode_ == "BAX_2") || (shortcode_ == "BAX_3") || (shortcode_ == "BAX_4") || (shortcode_ == "BAX_5") ||
        (shortcode_ == "DI1F15") || (shortcode_ == "DI1F16") || (shortcode_ == "DI1F17") || (shortcode_ == "DI1F18") ||
        (shortcode_ == "DI1N15"))
      skip_check = 1;
    std::map<int, std::vector<int> >::iterator it_;
    for (it_ = saci_to_order_size_vec_map_.begin(); it_ != saci_to_order_size_vec_map_.end(); it_++) {
      if ((it_->second.size() > 25) || (skip_check))  // required to avoid small overlap in eu/us hrs
      {
        std::nth_element(it_->second.begin(), it_->second.begin() + it_->second.size() / 2, it_->second.end());
        total_uts += it_->second.at(it_->second.size() / 2);  // summing up median order size of all queries
        // std::cout<<it_->first<<"\t"<<it_->second.at( it_->second.size()/2 )<<"\t"<< it_->second.size()<<"\n";
      }
    }
    // std::cout<<"\n";

    PnlStats temppnlstats;
    temppnlstats.pnl_ = PnlObj.pnl();
    temppnlstats.realized_pnl_ = PnlObj.realized_pnl();
    temppnlstats.opentrade_unrealized_pnl_ = PnlObj.opentrade_unrealized_pnl();
    temppnlstats.total_pnl_ = PnlObj.total_pnl();
    temppnlstats.min_pnl_ = PnlObj.min_pnl();
    temppnlstats.max_pnl_ = PnlObj.max_pnl();
    temppnlstats.max_drawdown_ = PnlObj.max_drawdown();
    temppnlstats.total_uts_ = total_uts;
    date_to_pnlstats_map_[yyyymmdd_] = temppnlstats;
  }

  void GeneratePNLStats() {
    int start_date_ = date_;
    int MAX_ATTEMPTS = 2 * num_of_days_;

    for (int days_counter_ = 0, max_attempt_counter_ = 0;
         days_counter_ < num_of_days_ && max_attempt_counter_ < MAX_ATTEMPTS; max_attempt_counter_++) {
      InitAllMembers();

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

      std::string filename_to_read = ff.str();

      while (filename_to_read.find(" ") != std::string::npos) {  // Liffe naming issues
        filename_to_read.replace(filename_to_read.find(" "), 1, "~");
      }

      max_attempt_counter_++;

      if (!HFSAT::FileUtils::exists(filename_to_read) && !HFSAT::FileUtils::exists(filename_to_read + ".gz")) {
        //	std::cerr << "DEBUG :: this file doesnt exist " << filename_to_read << " " << filename_to_read + ".gz\n"
        //;

        start_date_ = HFSAT::DateTime::CalcPrevWeekDay(start_date_);
        continue;
      }

      struct tm timeinfo = {0};

      timeinfo.tm_year = (start_date_ / 10000) - 1900;
      timeinfo.tm_mon = (start_date_ / 100) % 100 - 1;
      timeinfo.tm_mday = (start_date_ % 100);

      time_t unixtime_start_ = mktime(&timeinfo);
      time_t unixtime_end_ = mktime(&timeinfo);

      unixtime_start_ += (trades_start_time_);
      unixtime_end_ += (trades_end_time_);

      days_counter_++;

      //      std::cerr << "DEBUG:: reading file:: " << filename_to_read << " start_date_:: " << start_date_ << "
      //      unixtime_start_:: " << unixtime_start_ << " unixtime_end_:: " << unixtime_end_ << "\n" ;

      ComputePNLForDay(filename_to_read, start_date_, unixtime_start_, unixtime_end_);

      start_date_ = HFSAT::DateTime::CalcPrevWeekDay(start_date_);
    }

    if (!output_format_summary_) {
      std::map<int, double>::iterator pnl_itr_ = date_to_pnl_map_.begin();

      while (pnl_itr_ != date_to_pnl_map_.end()) {
        printf("Date : %d TotalVolume : %d TotalPnl : %lf ", pnl_itr_->first, date_to_volume_map_[pnl_itr_->first],
               pnl_itr_->second);
        printf("pnl: %lf realized: %lf unrealized: %lf total: %lf min: %lf max: %lf maxdd: %lf uts: %d\n",
               date_to_pnlstats_map_[pnl_itr_->first].pnl_, date_to_pnlstats_map_[pnl_itr_->first].realized_pnl_,
               date_to_pnlstats_map_[pnl_itr_->first].opentrade_unrealized_pnl_,
               date_to_pnlstats_map_[pnl_itr_->first].total_pnl_, date_to_pnlstats_map_[pnl_itr_->first].min_pnl_,
               date_to_pnlstats_map_[pnl_itr_->first].max_pnl_, date_to_pnlstats_map_[pnl_itr_->first].max_drawdown_,
               date_to_pnlstats_map_[pnl_itr_->first].total_uts_);
        pnl_itr_++;
      }
    } else {
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

  utc_start_time_ = (utc_start_time_ / 100) * 3600 + (utc_start_time_ % 100) * 60;
  utc_end_time_ = (utc_end_time_ / 100) * 3600 + (utc_end_time_ % 100) * 60;

  //  utc_start_time_ = atoi ( hours_start_str_.substr ( 0, 2 ).c_str () ) * 3600 + atoi ( hours_start_str_.substr ( 2,
  //  2 ).c_str () ) * 60 ;
  //  utc_end_time_ = atoi ( hours_end_str_.substr ( 0, 2 ).c_str () ) * 3600 + atoi ( hours_end_str_.substr ( 2, 2
  //  ).c_str () ) * 60 ;

  PNLStatsComputer pnl_status_computer_(shortcode_, yyyymmdd_, num_of_days_, utc_start_time_, utc_end_time_,
                                        output_format_summary_);

  pnl_status_computer_.GeneratePNLStats();

  return 0;
}
