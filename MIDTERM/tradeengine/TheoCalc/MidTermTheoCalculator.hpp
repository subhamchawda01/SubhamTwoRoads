#pragma once

#include "tradeengine/Executioner/BaseExecutioner.hpp"
#include "tradeengine/TheoCalc/RatioTheoCalculator.hpp"
#include "tradeengine/TheoCalc/SquareOffTheoCalculator.hpp"
#include "tradeengine/Utils/BarGenerator.hpp"
/*! \brief Header file for MidTermTheo
*/
class MidTermTheoCalculator : public RatioTheoCalculator {
 protected:
  std::string uncascade_time_;
  int cc_lkbk_;
  double trailing_pt_;
  double max_exposure_;
  double cc_alpha_;
  double cc_beta_;
  double time_wt_;
  double pos_wt_;
  int cascade_num_;
  int *cascade_size_;
  int current_cascade_;
  bool cascade_on_;
  bool passive_reduce_position_;
  bool aggressive_reduce_position_;
  // cascade_size_ = [1]*int(cascade_num_)
  // cascade_size_[0] = float(row[2])
  double decay_factor_;
  int lt_days_;
  int obv_lt_days_;
  double long_term_vol_std_;
  double long_term_vol_mean_;
  double long_term_obv_std_;
  // double filter_ptile_;
  // double close_thresh_;
  double prev_day_close_;
  double open_vol_ptile_;
  int open_vol_days_;
  int granularity_;
  int last_aggregation_time_;
  double bar_open_price_;
  double bar_close_price_;
  double bar_low_price_;
  double bar_high_price_;
  double bar_volume_;
  double day_open_volume_;
  double day_open_vol_;
  double day_open_price_;
  double support_price_;
  double resist_price_;
  double moment_cc_wt_mean_;
  double moment_cc_wt_std_;
  double opening_volume_avg_;
  double adj_ratio_;
  double prev_bar_pnl_;
  int lot_size_;
  bool agg_trading_;
  int uncascade_start_utc_mfm_;
  double midterm_stop_loss_;
  double cascading_price_;
  double prev_bar_close_;
  double prev_bar_high_;
  double prev_bar_low_;
  double day_low_;
  double day_high_;
  int subclass_;
  int max_pos_limit_;
  double midterm_pnl_;
  std::string strat_param_file_;
  std::vector<MidTermTheoCalculator*> comp_theo_vec_;
  void LoadParams();
  virtual void onBarUpdate(const unsigned int _security_id_, const HFSAT::MarketUpdateInfo &_market_update_info_);
  virtual void updateTrailingSL(double _price_);

 public:
  MidTermTheoCalculator(std::map<std::string, std::string> *key_val_map, HFSAT::Watch &_watch_,
                        HFSAT::DebugLogger &dbglogger_, int _trading_start_utc_mfm_, int _trading_end_utc_mfm_,
                        int _aggressive_get_flat_mfm, int eff_squareoff_start_utc_mfm_, double bid_multiplier_,
                        double ask_multiplier_);

  virtual ~MidTermTheoCalculator() {}

  // void SetupPNLHooks();
  virtual int total_pnl() { return total_pnl_; }
  double getLongTermVolStd() { return long_term_vol_std_; }

  double getLongTermVolMean() { return long_term_vol_mean_; }

  double getLongTermObvStd() { return long_term_obv_std_; }

  /** Aggregates each trade information to form a granular bar
  */
  void BarAggregator(const HFSAT::TradePrintInfo &_trade_print_info_) {
    double trade_price_ = _trade_print_info_.trade_price_ * adj_ratio_;
    if (!day_open_price_) {
      day_open_price_ = trade_price_;
    }
    if (day_open_vol_ != -1) {
      day_open_vol_ += _trade_print_info_.size_traded_ / adj_ratio_;
    }
    if (!bar_open_price_) {
      bar_open_price_ = trade_price_;
      bar_close_price_ = trade_price_;
      bar_low_price_ = trade_price_;
      bar_high_price_ = trade_price_;
    } else {
      bar_close_price_ = trade_price_;
      if (bar_high_price_ < trade_price_) {
        bar_high_price_ = trade_price_;
      }
      if (bar_low_price_ > trade_price_) {
        bar_low_price_ = trade_price_;
      }
    }
    bar_volume_ += _trade_print_info_.size_traded_ / adj_ratio_;
  }

  void OnMarketUpdate(const unsigned int _security_id_, const HFSAT::MarketUpdateInfo &_market_update_info_);

  void OnTradePrint(const unsigned int _security_id_, const HFSAT::TradePrintInfo &_trade_print_info_,
                    const HFSAT::MarketUpdateInfo &_market_update_info_);
  
  void ConfigureComponentDetails(MidTermTheoCalculator* comp_theo_);

  double GetMidTermPosition(){
    return midterm_pos_offset_;
  }

  void setStratParamFile(std::string _strat_param_file_){
    strat_param_file_ = _strat_param_file_;
    // dbglogger_ <<  GetSecondaryShc() << " " << strat_param_file_ << "\n";
    // ReloadConfig();
  }
  void ReloadConfig();
};
