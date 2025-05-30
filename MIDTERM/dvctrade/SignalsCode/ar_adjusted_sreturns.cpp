/**
    \author: (c) Copyright Two Roads Technological Solutions Pvt Ltd 2011
     Address:
         Suite No 353, Evoma, #14, Bhattarhalli,
  Old Madras Road, Near Garden City College,
  KR Puram, Bangalore 560049, India
  +91 80 4190 3551
*/

#include "dvctrade/Signals/ar_adjusted_sreturns.hpp"

namespace HFSAT {
// initialize 
// load from strats
  /* PARAM CLASS */
  ParamSet_ARAdjustedSPortReturns::ParamSet_ARAdjustedSPortReturns(const std::string& _paramfilename_, const int _tradingdate_)
    : ParamSet_BaseSignal(_paramfilename_, _tradingdate_),
      lags_in_seconds_vec_(),
      betas_vec_(),
      alphas_vec_(),
      epsilons_vec_() {
    LoadParamSet(_paramfilename_, _tradingdate_);
  }

  void ParamSet_ARAdjustedSPortReturns::LoadParamSet(const std::string& _paramfilename_, const int _tradingdate_) {
    char t_paramfilename_[1024];
    sprintf(t_paramfilename_, "%s_%d", _paramfilename_.c_str(), _tradingdate_);
    std::ifstream paramfile_;
    paramfile_.open(t_paramfilename_, std::ifstream::in);
    if (paramfile_.is_open()) {
      const int kParamFileLineBufferLen = 1024;
      char readline_buffer_[kParamFileLineBufferLen];
      bzero(readline_buffer_, kParamFileLineBufferLen);
      while (paramfile_.good()) {
	bzero(readline_buffer_, kParamFileLineBufferLen);
	paramfile_.getline(readline_buffer_, kParamFileLineBufferLen);

	std::string param_line(readline_buffer_);
	PerishableStringTokenizer st_(readline_buffer_, kParamFileLineBufferLen);
	const std::vector<const char *> &tokens_ = st_.GetTokens();

	if (tokens_.size() < 1) continue;

	if (strcmp(tokens_[0], "LAG_SECS") == 0) {
	  for (unsigned int i = 1u; i < tokens_.size(); i++) {
	    lags_in_seconds_vec_.push_back(atoi(tokens_[i]));
	  }
	} else if (strcmp(tokens_[0], "BETAS") == 0) {
	  for (unsigned int i = 1u; i < tokens_.size(); i++) {
	    betas_vec_.push_back(atof(tokens_[i]));
	  }
	} else if (strcmp(tokens_[0], "ALPHAS") == 0) {
	  for (unsigned int i = 1u; i < tokens_.size(); i++) {
	    alphas_vec_.push_back(atof(tokens_[i]));
	  }
	} else if (strcmp(tokens_[0], "EPSILONS") == 0) {
	  for (unsigned int i = 1u; i < tokens_.size(); i++) {
	    epsilons_vec_.push_back(atof(tokens_[i]));
	  }
	}
      }
      paramfile_.close();
    }
  }

  void ParamSet_ARAdjustedSPortReturns::ToString() { }

  /* SIGNAL CLASS */
  Signal_ARAdjustedSPortReturns::Signal_ARAdjustedSPortReturns(DebugLogger& _dbglogger_, const Watch& _watch_,
							       const ParamSet_ARAdjustedSPortReturns& _paramset_)
    :Signal_BaseSignal(_dbglogger_, _watch_, _paramset_),
     signal_paramset_(_paramset_) {

    num_updates_ = 0;
    // circular buffer 
    num_shortcodes_ = shortcodes_vec_.size();
    num_lags_ = signal_paramset_.lags_in_seconds_vec_.size();
    hist_length_ = 0u;

    for (unsigned int t_idx_ = 0u; t_idx_ < num_lags_; t_idx_++) {
      lag_idxs_.push_back((unsigned int)(signal_paramset_.lags_in_seconds_vec_[t_idx_] / 30));
      hist_length_ = std::max(lag_idxs_[t_idx_], hist_length_);
    }
    for (unsigned int t_idx_ = 0u; t_idx_ < num_lags_; t_idx_++) {
      lag_idxs_[t_idx_] = hist_length_ - lag_idxs_[t_idx_];
      std::cerr << lag_idxs_[t_idx_] << " ";
    }
    std::cerr << "\n";
    // compute returns first and then push_back 
    streaming_data_.resize(hist_length_, std::vector<double>(num_shortcodes_, 0.0));
    inst_prices_.resize(num_shortcodes_, 0.0);

    last_residuals_.resize(hist_length_, std::vector<double>(num_lags_, 0.0));
    residuals_.resize(num_lags_, 0.0);
    // subscribe to 30 secs ( our studies bardata frequency )
    watch_.subscribe_ThirtySecondPeriod(this);

    std::cerr  << "NumShortcodes: " << num_shortcodes_
	       << " NumLags: " << num_lags_
	       << " MaxIdx: " << hist_length_
	       << " SStartTime: " << signal_paramset_.signal_start_utc_mfm_
	       << " SEndTime: " << signal_paramset_.signal_end_utc_mfm_ 
	       << "\n";
    
  }

  void Signal_ARAdjustedSPortReturns::OnTimePeriodUpdate(const int num_pages_to_add) {

    if (!is_ready_) {
      for (unsigned int t_ctr_ = 0u; t_ctr_ < shortcodes_vec_.size(); t_ctr_++) {
	if (!shortcodes_vec_[t_ctr_]->is_ready()) {
	  return;
	}
      }
      is_ready_ = true;
    }

    if (watch_.msecs_from_midnight() >  signal_paramset_.signal_start_utc_mfm_ && 
	watch_.msecs_from_midnight() < signal_paramset_.signal_end_utc_mfm_) {
      // reset alpha every time ( no memory for now)
      alpha_ = 0;
      for (unsigned int t_idx_ = 0u; t_idx_ < num_lags_; t_idx_++) {
	// returns to stationary series
	inst_prices_[0] = shortcodes_vec_[0]->GetPriceRef(kPriceTypeMidprice);
	// no need for seperate vectors, fix this
	ref_price_vec_[0] = inst_prices_[0];
	// sum of weighted returns
	residuals_[t_idx_] = (inst_prices_[0] - streaming_data_[lag_idxs_[t_idx_]][0]) / (streaming_data_[lag_idxs_[t_idx_]][0]);
	for (unsigned int t_ctr_ = 1u; t_ctr_ < num_shortcodes_; t_ctr_++) {
	  inst_prices_[t_ctr_] = shortcodes_vec_[t_ctr_]->GetPriceRef(kPriceTypeMidprice);
	  ref_price_vec_[t_ctr_] = inst_prices_[t_ctr_];
	  residuals_[t_idx_] -= signal_paramset_.betas_vec_[(num_shortcodes_ - 1) * t_idx_ + (t_ctr_ - 1)] * 
	    (inst_prices_[t_ctr_] - streaming_data_[lag_idxs_[t_idx_]][t_ctr_]) / (streaming_data_[lag_idxs_[t_idx_]][t_ctr_]);
	}	
	// stationary series to white noise
	double t_Wn_ = residuals_[t_idx_] - signal_paramset_.alphas_vec_[t_idx_] * last_residuals_[lag_idxs_[t_idx_]][t_idx_];
	alpha_ += (t_Wn_ / signal_paramset_.epsilons_vec_[t_idx_]);
      }

      /*for (unsigned int i = 0u; i < num_shortcodes_; i++) {
	std::cout << "\n" << watch_.tv().tv_sec << "  ";
	for (unsigned int j = 0u; j < streaming_data_.size(); j++) {
	  std::cout << streaming_data_[j][i] << " ";
	}
	}*/
      /*std::cerr << "\n" << watch_.tv().tv_sec << " " << inst_prices_[0] << " " << inst_prices_[1]
		<< " " << (inst_prices_[0] - streaming_data_[lag_idxs_[0]][0]) / (streaming_data_[lag_idxs_[0]][0]) 
		<< " " << (inst_prices_[1] - streaming_data_[lag_idxs_[0]][1]) / (streaming_data_[lag_idxs_[0]][1])
		<< " " << (inst_prices_[0] - streaming_data_[lag_idxs_[1]][0]) / (streaming_data_[lag_idxs_[1]][0]) 
		<< " " << (inst_prices_[1] - streaming_data_[lag_idxs_[1]][1]) / (streaming_data_[lag_idxs_[1]][1])
		<< " " << (inst_prices_[0] - streaming_data_[lag_idxs_[2]][0]) / (streaming_data_[lag_idxs_[2]][0]) 
		<< " " << (inst_prices_[1] - streaming_data_[lag_idxs_[2]][1]) / (streaming_data_[lag_idxs_[2]][1])
		<< " " << (inst_prices_[0] - streaming_data_[lag_idxs_[3]][0]) / (streaming_data_[lag_idxs_[3]][0]) 
		<< " " << (inst_prices_[1] - streaming_data_[lag_idxs_[3]][1]) / (streaming_data_[lag_idxs_[3]][1])
		<< " " << residuals_[0] << " " << residuals_[1]
		<< " " << residuals_[2] << " " << residuals_[3]
		<< " " << alpha_;*/

      streaming_data_.push_back(inst_prices_);
      last_residuals_.push_back(residuals_);
      num_updates_++;
    }

    if (num_updates_ > hist_length_) {
      PropagateNewRisk();
    }
  }
}
