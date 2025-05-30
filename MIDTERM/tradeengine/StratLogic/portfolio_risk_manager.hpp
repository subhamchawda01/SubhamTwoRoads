/**
   \file StratLogic/portfolio_risk_manager.cpp

   \author: (c) Copyright Two Roads Technological Solutions Pvt Ltd 2011
   Address:
   Suite No 353, Evoma, #14, Bhattarhalli,
   Old Madras Road, Near Garden City College,
   KR Puram, Bangalore 560049, India
   +91 80 4190 3551
 */

#ifndef _PORTFOLIO_RISK_MANAGER_H
#define _PORTFOLIO_RISK_MANAGER_H

#include "tradeengine/TheoCalc/BaseTheoCalculator.hpp"

class PortfolioRiskManager : public HFSAT::TimePeriodListener {
 protected:
  HFSAT::DebugLogger& dbglogger_;
  HFSAT::Watch& watch_;
  std::vector<BaseTheoCalculator*> theo_vec_;
  std::vector<BaseTheoCalculator*> sqoff_theo_vec_;

  double total_gross_exposure_allowed_;
  double total_portfolio_pnl_stoploss_;
  double current_total_gross_exposure_;
  double current_total_portfolio_pnl_;
  bool risk_check_hit_;

 public:
  PortfolioRiskManager(HFSAT::DebugLogger& _dbglogger_, HFSAT::Watch& _watch_,
                       std::vector<BaseTheoCalculator*> _theo_vec_, std::vector<BaseTheoCalculator*> _sqoff_theo_vec_,
                       std::map<std::string, std::string>& pos_key_val_map_)
      : dbglogger_(_dbglogger_),
        watch_(_watch_),
        theo_vec_(_theo_vec_),
        sqoff_theo_vec_(_sqoff_theo_vec_),
        current_total_gross_exposure_(0),
        current_total_portfolio_pnl_(0),
        risk_check_hit_(false) {
    LoadParams(pos_key_val_map_);
    watch_.subscribe_FifteenSecondPeriod(this);
  }

  ~PortfolioRiskManager() {}

  void FreezeAllTheos(uint16_t mask_to_unset_) {
    for (auto theo : theo_vec_) {
      theo->TurnOffTheo(mask_to_unset_);
    }
    for (auto theo : sqoff_theo_vec_) {
      theo->TurnOffTheo(mask_to_unset_);
    }
  }

  void UnFreezeAllTheos(uint16_t mask_to_set_) {
    for (auto theo : theo_vec_) {
      theo->TurnOnTheo(mask_to_set_);
    }
    for (auto theo : sqoff_theo_vec_) {
      theo->TurnOnTheo(mask_to_set_);
    }
  }

  void LoadParams(std::map<std::string, std::string>& pos_key_val_map_) {
    total_gross_exposure_allowed_ = Parser::GetDouble(&pos_key_val_map_, "GROSS_EXPOSURE_LIMIT", 0) * 10000000;
    total_portfolio_pnl_stoploss_ = -1 * Parser::GetDouble(&pos_key_val_map_, "TOTAL_PORTFOLIO_STOPLOSS", 0);
  }

  void ResetRiskChecks(std::map<std::string, std::string>& pos_key_val_map_) {
    LoadParams(pos_key_val_map_);
    if ((current_total_portfolio_pnl_ > total_portfolio_pnl_stoploss_) &&
        (current_total_gross_exposure_ < total_gross_exposure_allowed_)) {
      if (risk_check_hit_) {
        DBGLOG_TIME_CLASS_FUNC << "RISK CHECKS RESET on Portfolio level "
                               << " Total PNL: " << current_total_portfolio_pnl_
                               << " SL: " << total_portfolio_pnl_stoploss_
                               << " Total gross exposure: " << current_total_gross_exposure_
                               << "  GE: " << total_gross_exposure_allowed_ << DBGLOG_ENDL_FLUSH;
        risk_check_hit_ = false;
        UnFreezeAllTheos(RISK_STATUS_SET);
      }
    }
  }

  // Two Checks to be done here, more can be added:
  // 1) Total Portfolio Pnl Check
  // 2) Total Gross Exposure Check
  void OnTimePeriodUpdate(const int num_pages_to_add_) {
    double total_gross_exposure_ = 0;
    double total_portfolio_pnl_ = 0;
    for (auto theo : theo_vec_) {
      total_portfolio_pnl_ += theo->GetPNL();
      total_gross_exposure_ += std::abs(theo->GetExposure());
    }

    if ((total_portfolio_pnl_ < total_portfolio_pnl_stoploss_) ||
        (total_gross_exposure_ > total_gross_exposure_allowed_)) {
      if (!risk_check_hit_) {
        DBGLOG_TIME_CLASS_FUNC << "RISK CHECKS HIT on Portfolio level "
                               << " Total PNL: " << total_portfolio_pnl_ << " SL: " << total_portfolio_pnl_stoploss_
                               << " Total gross exposure: " << total_gross_exposure_
                               << "  GE: " << total_gross_exposure_allowed_ << DBGLOG_ENDL_FLUSH;
        risk_check_hit_ = true;
        FreezeAllTheos(RISK_STATUS_UNSET);
      }
    }
    current_total_gross_exposure_ = total_gross_exposure_;
    current_total_portfolio_pnl_ = total_portfolio_pnl_;
  }
};

#endif  // _PORTFOLIO_RISK_MANAGER_H
