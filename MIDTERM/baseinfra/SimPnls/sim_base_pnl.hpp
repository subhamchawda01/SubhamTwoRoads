/**
    \file SmartOrderRouting/sim_base_pnl.hpp

    \author: (c) Copyright Two Roads Technological Solutions Pvt Ltd 2011
     Address:
         Suite No 353, Evoma, #14, Bhattarhalli,
         Old Madras Road, Near Garden City College,
         KR Puram, Bangalore 560049, India
         +91 80 4190 3551
 */
#ifndef BASE_SMARTORDERROUTING_SIM_BASE_PNL_H
#define BASE_SMARTORDERROUTING_SIM_BASE_PNL_H

#include "baseinfra/SmartOrderRouting/base_pnl.hpp"
#include "dvccode/Utils/bulk_file_writer.hpp"

namespace HFSAT {

/// \brief General class to compute PNL listening to L1 MarketData Updates and OrderRouting Updates ( Execution Updates
/// )
class SimBasePNL : public BasePNL, public TimePeriodListener {
 protected:
  BulkFileWriter& trades_writer_;
  double max_loss_;
  bool set_cons_total_pnl_;
  bool is_option_;
  OptionObject* option_;
  const SecurityMarketView* fut_smv_;

 public:
  SimBasePNL(DebugLogger& t_dbglogger_, Watch& r_watch_,
             SecurityMarketView& t_dep_market_view_, int t_runtime_id_, BulkFileWriter& t_trades_writer_);

  ~SimBasePNL() {}

  void OnTimePeriodUpdate(const int num_pages_to_add_);

  virtual void OnExec(int _new_position_, int _exec_quantity_, TradeType_t _buysell_, double _price_, int r_int_price_,
                      const int _security_id_);

  void SetMaxLoss(const double t_max_loss) { max_loss_ = t_max_loss; }

  inline void AdjustConsPNL() {
    if (!set_cons_total_pnl_)
      if (total_pnl_ < max_loss_) {
        // cons_total_pnl_ = max_loss_ ;
        set_cons_total_pnl_ = true;
      }
  }

  /// finally report total_pnl_, but only if we hit max_loss and then go up then report max_loss
  inline double ReportConservativeTotalPNL(bool t_conservative_close_) const {
    // undoing this to see real totalpnl and separately look at drawdown
    // if ( set_cons_total_pnl_ )
    // 	{
    // 	  return std::min ( max_loss_, total_pnl_ );
    // 	}
    if (!t_conservative_close_) {
      return total_pnl_;
    } else {
      double t_settle_price_ = 0.0;
      if (position_ > 0) {
        t_settle_price_ = last_bid_price_;
      } else {
        t_settle_price_ = last_ask_price_;
      }

      current_price_ = t_settle_price_;
      if (is_shc_di1_)  // BR_DI_1 we have a generic_ticker
      {
        numbers_to_dollars_ = -GetDIContractNumbersToDollars();
      }

      // std::cerr << t_settle_price_ << std::endl;
      return (pnl_ + (position_ * t_settle_price_ * numbers_to_dollars_));
    }
  }
};
}

#endif  // BASE_ORDERROUTING_SIM_BASE_PNL_H
