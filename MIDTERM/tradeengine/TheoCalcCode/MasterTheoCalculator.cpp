#include "tradeengine/TheoCalc/MasterTheoCalculator.hpp"
#include "tradeengine/Utils/Parser.hpp"
/*! \brief This class handles features/filters applying to multiple products.
 * After loading relevant products, filters can be applied to enable/disable them from trading.
 */

MasterTheoCalculator::MasterTheoCalculator(std::map<std::string, std::string>* key_val_map, HFSAT::Watch& _watch_,
                                           HFSAT::DebugLogger& _dbglogger_, int _trading_start_utc_mfm_,
                                           int _trading_end_utc_mfm_, int _aggressive_get_flat_mfm_)
    : BaseTheoCalculator(key_val_map, _watch_, _dbglogger_, _trading_start_utc_mfm_, _trading_end_utc_mfm_,
                         _aggressive_get_flat_mfm_, 0, 1, 1),
      filter_ptile_(0),
      mom_filter_ptile_(0),
      use_obv_filter_(false),
      mr_filter_(false),
      gaprevert_filter_(false),
      long_term_volatility_vec_(),
      long_term_obv_vec_() {
  dbglogger_ << watch_.tv() << " Creating MASTER THEO CALCULATOR secId " << secondary_id_ << DBGLOG_ENDL_FLUSH;
  LoadParams();
  InitializeDataSubscriptions();
}
/** Loads basic parameters
*/
void MasterTheoCalculator::LoadParams() {
  BaseTheoCalculator::LoadParams();
  bool status_ = Parser::GetBool(key_val_map_, "STATUS", false);
  status_mask_ = (status_ == true ? (status_mask_ | CONFIG_STATUS_SET) : (status_mask_ & CONFIG_STATUS_UNSET));
  filter_ptile_ = Parser::GetDouble(key_val_map_, "FILTER_PTILE", 25);
  mom_filter_ptile_ = Parser::GetDouble(key_val_map_, "MOM_FILTER_PTILE", 25);
  use_obv_filter_ = Parser::GetBool(key_val_map_, "USE_OBV_FILTER", false);
  mr_filter_ = Parser::GetBool(key_val_map_, "USE_MR_FILTER", false);
  gaprevert_filter_ = Parser::GetBool(key_val_map_, "USE_GAPREVERT_FILTER", false);
}

/** Involves basic filter conditions. Fetches individual ticker values and filters according to the params given.
*/
void MasterTheoCalculator::ConfigureMidTermDetails(std::map<std::string, BaseTheoCalculator*>& theo_map_) {
  midterm_detail = new MidTermDetails();
  for (std::map<std::string, std::string>::iterator it = key_val_map_->begin(); it != key_val_map_->end(); ++it) {
    if (it->first.find("_THEO_IDENTIFIER") == std::string::npos || it->first.find("MIDTERM") == std::string::npos) {
      continue;
    }
    std::string midterm_theo_identifier_ = it->second;
    if (theo_map_.find(midterm_theo_identifier_) == theo_map_.end()) {
      DBGLOG_TIME_CLASS_FUNC << "MIDTERM THEO NOT FOUND (exiting) " << midterm_theo_identifier_ << DBGLOG_ENDL_FLUSH;
    }
    else{
      //MOMENTUM
      if(use_obv_filter_){
        midterm_detail->midterm_theo_vec_.push_back(
            dynamic_cast<MidTermTheoCalculator*>(theo_map_[midterm_theo_identifier_]));
        double long_term_vol_ =
            midterm_detail->midterm_theo_vec_[midterm_detail->midterm_theo_vec_.size() - 1]->getLongTermVolMean();
        double long_term_obv_ =
            midterm_detail->midterm_theo_vec_[midterm_detail->midterm_theo_vec_.size() - 1]->getLongTermObvStd();
        dbglogger_ << it->second << " LTVOLATILITY: " << long_term_vol_ << " " << long_term_obv_ << DBGLOG_ENDL_FLUSH;
        long_term_volatility_vec_.push_back(long_term_vol_);
        long_term_obv_vec_.push_back(long_term_obv_);
      }
      //MR and GAPREVERT
      else if(mr_filter_ || gaprevert_filter_){
        midterm_detail->midterm_theo_vec_.push_back(
            dynamic_cast<MidTermTheoCalculator*>(theo_map_[midterm_theo_identifier_]));
        double long_term_vol_ =
            midterm_detail->midterm_theo_vec_[midterm_detail->midterm_theo_vec_.size() - 1]->getLongTermVolMean();
        dbglogger_ << it->second << " LTVOLATILITY: " << long_term_vol_ << DBGLOG_ENDL_FLUSH;
        long_term_volatility_vec_.push_back(long_term_vol_);
      }
      
      // Used in MACD and MA
      else {
        midterm_detail->midterm_theo_vec_.push_back(
            dynamic_cast<MidTermTheoCalculator*>(theo_map_[midterm_theo_identifier_]));
        double long_term_vol_ =
            midterm_detail->midterm_theo_vec_[midterm_detail->midterm_theo_vec_.size() - 1]->getLongTermVolStd();
        dbglogger_ << it->second << " LTVOLATILITY: " << long_term_vol_ << DBGLOG_ENDL_FLUSH;
        long_term_volatility_vec_.push_back(long_term_vol_);
      } 
    }

    
  }

  size_t n = (long_term_volatility_vec_.size() - 1) * filter_ptile_ / 100;
  double adjust_ = (double)((long_term_volatility_vec_.size() - 1) * filter_ptile_ / 100.0) - (double)n;

  std::sort(long_term_volatility_vec_.begin(), long_term_volatility_vec_.end());
  double long_term_filter_ =
      (1 - adjust_) * long_term_volatility_vec_[n] + adjust_ * (long_term_volatility_vec_[n + 1]);
  // Used in Momentum
  if (use_obv_filter_) {
    n = (long_term_obv_vec_.size() - 1) * mom_filter_ptile_ / 100;
    adjust_ = (double)((long_term_volatility_vec_.size() - 1) * mom_filter_ptile_ / 100.0) - (double)n;
    // n--;
    std::sort(long_term_obv_vec_.begin(), long_term_obv_vec_.end());
    double long_term_obv_filter_ = (1 - adjust_) * long_term_obv_vec_[n] + adjust_ * (long_term_obv_vec_[n + 1]);
    dbglogger_ << "FILTER: " << long_term_filter_ << " " << long_term_obv_filter_ << DBGLOG_ENDL_FLUSH;

    for (unsigned int i = 0; i < midterm_detail->midterm_theo_vec_.size(); i++) {
      if (long_term_filter_ > midterm_detail->midterm_theo_vec_[i]->getLongTermVolMean()) {
        midterm_detail->midterm_theo_vec_[i]->TurnOffTheo(CHILD_STATUS_UNSET);
      }
      if (long_term_obv_filter_ > midterm_detail->midterm_theo_vec_[i]->getLongTermObvStd()) {
        midterm_detail->midterm_theo_vec_[i]->TurnOffTheo(CHILD_STATUS_UNSET);
      }
    }
  
  } else if (mr_filter_ || gaprevert_filter_) {//Used in MR and GapRevert
    dbglogger_ << "FILTER: " << long_term_filter_ << DBGLOG_ENDL_FLUSH;
    for (unsigned int i = 0; i < midterm_detail->midterm_theo_vec_.size(); i++) {
      if (long_term_filter_ > midterm_detail->midterm_theo_vec_[i]->getLongTermVolMean()) {
        midterm_detail->midterm_theo_vec_[i]->TurnOffTheo(CHILD_STATUS_UNSET);
      }
    }
  } else {  // Used in Macd & MA
    dbglogger_ << "FILTER: " << long_term_filter_ << DBGLOG_ENDL_FLUSH;
    for (unsigned int i = 0; i < midterm_detail->midterm_theo_vec_.size(); i++) {
      if (long_term_filter_ > midterm_detail->midterm_theo_vec_[i]->getLongTermVolStd()) {
        midterm_detail->midterm_theo_vec_[i]->TurnOffTheo(CHILD_STATUS_UNSET);
      }
    }
  }
}

void MasterTheoCalculator::UpdateTheoPrices(const unsigned int _security_id_,
                                            const HFSAT::MarketUpdateInfo& _market_update_info_) {}

void MasterTheoCalculator::ComputeAndUpdateTheoListeners() { UpdateTheoListeners(); }

void MasterTheoCalculator::OnReady(const HFSAT::MarketUpdateInfo& _market_update_info_) {}

void MasterTheoCalculator::OnExec(const int _new_position_, const int _exec_quantity_,
                                  const HFSAT::TradeType_t _buysell_, const double _price_, const int r_int_price_,
                                  const int _security_id_, const int _caos_) {}

void MasterTheoCalculator::UpdateTheoListeners() {}

void MasterTheoCalculator::SetupPNLHooks() {}
void MasterTheoCalculator::PNLStats(HFSAT::BulkFileWriter* trades_writer_, bool dump_to_cout) {}
