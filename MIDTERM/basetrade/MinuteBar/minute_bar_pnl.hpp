/**
    \file MinuteBar/minute_bar_pnl.hpp

    \author: (c) Copyright Two Roads Technological Solutions Pvt Ltd 2011
     Address:
         Suite No 353, Evoma, #14, Bhattarhalli,
         Old Madras Road, Near Garden City College,
         KR Puram, Bangalore 560049, India
         +91 80 4190 3551
 */
#pragma once

#include "dvccode/Utils/bulk_file_writer.hpp"
#include "baseinfra/MinuteBar/minute_bar_order_manager.hpp"

namespace HFSAT {

class MinuteBarPNL : public ExecutionListener {
 protected:
  BulkFileWriter& trades_writer_;
  DebugLogger& dbglogger_;
  const Watch& watch_;
  MinuteBarOrderManager& order_manager_;
  int runtime_id_;

  char numbered_secname_[24];  ///< secname.runtime_id_ for pnl file
  std::string shortcode_;
  HFSAT::ExchSource_t exch_source_;

  double pnl_;           ///< without accounting for position ... cash accouting
  double realized_pnl_;  ///< the pnl recorded when the last time we were flat or we changed signs of position
  double opentrade_unrealized_pnl_;  ///< the unrealized pnl of this trade
  double min_pnl_till_now_;          // min total_pnl_ seen till now ... best estimate of min achieved during day
  double total_pnl_;  ///< realized_pnl_ + opentrade_unrealized_pnl_ ( of this trade ) ... best estimate of current pnl
  double last_closing_trade_pnl_;  ///< the pnl of only the last closed trade
  int position_;                   ///< current position ... which now includeds fix parser position

  // track drawn down online
  double max_pnl_;
  double drawdown_;

  double commish_dollars_per_unit_;
  double retail_commish_dollars_per_unit_;
  int di_reserves_;
  int asx_reserves_;
  mutable double numbers_to_dollars_;

  mutable double current_price_;
  double average_open_price_;  // Average buy / sell price for the position that is open.
  double last_bid_price_;
  double last_ask_price_;

  std::vector<std::string> last_7_trade_lines_;
  bool is_shc_di1_;
  bool is_asx_shc_;
  int asx_bond_face_value_;
  int asx_term_;
  bool is_asx_bond_;
  bool is_asx_ir_;

 public:
  MinuteBarPNL(DebugLogger& t_dbglogger_, Watch& r_watch_, MinuteBarOrderManager& order_manager, int t_runtime_id_,
               std::string shortcode, std::string exchange_symbol, BulkFileWriter& t_trades_writer_);

  ~MinuteBarPNL() {}

  void OnExec(int _new_position_, int _exec_quantity_, TradeType_t _buysell_, double _price_, int r_int_price_,
              const int _security_id_);

  inline double GetDIContractNumbersToDollars() const {
    if (current_price_ == 0.0) {
      // just to avoid inf values when current_price_ is 0.0, we return invalid value
      return 1;
    }

    double unit_price_ = 0;
    double term_ = double(di_reserves_ / 252.0);
    if (term_ > 0.000) {
      unit_price_ = 100000 / std::pow((current_price_ / 100 + 1), term_);
    }
    return (unit_price_ * CurrencyConvertor::Convert(kCurrencyBRL, kCurrencyUSD) / current_price_);
  }

  inline double GetASXBondPrice(double price_) const {
    double yield_ = 100.0 - price_;
    double j8_ = yield_ / 200;
    double j9_ = 1 / (1 + j8_);
    double j10_ = 1000 * (3 * (1 - pow(j9_, (asx_term_ * 2))) / j8_ + 100 * pow(j9_, (asx_term_ * 2)));
    return j10_;
  }

  inline double GetASXIRPrice(double price_) const {
    double yield_ = 100.0 - price_;
    double face_value_ = 1000000;
    double num_days_ = 90;
    double total_days_ = 365;

    return face_value_ / (1 + yield_ * num_days_ / total_days_);
  }

  inline double GetASXIRCurrValuation(double price) const {
    double yield_ = 100.0 - price;
    double face_value_ = 1000000;
    double num_days_ = 90;
    double total_days_ = 365;
    return face_value_ * total_days_ / (total_days_ + yield_ * num_days_ / 100);
  }

  inline double GetASXContractNumbersToDollars() const {
    if (is_asx_bond_) {
      return GetASXBondPrice(current_price_) / current_price_ * CurrencyConvertor::Convert(kCurrencyAUD, kCurrencyUSD);
    } else if (is_asx_ir_) {
      return (GetASXIRCurrValuation(current_price_) - GetASXIRCurrValuation(current_price_ - 0.01)) / 0.01 *
             CurrencyConvertor::Convert(kCurrencyAUD, kCurrencyUSD);
    }
    return numbers_to_dollars_;
  }

  inline double pnl() const { return pnl_; }
  inline double realized_pnl() const { return realized_pnl_; }
  inline double opentrade_unrealized_pnl() const { return opentrade_unrealized_pnl_; }
  inline double min_pnl_till_now() const { return min_pnl_till_now_; }
  inline double total_pnl() const { return total_pnl_; }
  inline double drawdown() const { return drawdown_; }
  inline double last_closing_trade_pnl() const { return last_closing_trade_pnl_; }
  inline int position() const { return position_; }
  inline double commish_dollars_per_unit() const { return commish_dollars_per_unit_; }
  inline double numbers_to_dollars() const { return numbers_to_dollars_; }
  inline double current_price() const { return current_price_; }
  std::string secname() { return numbered_secname_; }
};
}
