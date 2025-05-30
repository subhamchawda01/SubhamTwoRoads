/*
  \file dvctrade/ExecLogicCode/LFI_trading_manager.cpp

  \author: (c) Copyright Two Roads Technological Solutions Pvt Ltd 2011
  Address:
  Suite No 353, Evoma, #14, Bhattarhalli,
  Old Madras Road, Near Garden City College,
  KR Puram, Bangalore 560049, India
  +91 80 4190 3551
 */

#include "dvctrade/ExecLogic/LFI_trading_manager.hpp"

#include <string>

namespace HFSAT {
LFITradingManager::LFITradingManager(DebugLogger &_dbglogger_, const Watch &_watch_,
                                     std::vector<std::string> _structure_shortcode_vec_,
                                     std::string _common_paramfilename_, MultBasePNL *_mult_base_pnl_,
                                     std::string _base_stir_)
    : TradingManager(_dbglogger_, _watch_),
      secid_to_idx_map_(),
      secid_to_maturity_map_(),
      secid_to_pack_(),
      securities_trading_in_pack_(),
      pack_risk_(),
      pack_inflated_risk_(),
      pack_signal_mean_(),
      sec_id_to_sumvars_(),
      sec_id_to_new_sumvars_(),
      minimal_risk_positions_(),
      inflated_risk_positions_(),
      sum_denom_val_(),
      stddev_(),
      book_bias_(),
      positions_to_close_(),
      sec_id_to_zero_crossing_(),
      last_sign_(),
      positions_(),
      common_paramfilename_(_common_paramfilename_),
      lfi_listeners_(),
      shortcode_vec_(_structure_shortcode_vec_),
      sec_name_indexer_(&SecurityNameIndexer::GetUniqueInstance()),
      common_paramset_(nullptr),
      mult_base_pnl_(_mult_base_pnl_),
      slack_manager_(new SlackManager(BASETRADE)),
      spread_id_to_idx_vec_(),
      isOutright_(),
      outright_risk_(0),
      max_sum_outright_risk_(0),
      max_allowed_global_risk_(0),
      num_shortcodes_trading_(0),
      exposed_pnl_(0),
      locked_pnl_(0),
      total_pnl_(0),
      max_opentrade_loss_(-100000),
      max_loss_(-100000),
      break_msecs_on_max_opentrade_loss_(900000),
      max_global_risk_(500),
      last_max_opentrade_loss_hit_msecs_(0),
      read_max_opentrade_loss_(false),
      read_break_msecs_on_max_opentrade_loss_(false),
      read_max_loss_(false),
      read_max_global_risk_(false),
      getting_flat_due_to_opentrade_loss_(false),
      base_stir_(_base_stir_),
      mfm_(0),
      last_compute_getflat_mfm_(0),
      outright_risk_check_(false),
      std_dev_mat_(LINAL::Matrix(0, 0)),
      weight_adjusted_posdiff_(LINAL::Matrix(0, 0)),
      denom_mat_(LINAL::Matrix(0, 0)) {
  //

  mfm_ = 0;

  auto &sec_name_indexer = SecurityNameIndexer::GetUniqueInstance();
  secid_to_idx_map_.resize(sec_name_indexer.NumSecurityId(), -1);
  isOutright_.resize(sec_name_indexer.NumSecurityId(), false);
  sec_id_to_sumvars_.resize(sec_name_indexer.NumSecurityId(), 0);
  sec_id_to_new_sumvars_.resize(sec_name_indexer.NumSecurityId(), 0);
  sec_id_to_zero_crossing_.resize(sec_name_indexer.NumSecurityId(), 0);
  last_sign_.resize(sec_name_indexer.NumSecurityId(), 0);
  book_bias_.resize(sec_name_indexer.NumSecurityId(), 0);
  positions_to_close_.resize(sec_name_indexer.NumSecurityId(), 0);

  // Read the structure file for the given portfolio shortcode
  ReadStructure();

  // Currently we are directly parsing the file, ideally we would want to load through paramset
  LoadCommonParams();

  // Compute the stdev matrix for the given shortcodes
  PopulateStdDevMat();

  UpdatePacksAndBundles(shortcode_vec_);

  // To compute the cumulative pnls
  mult_base_pnl_->AddListener(this);
}

/**
 *
 */
void LFITradingManager::ReadStructure() {
  auto &sec_name_indexer = SecurityNameIndexer::GetUniqueInstance();

  positions_.clear();
  lfi_listeners_.clear();
  stddev_.resize(shortcode_vec_.size());

  for (auto i = 0u; i < shortcode_vec_.size(); i++) {
    stddev_[i] = SampleDataUtil::GetAvgForPeriod(shortcode_vec_[i], watch_.YYYYMMDD(), 60, "TrendStdev", true);

    if (stddev_[i] == 0) {
      std::stringstream st;
      st << "Exiting because the stdev of shortcode " + shortcode_vec_[i] << " is zero";

      ExitVerbose(kExitErrorCodeGeneral, st.str().c_str());
    }
    positions_.push_back(0);
    sum_denom_val_.push_back(0);
    minimal_risk_positions_.push_back(0.0);
    inflated_risk_positions_.push_back(0.0);
    lfi_listeners_.push_back(nullptr);

    secid_to_idx_map_[sec_name_indexer.GetIdFromString(shortcode_vec_[i])] = i;

    /// separate handling for DI1
    if (shortcode_vec_[i].substr(0, 3) == "DI1") {
      auto index = shortcode_vec_[i].find('-');
      if (shortcode_vec_[i].find('-') != std::string::npos) {
        std::string t_security1 = shortcode_vec_[i].substr(0, index);
        std::string t_security2 = shortcode_vec_[i].substr(index + 1, shortcode_vec_[i].size());

        unsigned t_secid_sec1_ = (sec_name_indexer.GetIdFromString(t_security1));
        unsigned t_secid_sec2_ = (sec_name_indexer.GetIdFromString(t_security2));
        unsigned t_secid_spread_ = (sec_name_indexer.GetIdFromString(shortcode_vec_[i]));

        spread_id_to_idx_vec_.push_back({t_secid_sec1_, t_secid_sec2_, t_secid_spread_});

        isOutright_[t_secid_spread_] = false;

      } else {
        isOutright_[sec_name_indexer.GetIdFromString(shortcode_vec_[i])] = true;
      }

    } else {
      if (shortcode_vec_[i].substr(0, 3) == "SP_") {
        std::size_t t_occurence_ = shortcode_vec_[i].find("_", 3);
        std::string t_security1_ =
            base_stir_ + "_" + shortcode_vec_[i].substr(3 + base_stir_.size(), t_occurence_ - 3 - base_stir_.size());
        std::string t_security2_ =
            base_stir_ + "_" +
            shortcode_vec_[i].substr(t_occurence_ + 1 + base_stir_.size(),
                                     shortcode_vec_[i].size() - t_occurence_ - base_stir_.size() - 1);

        unsigned t_secid_sec1_ = (sec_name_indexer.GetIdFromString(t_security1_));
        unsigned t_secid_sec2_ = (sec_name_indexer.GetIdFromString(t_security2_));
        unsigned t_secid_spread_ = (sec_name_indexer.GetIdFromString(shortcode_vec_[i]));

        spread_id_to_idx_vec_.push_back({t_secid_sec1_, t_secid_sec2_, t_secid_spread_});

        isOutright_[t_secid_spread_] = false;

      } else if (shortcode_vec_[i].substr(0, (base_stir_.size() + 1)) == (base_stir_ + "_")) {
        isOutright_[sec_name_indexer.GetIdFromString(shortcode_vec_[i])] = true;
      } else {
        std::stringstream st;
        st << "Unknown shortcode specified " << shortcode_vec_[i] << "\n";
        ExitVerbose(kExitErrorCodeGeneral, st.str().c_str());
      }
    }
  }

  for (unsigned i = 0; i < spread_id_to_idx_vec_.size(); i++) {
    for (unsigned j = 0; j < 3; j++) {
      unsigned idx_ = secid_to_idx_map_[spread_id_to_idx_vec_[i][j]];
      spread_id_to_idx_vec_[i][j] = idx_;  // TODO maybe name it something else...as this has indices not sed ids
    }
  }
}

void LFITradingManager::LoadCommonParams() {
  auto this_shortcode = base_stir_;
  if (shortcode_vec_.size() > 0) {
    this_shortcode = shortcode_vec_[0];
  }

  common_paramset_ = new ParamSet(common_paramfilename_, watch_.YYYYMMDD(), this_shortcode);

  max_opentrade_loss_ = common_paramset_->max_opentrade_loss_;
  read_max_opentrade_loss_ = common_paramset_->read_max_opentrade_loss_;
  max_loss_ = common_paramset_->max_loss_;
  read_max_loss_ = common_paramset_->read_max_loss_;
  max_global_risk_ = common_paramset_->max_global_risk_;
  read_max_global_risk_ = common_paramset_->read_max_global_risk_;
  break_msecs_on_max_opentrade_loss_ = common_paramset_->break_msecs_on_max_opentrade_loss_;
  read_break_msecs_on_max_opentrade_loss_ = common_paramset_->read_break_msecs_on_max_opentrade_loss_;
  use_combined_getflat_ = common_paramset_->combined_get_flat_model != std::string("NoModel");
  common_paramset_->min_model_scale_fact_ = std::max(std::min(common_paramset_->min_model_scale_fact_, 5.0), 0.0);

  if (!(read_max_loss_ && read_max_opentrade_loss_ && read_break_msecs_on_max_opentrade_loss_ &&
        read_max_global_risk_)) {
    ExitVerbose(kStirExitError, "Insufficient common parameters provided");
  }
}

void LFITradingManager::PopulateStdDevMat() {
  size_t t_matrix_dimension_ = spread_id_to_idx_vec_.size();
  size_t t_col_dimension_ = 1;
  size_t t_number_of_securities_ = stddev_.size();
  std_dev_mat_ = LINAL::Matrix(t_matrix_dimension_, t_matrix_dimension_, 0);
  weight_adjusted_posdiff_ = LINAL::Matrix(t_matrix_dimension_, t_col_dimension_, 0);
  denom_mat_ = LINAL::Matrix(t_number_of_securities_, t_number_of_securities_, 0);
  for (unsigned i = 0; i < t_matrix_dimension_; i++) {
    std::vector<unsigned> t_i_constituents = spread_id_to_idx_vec_[i];
    std_dev_mat_.set(i, i, stddev_[t_i_constituents[0]] + stddev_[t_i_constituents[1]] + stddev_[t_i_constituents[2]]);
    for (unsigned j = i + 1; j < t_matrix_dimension_; j++) {
      std::vector<unsigned> t_j_constituents = spread_id_to_idx_vec_[j];
      if (t_i_constituents[0] == t_j_constituents[0]) {
        double t_stdev_ = stddev_[t_i_constituents[0]];
        std_dev_mat_.set(i, j, t_stdev_);
        std_dev_mat_.set(j, i, t_stdev_);
      } else if (t_i_constituents[1] == t_j_constituents[1]) {
        double t_stdev_ = stddev_[t_i_constituents[1]];
        std_dev_mat_.set(i, j, t_stdev_);
        std_dev_mat_.set(j, i, t_stdev_);
      } else if (t_i_constituents[0] == t_j_constituents[1]) {
        double t_stdev_ = -stddev_[t_i_constituents[0]];
        std_dev_mat_.set(i, j, t_stdev_);
        std_dev_mat_.set(j, i, t_stdev_);
      } else if (t_i_constituents[1] == t_j_constituents[0]) {
        double t_stdev_ = -stddev_[t_i_constituents[1]];
        std_dev_mat_.set(i, j, t_stdev_);
        std_dev_mat_.set(j, i, t_stdev_);
      }
    }
  }
  InitializeVars();
}

void LFITradingManager::UpdatePacksAndBundles(const std::vector<std::string> &trading_shortcode_vec) {
  // initialize with -1 maturity/pack id so errors would be visible
  secid_to_maturity_map_.resize(sec_name_indexer_->NumSecurityId(), -1);
  secid_to_pack_.resize(sec_name_indexer_->NumSecurityId(), -1);

  for (auto &shortcode : trading_shortcode_vec) {
    int index = 0;

    // get appropriate index value to start maturity
    if (shortcode.substr(0, 3) == "GE_") {
      index = 3;
    } else if (shortcode.substr(0, 4) == "LFI_" || shortcode.substr(0, 4) == "LFL_") {
      index = 4;
    }

    // starts from 0
    auto maturity = atoi(shortcode.substr(index, shortcode.size() - index).c_str());

    // 1 year index id
    auto pack_id = maturity / 4;

    auto sec_id = sec_name_indexer_->GetIdFromString(shortcode);

    // set the security maturity number of  each sec_id
    secid_to_maturity_map_[sec_id] = maturity;
    secid_to_pack_[sec_id] = pack_id;

    DBGLOG_TIME_CLASS_FUNC << " SecId: " << sec_id << " Shortcode: " << shortcode
                           << " PackId: " << secid_to_pack_[sec_id] << " Maturity: " << secid_to_maturity_map_[sec_id]
                           << DBGLOG_ENDL_FLUSH;
    /*
     * packs are (0-3, 4-7, 8-11, 12-15)
     */

    if ((int)pack_risk_.size() < pack_id + 1) {
      pack_risk_.resize(pack_id + 1, 0);
      pack_inflated_risk_.resize(pack_id + 1, 0);
      pack_signal_mean_.resize(pack_id + 1, 0);
      securities_trading_in_pack_.resize(pack_id + 1, 0);
      pack_base_shc_stdev_.resize(pack_id + 1, 0);
    }

    std::string expiry = std::to_string(pack_id * 4);
    if (shortcode == std::string("GE_") + expiry || shortcode == std::string("LFL_") + expiry ||
        shortcode == std::string("LFI_") + expiry) {
      pack_base_shc_stdev_[pack_id] = stddev_[secid_to_idx_map_[sec_id]];
    }
    securities_trading_in_pack_[pack_id]++;
  }
}

/**
 * In getflat mode, compute how much positions to close for each expiries
 * @param t_security_id return for current security
 * @param force_compute Recompute the getflat positions irrespective of when we last computed
 *                      Used whenever we receive execution in some security
 */
int LFITradingManager::ComputeGetFlatPositions(int t_security_id, bool force_compute) {
  if (!use_combined_getflat_) {
    // In case we are not using combined getflat, just return the original value
    return positions_[secid_to_idx_map_[t_security_id]];
  }

  if (std::abs(outright_risk_) < 1) {
    // outright risk is sum(pos(i)*stdev(i)), so 1 is equivalent to 5 contracts of ZT
    return 0;
  }

  if (force_compute || watch_.msecs_from_midnight() - last_compute_getflat_mfm_ > 1000) {
    double sum_bias = 0.0;
    for (auto id = 0u; id < lfi_listeners_.size(); id++) {
      auto smv = lfi_listeners_[id]->smv();
      double bias = 0.0;

      if ((int)outright_risk_ < 0) {
        // we are short, comput the book bias from bid side
        bias = smv->mkt_size_weighted_price() - smv->bestbid_price();
      } else if ((int)outright_risk_ > 0) {
        // we are long, compute the book bias from ask side
        bias = smv->bestask_price() - smv->mkt_size_weighted_price();
      } else {
        // Don't do anything, we are already flat (almost)
      }

      book_bias_[smv->security_id()] = bias;
      sum_bias += bias;
    }

    if (sum_bias * 1000 == 0) sum_bias = 1;

    for (auto id = 0u; id < lfi_listeners_.size(); id++) {
      auto smv = lfi_listeners_[id]->smv();
      auto sec_id = smv->security_id();
      // Compute what's fraction for
      double this_fraction = book_bias_[sec_id] / sum_bias;
      positions_to_close_[sec_id] = (int)(this_fraction * outright_risk_) / stddev_[id];
      if (dbglogger_.CheckLoggingLevel(TRADING_INFO)) {
        DBGLOG_TIME_CLASS_FUNC_LINE << " Shortcode: " << smv->shortcode() << " Px: " << smv->mkt_size_weighted_price()
                                    << " mp: " << smv->mid_price() << " [" << smv->bestbid_size() << " "
                                    << smv->bestbid_price() << " * " << smv->bestask_price() << " "
                                    << smv->bestask_size() << "]"
                                    << " book_bias: " << book_bias_[sec_id] << " fraction: " << this_fraction
                                    << " pos_to_close: " << positions_to_close_[sec_id] << " or: " << outright_risk_
                                    << " pos: " << positions_[id] << " stdev: " << stddev_[id] << DBGLOG_ENDL_FLUSH;
      }
    }

    last_compute_getflat_mfm_ = watch_.msecs_from_midnight();
  }
  return positions_to_close_[t_security_id];
}

void LFITradingManager::PrintStatus() {
  DBGLOG_TIME_CLASS_FUNC << " exposed_pnl: " << exposed_pnl_ << " locked_pnl: " << locked_pnl_
                         << " total_pnl: " << total_pnl_ << DBGLOG_ENDL_FLUSH;
  /// for each lfi listener, print the stats
  for (auto lfi_listener : lfi_listeners_) {
    PrintStatus(lfi_listener->smv()->security_id(), false);
  }
  dbglogger_ << DBGLOG_ENDL_FLUSH;
}

void LFITradingManager::PrintStatus(unsigned int t_security_id, bool dump_now) {
  auto id = secid_to_idx_map_[t_security_id];
  auto pid = secid_to_pack_[t_security_id];
  DBGLOG_TIME_CLASS_FUNC << sec_name_indexer_->GetShortcodeFromId(t_security_id) << " is_outright: " << isOutright_[id]
                         << " pack_id: " << pid << " num_sec_in_pack: " << securities_trading_in_pack_[pid]
                         << " pack_base_std: " << pack_base_shc_stdev_[pid] << " pack_risk: " << pack_risk_[pid]
                         << " pack_infl_risk: " << pack_inflated_risk_[pid]
                         << " pack_signal_mean: " << pack_signal_mean_[pid]
                         << " s_vars: " << sec_id_to_sumvars_[t_security_id]
                         << " dmeaned_svars: " << sec_id_to_new_sumvars_[t_security_id]
                         << " min_risk_pos: " << minimal_risk_positions_[id]
                         << " infl_risk_pos: " << inflated_risk_positions_[id] << " \n";

  if (dump_now) {
    dbglogger_ << DBGLOG_ENDL_FLUSH;
  }
}

void LFITradingManager::AddListener(unsigned _security_id_, CurveTradingManagerListener *p_listener) {
  unsigned t_idx_ = secid_to_idx_map_[_security_id_];
  lfi_listeners_[t_idx_] = p_listener;

  stddev_[t_idx_] /= p_listener->smv()->min_price_increment();

  // Compute the outright risk from here
  max_sum_outright_risk_ += (p_listener->paramset()->max_position_) * stddev_[t_idx_];

  num_shortcodes_trading_++;

  if (num_shortcodes_trading_ == (int)shortcode_vec_.size()) {
    // All shortcode listners done
    // Compute the alloowed risk
    max_allowed_global_risk_ = common_paramset_->max_global_risk_ratio_ * max_sum_outright_risk_;
  }

  DBGLOG_TIME_CLASS_FUNC << " AddingListener: " << p_listener->smv()->shortcode()
                         << " MaxRisk: " << max_sum_outright_risk_ << " AllowedRisk: " << max_allowed_global_risk_
                         << DBGLOG_ENDL_FLUSH;
}

void LFITradingManager::UpdateOutrightRisk(unsigned _security_id_, int new_position_) {
  unsigned t_idx_ = secid_to_idx_map_[_security_id_];
  double t_outright_risk_ = outright_risk_ + stddev_[t_idx_] * (new_position_ - positions_[t_idx_]);

  /*
  DBGLOG_TIME_CLASS_FUNC << " id: " << _security_id_
                         << " name: " << sec_name_indexer_->GetShortcodeFromId(_security_id_)
                         << " np: " << new_position_ << " or_new: " << t_outright_risk_ << " or_old: " <<
  outright_risk_
                         << " " << exposed_pnl_ << DBGLOG_ENDL_FLUSH;
                         */

  UpdateExposedPNL(outright_risk_, t_outright_risk_);
  outright_risk_ = t_outright_risk_;

  ProcessCombinedGetFlat();

  mult_base_pnl_->UpdateTotalRisk(outright_risk_);
}

/**
 * Try getting flat on one/multiple securities based on conditions
 * a) MaxPosition
 */
void LFITradingManager::ProcessCombinedGetFlat() {
  if (std::abs(outright_risk_) > max_allowed_global_risk_) {
    // if current risk is more than allowed risk,
    // getflat on all securities which have same sign
    outright_risk_check_ = true;
    for (auto idx = 0u; idx < lfi_listeners_.size(); idx++) {
      if (positions_[idx] * outright_risk_ > 0) {
        // If this security is part of creating max_position
        lfi_listeners_[idx]->set_getflat_due_to_non_standard_market_conditions(true);
        DBGLOG_TIME_CLASS_FUNC << " getflat_due_to_risk_ "
                               << " total_risk: " << outright_risk_ << " allowed: " << max_allowed_global_risk_ << " "
                               << lfi_listeners_[idx]->smv()->shortcode() << " "
                               << lfi_listeners_[idx]->smv()->secname() << DBGLOG_ENDL_FLUSH;
      } else {
        // Either position is zero or in opposite sign.
        // We would want to make sure that we cancel all orders which can be harmful
        if (outright_risk_ < 0) {
          lfi_listeners_[idx]->DisAllowOneSideTrade(kTradeTypeSell);
        } else if (outright_risk_ > 0) {
          lfi_listeners_[idx]->DisAllowOneSideTrade(kTradeTypeBuy);
        }
        // Keep flags in a way that we are not placing any further orders in that contract
      }
    }
  } else {
    // resume trading for all securities
    if (outright_risk_check_) {
      for (auto lfi_listener : lfi_listeners_) {
        if (lfi_listener->IsDisallowed()) {
          lfi_listener->AllowTradesForSide(kTradeTypeNoInfo);
        }
        if (lfi_listener->getflat_due_to_non_standard_market_conditions()) {
          lfi_listener->set_getflat_due_to_non_standard_market_conditions(false);
          lfi_listener->Refresh();
          DBGLOG_TIME_CLASS_FUNC << " resume_normal_trading_"
                                 << " total_risk: " << outright_risk_ << " allowed: " << max_allowed_global_risk_ << " "
                                 << lfi_listener->smv()->shortcode() << " " << lfi_listener->smv()->secname()
                                 << DBGLOG_ENDL_FLUSH;
        }
      }
      outright_risk_check_ = false;
    }
  }
}

void LFITradingManager::UpdateExposedPNL(double _outright_risk_, double t_outright_risk_) {
  if (_outright_risk_ * t_outright_risk_ < 0 || int(t_outright_risk_) == 0) {
    locked_pnl_ += exposed_pnl_;
    exposed_pnl_ = 0;
  }
}

void LFITradingManager::UpdatePNL(int _total_pnl_) {
  total_pnl_ = _total_pnl_;
  exposed_pnl_ = total_pnl_ - locked_pnl_;
}

void LFITradingManager::UpdateRiskPosition() {
  // doesn't work correctly for outrights
  // gives individual positions rather than actual inflated

  // TODO why are we doing this ?
  LINAL::Matrix spread_posdiff_ = std_dev_mat_.times(weight_adjusted_posdiff_);

  // For all spreads in the portfolio
  for (unsigned i = 0; i < spread_id_to_idx_vec_.size(); i++) {
    if (spread_posdiff_.get(i, 0) > 0 &&
        positions_[spread_id_to_idx_vec_[i][2]] + spread_posdiff_.get(i, 0) >
            lfi_listeners_[spread_id_to_idx_vec_[i][2]]->paramset()->max_position_) {
      spread_posdiff_.set(i, 0, lfi_listeners_[spread_id_to_idx_vec_[i][2]]->paramset()->max_position_ -
                                    positions_[spread_id_to_idx_vec_[i][2]]);
    } else if (spread_posdiff_.get(i, 0) < 0 &&
               positions_[spread_id_to_idx_vec_[i][2]] + spread_posdiff_.get(i, 0) <
                   -lfi_listeners_[spread_id_to_idx_vec_[i][2]]->paramset()->max_position_) {
      spread_posdiff_.set(i, 0, -lfi_listeners_[spread_id_to_idx_vec_[i][2]]->paramset()->max_position_ -
                                    positions_[spread_id_to_idx_vec_[i][2]]);
    }
  }

  for (unsigned i = 0; i < minimal_risk_positions_.size(); i++) {
    minimal_risk_positions_[i] = positions_[i];
    inflated_risk_positions_[i] = positions_[i];
  }

  double inflated_risk_position_numer_;
  std::vector<unsigned> t_constituents_;

  // compute minimal risk
  for (unsigned i = 0; i < spread_id_to_idx_vec_.size(); i++) {
    t_constituents_ = spread_id_to_idx_vec_[i];
    double change = spread_posdiff_.get(i, 0);
    minimal_risk_positions_[t_constituents_[0]] -= change;
    minimal_risk_positions_[t_constituents_[1]] += change;
    minimal_risk_positions_[t_constituents_[2]] += change;
  }

  // compute inflated risk
  for (unsigned j = 0; j < denom_mat_.getRowDimension(); j++) {
    if (isOutright_[j]) {
      inflated_risk_position_numer_ = 0;
      for (unsigned i = 0; i < denom_mat_.getRowDimension(); i++) {
        if (i != j) {
          inflated_risk_position_numer_ -= denom_mat_.get(j, i) * stddev_[i] * positions_[i];
        }
      }
      for (unsigned i = 0; i < spread_id_to_idx_vec_.size(); i++) {
        t_constituents_ = spread_id_to_idx_vec_[i];
        unsigned t_sec1_id_ = t_constituents_[0];
        unsigned t_sec2_id_ = t_constituents_[1];
        unsigned t_spread_id_ = t_constituents_[2];
        double reduction_from_change_pos_ = denom_mat_.get(j, t_spread_id_) * positions_[j];
        inflated_risk_position_numer_ += ((denom_mat_.get(j, t_sec1_id_) * stddev_[t_sec1_id_]) -
                                          (denom_mat_.get(j, t_sec2_id_) * stddev_[t_sec2_id_]) -
                                          (denom_mat_.get(j, t_spread_id_) * stddev_[t_spread_id_])) *
                                         (spread_posdiff_.get(i, 0) - reduction_from_change_pos_);
      }
      if (sum_denom_val_[j] != 0) {
        inflated_risk_positions_[j] -= inflated_risk_position_numer_ / sum_denom_val_[j];
      }
    } else {
      inflated_risk_positions_[j] = minimal_risk_positions_[j];
    }
  }

  for (unsigned i = 0; i < lfi_listeners_.size(); i++) {
    // rounding off
    inflated_risk_positions_[i] =
        inflated_risk_positions_[i] >= 0 ? inflated_risk_positions_[i] + 0.5 : inflated_risk_positions_[i] - 0.5;
    minimal_risk_positions_[i] =
        minimal_risk_positions_[i] >= 0 ? minimal_risk_positions_[i] + 0.5 : minimal_risk_positions_[i] - 0.5;
  }

  for (unsigned i = 0; i < lfi_listeners_.size(); i++) {
    if (lfi_listeners_[i]) {
      if (isOutright_[i]) {
        DBGLOG_TIME << "Listener_Index:" << i << " Shortcode: "
                    << HFSAT::SecurityNameIndexer::GetUniqueInstance().GetShortcodeFromId(
                           lfi_listeners_[i]->smv()->security_id()) << " Position: " << positions_[i]
                    << " Minimal_Risk_Position: " << (int)minimal_risk_positions_[i]
                    << " Inflated_risk_Position: " << (int)inflated_risk_positions_[i] << DBGLOG_ENDL_FLUSH;
      } else {
        DBGLOG_TIME << "Listener_Index:" << i << " Shortcode: "
                    << HFSAT::SecurityNameIndexer::GetUniqueInstance().GetShortcodeFromId(
                           lfi_listeners_[i]->smv()->security_id()) << " Position:" << positions_[i]
                    << " Minimal_Risk_Position:" << (int)minimal_risk_positions_[i] << DBGLOG_ENDL_FLUSH;
      }
      lfi_listeners_[i]->UpdateRiskPosition((int)inflated_risk_positions_[i], (int)minimal_risk_positions_[i]);
    }
  }
}

void LFITradingManager::UpdateRiskPositionNoSpread() {
  for (unsigned i = 0; i < lfi_listeners_.size(); i++) {
    minimal_risk_positions_[i] = positions_[i];
    inflated_risk_positions_[i] = positions_[i];

    auto pack_inflated_risk = positions_[i] * common_paramset_->self_pos_projection_factor_ +
                              (1 - common_paramset_->self_pos_projection_factor_) *
                                  pack_inflated_risk_[secid_to_pack_[lfi_listeners_[i]->smv()->security_id()]];

    if (dbglogger_.CheckLoggingLevel(TRADING_INFO)) {
      PrintStatus(lfi_listeners_[i]->smv()->security_id(), false);
    }

    if (common_paramset_->compute_notional_risk_) {
      inflated_risk_positions_[i] = outright_risk_ / stddev_[i];
      inflated_risk_positions_[i] =
          inflated_risk_positions_[i] >= 0 ? inflated_risk_positions_[i] + 0.5 : inflated_risk_positions_[i] - 0.5;

    } else if (common_paramset_->self_pos_projection_factor_ < 0.999) {
      inflated_risk_positions_[i] = pack_inflated_risk;
    }

    lfi_listeners_[i]->UpdateRiskPosition((int)inflated_risk_positions_[i], (int)minimal_risk_positions_[i]);
  }
}

/**
 *
 * @param _security_id_
 * @param new_position_
 */
void LFITradingManager::OnPositionUpdate(int position_diff_, int new_position_, unsigned _security_id_) {
  if (secid_to_idx_map_[_security_id_] < 0) {
    std::stringstream st;
    st << "Unknown security with id " << _security_id_ << " pos: " << new_position_ << "Aborting\n";
    ExitVerbose(kExitErrorCodeZeroValue, st.str().c_str());
  }

  unsigned t_idx_ = secid_to_idx_map_[_security_id_];
  if (isOutright_[_security_id_]) UpdateOutrightRisk(_security_id_, new_position_);

  // Compute the pack risk.
  // here assumption is maturities in packs have very high simultaneous correlations

  // At later stage, we can use DV01 may be to compute the combined risk
  auto pid = secid_to_pack_[_security_id_];
  pack_risk_[pid] += (new_position_ - positions_[t_idx_]);

  auto this_pack = secid_to_pack_[_security_id_];
  for (auto i = 0u; i < pack_inflated_risk_.size(); i++) {
    // risk would be in inverse of stdevs ratio
    pack_inflated_risk_[i] += ((new_position_ - positions_[t_idx_]) * pack_stdev_[this_pack]) / pack_stdev_[i];
  }

  positions_[t_idx_] = new_position_;

  for (unsigned i = 0; i < spread_id_to_idx_vec_.size(); i++) {
    std::vector<unsigned> t_constituents_ = spread_id_to_idx_vec_[i];
    if (t_constituents_[0] == t_idx_ || t_constituents_[1] == t_idx_ || t_constituents_[2] == t_idx_) {
      unsigned t_sec1_id_ = t_constituents_[0];
      unsigned t_sec2_id_ = t_constituents_[1];
      unsigned t_spread_id_ = t_constituents_[2];
      weight_adjusted_posdiff_.set(i, 0, stddev_[t_sec1_id_] * positions_[t_sec1_id_] -
                                             stddev_[t_sec2_id_] * positions_[t_sec2_id_] -
                                             stddev_[t_spread_id_] * positions_[t_spread_id_]);
    }
  }
  UpdateRiskPositionNoSpread();
  // UpdateRiskPosition();
}

void LFITradingManager::InitializeVars() {
  // ensure that matrix is invertible.
  // it will be since it is a PSD.The only problem can occur in the unlikely event of an entire row/column being
  // zero,which will only happen if the standard deviations are zero
  if (std_dev_mat_.getRowDimension() != 0)  // no spreads added
  {
    std_dev_mat_ = getPINV(std_dev_mat_);  // TODO maybe do some sort of sanity check of the inverse ?
  }

  // compute the denomination matrix for shortcodes
  for (unsigned i = 0; i < denom_mat_.getRowDimension(); i++) {
    std::vector<std::vector<int> > columns_idx_appearances_;
    std::vector<unsigned> t_constituents_;
    for (unsigned j = 0; j < spread_id_to_idx_vec_.size(); j++) {
      t_constituents_ = spread_id_to_idx_vec_[j];
      if (t_constituents_[0] == i) {
        columns_idx_appearances_.push_back({(int)j, 1});
      } else if (t_constituents_[1] == i || t_constituents_[2] == i) {
        columns_idx_appearances_.push_back({(int)j, -1});
      }
    }
    for (unsigned j = 0; j < spread_id_to_idx_vec_.size(); j++) {
      t_constituents_ = spread_id_to_idx_vec_[j];
      unsigned t_sec1_id_ = t_constituents_[0];
      unsigned t_sec2_id_ = t_constituents_[1];
      unsigned t_spread_id_ = t_constituents_[2];
      for (unsigned k = 0; k < columns_idx_appearances_.size(); k++) {
        int idx_ = columns_idx_appearances_[k][0];
        int sign = columns_idx_appearances_[k][1];
        double v1_ = denom_mat_.get(i, t_sec1_id_);
        denom_mat_.set(i, t_sec1_id_, (v1_ - sign * stddev_[i] * std_dev_mat_.get(j, idx_)));
        double v2_ = denom_mat_.get(i, t_sec2_id_);
        denom_mat_.set(i, t_sec2_id_, (v2_ + sign * stddev_[i] * std_dev_mat_.get(j, idx_)));
        double vSpread_ = denom_mat_.get(i, t_spread_id_);
        denom_mat_.set(i, t_spread_id_, (vSpread_ + sign * stddev_[i] * std_dev_mat_.get(j, idx_)));
      }
    }
    double current_val_ = denom_mat_.get(i, i);
    denom_mat_.set(i, i, 1 + current_val_);
  }

  for (unsigned i = 0; i < denom_mat_.getRowDimension(); i++) {
    double sum_ = 0.0;
    for (unsigned j = 0; j < denom_mat_.getColumnDimension(); j++) {
      sum_ += pow(denom_mat_.get(i, j), 2) * stddev_[j];
    }
    sum_denom_val_[i] = sum_;
  }

  std::stringstream denom_stream;
  std::stringstream stdev_stream;
  denom_mat_.ToString(denom_stream);
  std_dev_mat_.ToString(stdev_stream);

  DBGLOG_TIME_CLASS_FUNC << " DENOM_MAT: \n" << denom_stream.str() << DBGLOG_ENDL_FLUSH;
  DBGLOG_TIME_CLASS_FUNC << " STDEV_MAT: \n" << stdev_stream.str() << DBGLOG_ENDL_FLUSH;
  DBGLOG_TIME_CLASS_FUNC << " SUM: \n";

  for (auto sum : sum_denom_val_) {
    dbglogger_ << sum << " ";
  }
  dbglogger_ << "\n";
  for (auto stdev : stddev_) {
    dbglogger_ << stdev << " ";
  }
  dbglogger_ << DBGLOG_ENDL_FLUSH;
}

/**
 *
 * @param current_sumvars
 * @param security_id
 * @return
 */
double LFITradingManager::RecomputeSignal(double current_sumvars, int t_security_id) {
  if (!common_paramset_->recompute_signal_) return current_sumvars;

  auto pid = secid_to_pack_[t_security_id];
  auto id = secid_to_idx_map_[t_security_id];

  auto this_listener = lfi_listeners_[id];
  // compute the new mean from old mean and new sumvars value
  auto old_mean = pack_signal_mean_[pid];

  auto new_mean =
      old_mean +
      (current_sumvars - sec_id_to_sumvars_[t_security_id]) / (securities_trading_in_pack_[pid] * stddev_[id]);

  // store the new sumvars value
  sec_id_to_sumvars_[t_security_id] = current_sumvars;
  pack_signal_mean_[pid] = new_mean;

  // transform the signal back into that maturity stdev space and return the reduced mean value
  // auto new_sumvar = current_sumvars - (new_mean * stddev_[id] * lfi_listeners_[id]->smv()->min_price_increment());

  // return the normalized sumvars
  // currently just for shake of compatibility with price, using a base stdev value
  // no need to adjust with min-price increment as it gets taken into account for min_model_scale_fact
  auto new_sumvar = (common_paramset_->min_model_scale_fact_ / this_listener->smv()->min_price_increment()) *
                    (current_sumvars / stddev_[id] - new_mean);

  sec_id_to_new_sumvars_[t_security_id] = new_sumvar;

  if (dbglogger_.CheckLoggingLevel(TRADING_INFO)) {
    // All logging data
    if (last_sign_[t_security_id] * new_sumvar < 0) {
      sec_id_to_zero_crossing_[t_security_id]++;
      last_sign_[t_security_id] = int(new_sumvar / std::abs(new_sumvar));
    } else if (last_sign_[t_security_id] == 0) {
      last_sign_[t_security_id] = int(new_sumvar / std::abs(new_sumvar));
    }

    // DBGLOG
    if (watch_.msecs_from_midnight() - mfm_ >= 1000) {
      for (unsigned i = 0; i < sec_id_to_sumvars_.size(); i++) {
        auto this_pid = secid_to_pack_[i];
        auto this_id = secid_to_idx_map_[i];
        if (this_id >= 0) {
          DBGLOG_TIME_CLASS_FUNC << "ThisUpdate: " << t_security_id << " sid: " << i << " "
                                 << lfi_listeners_[this_id]->smv()->shortcode() << " sv: " << sec_id_to_sumvars_[i]
                                 << " stdev: " << stddev_[this_id] << " pid: " << this_pid << " om: " << old_mean
                                 << " mean: " << pack_signal_mean_[this_pid] << " new_sv: " << sec_id_to_new_sumvars_[i]
                                 << " zc: " << sec_id_to_zero_crossing_[i] << " "
                                 << " secs: " << watch_.msecs_from_midnight() / 1000 << DBGLOG_ENDL_FLUSH;
        }
      }
      mfm_ = watch_.msecs_from_midnight();
    }
  }

  return new_sumvar;
}

void LFITradingManager::OnControlUpdate(const ControlMessage &_control_message_, const char *symbol,
                                        const int trader_id) {
  switch (_control_message_.message_code_) {
    case kControlMessageCodeGetflat: {
      if (dbglogger_.CheckLoggingLevel(TRADING_ERROR)) {
        DBGLOG_TIME << "getflat_due_to_external_getflat_ " << trader_id << DBGLOG_ENDL_FLUSH;
      }

      for (auto lfi_listener : lfi_listeners_) {
        if ((strcmp(_control_message_.strval_1_, "") == 0) ||
            strcmp(_control_message_.strval_1_, lfi_listener->smv()->shortcode().c_str()) == 0) {
          lfi_listener->OnControlUpdate(_control_message_, symbol, trader_id);
        }
      }

    } break;
    case kControlMessageCodeStartTrading: {
      if (dbglogger_.CheckLoggingLevel(TRADING_ERROR)) {
        DBGLOG_TIME_CLASS << "StartTrading Called " << trader_id << DBGLOG_ENDL_FLUSH;
      }

      for (auto lfi_listener : lfi_listeners_) lfi_listener->OnControlUpdate(_control_message_, symbol, trader_id);

    } break;

    case kControlMessageCodeSetMaxGlobalRisk:
    case kControlMessageCodeFreezeTrading:
    case kControlMessageCodeUnFreezeTrading:
    case kControlMessageCodeCancelAllFreezeTrading:
    case kControlMessageCodeSetTradeSizes:
    case kControlMessageCodeSetUnitTradeSize:
    case kControlMessageCodeSetMaxUnitRatio:
    case kControlMessageCodeSetMaxPosition:
    case kControlMessageCodeSetWorstCaseUnitRatio:
    case kControlMessageCodeAddPosition:
    case kControlMessageCodeSetWorstCasePosition:
    case kControlMessageCodeDisableImprove:
    case kControlMessageCodeEnableImprove:
    case kControlMessageCodeDisableAggressive:
    case kControlMessageCodeEnableAggressive:
    case kControlMessageCodeShowParams:
    case kControlMessageCodeCleanSumSizeMaps:
    case kControlMessageCodeSetEcoSeverity:
    case kControlMessageCodeForceIndicatorReady:
    case kControlMessageCodeForceAllIndicatorReady:
    case kControlMessageDisableSelfOrderCheck:
    case kControlMessageEnableSelfOrderCheck:
    case kControlMessageDumpNonSelfSMV:
    case kControlMessageCodeEnableAggCooloff:
    case kControlMessageCodeDisableAggCooloff:
    case kControlMessageCodeEnableNonStandardCheck:
    case kControlMessageCodeDisableNonStandardCheck:
    case kControlMessageCodeSetMaxIntSpreadToPlace:
    case kControlMessageCodeSetMaxIntLevelDiffToPlace:
    case kControlMessageCodeSetExplicitMaxLongPosition:
    case kControlMessageCodeSetExplicitWorstLongPosition: {
      for (auto i = 0u; i < lfi_listeners_.size(); i++) {
        if (strcmp(_control_message_.strval_1_, shortcode_vec_[i].c_str()) == 0) {
          DBGLOG_TIME_CLASS << "User Message for shortcode " << shortcode_vec_[i] << DBGLOG_ENDL_FLUSH;
          lfi_listeners_[i]->OnControlUpdate(_control_message_, symbol, trader_id);
        }
      }
    } break;

    case kControlMessageCodeShowIndicators:
    case kControlMessageCodeEnableLogging:
    case kControlMessageCodeDisableLogging:
    case kControlMessageCodeShowOrders: {
      PrintStatus();
      for (auto lfi_listener : lfi_listeners_) {
        lfi_listener->OnControlUpdate(_control_message_, symbol, trader_id);
      }
    } break;
    case kControlMessageDisableMarketManager:
    case kControlMessageEnableMarketManager:
    case kControlMessageCodeEnableZeroLoggingMode:
    case kControlMessageCodeDisableZeroLoggingMode:
    case kControlMessageCodeSetStartTime:
    case kControlMessageCodeSetEndTime: {
      for (auto lfi_listener : lfi_listeners_) {
        lfi_listener->OnControlUpdate(_control_message_, symbol, trader_id);
      }
    } break;

    case kControlMessageCodeSetMaxLoss: {
      if (strcmp(_control_message_.strval_1_, "") == 0) {
        if (_control_message_.intval_1_ > max_loss_ / FAT_FINGER_FACTOR &&
            _control_message_.intval_1_ < max_loss_ * FAT_FINGER_FACTOR) {
          max_loss_ = _control_message_.intval_1_;
        }
        if (dbglogger_.CheckLoggingLevel(TRADING_ERROR)) {
          DBGLOG_TIME_CLASS << "SetMaxLoss " << trader_id
                            << " called for abs_max_loss_ = " << _control_message_.intval_1_ << " and MaxLoss set to "
                            << max_loss_ << DBGLOG_ENDL_FLUSH;
        }
      } else {
        for (auto lfi_listener : lfi_listeners_) {
          if (strcmp(lfi_listener->smv()->shortcode().c_str(), _control_message_.strval_1_) == 0) {
            lfi_listener->OnControlUpdate(_control_message_, symbol, trader_id);
          }
        }
      }
    } break;
    case kControlMessageCodeSetOpenTradeLoss: {
      if (strcmp(_control_message_.strval_1_, "") == 0) {
        if (_control_message_.intval_1_ > max_opentrade_loss_ / FAT_FINGER_FACTOR &&
            _control_message_.intval_1_ < max_opentrade_loss_ * FAT_FINGER_FACTOR) {
          max_opentrade_loss_ = _control_message_.intval_1_;
        }

        if (dbglogger_.CheckLoggingLevel(TRADING_ERROR)) {
          DBGLOG_TIME_CLASS << "SetOpenTradeLoss " << trader_id
                            << " called for abs_opentrade_loss_ = " << _control_message_.intval_1_
                            << " and OpenTradeLoss set to " << max_opentrade_loss_ << DBGLOG_ENDL_FLUSH;
        }
      } else {
        for (auto lfi_listener : lfi_listeners_) {
          if (strcmp(lfi_listener->smv()->shortcode().c_str(), _control_message_.strval_1_) == 0) {
            lfi_listener->OnControlUpdate(_control_message_, symbol, trader_id);
          }
        }
      }
    } break;

    case kControlMessageCodeSetBreakMsecsOpenTradeLoss: {
      if (strcmp(_control_message_.strval_1_, "") == 0) {
        if (_control_message_.intval_1_ > 0) {
          break_msecs_on_max_opentrade_loss_ = _control_message_.intval_1_;
          break_msecs_on_max_opentrade_loss_ = std::max(1 * 60 * 1000, break_msecs_on_max_opentrade_loss_);
        }
        if (dbglogger_.CheckLoggingLevel(TRADING_ERROR)) {
          DBGLOG_TIME_CLASS << "SetBreakMsecsOpenTradeLoss " << trader_id
                            << " called for break_msecs_on_max_opentrade_loss_ = " << _control_message_.intval_1_
                            << " and BreakMsecsOpenTradeLoss set to " << break_msecs_on_max_opentrade_loss_
                            << DBGLOG_ENDL_FLUSH;
        }
      } else {
        for (auto lfi_listener : lfi_listeners_) {
          if (strcmp(lfi_listener->smv()->shortcode().c_str(), _control_message_.strval_1_) == 0) {
            lfi_listener->OnControlUpdate(_control_message_, symbol, trader_id);
          }
        }
      }
    } break;

    default:
      break;
  }
}

std::string LFITradingManager::SavePositionsAndCheck() {
  SendOutstandingPositionMail();
  std::ostringstream t_oss_;
  t_oss_ << "/spare/local/tradeinfo/StructureInfo/logs/LFI_structured_position." << watch_.YYYYMMDD();

  std::ofstream position_output_file_(t_oss_.str().c_str(), std::ofstream::out);

  DBGLOG_TIME_CLASS_FUNC_LINE << "Saving EOD positions in file " << t_oss_.str() << DBGLOG_ENDL_FLUSH;

  bool t_minimal_risk_ = false;
  for (auto i = 0u; i < positions_.size(); i++) {
    position_output_file_ << shortcode_vec_[i] << " Position: " << positions_[i]
                          << " Minimal risk position: " << minimal_risk_positions_[i]
                          << " Inflated Risk position: " << inflated_risk_positions_[i] << "\n";
    t_minimal_risk_ = t_minimal_risk_ == false && minimal_risk_positions_[i] == 0 ? false : true;
  }
  position_output_file_.close();
  std::stringstream output_;
  // if(t_minimal_risk_) always mail with pnl and positions
  {
    for (unsigned i = 0; i < inflated_risk_positions_.size(); i++) {
      output_ << sec_name_indexer_->GetShortcodeFromId(lfi_listeners_[i]->smv()->security_id())
              << " MR : " << (int)(minimal_risk_positions_[i] + 0.5)
              << " IR : " << (int)(inflated_risk_positions_[i] + 0.5) << " P: " << positions_[i] << "\n";
    }
  }

  std::ifstream t_avg_file_read_;
  /*
   * i don't need to read the file
   * again.modify it so that the stdev is
   * read in the first instance
   */

  std::string t_avg_file_name_ = "/spare/local/tradeinfo/StructureInfo/logs/LFI_structured_average";

  double average_ = 0.0;
  double number_of_days_ = 1;
  t_avg_file_read_.open(t_avg_file_name_.c_str(), std::ifstream::in);
  if (t_avg_file_read_.is_open()) {
    const int kLineBufferLen = 1024;
    char readline_buffer_[kLineBufferLen];
    bzero(readline_buffer_, kLineBufferLen);
    while (t_avg_file_read_.good()) {
      bzero(readline_buffer_, kLineBufferLen);
      t_avg_file_read_.getline(readline_buffer_, kLineBufferLen);
      PerishableStringTokenizer st_(readline_buffer_, kLineBufferLen);
      const std::vector<const char *> &tokens_ = st_.GetTokens();
      if (tokens_.size() >= 2) {
        average_ = atof(tokens_[0]);
        number_of_days_ = atof(tokens_[1]);
      }
    }
  }
  t_avg_file_read_.close();

  average_ = (total_pnl_ + number_of_days_ * average_) / (number_of_days_ + 1);
  number_of_days_++;
  output_ << "Average PNL so far : " << average_ << "\n";
  std::ofstream t_avg_file_write_(t_avg_file_name_.c_str());
  t_avg_file_write_ << average_ << " " << number_of_days_ << "\n";
  t_avg_file_write_.close();

  return output_.str();
}

/**
 *
 */
void LFITradingManager::SendOutstandingPositionMail() {
  if (positions_.size() <= 0 || lfi_listeners_.size() <= 0) return;

  std::stringstream st;
  bool positions_exist = false;
  st << " Strategy Exiting with positions: ";
  for (auto id = 0u; id < positions_.size(); id++) {
    if (positions_[id] != 0) {
      positions_exist = true;
      st << lfi_listeners_[id]->smv()->shortcode() << " " << lfi_listeners_[id]->smv()->secname()
         << " Pos: " << positions_[id];
    }
  }

  st << "\nCombinedRisk: " << outright_risk_ << " Equivalent in " << lfi_listeners_[0]->smv()->shortcode() << ": "
     << outright_risk_ / stddev_[0] << " \n";

  if (positions_exist) {
    slack_manager_->sendNotification(st.str());
  }
}
/**
 *
 * @param _trades_writer_
 */
void LFITradingManager::ReportResults(HFSAT::BulkFileWriter &_trades_writer_) {
  int t_total_volume_ = 0;
  int t_supporting_orders_filled_ = 0;
  int t_bestlevel_orders_filled_ = 0;
  int t_aggressive_orders_filled_ = 0;
  int t_improve_orders_filled_ = 0;

  // int t_total_pnl_ = lfi_listeners_[0]->order_manager().base_pnl().mult_total_pnl();
  int t_total_pnl_ = total_pnl_;

  std::stringstream st;
  int runtime_id = 0;
  st << "PNLSPLIT  ";

  if (lfi_listeners_.size() > 0) runtime_id = lfi_listeners_[0]->runtime_id();

  st << runtime_id << " ";
  for (auto i = 0u; i < lfi_listeners_.size(); i++) {
    t_total_pnl_ = lfi_listeners_[i]->order_manager().base_pnl().mult_total_pnl();
    auto &t_order_manager_ = lfi_listeners_[i]->order_manager();
    t_total_volume_ += t_order_manager_.trade_volume();
    t_supporting_orders_filled_ += t_order_manager_.SupportingOrderFilledPercent() * t_order_manager_.trade_volume();
    t_bestlevel_orders_filled_ += t_order_manager_.BestLevelOrderFilledPercent() * t_order_manager_.trade_volume();
    t_improve_orders_filled_ += t_order_manager_.ImproveOrderFilledPercent() * t_order_manager_.trade_volume();
    t_aggressive_orders_filled_ += t_order_manager_.AggressiveOrderFilledPercent() * t_order_manager_.trade_volume();
    std::cerr << lfi_listeners_[i]->smv()->shortcode() << " " << t_order_manager_.base_pnl().total_pnl() << " "
              << t_order_manager_.trade_volume() << std::endl;

    st << lfi_listeners_[i]->smv()->shortcode() << " " << t_order_manager_.base_pnl().total_pnl() << " "
       << t_order_manager_.trade_volume() << " " << (int)t_order_manager_.SupportingOrderFilledPercent() << " "
       << (int)t_order_manager_.BestLevelOrderFilledPercent() << " " << t_order_manager_.ImproveOrderFilledPercent()
       << " " << t_order_manager_.AggressiveOrderFilledPercent() << " ";
  }

  st << "\n";
  _trades_writer_ << st.str();

  st.clear();
  st.str(std::string());

  int num_messages_ = 0;
  for (unsigned i = 0; i < lfi_listeners_.size(); i++) {
    auto &t_order_manager_ = lfi_listeners_[i]->order_manager();
    num_messages_ +=
        (t_order_manager_.SendOrderCount() + t_order_manager_.CxlOrderCount() + t_order_manager_.ModifyOrderCount());

    st << "EOD_MSG_COUNT: " << lfi_listeners_[i]->runtime_id() << " " << lfi_listeners_[i]->smv()->secname() << " "
       << (t_order_manager_.SendOrderCount() + t_order_manager_.CxlOrderCount() + t_order_manager_.ModifyOrderCount())
       << "\n";
  }

  st << "EOD_MSG_COUNT: " << runtime_id << " TOTAL " << num_messages_ << "\n";
  _trades_writer_ << st.str();

  std::vector<int> t_positions_;
  // TODO:pnl computation logic
  for (auto i = 0u; i < lfi_listeners_.size(); i++) {
    const SmartOrderManager &t_order_manager_ = lfi_listeners_[i]->order_manager();

    DBGLOG_TIME_CLASS_FUNC_LINE << "Number of trades in "
                                << HFSAT::SecurityNameIndexer::GetUniqueInstance().GetShortcodeFromId(
                                       lfi_listeners_[i]->smv()->security_id()) << " : "
                                << t_order_manager_.trade_volume() << DBGLOG_ENDL_FLUSH;
    DBGLOG_TIME_CLASS_FUNC_LINE << "Closing position of "
                                << HFSAT::SecurityNameIndexer::GetUniqueInstance().GetShortcodeFromId(
                                       lfi_listeners_[i]->smv()->security_id()) << " : " << positions_[i]
                                << DBGLOG_ENDL_FLUSH;
  }

  if (t_total_volume_ > 0) {
    t_supporting_orders_filled_ /= t_total_volume_;
    t_bestlevel_orders_filled_ /= t_total_volume_;
    t_improve_orders_filled_ /= t_total_volume_;
    t_aggressive_orders_filled_ /= t_total_volume_;
  }

  printf("SIMRESULT %d %d %d %d %d %d\n", t_total_pnl_, (t_total_volume_), t_supporting_orders_filled_,
         t_bestlevel_orders_filled_, t_aggressive_orders_filled_, t_improve_orders_filled_);
}
}
