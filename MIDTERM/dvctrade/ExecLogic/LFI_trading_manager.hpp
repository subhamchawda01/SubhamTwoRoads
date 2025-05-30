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
#include "dvccode/Utils/slack_utils.hpp"

#include "baseinfra/MarketAdapter/market_defines.hpp"
#include "baseinfra/MarketAdapter/shortcode_security_market_view_map.hpp"
#include "baseinfra/SmartOrderRouting/mult_base_pnl.hpp"

#include "dvctrade/ExecLogic/base_trading.hpp"
#include "dvctrade/ExecLogic/curve_trading_manager.hpp"
#include "dvctrade/ExecLogic/curve_trading_manager_listener.hpp"
#include "dvctrade/ExecLogic/trading_manager.hpp"
#include "dvctrade/linal/linal_util.hpp"

#define FAT_FINGER_FACTOR 5

namespace HFSAT {

class LFITradingManager : public TradingManager, public MultBasePNLListener, public CurveTradingManager {
 protected:
  std::vector<int> secid_to_idx_map_;

  ///< for 3 month expiry shortcodes, maintain the maturity starting from 0
  std::vector<int> secid_to_maturity_map_;
  std::vector<int> secid_to_pack_;

  std::vector<int> securities_trading_in_pack_;

  ///< Risk computed for each pack
  /// Using risks across these rather than individually because in most of the cases
  /// our fills would get very similar in shorter part of the curve
  std::vector<double> pack_risk_;

  /// Pack Stdev Adjusted Risk
  std::vector<double> pack_inflated_risk_;

  /// Stdev of the PC1 of the shortcodes in pack
  std::vector<double> pack_stdev_ = {0.019, 0.13, 0.36, 0.47};

  std::vector<double> pack_base_shc_stdev_;

  /// Average of stdev adjusted sumvars for the pack
  std::vector<double> pack_signal_mean_;

  /// Original sumvars for the security
  std::vector<double> sec_id_to_sumvars_;

  /// Sumvars after de-meaning
  std::vector<double> sec_id_to_new_sumvars_;

  std::vector<double> minimal_risk_positions_;
  std::vector<double> inflated_risk_positions_;

  std::vector<double> sum_denom_val_;

  /// Stdev of outrights
  std::vector<double> stddev_;

  std::vector<double> book_bias_;
  std::vector<int> positions_to_close_;

  /// Keeping track of how frequently my signal is changing
  std::vector<int> sec_id_to_zero_crossing_;

  /// Last sign of the sumvars
  std::vector<int> last_sign_;

  std::vector<int> positions_;

  std::string common_paramfilename_;

  std::vector<CurveTradingManagerListener *> lfi_listeners_;

  std::vector<std::string> shortcode_vec_;
  SecurityNameIndexer *sec_name_indexer_;

  ParamSet *common_paramset_;
  MultBasePNL *mult_base_pnl_;
  SlackManager *slack_manager_;

  std::vector<std::vector<unsigned> > spread_id_to_idx_vec_;
  std::vector<bool> isOutright_;

  double outright_risk_;
  double max_sum_outright_risk_;
  double max_allowed_global_risk_;

  int num_shortcodes_trading_;

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
  bool outright_risk_check_;
  // minimal representation math
  LINAL::Matrix std_dev_mat_;
  LINAL::Matrix weight_adjusted_posdiff_;
  LINAL::Matrix denom_mat_;

  void InitializeVars();
  void ReadStructure();
  void LoadCommonParams();
  void PopulateStdDevMat();
  void UpdateRiskPosition();
  void UpdateRiskPositionNoSpread();

  void UpdatePacksAndBundles(const std::vector<std::string> &trading_shortcode_vec);

 public:
  LFITradingManager(DebugLogger &_dbglogger_, const Watch &_watch_, std::vector<std::string> _structure_shortcode_vec_,
                    std::string _common_paramfilename_, MultBasePNL *_mult_base_pnl_, std::string _base_stir_);
  virtual ~LFITradingManager() {}

  void ReportResults(HFSAT::BulkFileWriter &_trades_writer_);

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

  void UpdateExposedPNL(double _outright_risk_, double t_outright_risk_);
  void UpdatePNL(int _total_pnl_);
  void OnControlUpdate(const ControlMessage &_control_message_, const char *symbol, const int trader_id) override;
};
}
