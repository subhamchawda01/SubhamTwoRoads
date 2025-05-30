/**
   \file SmartOrderRouting/mult_base_pnl.hpp

   \author: (c) Copyright Two Roads Technological Solutions Pvt Ltd 2011
   Address:
   Suite No 353, Evoma, #14, Bhattarhalli,
   Old Madras Road, Near Garden City College,
   KR Puram, Bangalore 560049, India
   +91 80 4190 3551
 */
#ifndef BASE_SMARTORDERROUTING_MULT_BASE_PNL_H
#define BASE_SMARTORDERROUTING_MULT_BASE_PNL_H

#include "dvccode/Utils/bulk_file_writer.hpp"
#include "baseinfra/OrderRouting/order_manager_listeners.hpp"
#include "baseinfra/SmartOrderRouting/base_pnl.hpp"
#include "baseinfra/OrderRouting/base_order_manager.hpp"
#include "baseinfra/MarketAdapter/security_market_view_change_listener.hpp"
#include "baseinfra/MarketAdapter/security_market_view.hpp"

namespace HFSAT {

/// \brief General class to compute PNL listening to L1 MarketData Updates and OrderRouting Updates ( Execution Updates
/// )
class MultBasePNLListener {
 public:
  virtual ~MultBasePNLListener(){};
  virtual void UpdatePNL(int _total_pnl_) = 0;
};

class MultBasePNL : public BasePNLListener {
 protected:
  DebugLogger& dbglogger_;
  const Watch& watch_;
  std::vector<int> pnl_vec_;
  int total_pnl_;
  int min_pnl_till_now_;
  int max_drawdown_;
  double total_risk_;
  double port_risk_;
  int port_listener_index_;
  BasePNLListener* port_listener_;  // This is required to update total pnl and risk in case we have more than 1
                                    // mult_base_pnl (Options Trading)
  std::vector<MultBasePNLListener*> mult_base_pnl_listeners_;
  std::vector<BasePNL*> base_pnl_vec_;
  std::vector<MultBasePNL*> mult_base_pnl_vec_;

 public:
  MultBasePNL(DebugLogger& t_dbglogger_, Watch& r_watch_)
      : dbglogger_(t_dbglogger_),
        watch_(r_watch_),
        pnl_vec_(),
        total_pnl_(0),
        min_pnl_till_now_(0),
        max_drawdown_(),
        total_risk_(0.0),
        port_risk_(0.0),
        port_listener_index_(-1),
        port_listener_(NULL),
        mult_base_pnl_listeners_(),
        base_pnl_vec_() {}

  virtual ~MultBasePNL(){};

  /// @brief Update opentrade_unrealized_pnl_ based on market change

  inline void AddPortListener(int _pnl_listener_index_, BasePNLListener* _pnl_listener_) {
    port_listener_index_ = _pnl_listener_index_;
    port_listener_ = _pnl_listener_;
  }

  inline void AddListener(MultBasePNLListener* _mult_base_pnl_listener_) {
    mult_base_pnl_listeners_.push_back(_mult_base_pnl_listener_);
  }

  inline void OnPNLUpdate(int index_, int t_pnl_, int& t_mult_pnl_, double& t_mult_risk_, int& t_port_pnl_,
                          double& t_port_risk_) {
    total_pnl_ += t_pnl_ - pnl_vec_[index_];
    if (total_pnl_ < min_pnl_till_now_) {
      min_pnl_till_now_ = total_pnl_;
    }
    pnl_vec_[index_] = t_pnl_;
    t_mult_pnl_ = total_pnl_;

    if (port_listener_ != NULL) {
      port_listener_->OnPNLUpdate(port_listener_index_, total_pnl_, t_port_pnl_, t_port_risk_, t_port_pnl_,
                                  t_port_risk_);
    }
    NotifyListeners();
    t_mult_risk_ = total_risk_;
    t_port_risk_ = port_risk_;
  }

  inline void UpdateTotalRisk(double t_total_risk_) { total_risk_ = t_total_risk_; }

  inline void UpdatePortRisk(double t_port_risk_) { port_risk_ = t_port_risk_; }

  inline void NotifyListeners() {
    for (auto i = 0u; i < mult_base_pnl_listeners_.size(); i++) {
      mult_base_pnl_listeners_[i]->UpdatePNL(total_pnl_);
    }
  }

  inline int AddSecurity(BasePNL* p_base_pnl_) {
    base_pnl_vec_.push_back(p_base_pnl_);
    pnl_vec_.push_back(0);
    return pnl_vec_.size();
  }

  inline int AddSecurity(MultBasePNL* p_base_pnl_) {
    mult_base_pnl_vec_.push_back(p_base_pnl_);
    pnl_vec_.push_back(0);
    return pnl_vec_.size();
  }

  inline int total_pnl() { return total_pnl_; }
  inline int min_pnl() { return min_pnl_till_now_; }
};
}

#endif  // BASE_ORDERROUTING_MULT_BASE_PNL_H
