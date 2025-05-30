/*
 \file dvctrade/ExecLogic/LFI_trading_manager.hpp

 \author: (c) Copyright Two Roads Technological Solutions Pvt Ltd 2011
 Address:
 Suite No 353, Evoma, #14, Bhattarhalli,
 Old Madras Road, Near Garden City College,
 KR Puram, Bangalore 560049, India
 +91 80 4190 3551
 */

#pragma once

#include "dvccode/CommonDataStructures/perishable_string_tokenizer.hpp"
#include "dvccode/CommonDataStructures/vector_utils.hpp"
#include "dvccode/ExternalData/external_time_listener.hpp"
#include "dvccode/Utils/slack_utils.hpp"

#include "baseinfra/BaseUtils/curve_utils.hpp"
#include "baseinfra/MarketAdapter/market_defines.hpp"
#include "baseinfra/MarketAdapter/shortcode_security_market_view_map.hpp"
#include "baseinfra/SmartOrderRouting/mult_base_pnl.hpp"
//#include "dvccode/ORSMessages/control_message_listener.hpp"

#include "dvctrade/ExecLogic/base_trading.hpp"
#include "dvctrade/ExecLogic/curve_trading_manager.hpp"
#include "dvctrade/ExecLogic/curve_trading_manager_listener.hpp"
#include "dvctrade/ExecLogic/exec_logic_utils.hpp"
#include "dvctrade/ExecLogic/trading_manager.hpp"

#define FAT_FINGER_FACTOR 5

namespace HFSAT {

class DI1TradingManager : public TradingManager,
                          public MultBasePNLListener,
                          public CurveTradingManager,
                          public TimePeriodListener {
 protected:
  std::vector<int> secid_to_idx_map_;

  /// Original sumvars for the security
  std::vector<double> sec_id_to_sumvars_;

  /// Stdev of outrights
  std::vector<double> stddev_;
  std::vector<double> risk_ratio_vec_;

  std::vector<double> book_bias_;
  std::vector<int> positions_to_close_;

  /// Keeping track of how frequently my signal is changing
  std::vector<int> sec_id_to_zero_crossing_;

  /// Last sign of the sumvars
  std::vector<int> last_sign_;

  std::vector<int> positions_;

  std::string common_paramfilename_;

  std::vector<CurveTradingManagerListener *> di1_listeners_;

  std::vector<int> pnl_samples_;

  std::vector<std::string> shortcode_vec_;
  // std::vector<std::vector<int>> shortcode_to_exec_shc_vec_; //Exec

  std::vector<int> pnl_sampling_timestamps_;
  unsigned int sample_index_;

  const bool livetrading_;
  int trading_start_utc_mfm_;
  int trading_end_utc_mfm_;

  SecurityNameIndexer *sec_name_indexer_;
  std::vector<bool> is_liquid_shc_;
  std::vector<bool> use_min_portfolio_risk_shc_;
  std::vector<SecurityMarketView *> mkt_tilt_source_shc_;
  std::vector<double> mkt_tilt_thresh_;
  std::vector<double> last_bid_di_mkt_tilt_cancel_msecs_;
  std::vector<double> last_ask_di_mkt_tilt_cancel_msecs_;

  ParamSet *common_paramset_;
  MultBasePNL *mult_base_pnl_;
  SlackManager *slack_manager_;

  double outright_risk_;
  double max_sum_outright_risk_;
  double max_allowed_global_risk_;

  int num_shortcodes_trading_;

  std::vector<bool> is_di_mkt_tilt_buy_cancelled_;
  std::vector<bool> is_di_mkt_tilt_sell_cancelled_;

  int exposed_pnl_;
  int locked_pnl_;
  int total_pnl_;

  int max_opentrade_loss_;
  int max_loss_;
  int break_msecs_on_max_opentrade_loss_;
  int max_global_risk_;
  int last_max_opentrade_loss_hit_msecs_;

  bool read_max_opentrade_loss_;
  bool read_break_msecs_on_max_opentrade_loss_;
  bool read_max_loss_;
  bool read_max_global_risk_;

  bool getting_flat_due_to_opentrade_loss_;
  bool use_combined_getflat_;

  std::string base_stir_;  // LFI,LFL,GE etc.

  int mfm_;
  int last_compute_getflat_mfm_;
  bool cancel_due_to_tilt_;
  bool outright_risk_check_;

  void ReadStructure();
  void LoadCommonParams();
  void UpdateRiskPosition();

 public:
  DI1TradingManager(DebugLogger &_dbglogger_, const Watch &_watch_, std::vector<std::string> _structure_shortcode_vec_,
                    const bool _livetrading_, std::string _common_paramfilename_, MultBasePNL *_mult_base_pnl_,
                    unsigned int _trading_start_utc_mfm_, unsigned int _trading_end_utc_mfm_, std::string _base_stir_);
  virtual ~DI1TradingManager() {}

  void ReportResults(HFSAT::BulkFileWriter &_trades_writer_);

  virtual void OnTimePeriodUpdate(const int num_pages_to_add_) override;

  void Initialize(string shortcode_);

  inline bool MaxLossReached() { return total_pnl_ < -max_loss_ ? true : false; }

  inline int MaxGlobalRisk() { return max_sum_outright_risk_; }

  inline double CombinedRisk() { return outright_risk_; }

  inline bool MaxOpentradeLossReached() {
    auto maxloss_reached = exposed_pnl_ < -max_opentrade_loss_;
    /*
     DBGLOG_TIME_CLASS_FUNC << " mlr: " << maxloss_reached << " lmolhm: " << last_max_opentrade_loss_hit_msecs_
     << " brk: " << break_msecs_on_max_opentrade_loss_
     << " getflat: " << getting_flat_due_to_opentrade_loss_ << " or: " << outright_risk_
     << " mr: " << max_global_risk_ << DBGLOG_ENDL_FLUSH;
     */
    if (!getting_flat_due_to_opentrade_loss_) {
      if (maxloss_reached) {
        last_max_opentrade_loss_hit_msecs_ = watch_.msecs_from_midnight();
        getting_flat_due_to_opentrade_loss_ = true;
      }
    } else {
      if (last_max_opentrade_loss_hit_msecs_ > 0 &&
          watch_.msecs_from_midnight() - last_max_opentrade_loss_hit_msecs_ > break_msecs_on_max_opentrade_loss_ &&
          // If outright risk has gone below maximum global-risk provided
          // This currently depends on fact that we provide max-global-risk value correctly
          // best way would be to compute from each paramset and sum
          std::abs(outright_risk_) < 0.05 * max_sum_outright_risk_) {
        getting_flat_due_to_opentrade_loss_ = false;
      }
    }
    return getting_flat_due_to_opentrade_loss_;
  }

  inline void UpdateCancelFlag(const TradeType_t _buysell_, unsigned int _traded_sec_idx_) {
    if (_buysell_ == kTradeTypeBuy) {
      for (unsigned i = 0; i < di1_listeners_.size(); i++) {
        if (di1_listeners_[i]->paramset()->read_cancel_on_market_tilt_source_) {
          if (mkt_tilt_source_shc_[i]->mkt_size_weighted_price() - mkt_tilt_source_shc_[i]->bestbid_price() <
              mkt_tilt_thresh_[i]) {
            if (dbglogger_.CheckLoggingLevel(TRADING_INFO)) {
              PrintStatus(di1_listeners_[i]->smv()->security_id(), false);

              DBGLOG_TIME_CLASS_FUNC << " BidSideCancel msecs : " << last_bid_di_mkt_tilt_cancel_msecs_[i] << " "
                                     << watch_.msecs_from_midnight() << DBGLOG_ENDL_FLUSH;
            }

            di1_listeners_[i]->order_manager().CancelBidsEqAboveIntPrice(di1_listeners_[i]->smv()->bestbid_int_price());
            last_bid_di_mkt_tilt_cancel_msecs_[i] = watch_.msecs_from_midnight();
            is_di_mkt_tilt_buy_cancelled_[i] = true;
            cancel_due_to_tilt_ = true;
            di1_listeners_[i]->set_zero_bid_di_trade();

            if (dbglogger_.CheckLoggingLevel(TRADING_INFO)) {
              PrintStatus(di1_listeners_[i]->smv()->security_id(), false);

              DBGLOG_TIME_CLASS_FUNC << " BidSideCancel msecs : " << last_bid_di_mkt_tilt_cancel_msecs_[i] << " "
                                     << watch_.msecs_from_midnight() << DBGLOG_ENDL_FLUSH;
            }
          }
        }
      }
    } else if (_buysell_ == kTradeTypeSell) {
      for (unsigned i = 0; i < di1_listeners_.size(); i++) {
        if (di1_listeners_[i]->paramset()->read_cancel_on_market_tilt_source_) {
          if (mkt_tilt_source_shc_[i]->bestask_price() - mkt_tilt_source_shc_[i]->mkt_size_weighted_price() <
              mkt_tilt_thresh_[i]) {
            if (dbglogger_.CheckLoggingLevel(TRADING_INFO)) {
              PrintStatus(di1_listeners_[i]->smv()->security_id(), false);

              DBGLOG_TIME_CLASS_FUNC << " AskSideCancel msecs : " << last_ask_di_mkt_tilt_cancel_msecs_[i] << " "
                                     << watch_.msecs_from_midnight() << DBGLOG_ENDL_FLUSH;
            }

            di1_listeners_[i]->order_manager().CancelAsksEqAboveIntPrice(di1_listeners_[i]->smv()->bestask_int_price());
            last_ask_di_mkt_tilt_cancel_msecs_[i] = watch_.msecs_from_midnight();
            is_di_mkt_tilt_sell_cancelled_[i] = true;
            cancel_due_to_tilt_ = true;
            di1_listeners_[i]->set_zero_ask_di_trade();
            if (dbglogger_.CheckLoggingLevel(TRADING_INFO)) {
              PrintStatus(di1_listeners_[i]->smv()->security_id(), false);

              DBGLOG_TIME_CLASS_FUNC << " AskSideCancel msecs : " << last_ask_di_mkt_tilt_cancel_msecs_[i] << " "
                                     << watch_.msecs_from_midnight() << DBGLOG_ENDL_FLUSH;
            }
          }
        }
      }
    }
  }

  inline void ResetCancelFlag() {
    if (cancel_due_to_tilt_) {
      for (unsigned i = 0; i < di1_listeners_.size(); i++) {
        if (di1_listeners_[i]->paramset()->read_cancel_on_market_tilt_source_) {
          if (mkt_tilt_source_shc_[i]->mkt_size_weighted_price() - mkt_tilt_source_shc_[i]->bestbid_price() >
                  mkt_tilt_thresh_[i] &&
              watch_.msecs_from_midnight() - last_bid_di_mkt_tilt_cancel_msecs_[i] >
                  di1_listeners_[i]->paramset()->cancel_on_market_tilt_msecs_) {
            is_di_mkt_tilt_buy_cancelled_[i] = false;
          }
          if (mkt_tilt_source_shc_[i]->bestask_price() - mkt_tilt_source_shc_[i]->mkt_size_weighted_price() &&
              watch_.msecs_from_midnight() - last_ask_di_mkt_tilt_cancel_msecs_[i] >
                  di1_listeners_[i]->paramset()->cancel_on_market_tilt_msecs_) {
            is_di_mkt_tilt_sell_cancelled_[i] = false;
          }
        }
      }
      // Updating the cancel_due_to_tilt_ variable
      // in case it got updated
      cancel_due_to_tilt_ = (VectorUtils::LinearSearchValue(is_di_mkt_tilt_buy_cancelled_, true) ||
                             VectorUtils::LinearSearchValue(is_di_mkt_tilt_sell_cancelled_, true))
                                ? true
                                : false;

      if (!cancel_due_to_tilt_) {
        // That means it has changed from true to false
        // And thus we need to update Risk Position
        UpdateRiskPosition();
      }
    }
  }

  double RecomputeSignal(double current_sumvars, int t_security_id);

  inline int total_pnl() { return total_pnl_; }
  inline int exposed_pnl() { return exposed_pnl_; }

  std::string SavePositionsAndCheck();
  void SendOutstandingPositionMail();

  int ComputeGetFlatPositions(int security_id, bool force_compute);

  void PrintStatus();
  void PrintStatus(unsigned int security_id, bool dump_now);

  void AddListener(unsigned _security_id_, CurveTradingManagerListener *p_listener_);
  void OnPositionUpdate(int _position_diff_, int _new_position_, unsigned int _security_id_);
  void UpdateOutrightRisk(unsigned _security_id_, int new_position_);
  /// process getflat due to risk position
  void ProcessCombinedGetFlat();
  //  void OnExec(const int t_new_position_, const int _exec_quantity_, const TradeType_t _buysell_, const double
  //  _price_,
  //              const int r_int_price_, const int _security_id_) override;
  void UpdateExposedPNL(double _outright_risk_, double t_outright_risk_);
  void UpdatePNL(int _total_pnl_);
  void OnControlUpdate(const ControlMessage &_control_message_, const char *symbol, const int trader_id) override;
};
}
