#ifndef _EXECUTIONER_POSTMARKET_HPP
#define _EXECUTIONER_POSTMARKET_HPP

#include "baseinfra/MarketAdapter/shortcode_security_market_view_map.hpp"
#include "baseinfra/OrderRouting/basic_order_manager.hpp"
#include "baseinfra/OrderRouting/order_manager_listeners.hpp"
#include "dvccode/CDef/defines.hpp"
#include "dvccode/CommonTradeUtils/watch.hpp"
#include "dvccode/ExternalData/external_time_listener.hpp"
#include "dvccode/Utils/rdtscp_timer.hpp"
#include "tradeengine/CommonInitializer/defines.hpp"

class MarketOrderExecution : public HFSAT::ExecutionListener,
                             public HFSAT::OrderChangeListener,
                             public HFSAT::OrderRejectListener {
 private:
  std::string shc_;
  HFSAT::Watch& watch_;
  HFSAT::DebugLogger& dbglogger_;
  HFSAT::SecurityMarketView* secondary_smv_;
  HFSAT::BasicOrderManager* basic_om_;
  HFSAT::BasePNL* base_pnl_;
  int start_time_mfm_;
  int trigger_time_mfm_;
  int midnight_mfm_;
  int abs_pos_to_exec_;
  HFSAT::TradeType_t buysell_;
  int position_;
  HFSAT::BaseOrder* order_;
  double total_traded_value_;
  int size_remanining_;
  double price_;
  int int_price_;
  int unique_exec_id_;
  bool polling_on_;
  std::vector<MarketOrderExecution*>* mkt_exec_vec_;

 public:
  MarketOrderExecution(HFSAT::Watch& _watch_, HFSAT::DebugLogger& dbglogger_, HFSAT::SecurityMarketView* _secondary_smv,
                       HFSAT::BasicOrderManager* _basic_om_, int _start_time_mfm_, int _size_to_exec_,
                       HFSAT::TradeType_t _buysell_, double _price_);

  MarketOrderExecution(HFSAT::Watch& _watch_, HFSAT::DebugLogger& dbglogger_, HFSAT::SecurityMarketView* _secondary_smv,
                       HFSAT::BasicOrderManager* _basic_om_, int _start_time_mfm_, int _size_to_exec_,
                       HFSAT::TradeType_t _buysell_, double _price_, int _trigger_time_mfm_, int _tradingdate_,
                       bool polling_on_);

  ~MarketOrderExecution() {}

  void OnExec(const int _new_position_, const int _exec_quantity_, const HFSAT::TradeType_t _buysell_,
              const double _price_, const int r_int_price_, const int _security_id_, int _caos_);
  void OnTimePeriodUpdate();

  void PlaceOrder();
  void myusleep(double micros_to_sleep_);
  int GetPosition() { return position_; }
  int GetPosToExec() {
    return ((buysell_ == HFSAT::TradeType_t::kTradeTypeSell) ? -1 * abs_pos_to_exec_ : abs_pos_to_exec_);
  }
  std::string GetSecondaryShc() { return shc_; }
  void PrintStatus() {
    dbglogger_ << " SHC: " << shc_ << " POS: " << position_ << " POS REMAINING: " << (GetPosToExec() - position_)
               << " TTV: " << total_traded_value_ << " PNL: " << base_pnl_->total_pnl() << DBGLOG_ENDL_FLUSH;
  }

  void OnOrderChange(HFSAT::BaseOrder* _order_) {
    if (order_ && order_->order_status_ == HFSAT::kORRType_Conf) {
      dbglogger_ << watch_.tv() << " " << shc_ << " "
                 << "Order confirmed" << ((buysell_ == HFSAT::TradeType_t::kTradeTypeBuy) ? "BUY " : "SELL ")
                 << " size: " << size_remanining_ << " " << unique_exec_id_ << DBGLOG_ENDL_FLUSH;
    }
    if (order_ && (order_->order_status_ == HFSAT::kORRType_Exec || order_->order_status_ == HFSAT::kORRType_Rejc ||
                   order_->order_status_ == HFSAT::kORRType_Cxld)) {
      if (order_->order_status_ == HFSAT::kORRType_Cxld || order_->order_status_ == HFSAT::kORRType_Rejc) {
        dbglogger_ << watch_.tv() << " " << shc_ << " "
                   << "Order rejected" << ((buysell_ == HFSAT::TradeType_t::kTradeTypeBuy) ? "BUY " : "SELL ")
                   << " size: " << size_remanining_ << " " << unique_exec_id_ << DBGLOG_ENDL_FLUSH;
      } else {
        dbglogger_ << watch_.tv() << " " << shc_ << " "
                   << "Order finished" << ((buysell_ == HFSAT::TradeType_t::kTradeTypeBuy) ? "BUY " : "SELL ")
                   << " size: " << size_remanining_ << " " << unique_exec_id_ << DBGLOG_ENDL_FLUSH;
      }

      order_ = NULL;
    }

    OnTimePeriodUpdate();
  }

  void OnOrderReject(HFSAT::BaseOrder* _order_) { OnOrderChange(_order_); }

  void OnOrderChange(){};

  void OnOrderReject(){};

  void SetExecId(int id) { unique_exec_id_ = id; }

  void setPolling(bool flag) { polling_on_ = flag; }
  bool getPolling() { return polling_on_; }

  void assignExecVec(std::vector<MarketOrderExecution*>* _mkt_vec) { mkt_exec_vec_ = _mkt_vec; }

  void activateExecVec() {
    // int size_ = mkt_exec_vec_->size();
    if (mkt_exec_vec_ != NULL) {
      dbglogger_ << "mkt exec not null " << mkt_exec_vec_->size() << "\n";
      for (unsigned int i = 0; i < mkt_exec_vec_->size(); i++) {
        if (!mkt_exec_vec_->at(i)->getPolling()) {
          mkt_exec_vec_->at(i)->setPolling(true);
          mkt_exec_vec_->at(i)->PlaceOrder();
        }
      }
    }
  }
  void SetBasePNL(HFSAT::BasePNL* base_pnl) { base_pnl_ = base_pnl; }
};

#endif  // _EXECUTIONER_POSTMARKET_HPP
