/**
    \author: (c) Copyright Two Roads Technological Solutions Pvt Ltd 2011
     Address:
         Suite No 353, Evoma, #14, Bhattarhalli,
  Old Madras Road, Near Garden City College,
  KR Puram, Bangalore 560049, India
  +91 80 4190 3551
*/

#include "dvctrade/Signals/base_signal.hpp"

namespace HFSAT {
// initialize 
// load from strats
  /* PARAM CLASS */
  ParamSet_BaseSignal::ParamSet_BaseSignal(const std::string& _paramfilename_, const int _tradingdate_)
    : instruments_vec_(),
      place_threshold_(100),
      keep_threshold_(100),
      increase_threshold_(100),
      decrease_threshold_(100),
      instrument_lots_vec_(),
      max_portfolio_lots_(0),
      signal_start_utc_mfm_(0),
      signal_end_utc_mfm_(0) {
    LoadParamSet(_paramfilename_, _tradingdate_);
  }

  void ParamSet_BaseSignal::LoadParamSet(const std::string& _paramfilename_, const int _tradingdate_) {
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

	if (strcmp(tokens_[0], "INSTRUMENTS") == 0) {
	  for (unsigned int i = 1u; i < tokens_.size(); i++) {
	    instruments_vec_.push_back(tokens_[i]);
	  }
	} else if (strcmp(tokens_[0], "PLACE_THRESHOLD") == 0) {
	  place_threshold_ = atof(tokens_[1]);
	} else if (strcmp(tokens_[0], "KEEP_THRESHOLD") == 0) {
	  keep_threshold_ = atof(tokens_[1]);
	} else if (strcmp(tokens_[0], "INCREASE_THRESHOLD") == 0) {
	  increase_threshold_ = atof(tokens_[1]);
	} else if (strcmp(tokens_[0], "DECREASE_THRESHOLD") == 0) {
	  decrease_threshold_ = atof(tokens_[1]);
	} else if (strcmp(tokens_[0], "INST_LOTS") == 0) {
	  for (unsigned int i = 1u; i < tokens_.size(); i++) {
	    instrument_lots_vec_.push_back(atoi(tokens_[i]));
	  }
	} else if (strcmp(tokens_[0], "PORT_MAXLOTS") == 0) {
	  max_portfolio_lots_ = atof(tokens_[1]);
	} else if (strcmp(tokens_[0], "SIGNAL_START_TIME") == 0) {
	  signal_start_utc_mfm_ = (HFSAT::DateTime::GetUTCSecondsFromMidnightFromTZHHMM(_tradingdate_, atoi(tokens_[1] + 4), tokens_[1]) * 1000);
	} else if (strcmp(tokens_[0], "SIGNAL_END_TIME") == 0) {
	  signal_end_utc_mfm_ = (HFSAT::DateTime::GetUTCSecondsFromMidnightFromTZHHMM(_tradingdate_, atoi(tokens_[1] + 4), tokens_[1]) * 1000);
	} else if (strcmp(tokens_[0], "TRADING_DATE") == 0) {
	  if (atoi(tokens_[1]) != _tradingdate_) {
	    std::cerr << "MISMATCH: execution tradingdate is " << _tradingdate_ << " signal tradingdate is " << atoi(tokens_[1]) << "\n";
	    paramfile_.close();
	    exit(-1);
	  }
	}
      }
      paramfile_.close();
    }
  }

  void ParamSet_BaseSignal::ToString() { }

  /* SIGNAL CLASS */
  Signal_BaseSignal::Signal_BaseSignal(DebugLogger& _dbglogger_, const Watch& _watch_,
				       const ParamSet_BaseSignal& _paramset_)
    :dbglogger_(_dbglogger_),
     watch_(_watch_),
     paramset_(_paramset_),
     alpha_(0.0),
     signal_risk_vec_(),
     ref_price_vec_(),
     is_ready_(false) {
    HFSAT::ShortcodeSecurityMarketViewMap &t_shortcode_smv_map_ = 
      HFSAT::ShortcodeSecurityMarketViewMap::GetUniqueInstance();
    for (unsigned int i = 0u; i < paramset_.instruments_vec_.size(); i++) {
      shortcodes_vec_.push_back(t_shortcode_smv_map_.GetSecurityMarketView(paramset_.instruments_vec_[i]));
    }
    ref_price_vec_.resize(paramset_.instruments_vec_.size());
    for (unsigned int i = 0u; i < paramset_.instrument_lots_vec_.size(); i++) {
      signal_risk_vec_.push_back(paramset_.instrument_lots_vec_[i]);
    }
  }

  void Signal_BaseSignal::PropagateNewRisk() {
    if (alpha_ > 0) {
      if (alpha_ > paramset_.place_threshold_) {
	// buy_entry
	std::transform(paramset_.instrument_lots_vec_.begin(), paramset_.instrument_lots_vec_.end(), signal_risk_vec_.begin(),
		       std::bind1st(std::multiplies<int>(), 1));
      } else if (alpha_ < paramset_.keep_threshold_) {
	// buy_exit
	std::transform(paramset_.instrument_lots_vec_.begin(), paramset_.instrument_lots_vec_.end(), signal_risk_vec_.begin(),
		       std::bind1st(std::multiplies<int>(), 0));
      }
    } else {
      if (((-1) * alpha_) > paramset_.place_threshold_) {
	// sell_entry
	std::transform(paramset_.instrument_lots_vec_.begin(), paramset_.instrument_lots_vec_.end(), signal_risk_vec_.begin(),
		       std::bind1st(std::multiplies<int>(), -1));
      } else if (((-1) * alpha_) < paramset_.keep_threshold_) {
	// sell_exit
	std::transform(paramset_.instrument_lots_vec_.begin(), paramset_.instrument_lots_vec_.end(), signal_risk_vec_.begin(),
		       std::bind1st(std::multiplies<int>(), 0));
      }
    }

    if (watch_.msecs_from_midnight() > paramset_.signal_end_utc_mfm_) {
      // get_flat
	std::transform(paramset_.instrument_lots_vec_.begin(), paramset_.instrument_lots_vec_.end(), signal_risk_vec_.begin(),
		       std::bind1st(std::multiplies<int>(), 0));
    }

    NotifyPortRiskListener();
  }

}
