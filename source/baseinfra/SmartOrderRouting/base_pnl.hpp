/**
   \file SmartOrderRouting/base_pnl.hpp

   \author: (c) Copyright Two Roads Technological Solutions Pvt Ltd 2011
   Address:
   Suite No 353, Evoma, #14, Bhattarhalli,
   Old Madras Road, Near Garden City College,
   KR Puram, Bangalore 560049, India
   +91 80 4190 3551
 */
#ifndef BASE_SMARTORDERROUTING_BASE_PNL_H
#define BASE_SMARTORDERROUTING_BASE_PNL_H

#include "baseinfra/OrderRouting/order_manager_listeners.hpp"
#include "baseinfra/OrderRouting/base_order_manager.hpp"
#include "baseinfra/MarketAdapter/security_market_view_change_listener.hpp"
#include "baseinfra/MarketAdapter/security_market_view.hpp"

namespace HFSAT {

/// \brief General class to compute PNL listening to L1 MarketData Updates and OrderRouting Updates ( Execution Updates
/// )
class BasePNLListener {
 public:
  virtual ~BasePNLListener(){};
  virtual void OnPNLUpdate(int index_, int t_pnl_, int& t_mult_pnl_, double& t_mult_risk_, int& t_port_pnl_,
                           double& t_port_risk_) = 0;
  virtual int total_pnl() = 0;
};

class BasePNL : public ExecutionListener, public SecurityMarketViewChangeListener {
 protected:
  DebugLogger& dbglogger_;
  const Watch& watch_;
  SecurityMarketView& dep_market_view_;
  int runtime_id_;

  char numbered_secname_[40];  ///< secname.runtime_id_ for pnl file

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
  int mult_base_pnl_;
  double mult_risk_;

  // In case we have more than 1 mult_pnl in the same exec logic and we want to calculate total Pnl and risk adjusted
  // position (like Options Trading)
  int port_base_pnl_;
  double port_risk_;

  BasePNLListener* base_pnl_listener_;
  int base_pnl_listener_index_;
  bool is_shc_di1_;
  bool is_asx_shc_;
  int asx_bond_face_value_;
  int asx_term_;
  bool is_asx_bond_;
  bool is_asx_ir_;

 public:
  BasePNL(DebugLogger& t_dbglogger_, Watch& r_watch_, SecurityMarketView& t_dep_market_view_, int t_runtime_id_, bool t_are_we_running_bardata_sim = false);

  virtual ~BasePNL(){};

  virtual void OnRetailExec(int _new_position_, int _exec_quantity_, TradeType_t _buysell_, double _price_,
                            int r_int_price_, const int _security_id_, double _retail_commish_ = -1) {
    double dma_commish_ = commish_dollars_per_unit_;

    // change commishion for this trade
    if (_retail_commish_ > 0.0) {
      // passing retail fees as an argument for flexibility
      // if valid use this
      commish_dollars_per_unit_ = _retail_commish_;
    } else {
      commish_dollars_per_unit_ = retail_commish_dollars_per_unit_;
    }
    OnExec(_new_position_, _exec_quantity_, _buysell_, _price_, r_int_price_, _security_id_, 0);
    commish_dollars_per_unit_ = dma_commish_;
  }

  virtual void OnExec(int _new_position_, int _exec_quantity_, TradeType_t _buysell_, double _price_, int r_int_price_,
                      const int _security_id_, const int _caos_) = 0;

  std::string ToString();

  /// @brief Update opentrade_unrealized_pnl_ based on market change
  virtual void OnMarketUpdate(const unsigned int _security_id_, const MarketUpdateInfo& _market_update_info_) {
    current_price_ = _market_update_info_.mkt_size_weighted_price_;
    if ((fabs(current_price_ - kInvalidPrice) < 1e-5) || (std::isnan(current_price_)) || (std::isinf(current_price_))) {
      return;
    }

    if (is_shc_di1_)  // BR_DI_1 we have a generic_ticker
    {
      numbers_to_dollars_ = -GetDIContractNumbersToDollars();
    }
    if (is_asx_shc_) {
      numbers_to_dollars_ = GetASXContractNumbersToDollars();
    }

    if (dep_market_view_.is_ready()) {
      total_pnl_ = pnl_ + (position_ * current_price_ * numbers_to_dollars_);
    }
    if (total_pnl_ < min_pnl_till_now_) {
      min_pnl_till_now_ = total_pnl_;
    }
    opentrade_unrealized_pnl_ = total_pnl_ - realized_pnl_;

    AdjustConsPNL();
    if (base_pnl_listener_ != NULL) {
      base_pnl_listener_->OnPNLUpdate(base_pnl_listener_index_, total_pnl_, mult_base_pnl_, mult_risk_, port_base_pnl_,
                                      port_risk_);
    }
  }

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
    assert(asx_term_ != 0);

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

  inline void OnTradePrint(const unsigned int _security_id_, const TradePrintInfo& _trade_print_info_,
                           const MarketUpdateInfo& _market_update_info_) {
    last_bid_price_ = _market_update_info_.bestbid_price_;
    last_ask_price_ = _market_update_info_.bestask_price_;
  };

  inline void AddListener(int _base_pnl_listener_index_, BasePNLListener* _base_pnl_listener_) {
    base_pnl_listener_ = _base_pnl_listener_;
    base_pnl_listener_index_ = _base_pnl_listener_index_;
  }

  inline const char* numbered_secname() const { return numbered_secname_; }
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
  inline unsigned int security_id() const { return dep_market_view_.security_id(); }

  inline const double AverageOpenPrice() const { return average_open_price_; }

  virtual void SetMaxLoss(const double t_max_loss){};
  virtual void AdjustConsPNL(){};
  virtual int mult_total_pnl() { return base_pnl_listener_->total_pnl(); }
  virtual double ReportConservativeTotalPNL(bool conservative_close_) const { return total_pnl_; }
};
}

#endif  // BASE_ORDERROUTING_BASE_PNL_H
